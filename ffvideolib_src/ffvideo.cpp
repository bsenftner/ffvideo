

/////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdarg.h>

#include "ffvideo.h"

static bool                     gFFVideoInitialized    = false;
static int                      gFFVideoLogLevel       = AV_LOG_INFO; // AV_LOG_INFO; AV_LOG_DEBUG AV_LOG_QUIET

//////////////////////////////////////////////////////////////////////////////////////
// initialize an instance of the library for use playing video
// The args are defaulted to be false and false, so providing them can be skipped. 
//	vflip							causes video frames to be vertically flipped
//  loglevel debug		causes verbose output to ffvideo.log
//////////////////////////////////////////////////////////////////////////////////////
void FFVideo::Initialize(bool vflip, bool debug )
{
	mp_videoProcessingThread = NULL;
	mp_frameMgr = NULL;

	// setup the default auto frame interval profile:
	m_auto_frame_interval = false;
	m_auto_interval_thresholds.push_back(7680);
	m_auto_intervals.push_back(3);
	m_auto_interval_thresholds.push_back(1920);
	m_auto_intervals.push_back(2);
	m_auto_interval_thresholds.push_back(128);
	m_auto_intervals.push_back(1);

	if (!gFFVideoInitialized)
	{
		gFFVideoInitialized = true;
		
		av_log_set_callback( query_callback );
		// av_log_set_callback( &FFVideo::logging_callback );

		av_log_set_flags(AV_LOG_SKIP_REPEATED);
		av_log_set_level( gFFVideoLogLevel );

		avdevice_register_all();		// necessary for USB devices
		avformat_network_init();		// Do global initialization of network components
	}
	else 
	{
		av_log_set_callback( &FFVideo::logging_callback );
	}

	// generic media reader & video player stuff below:

	m_vid_src.clear();			
	m_media_fname.clear();
	m_ip_url.clear();
	m_usb_pin					    = 0;
	m_width						    = 0;
	m_height					    = 0;
	mp_opts						    = NULL;			// key/value dictionary of options we request
	mp_format_context     = NULL;
	mp_video_stream       = NULL;			// the video stream within mp_format_context->streams[]
	mp_orig_codec_context = NULL;			// codec context we demux/decompress/de-whatever with
	mp_codec_context			= NULL;			// actual codec context used, as the above is reference only
	m_timebase						= 0.0;			// each stream has its own timebase, multiply against a timestamp to convert timestamp to seconds
	m_timebase_num        = 0;
	m_timebase_dem        = 0;
	m_expected_frame_rate = 0.0;

	// we decompress into a small circular array of decompression buffers:
	for (int i = 0; i < FFVIDEO_DECOMPRESSION_AVFRAME_COUNT; i++) 
		mp_decompressed_frame[i] = NULL;
	//
	m_decompress_index = 0;		// the next mp_decompressed_frame index we will decompress into
	m_display_index    = 0;		// the next mp_decompressed_frame index we will display from

	// pretty much everything to do with sending the frames to the client:
	mp_frameMgr = new FFVideo_FrameMgr( this );
	//
	// atomic bools used to identify when to stop VideoProcessLoop() looping and that VideoProcessLoop() has exited:
	m_stop_video_processing_loop = false;
	m_video_processing_loop_ended = false;

	// handle caller options: 
	mp_frameMgr->mp_frame_dest->m_vflip = vflip;

	// the one and only AVPacket in this library instance:
	mp_packet = av_packet_alloc();
	//
	if (debug)
	{
		gFFVideoLogLevel = AV_LOG_DEBUG;
		av_log_set_level(gFFVideoLogLevel);
	}
}

//////////////////////////////////////////////////////////////////////////////////////
FFVideo::~FFVideo()
{
	KillStream();
	if (mp_packet)
	   av_packet_free(&mp_packet);
}

//////////////////////////////////////////////////////////////////////////////////////
void FFVideo::KillStream()
{
	StopStream();

	if (IsRunning()) 
	{
		// tell VideoProcessLoop() (running in it's own thread) to exit:
		m_stop_video_processing_loop = true;
		//
		uint32_t spins = 0;
		while (!m_video_processing_loop_ended)
		{
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(200ms);
			//
			spins++;
		}
		mp_videoProcessingThread->join();
		delete mp_videoProcessingThread;
		mp_videoProcessingThread = NULL;
		m_stop_video_processing_loop = false;	// reset for next use
		m_video_processing_loop_ended = false;
	}
	
	if (mp_frameMgr)
	{
		if (mp_frameMgr->mp_frame_dest->m_frame_exporter.IsRunning())
		{
			// we're killing, so stop the exporter regardless of any still in queue
			mp_frameMgr->mp_frame_dest->m_frame_exporter.StopExporter();
		}

		delete mp_frameMgr;
		mp_frameMgr = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////////////////
// callback installed via av_log_set_callback() to receive ffmpeg library messages:
void FFVideo::logging_callback(void *ptr, int level, const char *fmt, va_list vargs)
{
	static char message[8192];

	std::string std_fmt = fmt; // std::string(fmt);

	// get rid of unknown format descriptors from h264 module:
	replaceAll( std_fmt, std::string("%td"), std::string("%d") );

	// Create the actual message
#ifdef WIN32
	vsnprintf_s(message, sizeof(message), _TRUNCATE, std_fmt.c_str(), vargs);
#else
	vsnprintf(message, sizeof(message), std_fmt.c_str(), vargs);
#endif

	if (strncmp( message, "real-time buffer", 16) == 0)
		return;

	// only if the log level is appropriate do we write out the log message:
	if (level > gFFVideoLogLevel) 
		return;

	// write it: 
	FILE *fp = fopen( "ffvideo.log", "a" );
	if (fp)
	{
		fprintf( fp, "%s", message );
		fclose(fp);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// Use this to specify a horizontal frame resolution to playback frame interval mapping. 
// Backstory: when playing IP video the resolution that will be delivered may not be what
// is expected, or even known beforehand. To handle high frame resolutions the library 
// client can choose to set a frame interval: number of frames to advance between delivery
// of a new frame to the client. A frame interval of 1 delivers every frame, while a frame
// interval of 2 delivers every other frame to the library client.
// This routine allows setting of two parallel vectors, like so:
//		thresholds:	7680, 5120, 3840
//		intervals:	3, 2, 1
// ...which corresponds to an auto interval profile where resolutions at and above 7680 receive
// a frame interval of 3 (deliver every 3rd frame), frame resolutions at and above 5120 receive
// a frame interval of 2, and resolutions below that receive the normal deliery of every frame. 
// The passed vectors must be sent in ascending order or this will not work as expected. 
bool FFVideo::SetAutoFrameIntervalProfile(std::vector<uint32_t>& thresholds, std::vector<uint32_t>& intervals)
{
	if (m_is_opening)
	{
		ReportLog("Cannot SetAutoFrameIntervalProfile() while starting playback. Set before calling Play()");
		return false;
	}
	if (thresholds.size() != intervals.size())
	{
		ReportLog("SetAutoFrameIntervalProfile() parameter vectors need to be same length");
		return false;
	}

	m_auto_interval_thresholds = thresholds;
	m_auto_intervals = intervals;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////
void FFVideo::CloseStream(void)
{
	if (m_is_opening)
	{
		// if user tries to stop stream microseconds after starting it, 
		// ignore that to prevent crashes from structs deallocating while allocating them:
		return;
	}

	// deallocate structs:
	for (int i = 0; i < FFVIDEO_DECOMPRESSION_AVFRAME_COUNT; i++)
	{
		if (mp_decompressed_frame[i])
		{
			av_frame_free( &mp_decompressed_frame[i] );
			mp_decompressed_frame[i] = NULL;
		}
	}

	// Close the codecs:
	if (mp_codec_context)
	{
		avcodec_close( mp_codec_context );
		mp_codec_context = NULL;
	}
	if (mp_orig_codec_context)
	{
		avcodec_close( mp_orig_codec_context );
		mp_orig_codec_context = NULL;
	}

	// close the stream:
	if (mp_format_context)
	{
		avformat_close_input( &mp_format_context );
		mp_format_context = NULL;
	}
	if (mp_opts)
	{
		av_dict_free( &mp_opts );
		mp_opts = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////
// This is the library's video packet ingestion. Handle media file seeks, frame counting,
// setting of the I/O timeout dynamically via stream state, media file pausing, decompression
// buffer selection, reading of 1 stream packet, stream termination, enterance into drain
// mode, media file auto-loop-at-end logic, advancement over corrupt frames, packet to AVFrame
// decompression, and management of stream termination/end. This also tries to handle the 
// case where the library client deletes everything mid-process.  
void FFVideo::ProcessPacket(uint64_t& wait_milliseconds_for_next_packet, bool& terminal_flag)
{
	if (!mp_frameMgr || !mp_format_context)
	{
		terminal_flag = true;
		return;
	}

	// all stream seeking ultimately takes place here (except end of stream looping, that's lower in this same routine)
	mp_frameMgr->HandleSeekRequests();	// if no seek, does nothing

	if (mp_frameMgr->m_first_frame)
	{
		// play media files as fast as possible:
		if (mp_frameMgr->m_stream_type == 0)
			wait_milliseconds_for_next_packet = 0;

		// no timeout on first frame because we're buffering
		mp_frameMgr->SetReadTimeout(0.0f);
	}
	else // not first frame
	{
		if (mp_frameMgr->m_read_timeout != 0.0f)			// check for a user defined stream reading timeout
		{
			mp_frameMgr->SetNextIOTimeout(mp_frameMgr->m_read_timeout);
		}
		else mp_frameMgr->SetNextIOTimeout(6.0);		// no user defined timeout, allow 6.0 seconds to read next frame/packet
	}

	// if we're paused we should get out of here. But we might have just done a seek...
	// if paused but just finished a seek, let it play till the next keyframe:
	if (mp_frameMgr->IsPlaybackPaused())
	{
		if (mp_frameMgr->AnyPostSeekProcessingActive() == false)
			return; // yes, we really are not processing any frames
	}

	// an array of decompression buffers is treated as a circular buffer,
	// make sure we are not wrapping around before the oldest work is handled: 
	uint32_t diff = m_decompress_index - m_display_index;
	if (diff >= FFVIDEO_DECOMPRESSION_AVFRAME_COUNT / 2)
		return;

	// the current video frame packet:
	AVPacket* curr_packet = mp_packet;

	if (!mp_frameMgr || !mp_format_context) // client/user forcable quitting? 
	{
		terminal_flag = true;
		return;
	}

	int32_t stream_type = mp_frameMgr->m_stream_type;

	// let's read a media stream packet/frame:
	int stream_status = av_read_frame(mp_format_context, curr_packet);
	if (stream_status < 0)
	{
		char errbuff[256];
		av_strerror( stream_status, errbuff, sizeof(errbuff) );
		av_log( mp_codec_context, AV_LOG_ERROR, "%s\n", errbuff );

		ReportLog("av_read_frame: error %s\n", errbuff);

		if (stream_type == 0) 
			   mp_frameMgr->m_media_has_ended = true;	// end of media file // 0=Media, 1=USB, 2=IP
		else mp_frameMgr->m_stream_has_died = true;	// camera/IP/USB stream has terminated unexpectedly 

		mp_frameMgr->m_drain_mode = true;
		terminal_flag = true;

		// Media files might want to auto-loop without stopping: 
		if ((stream_type == 0) && m_loop_media_file)
		{
			int64_t ts(0), rel(0);
			Seek(ts, rel, false);

			mp_frameMgr->m_media_has_ended = false;
			mp_frameMgr->m_drain_mode = false;
			mp_frameMgr->mp_frame_dest->m_frame_export_interval = 0;

			terminal_flag = false;
		}

		return;
	}

	// check for corruption errors: 
	if (!(curr_packet->flags & AV_PKT_FLAG_CORRUPT))
	{
		if (curr_packet->stream_index == mp_video_stream->index)
		{
			int ret(0);

			// Decode video frame(s) using new API:
			if (mp_frameMgr->m_drain_mode)
			{
				if (!mp_codec_context)
				{
					terminal_flag = true;
					ReportLog("avcodec_send_packet: not called because mp_codec_contex already deallocated\n");

					av_packet_unref(curr_packet); // Free the packet that was allocated by av_read_frame 
					return;
				}

				ret = avcodec_send_packet(mp_codec_context, NULL);
			}
			else
			{
				ret = avcodec_send_packet(mp_codec_context, curr_packet);
			}
			if (ret < 0)
			{
				if (ret == AVERROR_EOF)
				{
					if (stream_type == 0)
					{
						if (!m_loop_media_file)
						{
							mp_frameMgr->m_media_has_ended = true;
							terminal_flag = true;
							ReportLog("avcodec_send_packet: media ended\n");
						}
						else
						{
							int64_t ts(0), rel(0);
							Seek(ts, rel, false);
							ReportLog("avcodec_send_packet: media ending triggered reloop seek\n");
						}
					}

					av_packet_unref(curr_packet); // Free the packet that was allocated by av_read_frame 
					return;
				}
			}
			else
			{
				while (!ret)
				{
					uint32_t use_this_index = m_decompress_index % FFVIDEO_DECOMPRESSION_AVFRAME_COUNT;

					AVFrame* decompress_frame = mp_decompressed_frame[use_this_index];
					if (!decompress_frame)
					{
						// playback stopped and mp_decompressed_frame[] has already been deallocated
						av_packet_unref(curr_packet); // Free the packet that was allocated by av_read_frame 
						return;
					}

					ret = avcodec_receive_frame(mp_codec_context, decompress_frame);
					if (!ret) // returning 0 means success
					{
						int32_t stream_type = mp_frameMgr->m_stream_type;
						if (stream_type == 0) // media file
						{
							double play_pos = decompress_frame->best_effort_timestamp * m_timebase;
							mp_frameMgr->m_est_play_pos = play_pos; // store in atomic, is seconds
							mp_frameMgr->m_est_frame_num = (int32_t)(play_pos * m_expected_frame_rate);
							decompress_frame->display_picture_number = (int)(play_pos * m_expected_frame_rate);
						}
						else // USB and IP streams
						{
							double play_pos = mp_frameMgr->m_clock.s();
							mp_frameMgr->m_est_play_pos = play_pos; // store in atomic, is seconds
							mp_frameMgr->m_est_frame_num = (int32_t)mp_frameMgr->m_frames_received;
							decompress_frame->display_picture_number = (int32_t)mp_frameMgr->m_frames_received;
						}

						// the next routine, ProcessPacketToFrame(), uses m_decompress_index's advancing 
						// to identify a new frame to display.
						// By not advancing m_decompress_index, we can skip displaying the frame:
						bool skip_this_frame(false);
						//
						// if the discard flag is set in this frame, we decompress but do not display:
						if (mp_decompressed_frame[use_this_index]->flags & AV_FRAME_FLAG_DISCARD)
							skip_this_frame = true;
						//
						if (mp_frameMgr->m_seek_skip_count > 0)
						{
							skip_this_frame = true;
							mp_frameMgr->m_seek_skip_count--;
						}
						else if (mp_frameMgr->m_post_seek_nonkeyframeskip)
						{
							if (decompress_frame->key_frame == 1)
							{
								mp_frameMgr->m_post_seek_nonkeyframeskip = false;
								mp_frameMgr->m_post_seek_renders = 3;
								mp_frameMgr->m_post_seek_render_is_really_a_step = false;
							}
							else skip_this_frame = true;
						}

						if (!skip_this_frame)
							m_decompress_index++; // incrementing will trigger display 
					}
					else
					{
						if (ret == AVERROR_EOF)
						{
							if (mp_frameMgr->m_drain_mode)
							{
								mp_frameMgr->m_drain_complete = true;
							}

							av_packet_unref(curr_packet); // Free the packet that was allocated by av_read_frame 

							return;
						}
					}
				}
				if (ret == AVERROR_EOF && !m_loop_media_file)
					mp_frameMgr->m_drain_mode = true;
			}

		}	// end this packet has our video stream index 

	}	// end packet is not corrupt

	else if (curr_packet->flags & AV_PKT_FLAG_CORRUPT)
	{	
		av_log( mp_codec_context, AV_LOG_INFO, "packer_reader: corrupt frame\n" );
	}

	// Free the packet that was allocated by av_read_frame 
	av_packet_unref(curr_packet);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// This is the "packet reader" that spins reading media packets from the stream in its own thread.
// This logic transforms media packets into decompressed YUV video frames, stacking them
// for use by a FFVideo_Player (who sends them to the client.)
void FFVideo::VideoProcessLoop(void)
{
	const uint64_t loop_speed( 120 );	// this initial high value is only in effect during initial before-play-started buffering

	// we want to read media packets ASAP, so we'll check for new packets loop_speed times per second:
	uint64_t milliseconds = 1000 / loop_speed; // the demoninator is how many times per second we will loop

	uint64_t milliseconds_frame_display = 1000 / loop_speed; 

	uint64_t nanosleep_param = milliseconds * 10; // each ms is 10 nanosleep units

	bool stop_processing_packets = false;
	bool break_out_of_loop = false;

	uint32_t last_display_index = 9999;

	while (true)
	{
		if (m_stop_video_processing_loop)
		{
			break;
		}
		else
		{
			if (stop_processing_packets == false)
				ProcessPacket( milliseconds, stop_processing_packets );

			if (mp_frameMgr)
			{
				if (mp_frameMgr->m_drain_complete)
					break;

				mp_frameMgr->ProcessPacketToFrame( last_display_index, milliseconds_frame_display, break_out_of_loop );
				if (break_out_of_loop)
				{
					mp_frameMgr->m_drain_complete = true;
					mp_frameMgr->m_is_playing = false;
					mp_frameMgr->m_paused = false;
					break;
				}

				uint64_t next_milliseconds( milliseconds_frame_display );
				if (mp_frameMgr->m_stream_type == 0) // media file
				{
					// only media files can be paused:
					if (mp_frameMgr->IsPlaybackPaused() && mp_frameMgr->AnyPostSeekProcessingActive() == false)
						 next_milliseconds = 250;
				}
				milliseconds = next_milliseconds; 
				nanosleep_param = milliseconds * 10; // each ms is 10 nanosleep units
			}
		}
		nanosleep(nanosleep_param);
	}
	
	m_video_processing_loop_ended = true;
}

//////////////////////////////////////////////////////////////////////////////////////
// this is installed into libavformat as the callback for I/O interruption
int FFVideo_FrameMgr::interrupt_callback( void* context )
{
	FFVideo_FrameMgr* av_player = (FFVideo_FrameMgr*)context;
	if (av_player)
	{
		float now = av_player->m_clock.ms();
		if ((av_player->m_next_interrupt_timeout > 0.0f) && (now >= av_player->m_next_interrupt_timeout))
			return 1;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////
bool FFVideo::StartIPorMediaFile(std::string& printf_safe_vid_src)
{
	// if not NULL, a previous stream was left open:
	assert (mp_format_context == NULL); 

	// if multiple windows are opening at once, and any of them are USB, query_capture may be
	// enabled. That's how ffmpeg uses itself to report USB device information for use.  
	while (is_query_capture_on())
	{
		using namespace std::chrono_literals;
		// Should be done in a moment.
		std::this_thread::sleep_for(500ms);
	}

	// expects vid_src is already correct and will contain
	// either the rtsp/http/rtmp  camera string or a media file name

	av_log( mp_codec_context, AV_LOG_INFO, "StartIPorMediaFile: %s\n", m_vid_src.c_str() );

	// adds things to the option dictionary based upon stream type
	// also allocates mp_format_context to set an I/O callback
	setup_av_options(); 

	// mp_format_context will be set to a valid AVFormatContext if avformat_open_input() succeeeds
	if (avformat_open_input( &mp_format_context, m_vid_src.c_str(), NULL, &mp_opts ) !=0)
	{
		ReportLog("Could not media stream at %s.", m_vid_src.c_str() );
		return false; 
	}

	return true;
}


#define CHECK_USER_QUICK_TERMINATE if (!mp_frameMgr || (mp_frameMgr && mp_frameMgr->m_drain_mode) || !mp_format_context) {	m_is_opening = false;	return false; }

//////////////////////////////////////////////////////////////////////////////////////
bool FFVideo::StartStream( FFVIDEO_USB_Camera_Format* camera, double start_time )
{
	if (mp_frameMgr->IsPlaybackPaused())
	{
		UnPause();
		return true;
	}

	m_is_opening = true;

	std::string printf_safe_vid_src(m_vid_src.c_str());
	replaceAll(printf_safe_vid_src, "%", "%%");

	mp_frameMgr->PrepareForPlayback();

	// special set up for USB:
	if (mp_frameMgr->m_stream_type == 1) // 0=Media, 1=USB, 2=IP
	{
		if (!StartUSB( camera ))
		{
			m_is_opening = false;
			return false;
		}
	}
	// otherwise m_vid_src contains either the rtsp/http camera string or a media file name
	else
	{
		if (!StartIPorMediaFile(printf_safe_vid_src))
		{
			m_is_opening = false;
			return false;
		}
	}

	// "Start" is just the opening-of-the-src-logic...
	av_log( mp_codec_context, AV_LOG_INFO, "StartStream: %s\n", m_vid_src.c_str() );

	CHECK_USER_QUICK_TERMINATE

	// asking the library to generate presentation timestamps seems to help:
	mp_format_context->flags |= AVFMT_FLAG_GENPTS;


	CHECK_USER_QUICK_TERMINATE

	// REMEMBER THIS CALL HAS AN INTERNAL EXIT() SITUATION THAT NEEDS TO BE REMOVED!!
	AVDictionary** opts = setup_find_stream_info_opts(mp_format_context, NULL);
	int orig_nb_streams = mp_format_context->nb_streams;

	CHECK_USER_QUICK_TERMINATE

 	int32_t err = avformat_find_stream_info(mp_format_context, opts);

	for (int32_t i = 0; i < orig_nb_streams; i++)
		  av_dict_free(&opts[i]);
	av_freep(&opts);

	if (err < 0)
	{
		av_log(mp_codec_context, AV_LOG_WARNING, "%s: could not find codec parameters\n", printf_safe_vid_src.c_str());
		m_is_opening = false;
		return false;
	}
	

	CHECK_USER_QUICK_TERMINATE

	if (mp_format_context->pb)
		 mp_format_context->pb->eof_reached = 0; // copied over from ffplay.c

	// copied over from ffplay ffmpeg 4.2.2:
	if (mp_frameMgr->m_seek_by_bytes < 0)
	{
		mp_frameMgr->m_seek_by_bytes = !!(mp_format_context->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", mp_format_context->iformat->name);
	}

	// seek is only available to only to media files:
	if (mp_frameMgr->m_stream_type == 0)
	{
		if (start_time > 0.0)
		{
			mp_frameMgr->m_start_time = (int64_t)(start_time * AV_TIME_BASE);
		}
		
		if (mp_frameMgr->m_start_time != AV_NOPTS_VALUE)
		{
			int64_t timestamp;

			timestamp = mp_frameMgr->m_start_time;
			/* add the stream start time */
			if (mp_format_context->start_time != AV_NOPTS_VALUE)
				timestamp += mp_format_context->start_time;

			int32_t ret = avformat_seek_file(mp_format_context, -1, INT64_MIN, timestamp, INT64_MAX, 0);
			if (ret < 0)
			{
				av_log(NULL, AV_LOG_WARNING, "%s: could not seek to position %0.3f\n", m_media_fname.c_str(), (double)timestamp / AV_TIME_BASE);
			}
		}
	}

	CHECK_USER_QUICK_TERMINATE

	// pFormatCtx->streams[] is an array of stream pointers
	// pFormatCtx->nb_streams is how many streams we have
	//
	// Find the first video stream
	mp_video_stream = NULL;

	int32_t stream_index[AVMEDIA_TYPE_NB];						// used to find the video stream's index
	memset(stream_index, -1, sizeof(stream_index));

	stream_index[AVMEDIA_TYPE_VIDEO] = av_find_best_stream(mp_format_context, AVMEDIA_TYPE_VIDEO, stream_index[AVMEDIA_TYPE_VIDEO], -1, NULL, 0);

	CHECK_USER_QUICK_TERMINATE

	AVCodecParameters* codecPars = NULL;
	if (stream_index[AVMEDIA_TYPE_VIDEO] >= 0)
	{
		mp_video_stream = mp_format_context->streams[stream_index[AVMEDIA_TYPE_VIDEO]];

		codecPars = mp_video_stream->codecpar;
		if (codecPars->width)
		{
			// ffmpeg 4.2.2 version: we know the video resolution now:
			m_height = codecPars->height;
			m_width = codecPars->width;
			mp_frameMgr->m_im.Reallocate(m_height, m_width);
		}
	}
	if (mp_video_stream == NULL)
	{
		ReportLog("Failed to locate a video stream from that source.");
		m_is_opening = false;
		return false;
	}

	// Find the decoder for the video stream:
	const AVCodec* pCodec = avcodec_find_decoder(codecPars->codec_id);
	if (pCodec == NULL)
	{
		ReportLog("Unsupported codec.");
		m_is_opening = false;
		return false;
	}

	CHECK_USER_QUICK_TERMINATE

	// Codec can not be used directly, it must have a context allocated and params copied there:
	mp_codec_context = avcodec_alloc_context3(pCodec);
	if (!mp_codec_context)
	{
		ReportLog("Failed to allocate codec context.");
		m_is_opening = false;
		return false;
	}

	CHECK_USER_QUICK_TERMINATE

	if (avcodec_parameters_to_context( mp_codec_context, codecPars ) != 0)
	{
		ReportLog("Failed to copy codec context.");
		m_is_opening = false;
		return false;
	}

	AVRational& time_base = mp_video_stream->time_base;

	m_timebase = av_q2d( time_base ); // packet unit of time
	m_timebase_num = time_base.num;
	m_timebase_dem = time_base.den;
	
	m_expected_frame_rate = -1;
	if (mp_video_stream->avg_frame_rate.den != 0)
		 m_expected_frame_rate = av_q2d(mp_video_stream->avg_frame_rate);

	char* process_path = "stream";
	double possible_duration(-1.0);
	//
	m_expected_frame_rate = 0;
	switch (mp_frameMgr->m_stream_type)
	{
	case 0: // 0=Media, 1=USB, 2=IP
	{
		// only media files have a duration, so print it here: 
		int64_t ts;
		if (GetDuration(ts))
		{
			possible_duration = (double)ts / (double)AV_TIME_BASE; // convert to seconds for possible use by StreamInfoCallback

			int hours(0), minutes(0), seconds(0);
			if (CalcTimestampParts(ts, hours, minutes, seconds))
			{
				ReportLog("Media file duration is %2d:%02d:%02d", hours, minutes, seconds);
			}
		}

		if (mp_video_stream->avg_frame_rate.den != 0)
			m_expected_frame_rate = av_q2d(mp_video_stream->avg_frame_rate);
		else
		{
			m_expected_frame_rate = 24.0;
			process_path = "media default";
		}
	}
	break;

	case 1:
		if (camera)
			m_expected_frame_rate = camera->m_max_fps;
		else
		{
			m_expected_frame_rate = 15.0;
			process_path = "usb default";
		}
		break;

	default:
	case 2:
		if (mp_video_stream->avg_frame_rate.den != 0)
			m_expected_frame_rate = av_q2d(mp_video_stream->avg_frame_rate);
		else
		{
			m_expected_frame_rate = 30.0;
			process_path = "IP default";
		}
		break;
	}
	ReportLog("expected_frame_rate is %1.9f, from %s", m_expected_frame_rate, process_path);

	CHECK_USER_QUICK_TERMINATE

	// Open codec
	if (avcodec_open2( mp_codec_context, pCodec, &mp_opts ) < 0)
	{
		ReportLog( "Failed to open codec." );
		m_is_opening = false;
		return false;
	}

	// throw away useless packets, like size 0 in AVI files:
	mp_video_stream->discard = AVDISCARD_DEFAULT; 

	// any non video streams, discard the packets:
	for (uint32_t i = 0; i < (uint32_t)mp_format_context->nb_streams; i++)
		  if (mp_format_context->streams[i] != mp_video_stream)
			   mp_format_context->streams[i]->discard = AVDISCARD_ALL;

	CHECK_USER_QUICK_TERMINATE

	m_decompress_index = 0;		// both these refer to the mp_decompressed_frame[] array
	m_display_index = 0;

	// in case newer ffmpeg 4.2+ versions do not provide this info earlier:
	if (m_width == 0)
	{
		m_height = mp_codec_context->height;
		m_width  = mp_codec_context->width;
		mp_frameMgr->m_im.Reallocate(m_height, m_width);
	}

	// if auto-frame interval then we scan the auto interval threshold profile:
	if (m_auto_frame_interval)
	{
		for (size_t i = 0; i < m_auto_interval_thresholds.size(); i++)
		{
			if (m_width >= (int32_t)m_auto_interval_thresholds[i])
			{
				mp_frameMgr->m_frame_interval = m_auto_intervals[i];
				break;
			}
		}
	}

	CHECK_USER_QUICK_TERMINATE

	// Allocate video frames for decompressed stream frames:
	for (int i = 0; i < FFVIDEO_DECOMPRESSION_AVFRAME_COUNT; i++) 
	{
		mp_decompressed_frame[i] = av_frame_alloc();
		if (mp_decompressed_frame[i] == NULL)
		{
			ReportLog( "Failed to allocate decompression frame storage." );
			m_is_opening = false;
			return false;
		}
	}

	CHECK_USER_QUICK_TERMINATE

	// ready to start streaming

	if (mp_frameMgr->mp_frame_dest->m_frame_export_interval > 0)
	{
	  // could set this here, but it should be set with all other inits. See if I forget...
	  assert( mp_frameMgr->mp_frame_dest->m_frame_export_count == 0 );

		mp_frameMgr->mp_frame_dest->m_frame_exporter.StartExporter();
	}

	// needed on 2nd, 3rd and so on plays as the thread ends when the stream ends
	if (!IsRunning())
	{
		mp_videoProcessingThread = new std::thread( &FFVideo::VideoProcessLoop, this );
	}
	mp_frameMgr->m_is_playing = true; // set flag to begin packet read processing

	m_is_opening = false;							// no longer opening, we're opened
  return true;
}

#undef CHECK_USER_QUICK_TERMINATE


//////////////////////////////////////////////////////////////////////////////////////
void FFVideo::SetDisplayFrameCallback(DISPLAY_FRAME_CALLBACK_CB p_process_frame, void* p_object)
{
	if (mp_frameMgr)
	{
		std::unique_lock<std::shared_mutex> lock(mp_frameMgr->m_cb_lock);
		mp_frameMgr->mp_frame_dest->mp_process_frame = p_process_frame;
		mp_frameMgr->mp_frame_dest->mp_process_frame_object = p_object;
		lock.unlock();
	}
}

//////////////////////////////////////////////////////////////////////////////////////
void FFVideo::SetStreamTerminatedCallBack(TERMINATED_STREAM_CALLBACK_CB p_stream_term, void* p_object)
{
	if (mp_frameMgr)
	{
		std::unique_lock<std::shared_mutex> lock(mp_frameMgr->m_cb_lock);
			mp_frameMgr->mp_term_callback = p_stream_term;
			mp_frameMgr->mp_term_object = p_object;
		lock.unlock();
	}
}

//////////////////////////////////////////////////////////////////////////////////////
void FFVideo::SetStreamFinishedCallBack(STREAM_FINISHED_CALLBACK_CB p_stream_fin, void* p_object)
{
	if (mp_frameMgr)
	{
		std::unique_lock<std::shared_mutex> lock(mp_frameMgr->m_cb_lock);
			mp_frameMgr->mp_stream_ended_callback = p_stream_fin;
			mp_frameMgr->mp_stream_ended_object = p_object;
		lock.unlock();
	}
}

//////////////////////////////////////////////////////////////////////////////////////
void FFVideo::SetStreamLoggingCallback(STREAM_LOGGING_CALLBACK_CB p_stream_logger, void* p_object)
{
	if (mp_frameMgr)
	{
		std::unique_lock<std::shared_mutex> lock(mp_frameMgr->m_cb_lock);
			mp_frameMgr->mp_stream_logging_callback = p_stream_logger;
			mp_frameMgr->mp_stream_logging_object = p_object;
		lock.unlock();
	}
}






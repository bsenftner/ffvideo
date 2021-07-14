

/////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdarg.h>

#include "ffvideo.h"

#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


//////////////////////////////////////////////////////////////////////////////////////
// Constructor for class that delivers frames to the client
FFVideo_FrameDestination::FFVideo_FrameDestination(FFVideo_FrameMgr* parent)
{
	mp_parent = parent;
	//
	m_frame_interval = 1;									// needed by frame callback
	m_frame_count = 0;
	//
	m_frame_exporter.mp_parent = this;		// needed by frame export callback
	m_frame_export_interval = 0;
	m_frame_export_count = 0;

	m_vflip = true;

	// client callbacks:
	mp_process_frame = NULL;
	mp_process_frame_object = NULL;

	mp_frame_filter = new FFVIDEO_FrameFilter();

	m_scrub_pos = -1;										// client position viewing the scrub buffer
	m_scrub_index = -1;
	m_scrub_max_size = 30;										// defaults to 30 frames
	m_scrub_frames.resize(m_scrub_max_size);  // preallocate the vector 
	for (int32_t i = 0; i < m_scrub_max_size; i++)
	{
		m_scrub_frames[i].m_im.Clone(mp_parent->m_im);
		m_scrub_frames[i].m_frame_num = 0;
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Destructor for class that delivers frames to the client
FFVideo_FrameDestination::~FFVideo_FrameDestination()
{
	delete mp_frame_filter;

	m_frame_exporter.StopExporter();
}

/////////////////////////////////////////////////////////////////////////////////////
void FFVideo_FrameDestination::DeliverFrameToClient(
	bool first_frame,								// flag identifying if this is the first frame delivered from a new stream
	AVFrame* src_frame,							// raw frame as delivered from who knows where, could be a partial frame
	uint32_t display_index,					// display index of frame
	int32_t estimated_frame_number)	// what frame number in the media it is supposed to be
{
	FFVideo_Image& im = mp_parent->m_im;

	if (IsEmptyAVrame(src_frame))
	{
		// should not happen, but poorly encoded video happens
		m_frame_count++;
		return;
	}

	FFVideo* p_root = mp_parent->mp_parent;

	// always apply frame filtering because this also compensates for partial frames and corrupt frames:
	std::shared_lock<std::shared_mutex> rlock(p_root->m_post_process_lock);
	int ret = mp_frame_filter->FilterFrame(p_root->mp_format_context, p_root->mp_video_stream, src_frame, p_root->m_post_process );
	rlock.unlock();
	if (ret < 0)
		return;

	// frame filtering can change our output resolution:
	if (src_frame->width != im.m_width || src_frame->height != im.m_height)
	{
		im.Reallocate( src_frame->height, src_frame->width );

		// only media files have scrub buffer when paused support:
		if (mp_parent->m_stream_type == 0)
		{
			m_scrub_index = -1; // I think this resets the scrub buffer; that's what needs to happen here; resolution just changed. 
		}
	}

	// has the lib client installed a frame or export frame callback? ?

	bool do_frame_callback = ((first_frame) || ((display_index % (uint32_t)m_frame_interval) == 0));
	     do_frame_callback = (do_frame_callback && mp_process_frame);
  //
	bool do_frame_export = ((m_frame_export_interval > 0) && ((first_frame) || ((display_index % (uint32_t)m_frame_export_interval) == 0)));

	if (do_frame_callback || do_frame_export || (mp_parent->m_stream_type == 0))
	{
		// we're delivering this frame somewhere, so do the frame's prep:
		// (if a media file, we want the scrub buffer to hold every frame, 
		// regardless of frame_interval or frame_export_interval)

		int32_t true_bytes_per_row = src_frame->width * 4; // each RGBA is 4 bytes

		// check if frame format conversion gave us pixels rows the wrong length:
		if (src_frame->linesize[0] != true_bytes_per_row)
		{
			// this case only seems to happen with WMV and AVI files; not all of them, only some. apparently expected behavior!!!

			if (src_frame->linesize[0] > true_bytes_per_row)
			{
				for (int32_t i = 0; i < src_frame->height; i++)
				{
					// copy 1 row of RGBA pixels into our image storage:
					std::memcpy(&im.mp_pixels[i * src_frame->width], &src_frame->data[0][i * src_frame->linesize[0]], true_bytes_per_row);
				}
			}
			else
			{
				return; // rows given are too short, so we're going to abandon them. This case does not appear to occur.
			}
		}
		else
		{
			// copy RGBA pixels into our image storage:
			std::size_t pixels_size = (std::size_t)src_frame->height * (std::size_t)src_frame->width * 4;
			std::memcpy(im.mp_pixels, src_frame->data[0], pixels_size);
		}

		if (m_vflip) // library client can turn this bool false
		{
			im.MirrorVertical();
		}

		// only media files have scrub buffer when paused support:
		if (mp_parent->m_stream_type == 0)
		{
			// store this frame inside the "scrub frames", removing oldest if overflowing:
			//
			int32_t index = ++m_scrub_index;						// location for new frame
			if (index >= m_scrub_max_size)
			{
				// we've overflowed
				index = index % m_scrub_max_size;
			}
			m_scrub_frames[index].m_im.Clone(im);
			m_scrub_frames[index].m_frame_num = estimated_frame_number;

			// we've been asked to deliver a frame to the client. However, we might be backwards in time due to frame scrubbing.
			// if we're back in time, deliver the back in time frames before delivering the frame we were asked to deliver:
			if (m_scrub_pos > -1)
			{
				while (--m_scrub_pos > -1)
				{
					int32_t scrub_max_size = m_scrub_max_size; // because atomic

					int32_t index = (m_scrub_index - m_scrub_pos) % scrub_max_size;

					std::shared_lock<std::shared_mutex> slock(mp_parent->m_cb_lock);
					if (mp_process_frame)
						(mp_process_frame)(mp_process_frame_object, m_scrub_frames[index].m_im, m_scrub_frames[index].m_frame_num);
					slock.unlock();
				}
			}
			m_scrub_pos = -1; // means no scrub frames
		}
	}


	// if the frame goes anywhere, lock:
	if (do_frame_callback || do_frame_export)
	{
		std::shared_lock<std::shared_mutex> frlock(mp_parent->m_cb_lock);

		// if the frame is being delivered to the client's frame callback:
		if (do_frame_callback)
		{
			(mp_process_frame)(mp_process_frame_object, im, estimated_frame_number);
		}

		// if the frame is being exported:
		if (do_frame_export)
		{
			m_frame_export_count++;

			m_frame_exporter.Add(im, estimated_frame_number, m_frame_export_count);
		}

		frlock.unlock();
	}

	m_frame_count++;
}

void FFVideo_FrameExporter::ExportProcessLoop(void)
{
	uint64_t milliseconds = 1000 / 120; // the demoninator is how many times per second we will loop

	uint64_t nanosleep_param = milliseconds * 10; // each ms is 10 nanosleep units

	while (true)
	{
		if (m_stop_export_processing_loop)
		{
			break;
		}
		else
		{
			std::shared_lock<std::shared_mutex> rlock(m_queue_lock);
			bool work_to_do = !m_exportQue.empty();
			rlock.unlock();

			while (work_to_do)
			{
				std::unique_lock<std::shared_mutex> lock(m_queue_lock);
				FFVideo_ExportFrame ef = m_exportQue.front();
				m_exportQue.pop();
				work_to_do = !m_exportQue.empty();
				lock.unlock();

				float	  scale_factor = mp_parent->m_export_scale;
				int32_t quality = mp_parent->m_export_quality;
				bool	  save_success = true;

				// we do not scale up in this app, so anything above this is treated as 1.0f
				if (scale_factor >= 0.9999f)
				{
					save_success = ef.m_im.SaveJpgTurbo(ef.m_fname.c_str(), quality);
				}
				else
				{
					int32_t       rescaled_width  = (int32_t)((float)ef.m_im.m_width * scale_factor + 0.5f);
					int32_t				rescaled_height = (int32_t)((float)ef.m_im.m_height * scale_factor + 0.5f);

					ef.m_im.Rescale( rescaled_height, rescaled_width );
					ef.m_im.SaveJpgTurbo(ef.m_fname.c_str(), quality);
				}

				if (mp_export_frame_cb)
				{
				  // at this point "save_success" tell if more saving will continue...
					// verify there is more work to do after this image export, so we can deliver
					// a true "status" - are there more exports to be expected? 
					bool status( save_success );
					if (!work_to_do)
					{
						if (mp_parent->mp_parent->m_drain_complete)
							 status = false;
					}

					(mp_export_frame_cb)(mp_export_frame_object, ef.m_frame_num,ef.m_export_num,  ef.m_fname.c_str(), status);
				}

				if (!save_success)
				{
					mp_parent->m_frame_export_interval = -1;	// disable, -1 signals disabled in error
					m_stop_export_processing_loop = true;
					break;
				}
			}
		}
		nanosleep(nanosleep_param);
	}

  m_export_processing_loop_ended = true;
}

//////////////////////////////////////////////////////////////////////////////////////
// Constructor for class that delivers the video frames to the library client. 
// Whereas FFVideo handles the reading of media stream packets, this class 
// takes over after the packet has been read, handling the conversion to frames, 
// and delivering the frames to the client. 
FFVideo_FrameMgr::FFVideo_FrameMgr(FFVideo* parent)
{
	mp_parent = parent;
	m_stream_type = 0;			// 0=Media, 1=USB, 2=IP
	m_is_playing = false;
	m_paused = false;
	m_drain_mode = false;
	m_drain_complete = false;
	m_media_has_ended = false;
	m_stream_has_died = false;

	m_first_frame = true;
	m_frames_received = 0;
	m_fps_frames_received = 0;

	m_clock.Start();

	m_fps = 0.0f;
	m_frame_interval = 1;
	m_est_play_pos = 0.0;
	m_est_frame_num = 0;
	m_start_time = AV_NOPTS_VALUE;

	m_seek_by_bytes = -1;
	m_seek_req = false;	// no seek request
	m_seek_flags = 0;
	m_seek_anchor = -1;
	m_seek_pos = 0;
	m_seek_rel = 0;
	m_seek_skip_count = 0;		// after a seek, frames left to clear from old position
	m_post_seek_nonkeyframeskip = false;
	m_post_seek_renders = 0;
	//
	m_post_seek_render_is_really_a_step = false;

	m_next_interrupt_timeout = 0.0f;	// gets set by SetNextIOTimeout( real secs )
	//
	m_open_timeout = 0.0f;	// default to no user defined I/O timeout when opening a stream
	m_read_timeout = 0.0f;	// default to no user defined I/O timeout when reading a stream

	// end-user stream ended/terminated callbacks:
	mp_term_callback = NULL;
	mp_term_object = NULL;
	//
	mp_stream_ended_callback = NULL;
	mp_stream_ended_object = NULL;
	//
	mp_stream_logging_callback = NULL;
	mp_stream_logging_object = NULL;

	mp_frame_dest = new FFVideo_FrameDestination(this);
}

//////////////////////////////////////////////////////////////////////////////////////
FFVideo_FrameMgr::~FFVideo_FrameMgr()
{
	if (mp_frame_dest)
	{
		delete mp_frame_dest;
		mp_frame_dest = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////////////////
void FFVideo_FrameMgr::HandleSeekRequests(void)
{
	// seek only available to media files:
	if (m_stream_type != 0)
		return;
	// can only be media file now

	bool seek_req = m_seek_req; // because atomic
	if (seek_req == false)			// no seek request
		return;

	// make sure prev seek's work is completed:
	if (AnyPostSeekProcessingActive())
		return;

	// OKAY! we have a seek and any previous seek's work has compelted:
	int64_t seek_target = m_seek_pos;
	int64_t seek_min = m_seek_rel > 0 ? seek_target - m_seek_rel + 2 : INT64_MIN;
	int64_t seek_max = m_seek_rel < 0 ? seek_target - m_seek_rel - 2 : INT64_MAX;
	// FIXME the +-2 is due to rounding being not done in the correct direction in generation of the seek_pos/seek_rel variables


	int nh(0), nm(0), ns(0);
	double now_posd = m_est_play_pos;		// seconds
	int64_t now_pos = (int64_t)(now_posd * 1000000);	// now microseconds, same as timestamps
	bool now_good = (now_posd >= 0) ? mp_parent->CalcTimestampParts(now_pos, nh, nm, ns) : false;

	int dh(0), dm(0), ds(0);
	if (mp_parent->CalcTimestampParts(seek_target, dh, dm, ds))
	{
		mp_parent->ReportLog("avformat_seek_file: from %02d:%02d:%02d to %02d:%02d:%02d", nh, nm, ns, dh, dm, ds);
	}
	else
	{
		mp_parent->ReportLog("avformat_seek_file: pos %" PRId64, seek_target);
	}

	int32_t ret = avformat_seek_file(mp_parent->mp_format_context, -1, seek_min, seek_target, seek_max, m_seek_flags);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "%s: error while seeking\n", mp_parent->m_media_fname.c_str());
		mp_parent->ReportLog("avformat_seek_file: seek_target error!\n");
	}
	else
	{
		m_seek_skip_count = 2;
		m_post_seek_nonkeyframeskip = true;

		// we performed a seek(), so eliminate any stored scrub frames for paused stepping backwards:
		mp_frame_dest->m_scrub_pos = -1; // means no scrub frames
	}
	m_seek_req = false; // seek() request completed, just post seek skips & renders yet to do
}

#define PPTF_EARLY_EXIT if(!decompressed_frame){terminal_flag=true;m_drain_complete=true;return;}

//////////////////////////////////////////////////////////////////////////////////////
void FFVideo_FrameMgr::ProcessPacketToFrame(uint32_t& last_display_index,
	uint64_t& wait_milliseconds_for_next_frame_display,
	bool& terminal_flag)
{
	if (IsPlaybackPaused())
	{
		// we're paused, but let's check if we want seek refreshes:
		if (AnyPostSeekProcessingActive() == false)
		{
			// no, no seek refreshes are needed to complete a prior seek(), so we can get out of here, 'cause we're paused: 
			return;
		}
	}

	uint32_t raw_display_index = mp_parent->m_display_index; // the index of our frame, 0 based

	// make sure we are working on a new frame: 
	if (raw_display_index == last_display_index)
		return;

	if (!m_drain_mode)
	{
		// make sure there are frame_interval+1 frames ahead of us:
		if (raw_display_index + m_frame_interval + 1 >= mp_parent->m_decompress_index)
			return;
	}

	// when this is true, our first frame and all there after will have 
	// m_frame_interval frames decompressed ahead of our current display frame:
	if (m_first_frame)
	{
		mp_frame_dest->m_frame_count = 0;
		mp_frame_dest->m_frame_interval = m_frame_interval;
		mp_frame_dest->m_scrub_pos = -1;
	}

	// prevent media files from advancing when paused:
	if (m_stream_type == 0 && (AnyPostSeekProcessingActive() == false) && m_paused)
		return;

	m_fps_frames_received++;
	m_fps = (float)m_fps_frames_received / m_clock.s();
	//
	m_frames_received = raw_display_index + 1;

	// figure out which decompression buffer to use:
	uint32_t decompress_index = raw_display_index % FFVIDEO_DECOMPRESSION_AVFRAME_COUNT;

	AVFrame* decompressed_frame = mp_parent->mp_decompressed_frame[decompress_index];

	// it is possible for client to stop playback and delete our structs out from under us:
	PPTF_EARLY_EXIT

		// we play as fast as possible, minimal wait between frames: 
		wait_milliseconds_for_next_frame_display = 1; // assume USB/IP stream, with unknown true rate of delivery
	if (m_stream_type == 0)
	{
	//	wait_milliseconds_for_next_frame_display = 0;								// media files play as fast as possible
		m_seek_anchor = decompressed_frame->best_effort_timestamp;	// remember where we are in the file now
	}

	mp_parent->m_display_index++;

	if (IsPlaybackPaused() && (m_stream_type != 0)) // can only be a live stream, this is after frame has been consumed
		return; // live streams advance their frame when paused

	// it is possible for client to stop playback and delete our structs out from under us:
	decompressed_frame = mp_parent->mp_decompressed_frame[decompress_index];
	PPTF_EARLY_EXIT

	if (!m_drain_complete)
	{
		// handle user callbacks: 
		mp_frame_dest->DeliverFrameToClient(m_first_frame, decompressed_frame, raw_display_index, raw_display_index);
		m_first_frame = false;
	}

	// remember last frame we delivered to the client:
	last_display_index = raw_display_index;

	// check for needing to display the 1st frame at new seek destination:
	if (m_post_seek_renders > 0)
	{
		m_post_seek_renders--;

		if (m_post_seek_render_is_really_a_step == false)
		{
			// we performed a seek(), so eliminate any stored scrub frames for paused stepping backwards:
			mp_frame_dest->m_scrub_pos = -1; // means no scrub frames
		}
		// m_post_seek_render_is_really_a_step must be on for a single frame step, so turn it back off:
		else m_post_seek_render_is_really_a_step = false;
	}

	if (m_drain_mode && (raw_display_index >= mp_parent->m_decompress_index))
	{
		terminal_flag = true;	// we are all done!!
		m_drain_complete = true;

		bool callback_error(false);

		if (m_media_has_ended)
		{
			std::shared_lock<std::shared_mutex> lock(m_cb_lock);

			if (mp_stream_ended_callback)
			{
				try
				{
					(mp_stream_ended_callback)(m_frames_received, mp_stream_ended_object);
				}
				catch (...)
				{
					callback_error = false;
				}
			}

			lock.unlock();
		}
		if (m_stream_has_died)
		{
			std::shared_lock<std::shared_mutex> lock(m_cb_lock);

			if (mp_term_callback)
			{
				try
				{
					(mp_term_callback)(mp_term_object);
				}
				catch (...)
				{
					callback_error = false;
				}
			}

			lock.unlock();
		}
	}
}

bool FFVideo_FrameMgr::SetFrameExporting(
		int32_t export_interval, std::string& export_dir, std::string& export_base, 
		float scale, int32_t quality, 
		EXPORT_FRAME_CALLBACK_CB frame_export_cb, void* frame_export_object )
{
	// must be set before playback has begun: 
	if (HasPlaybackStarted())
		return false;

	if (export_interval > 0)
	{
		// we are enabling frame exporting. Examine the directory first:

		// bad directory?
		if (export_dir.size() < 1)
		{
			mp_frame_dest->m_frame_export_interval = 0; // disable
			return false;
		}

		// get the directory inside a boost::filesystem::path
		boost::filesystem::path dir(export_dir);

		// export directory must already exist:
		if (!boost::filesystem::is_directory(dir))
		{
			mp_frame_dest->m_frame_export_interval = 0; // disable
			return false;
		}
		
		mp_frame_dest->m_frame_export_interval = export_interval;
		mp_frame_dest->m_export_dir = export_dir + std::string("\\");
		mp_frame_dest->m_export_base = export_base;
		mp_frame_dest->m_export_scale = scale;
		mp_frame_dest->m_export_quality = quality;
		mp_frame_dest->m_frame_exporter.mp_export_frame_cb = frame_export_cb;
		mp_frame_dest->m_frame_exporter.mp_export_frame_object = frame_export_object;
	}
	else // we are disabling frame exporting
	{
		mp_frame_dest->m_frame_export_interval = 0;
	}

	return true;
}
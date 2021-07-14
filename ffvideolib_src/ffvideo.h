#pragma once
#ifndef _FFVIDEO_H_
#define _FFVIDEO_H_


#include <Windows.h>

#include "ffvideo_frameMgr.h"

///////////////////////////////////////////////////////////////////////////////////////
// This is the object clients use to operate this library. One should not use 
// any the other classes defined in this header or ffvideoDISPLAY_FRAME_CALLBACK_CB_low.h to operate this lib. 
///////////////////////////////////////////////////////////////////////////////////////
class FFVideo 
{
public:
	// required form for thread constructor
	FFVideo() : m_auto_frame_interval(false), m_capture_log(false), m_expected_frame_rate(0),
		m_video_processing_loop_ended(false), m_stop_video_processing_loop(false), mp_videoProcessingThread(0),
		m_usb_pin(0), m_width(0), m_height(0), mp_format_context(0), mp_opts(0), mp_packet(0),
		mp_video_stream(0), mp_codec_context(0), mp_orig_codec_context(0), m_timebase(0), mp_frameMgr(0) {};
	//
	// 2nd required for for thread constructor
	FFVideo(const FFVideo& obj){}
	
	// a utility that tells if the video processing thead is running in the background:
	bool IsRunning(void)
	{ 
		if (!mp_videoProcessingThread) 
			 return false;

		// set to true entering Process thread, goes false when exiting Process:
		bool ret = !m_video_processing_loop_ended; 

		return ret; 
	}

	~FFVideo();

	// Options for one new instance of the video playback library:
	//	The defaulted parameters may be ignored. They are how options are passed to each
	//  instance of the library, and the playback of the one stream the instance will control. 
	//  Current options are:
	//     vflip        video sources can be up side down, most are from an OpenL buffer perspective
	//									this param defaults to true, but can be set false to disable
	//     debug        Activates verbose logging to ffvideo.log
	//
	void Initialize( bool vflip=true, bool debug=false );

	// 3 steps to use this library:
	//	step 1) setup the callbacks below before using the functional interface
	//  step 2) use the function calls to play video streams
	// (step 3) receive calls from this library to your callbacks delivering video frames

	// Step 1 interfaces to use this library: set up your library client callbacks:
	//
	// These are the callbacks that need to be setup before calling the playback functions below:

	// display frame callback, when set, it will be called each frame (including first frame)
	typedef void(*DISPLAY_FRAME_CALLBACK_CB)(void* p_object, FFVideo_Image& im, int32_t frame_num);
	//
	void SetDisplayFrameCallback(DISPLAY_FRAME_CALLBACK_CB p_process_frame, void* p_object);

	// note: there is also a "frame export callback" setup by SetFrameExportingParams(), below

	// unexpected stream termination callback, for USB and IP cameras if set,
	// your function is called when the USB/IP stream unexpectedly stops, 
	// i.e. when camera unexpectedly disconnects, cable is disconnected, or the network drops
	typedef void(*TERMINATED_STREAM_CALLBACK_CB)(void* p_object);
	//
	void SetStreamTerminatedCallBack(TERMINATED_STREAM_CALLBACK_CB p_stream_term, void* p_object);

	// media stream finished callback, when a playing media file this is called after last frame is processed
	typedef void(*STREAM_FINISHED_CALLBACK_CB) (uint32_t frame_num, void* p_object);
	//
	void SetStreamFinishedCallBack(STREAM_FINISHED_CALLBACK_CB p_stream_fin, void* p_object);

	// logging messages callback for use by library client, log messages generated 
	// by library are sent thru this to the client's logic:
	typedef void(*STREAM_LOGGING_CALLBACK_CB) (const char* msg, void* p_object);
	//
	void SetStreamLoggingCallback(STREAM_LOGGING_CALLBACK_CB p_stream_logger, void* p_object);

	// when playing a video stream, one of the optional parameters is a "frame interval" - number of frames
	// to skip delivery to the client, in an even interval, between sending frames. Normal playback has a
	// frame interval of 1 - send every frame. Set to 2 and every other frame is delivered to the client.  
	// This is useful if the lib client has high procesing overhead, and the video resolution is high. 
	// However, in the case of IP video, the resolution to be received is not known when Play() is initiated. 
	// To handle this, the lib client can specity two parallel vectors of integers:
	//		std::vector<uint32_t> thresholds;	video width equal and above which to use the corresponding frame interval
	//		std::vector<uint32_t> intervals;  frame intervals to use for each resolution threshold
	// The default auto frame interval profile is:
	//		thresholds: 7680, 5120, 3840, 1920 
	//		intervals:	2, 1, 1, 1
	// Note: thresholds must be in descending order
	bool SetAutoFrameIntervalProfile(std::vector<uint32_t>& thresholds, std::vector<uint32_t>& intervals);

	// similar to the frame_interval controlling how often the frame display callback is triggered, this can
	// enable/diable frame exporting on an interval of the frames decompressed from the stream.
	// Default is disabled, this is enabled by setting the export_interval > 0. Disable by setting export_interval < 1.
	// The export directory must be set, exist, the permission to write into the directory must be present. 
	// Frames are exported as jpg files using the quality passed as the jpg compression. 
	// The filename is the export dir + export_base + an ISO 8601 timestamp, with milliseconds. 
	// Failing to write triggers frame exporting to disable. 
	bool SetFrameExportingParams(int32_t export_interval, std::string& export_dir, std::string& export_base, 
															 float scale = 1.0f, int32_t quality = 80, 
															 EXPORT_FRAME_CALLBACK_CB frame_export_cb = NULL, void* frame_export_object = NULL);

	void GetFrameExportingParams(int32_t& export_interval, std::string& export_dir, std::string& export_base, 
															 float& scale, int32_t& quality);

	// returns true if frame exporting was enabled and the write_fail_limit exceeded, disabling frame exports:
	bool GetFrameExportingError(void);

	// returns number of images waiting to be written to disk:
	int32_t GetFrameExportingQueueSize(void);

	// step 2 interfaces to use this library: use these to begin and modify video playback:

	// Before opening any video stream, or while reading from an opened video stream, 
	// one may set timeouts for I/O operations to be requested. If no timeouts are set,
	// the default behavior is:
	//		When opening a stream, the open timeout defaults to 12.0 seconds
	//		When reading packets & playing video, the read timeout is reset to 5.0 secs each frame
	//
	// When opening a stream, two I/O operations take place. First the stream is actually opened.
	// Next a portion of the stream is read, to determine the format, codec, frame rate and so on.
	// That is why the read timeout defaults to 12.0 seconds - to cover both I/O operations, even
	// when dealing with a congested, poor quality Internet connection. 
	// 
	// SetOpenTimeout( float seconds ) sets how long to allow for opening and negotiating the stream;
	// When using SetOpenTimeout() be sure to allow enough time for the initial opening and format
	// discovery. That suggests calling SetOpenTimeout() with a large-ish duration before opening.
	//
	// SetReadTimeout( float seconds ) sets how long to allow for reading the next video frame; this
	// may be called before the stream is opened, as well as during any frame callback. The behavior
	// is to reset an internal timeout each frame with the duration provided by SetReadTimeout();
	// meaning the timeout is per frame, and 'sticky' - use of SetReadTimeout() persists for each
	// frame read, until SetReadTimeout() is called again, the stream ends, or is closed.
	//
	void SetOpenTimeout( float duration = 5.0f ) // if set to 0.0f, no open timeout will occur
	{
		if (mp_frameMgr)
			 mp_frameMgr->SetOpenTimeout( duration );
	}		
	float GetOpenTimeout( void ) 
	{
		return (mp_frameMgr) ? mp_frameMgr->GetOpenTimeout() : -1.0f;
	}
	//
	// Note the read timeout tends to only be triggered with IP streams, as the USB video subsystem appears
	// to notify libavformat's logic and we quickly receive an EOF. Whereas bad IP streams actually timeout:
	//
	void SetReadTimeout( float duration = 5.0f ) // if set to 0.0f, no read timeout will occur
	{
		if (mp_frameMgr)
			 mp_frameMgr->SetReadTimeout( duration );
	}		
	float GetReadTimeout( void ) 
	{
		return (mp_frameMgr) ? mp_frameMgr->GetReadTimeout() : -1.0f;
	}

	// if using a USB camera, here are some utilities:
	bool GetUSB_CameraNames(std::vector<std::string>& usb_names); // returned usb_names is in USB PIN order, allowing PIN use as index
	bool GetCameraPixelFormats( const char* cameraName, std::vector<FFVIDEO_USB_Camera_Format>& usb_formats );
	
	// these three are only meaningful for media files:
	bool IsLooping(void) { return m_loop_media_file; }
	void SetMediaFileLoopFlag(bool flag) { m_loop_media_file = flag; }
	//
	// how many frames can be stepped backwards when a media file is paused:
	// call before requesting playback; if not called, this defaults to 30. 
	// If skipping more than single frames is used, the scrub buffer may contain displayed frames as well as potentially not-displayed
	// frames that were decompressed in order to reach the requested frame, due to FFmpeg's seeks being to the nearest keyframe. 
	void SetScrubBufferSize(int32_t size);

	void SetPostProcessFilter( std::string& filter );

	// use one of these 3 functions to begin a video stream's playback:
	// by setting frame_interval to 0, auto-frame_interval is enabled. This is the auto-selection of a frame interval based upon
	// frame resolution. See SetAutoFrameIntervalProfile(), above, for details of setting a custom auto-frame_interval profile. 
	bool OpenUSBCamera(int32_t pin = 0, int32_t frame_interval = 1, FFVIDEO_USB_Camera_Format* camera = NULL);
	bool OpenIPCamera(const std::string& url, int32_t frame_interval = 1);
	bool OpenMediaFile(const std::string& fname, int32_t frame_interval = 1, bool loop_flag = false, double start_offset = 0.0);

	// Use one of these three functions to modify active playback. Note that after StopStream() 
	// one of the three OpenUSBCamera(), OpenIPCamera(), or OpenMediaFile() must be called to 
	// receive frames from that same video source again.
	void Pause();				// only valid for media files, if not a media file call routes to StopStream();
	//
	// Step() only works for media files when they are paused:
	// triggers the FrameCallback to be called in the direction indicated.
	// returns true if successful.
	// Will be unsuccessful if:
	//     * backwards buffer is exhaused,
	//     * called when playing,
	//     * called when yet performing post-seek-renders (post seek work)
	bool Step(FFVIDEO_FRAMESTEP_DIRECTION direction);
	//
	void UnPause();	
	void StopStream();	
	void KillStream();	// regardless of state, kill everything
		
	// Seek() functions return a bool for success (true) or failure (false),
	// they will only return failure if the media duration is not known when called, or not a media file stream.
	//
	// Seek relative to the current position: (closest possible)
	bool SeekRelative(double offset); // +/- seconds
	//
	// Seek to a normalized position: (closest possible)
	bool SeekNormalized(double normalized_position); // if not normalized, seeks to either start or end, depending upon passed value
	//
	// Seek to a frame number: (closest possible)
	bool SeekToFrame(uint32_t frame_number);
	//
	// lowest level Seek(), is called by the 3 variations above
	bool Seek(int64_t pos, int64_t rel, bool seek_by_bytes);
	//
	// ts returns in the media file's timebase, needs to be multiplied by GetTimeBase() to become seconds: 
	bool GetDuration(int64_t& ts); // returns false if not media file, or media file duration not yet known
	//
	// duration returns as seconds (GetTimeBase() multiplication described above performed internally in this function):
	bool GetDurationSeconds(double& duration); // returns false if not media file, or media file duration not yet known
	//
	// pass normTime in set within 0-1 and get back ts within media file's duration;
	// returns false if not media file, or media file duration not yet known:
	bool CalcTimestampFromNormalizedPosition(int64_t& ts, double normTime);
	//
	bool CalcTimestampParts(int64_t& ts, int32_t& hours, int32_t& minutes, int32_t& seconds);

	bool HasPlaybackStarted(void) { return (mp_frameMgr) ? mp_frameMgr->HasPlaybackStarted() : false; }
	bool IsPlaybackPaused(void) { return (mp_frameMgr) ? mp_frameMgr->IsPlaybackPaused() : false; }
	bool IsPlaybackActive(void) { return (mp_frameMgr) ? mp_frameMgr->IsPlaybackActive() : false; }


	const char* GetVersion( void ) { return "FFVideo V1.0"; }

	float GetReceivedFPS(void) {
		if (mp_frameMgr) { return mp_frameMgr->m_fps; } 
		return -1.0f;
	}

	int32_t  GetStreamType(void) {
		if (mp_frameMgr) { return mp_frameMgr->m_stream_type; }
		return -1;
	}

	double GetTimeBase(void) { return m_timebase; }											// valid once playing;
	double GetExpectedFrameRate(void) { return m_expected_frame_rate; } // valid once playing;

	private:
	////////////////////////////////////////////////////////////////////////////////////
	// end "public" API, below is internal use only:
	////////////////////////////////////////////////////////////////////////////////////

	friend class FFVideo_FrameDestination;
	friend class FFVIDEO_FrameFilter;
	friend class FFVideo_FrameMgr;


	// class sub-thread function that spins reading media packets, converting them to video frames:
	void VideoProcessLoop(void);
	//
	// thread variables:
	std::thread* mp_videoProcessingThread;
	//
	std::atomic<bool>		m_stop_video_processing_loop;
	std::atomic<bool>		m_video_processing_loop_ended;
	

	void ReportLog(const char* formatStr, ...);

	bool StartStream( FFVIDEO_USB_Camera_Format* camera = NULL, double start_time = 0.0 );

	void CloseStream( void ); // frees up old structs when new stream is opened

	bool StartUSB(FFVIDEO_USB_Camera_Format* camera = NULL);
	bool StartUSBGuessingCamera( void );

	bool StartIPorMediaFile(std::string& printf_safe_vid_src);

	std::string                 m_vid_src;					// used by all, but contents & format of contents changes per stream type
	std::string                 m_media_fname;			// only used by media files
	uint32_t                    m_usb_pin;					// only used by USB cams
	std::string                 m_ip_url;						// only used by IP cameras

	int32_t									    m_width, m_height;  // copies of this info, to reduce access to lock protected structs

	mutable std::shared_mutex   m_post_process_lock;// thread protection for post process
	//
	std::string									m_post_process;			// optional AVFilterGraph video filter post process
																									// can be set to a text string defining an unlimited series of video filters, chained together, 
																									// see https://ffmpeg.org/ffmpeg-filters.html

	bool                        m_auto_frame_interval;
	std::vector<uint32_t>				m_auto_interval_thresholds;
	std::vector<uint32_t>				m_auto_intervals;

	std::atomic<bool>						m_loop_media_file;
	std::atomic<bool>						m_is_opening;

	static void replaceAll(std::string& str, const std::string& from, const std::string& to);

					void start_query_capture( void );
	static	void query_capture( const char *message );
					void stop_query_capture( std::vector<std::string>& captured_log );
					bool is_query_capture_on( void );

	static void query_callback(void *ptr, int level, const char *fmt, va_list vargs);
	static void logging_callback(void *ptr, int level, const char *fmt, va_list vargs);

	bool											m_capture_log;				// queries of USB camera pixel formats are returned in the log, so this helps to capture them
	std::vector<std::string>	m_captured_log;

	AVDictionary*							mp_opts;
	AVFormatContext*					mp_format_context;
	AVStream*									mp_video_stream;
	AVCodecContext*						mp_codec_context;
	AVCodecContext*						mp_orig_codec_context;
														
	int32_t										m_timebase_num;
	int32_t										m_timebase_dem;
	double										m_timebase;
	double										m_expected_frame_rate;

	AVFrame*									mp_decompressed_frame[FFVIDEO_DECOMPRESSION_AVFRAME_COUNT];
	std::atomic<uint32_t>			m_decompress_index;  // which mp_decompressed_frame will be decompressed into next
	std::atomic<uint32_t>			m_display_index;		 // which mp_decompressed_frame the next mp_display_buffer will get

	FFVideo_FrameMgr*					mp_frameMgr;

	AVPacket*									mp_packet;

	void ProcessPacket(uint64_t& wait_milliseconds_for_next_packet, bool& terminal_flag);

	// from ffplay.c ffmpeg 4.2.2
	int32_t check_stream_specifier(AVFormatContext* s, AVStream* st, const char* spec);
	AVDictionary** setup_find_stream_info_opts(AVFormatContext* s, AVDictionary* codec_opts);
	AVDictionary* filter_codec_opts(AVDictionary* opts, enum AVCodecID codec_id,
	                                AVFormatContext* s, AVStream* st, const AVCodec* codec);

	void setup_av_options( const char*camName = NULL, FFVIDEO_USB_Camera_Format* usb_format = NULL );
	void av_options_report( void );	// writes the options unused to the log
};
//////////////////////////////////////////////////////////////////////////////////////





#endif // _FFVIDEO_H_
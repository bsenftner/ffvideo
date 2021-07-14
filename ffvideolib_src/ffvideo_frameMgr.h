#pragma once
#ifndef _FFVIDEO_FRAMEMGR_H_
#define _FFVIDEO_FRAMEMGR_H_


#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <functional>


extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "libavutil/error.h"
#include "libavfilter/avfilter.h"
#include "libavutil/log.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/avstring.h"
}
#pragma comment(lib, "libavformat.a")

#include "BCTime.h"
#include "ffvideo_image.h"
#include "ffvideo_frameExporter.h"

class FFVideo_FrameMgr;
class FFVideo;

#define FFVIDEO_DECOMPRESSION_AVFRAME_COUNT (24)

#ifdef WIN32
#if defined(_MSC_VER) && _MSC_VER < 1900

#define snprintf _snprintf
#define vsnprintf _vsnprintf

#endif // defined(_MSC_VER) && _MSC_VER < 1900
#endif // WIN32

BOOLEAN nanosleep(LONGLONG ns);	// utility that sleeps in 100ns units

//------------------------------------------------------------------------------
// this describes one pixel format of a USB camera, where a specific USB cammera
// has a std::vector of these describing the formats available to that camera.
typedef struct _FFVIDEO_USB_Camera_Format
{
	std::string m_name;
	std::string m_pixelFormat;
	std::string m_vcodec;						
	int32_t			m_min_width, m_min_height; 
	float				m_min_fps;
	int32_t			m_max_width, m_max_height; 
	float				m_max_fps;
} FFVIDEO_USB_Camera_Format;

//------------------------------------------------------------------------------
// define the possible directions one can step a paused media file:
enum class FFVIDEO_FRAMESTEP_DIRECTION
{
	BACKWARD = 0,	FORWARD
};

//-----------------------------------------------------------------------------------
// container for video frames available for single steps forward/backward (scrubbing)
class FFVIDEO_ScrubFrame
{
	friend class FFVideo_FrameDestination;
	friend class FFVideo;

public:
	FFVIDEO_ScrubFrame() : m_frame_num(0) {}
	FFVIDEO_ScrubFrame(FFVideo_Image& im, int32_t frame_num)
	{
		m_im.Clone(im);
		m_frame_num = frame_num;
	}
	~FFVIDEO_ScrubFrame() {}

	// copy constructor
	FFVIDEO_ScrubFrame(const FFVIDEO_ScrubFrame& src) { *this = src; }

	// copy assignement operator
	FFVIDEO_ScrubFrame& operator = (const FFVIDEO_ScrubFrame& src)
	{
		if (this != &src)
		{
			m_im.Clone(src.m_im);
			m_frame_num = src.m_frame_num;
		}
		return (*this);
	}

private:
	FFVideo_Image m_im;
	uint32_t			m_frame_num;
};


//------------------------------------------------------------------------------
class FFVIDEO_FrameFilter
{
	friend class FFVideo_FrameDestination;

private:
	FFVIDEO_FrameFilter()
	{
		mp_buffersrc_ctx  = NULL;
		mp_buffersink_ctx = NULL;
		mp_filter_graph   = NULL;

		// these "last" members track the active w, h & pixel format,
		// in case they change dynamically during stream playback:
		m_last_width  = 0;		
		m_last_height = 0;
		m_last_format = AV_PIX_FMT_NONE;
		// also, don't forget m_last_post_process changing triggers a reinit as well
	}
	~FFVIDEO_FrameFilter() 
	{			
		if (mp_filter_graph)
			avfilter_graph_free( &mp_filter_graph );
	};

	// called when a new filter graph is needed, i.e. any time width, height or format change:
	int Init( AVFormatContext* p_format_context, AVStream* p_video_stream, 
						AVFrame* p_video_frame, std::string& post_process )
	{
		// we'll be recreating the filter graph, so if one exists, delete it: 
		if (mp_filter_graph)
			avfilter_graph_free(&mp_filter_graph);

		mp_filter_graph = avfilter_graph_alloc();
		if (!mp_filter_graph)
		{
			av_log(NULL, AV_LOG_ERROR, "FFVIDEO_FrameFilter: out of memory\n");
			return AVERROR(ENOMEM);
		}

		AVFilterInOut* p_outputs = NULL; 
		AVFilterInOut* p_inputs  = NULL;

		// this char buffer becomes the filter itself
		char graph_filter_text[1024]; 

		// this controls the type of scale interpolation:
		mp_filter_graph->scale_sws_opts = av_strdup("flags=bicubic");   

		AVCodecParameters* p_codecpars = p_video_stream->codecpar; 
		AVRational&     time_base      = p_video_stream->time_base;
		AVRational      fr             = av_guess_frame_rate( p_format_context, p_video_stream, NULL);

		// buffer video source; the decoded frames from the decoder will be filtered thru here:
		snprintf( graph_filter_text, 
							sizeof(graph_filter_text),
						  "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
						  p_video_frame->width, p_video_frame->height, p_video_frame->format,
						  time_base.num, time_base.den,
						  p_codecpars->sample_aspect_ratio.num, FFMAX(p_codecpars->sample_aspect_ratio.den, 1) );
		//
		if (fr.num && fr.den)
		{
			av_strlcatf(graph_filter_text, sizeof(graph_filter_text), ":frame_rate=%d/%d", fr.num, fr.den);
		}
		//
		int ret = avfilter_graph_create_filter( &mp_buffersrc_ctx, avfilter_get_by_name("buffer"), "in", graph_filter_text, NULL, mp_filter_graph );
		if (ret < 0) 
		{
			av_log(NULL, AV_LOG_ERROR, "FFVIDEO_FrameFilter: Cannot create buffer source\n");
			avfilter_inout_free(&p_inputs);
			avfilter_inout_free(&p_outputs);
			return ret;
		}	

		// buffer video sink; this terminates the filter chain:
		ret = avfilter_graph_create_filter( &mp_buffersink_ctx, avfilter_get_by_name("buffersink"), "out", NULL, NULL, mp_filter_graph );
		if (ret < 0) 
		{
			av_log(NULL, AV_LOG_ERROR, "FFVIDEO_FrameFilter: Cannot create buffer sink\n");
			avfilter_inout_free(&p_inputs);
			avfilter_inout_free(&p_outputs);
			return ret;
		}

		// Using av_opt_set_int_list() is unclear. I want this to make the filter graph produce an RGBA final buffer.
		// With some wmv and avi files, converting to RGBA produces incorrect linesize[0] (bytes per image row) (2 extra pixels)
		static const enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_RGBA, AV_PIX_FMT_NONE };
		//
		ret = av_opt_set_int_list( mp_buffersink_ctx, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
		if (ret < 0) 
		{
			av_log(NULL, AV_LOG_ERROR, "FFVIDEO_FrameFilter: Cannot set output pixel format\n");
			avfilter_inout_free(&p_inputs);
			avfilter_inout_free(&p_outputs);
			return ret;
		}

		AVFilterContext* last_filter = mp_buffersink_ctx;

		// this can be set to a text string defining an unlimited series of video filters, chained together, 
		// see https://ffmpeg.org/ffmpeg-filters.html
		//
		// these do "something"...
		// "hqdn3d=4.0:3.0:6.0:4.5"; 
		// "vignette=PI/4"; 
		// "lutrgb=r=negval:g=negval:b=negval,split [main][tmp]; [tmp] crop=iw:ih/2:0:0, vflip [flip]; [main][flip] overlay=0:H/2,edgedetect=mode=colormix:high=0"; 
		// "drawgrid=width=100:height=100:thickness=2:color=red@0.5"; 
		// "drawbox=10:20:200:60:red@0.5"; 
		// "avgblur=sizeX=1.5"; 
		//	this one blurs every 4 frames into one:
		//	"tblend=average,framestep=2,tblend=average,framestep=2,setpts=0.25*PTS"
		// kinda hard to see: "chromakey=black:similarity=0.25"
		// "colorbalance=rs=.3";
		const char* optional_post_processing_filters = NULL; 
		if (post_process.size() > 0 && post_process.compare("none") != 0)
		{
			m_last_post_process = post_process;
		  optional_post_processing_filters = m_last_post_process.c_str();
		}

		if (optional_post_processing_filters)
		{
			p_outputs	= avfilter_inout_alloc();
			p_inputs	= avfilter_inout_alloc();
			if (!p_outputs || !p_inputs)
			{
				av_log(NULL, AV_LOG_ERROR, "FFVIDEO_FrameFilter: out of memory 2\n");
				ret = AVERROR(ENOMEM);
				avfilter_inout_free(&p_inputs);
				avfilter_inout_free(&p_outputs);
				return ret;
			}
			// Set the endpoints for the filter graph. The filter_graph will be linked to the graph described by filters_descr.
			//
			// The buffer source output must be connected to the input pad of the first filter described by filters_descr; 
			// since the first filter input label is not specified, it is set to "in" by default.
			//
			p_outputs->name       = av_strdup("in");
			p_outputs->filter_ctx = mp_buffersrc_ctx;
			p_outputs->pad_idx    = 0;
			p_outputs->next       = NULL;
			//
			// The buffer sink input must be connected to the output pad of the last filter described by filters_descr; 
			// since the last filter output label is not specified, it is set to "out" by default.
			//
			p_inputs->name       = av_strdup("out");
			p_inputs->filter_ctx = mp_buffersink_ctx;
			p_inputs->pad_idx    = 0;
			p_inputs->next       = NULL;

			if ((ret = avfilter_graph_parse_ptr( mp_filter_graph, optional_post_processing_filters, &p_inputs, &p_outputs, NULL)) < 0)
			{
				av_log(NULL, AV_LOG_ERROR, "FFVIDEO_FrameFilter: avfilter_graph_parse_ptr failed\n");
				avfilter_inout_free(&p_inputs);
				avfilter_inout_free(&p_outputs);
				return ret;
			}
		}
		else
		{
			ret = avfilter_link( mp_buffersrc_ctx, 0, mp_buffersink_ctx, 0 );
			if (ret < 0)
			{
				av_log(NULL, AV_LOG_ERROR, "FFVIDEO_FrameFilter: avfilter_link failed\n");
				avfilter_inout_free(&p_inputs);
				avfilter_inout_free(&p_outputs);
				return ret;
			}
		}

		// "Reorder the filters to ensure that inputs of the custom filters are merged first"
		uint32_t nb_filters = mp_filter_graph->nb_filters;
		for (uint32_t i = 0; i < mp_filter_graph->nb_filters - nb_filters; i++)
		{
			FFSWAP(AVFilterContext*, mp_filter_graph->filters[i], mp_filter_graph->filters[i + nb_filters]);
		}

		// And this is construction of the filter that fixes corrupt video frames:
		if ((ret = avfilter_graph_config(mp_filter_graph, NULL)) < 0)
		{
			avfilter_inout_free(&p_inputs);
			avfilter_inout_free(&p_outputs);
			return ret;
		}

		avfilter_inout_free( &p_inputs  );
		avfilter_inout_free( &p_outputs );

		return ret;
	}

	int FilterFrame( AVFormatContext* p_format_context, AVStream* p_video_stream, 
									 AVFrame* p_video_frame, std::string& post_process )
	{
		int ret = 0;

		if (m_last_width  != p_video_frame->width  ||  // Note: ffplay also has tests for vfilter_idx & pkt_serial
				m_last_height != p_video_frame->height ||  // but so far I've not figured out what those are...
				m_last_format != p_video_frame->format ||
				m_last_post_process.compare( post_process ) != 0)
		{
			ret = Init( p_format_context, p_video_stream, p_video_frame, post_process ); 
			if (ret >= 0)
			{
				// success
				m_last_width  = p_video_frame->width;
				m_last_height = p_video_frame->height;
				m_last_format = p_video_frame->format;
				// m_last_post_process = post_process; already updated, happens inside Init()
			}
		}

		if (ret >= 0)
			ret = av_buffersrc_add_frame( mp_buffersrc_ctx, p_video_frame );

		if (ret >= 0)
		{
			ret = av_buffersink_get_frame_flags( mp_buffersink_ctx, p_video_frame, 0 );
			if (ret < 0)
			{
				if (ret == AVERROR_EOF) // note: has never happened
					av_log(NULL, AV_LOG_INFO, "FFVIDEO_FrameFilter: av_buffersink_get_frame_flags returned AVERROR_EOF\n");
				ret = 0;
			}
		}

		return ret;
	}

	AVFilterContext*   mp_buffersrc_ctx;
	AVFilterContext*   mp_buffersink_ctx;
	AVFilterGraph*     mp_filter_graph;
	int                m_last_width, m_last_height, m_last_format;
	std::string				 m_last_post_process;
};


// ---------------------------------------------------------------------------------
// The library uses callbacks to deliver video frames and events of significance to the client.
// The new frame callback, has a "frame interval" parameter that is the number of frames to advance 
// within the video stream for each frame delivered to the client. This class here handles that 
// frame interval logic, so the player class can just play frames. Likewise this also handles
// retention of frames for scrubbing when mediafiles pause. 
// ---------------------------------------------------------------------------------
class FFVideo_FrameDestination
{
	friend class FFVideo_FrameExporter;
	friend class FFVIDEO_FrameFilter;
	friend class FFVideo_FrameMgr;
	friend class FFVideo;

private:
	FFVideo_FrameDestination( FFVideo_FrameMgr* parent );
	~FFVideo_FrameDestination();

	void DeliverFrameToClient( bool first_frame, AVFrame* frame, uint32_t display_index, int32_t estimated_frame_number);

	FFVideo_FrameMgr*						mp_parent; 
	int32_t											m_frame_interval;	// how many frames to advance between deliveries of a frame to the client
	int32_t											m_frame_count;

	int32_t											m_frame_export_count;

	FFVideo_FrameExporter				m_frame_exporter;								// queue for frames to be written to disk
	int32_t											m_frame_export_interval;				// independant of frame interval, how often to write the frame to disk?
	std::string									m_export_dir;										// export directory
	std::string									m_export_base;									// basename before timestamp
	float												m_export_scale;									// image scale factor, defaults to 1.0f
	int32_t											m_export_quality;								// jpg quality setting when saved

	// we normally vertically flip all video frames because images are bottom origin; 
	// our constructor sets this to true, but if set to false, that flipping won't happen: 
	bool												m_vflip;			

	// the "process frame callback" is really the frame display callback
	typedef void(*DISPLAY_FRAME_CALLBACK_CB)(void* p_object, FFVideo_Image& im, int32_t frame_num);
	//
	DISPLAY_FRAME_CALLBACK_CB		mp_process_frame;
	void*												mp_process_frame_object;

	FFVIDEO_FrameFilter*				mp_frame_filter;

	bool IsEmptyAVrame(AVFrame* frame) { return ((frame->format == AV_PIX_FMT_NONE) || (frame->pict_type == AV_PICTURE_TYPE_NONE)); }

	void SetScrubBufferSize(int32_t size) { m_scrub_max_size = size; m_scrub_frames.resize(size); }

	std::vector<FFVIDEO_ScrubFrame>	m_scrub_frames;
	int32_t													m_scrub_index;		// where in m_scrub_frames we are in saving played frames
	int32_t													m_scrub_pos;			// -1 means not scrubbing, positive values are backwards from play position
	std::atomic<int32_t>						m_scrub_max_size;
};

// ---------------------------------------------------------------------------------
class FFVideo_FrameMgr 
{
	friend class FFVideo_FrameExporter;
	friend class FFVideo_FrameDestination;
	friend class FFVideo;

private:
	FFVideo_FrameMgr( FFVideo* parent );
	~FFVideo_FrameMgr();

	FFVideo*									mp_parent;
	std::atomic<int32_t>			m_stream_type;					// 0=Media, 1=USB, 2=IP
	std::atomic<bool>					m_is_playing;						// if true, packet streaming has begun
	std::atomic<bool>					m_paused;								// if true, m_is_playing must also be true for playback to be paused
	std::atomic<bool>					m_drain_mode;						// when stream ends, frames need to drain out of pipeline before really over
	std::atomic<bool>			 		m_drain_complete;			  // ditto
	std::atomic<bool>					m_media_has_ended;			// when media file ends
	std::atomic<bool>					m_stream_has_died;			// when stream terminates

	std::atomic<bool>					m_first_frame;
	std::atomic<uint32_t>			m_frames_received;			// frame counter of frames decompressed
																									
	std::atomic<uint32_t>			m_fps_frames_received;	// resets after pauses so fps calculation is correct
	BCTime										m_clock;								// resets on unpause so fps can use m_fps_frames_received and be correct
																									
	std::atomic<float>				m_fps;									// fps of frames received from packet reader
	int32_t										m_frame_interval;				// used by all stream types

	std::atomic<double>				m_est_play_pos;					// estimated play position (based on best effort timestamp)
	std::atomic<int32_t>			m_est_frame_num;				// estimated frame number (estimated due to seeks)
	int32_t										m_seek_by_bytes;				// if <0 that's off
	int64_t										m_start_time;
	//
	std::atomic<bool>					m_seek_req;
	std::atomic<uint32_t>			m_seek_flags;
	int64_t										m_seek_anchor;					// when playing, this is the current packet's pos, m_seek_pos
	int64_t										m_seek_pos;
	int64_t										m_seek_rel;
	int32_t										m_seek_skip_count;
	bool											m_post_seek_nonkeyframeskip;
	int32_t										m_post_seek_renders;
	bool											m_post_seek_render_is_really_a_step;

	mutable std::shared_mutex m_cb_lock;

	FFVideo_FrameDestination*	mp_frame_dest;					// frame callbacks wrapper for destination delivery
	FFVideo_Image							m_im;		

	// this is how frame exporting is enabled, specify an interval < 1 to disable.
	// The export directory must exist, or failure and disabling of exports. 
	// Frames are exported as jpg files using the quality passed as the jpg compression. 
	// The filename is the export dir + export_base + an ISO 8601 timestamp, with milliseconds. 
	// Failing to write fails silently, incrementing a failed export counter. 
	bool SetFrameExporting(int32_t export_interval, 
												 std::string& export_dir, 
												 std::string& export_base, 
												 float scale, 
												 int32_t quality,
												 EXPORT_FRAME_CALLBACK_CB frame_export_cb,
												 void* frame_export_object );

	void SetScrubBufferSize(int32_t size) { mp_frame_dest->SetScrubBufferSize(size); }

	// the interrupt_callback() function is a function callback registered with ffmpeg; 
	// logic within libavformat executes the callback during two I/O operations related to
	// playing a stream: opening and playing stream. because 2 different operations
	// are handled by this libavformat callback, clients of this class have two different
	// timeout controls: one for opening a stream and one for reading from a stream. (see below)
	static int interrupt_callback( void *context );
	//
	// consider this an internal logic class global; it is the specific time of the next I/O timeout, regardless of type
	std::atomic<float>           m_next_interrupt_timeout; // used by mp_parent->interrupt_callback()
	std::atomic<float>           m_open_timeout;						// if nonzero, max allowed duration 4 opening a stream and finding its info
	std::atomic<float>           m_read_timeout;						// if nonzero, max allowed duration 4 reading a frame from a stream
	//
	void SetOpenTimeout( float secs ){	m_open_timeout = secs; }		
	//		
	float GetOpenTimeout( void ) { return m_open_timeout; }		
	//		
	void SetReadTimeout( float secs ) {	m_read_timeout = secs; }		
	//
	float GetReadTimeout( void ) { return m_read_timeout;	}		
	//
	void SetNextIOTimeout( float secs )	{	m_next_interrupt_timeout = m_clock.ms() + secs * 1000.0f;	}

	// class storage for end-user stream ended/termined callbacks:

	// unexpected stream terminated callback
	// when set, it will be called when the video stream unexpectedly stops, i.e. when camera unexpectedly disconects or network drops
	typedef void(*TERMINATED_STREAM_CALLBACK_CB)(void* p_object);
	//
	TERMINATED_STREAM_CALLBACK_CB mp_term_callback;
	void*                         mp_term_object;

	// media stream finished callback, when playing media file this is called after last frame processed
	typedef void(*STREAM_FINISHED_CALLBACK_CB) (uint32_t frame_num, void* p_object);
	//
	STREAM_FINISHED_CALLBACK_CB mp_stream_ended_callback;
	void*                       mp_stream_ended_object;

	// stream logging callback for library clients; any log messages generated 
	// by library are sent through this to client's logic:
	typedef void(*STREAM_LOGGING_CALLBACK_CB) (const char* msg, void* p_object);
	//
	STREAM_LOGGING_CALLBACK_CB  mp_stream_logging_callback;
	void*                       mp_stream_logging_object;


	// this picks up the YUV decompressed frames generatedby the FFVideo thread and selectively sends RGBA frames to the 
	// client at their appropriate display times:
	void ProcessPacketToFrame(uint32_t& last_display_index, uint64_t& wait_milliseconds_for_next_frame_display, bool& terminal_flag);

	// if a media file, it could receive a seek request; if so this handles it: 
	void HandleSeekRequests(void);

	bool IsPlaybackPaused( void ) { return m_is_playing && m_paused; }
	bool HasPlaybackStarted( void ) { return m_is_playing; }
	bool IsPlaybackDraining( void ) { return m_is_playing && !m_paused && m_drain_mode; }

	bool IsPlaybackActive( void ) 
	{
		if (m_stream_type != 0) // if not a media file
		{
			return m_is_playing; // live streams don't really pause, they throw away frames when paused
		}
		return m_is_playing && !m_paused; 
	}

	void PrepareForPlayback( void )
	{
		m_first_frame = true;
		m_frames_received = 0;
		m_fps_frames_received = 0;
		m_fps = 0.0f;
		//
		m_is_playing = false;
		m_paused = false;
		m_drain_mode = false;
		m_media_has_ended = false;
		m_stream_has_died = false;
		m_clock.Reset();
		m_seek_req = false;
		m_seek_anchor = -1;
		m_est_frame_num = -1;
		m_est_play_pos = 0.0;
		m_seek_pos = 0;
		m_post_seek_nonkeyframeskip = false;
		m_post_seek_renders = 0;
		m_post_seek_render_is_really_a_step = false;
		m_seek_skip_count = 0;
		m_start_time = AV_NOPTS_VALUE;
		//
		mp_frame_dest->m_scrub_pos = -1;
		mp_frame_dest->m_frame_export_count = 0;
		if (mp_frame_dest->m_frame_export_interval < 0)
			mp_frame_dest->m_frame_export_interval = 0; // erase error state
	}

	void PausePlayback( void )
	{
		if (m_is_playing)
			m_paused = true;
	}

	void UnPausePlayback( void )
	{
		if (m_is_playing)
		{
			m_fps_frames_received = 0;
			m_clock.Reset();
			m_paused = false;
		}
	}

	bool AnyPostSeekProcessingActive(void)
	{
		if (m_stream_type != 0)
			return false;
		if (m_seek_skip_count > 0)
			return true;
		if (m_post_seek_nonkeyframeskip)
			return true;
		if (m_post_seek_renders > 0)
			return true;
		return false;
	}
};



#endif // _FFVIDEO_FRAMEMGR_H_


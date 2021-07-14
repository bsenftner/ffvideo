
/////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdarg.h>

#include "ffvideo.h"


//////////////////////////////////////////////////////////////////////////////////////
void FFVideo::SetScrubBufferSize(int32_t size)
{
	mp_frameMgr->SetScrubBufferSize(size);
}

//////////////////////////////////////////////////////////////////////////////////////
bool FFVideo::OpenIPCamera(const std::string& url, int32_t frame_interval)
{
	StopStream();

	m_ip_url = url;

	if (frame_interval == 0)
	     m_auto_frame_interval = true;
	else m_auto_frame_interval = false;

	if (frame_interval <= 0)
		   mp_frameMgr->m_frame_interval = 1;
	else mp_frameMgr->m_frame_interval = frame_interval;

	// 0=Media, 1=USB, 2=IP
	mp_frameMgr->m_stream_type = 2;

	m_vid_src = m_ip_url;

	// note parameter 1: not a USB playback, so no USB camera pointer
	// note parameter 2 is ignored by IP streams, that's a media file's start offset
	return StartStream(NULL, 0.0);
}

//////////////////////////////////////////////////////////////////////////////////////
bool FFVideo::OpenUSBCamera(int32_t pin, int32_t frame_interval, FFVIDEO_USB_Camera_Format* camera)
{
	StopStream();

	if (pin < 0)
	{
		ReportLog("Error! USB pin < 0");
		return false;
	}
	m_usb_pin = pin;

	if (frame_interval == 0)
		   m_auto_frame_interval = true;
	else m_auto_frame_interval = false;

	if (frame_interval <= 0)
		   mp_frameMgr->m_frame_interval = 1;
	else mp_frameMgr->m_frame_interval = frame_interval;

	// 0=Media, 1=USB, 2=IP
	mp_frameMgr->m_stream_type = 1;

	// USB gets m_vid_src set internally, after some logic to figure it out
	// note parameter 2 is ignored by USB streams, that's a media file's start offset
	return StartStream(camera, 0.0);
}

//////////////////////////////////////////////////////////////////////////////////////
bool FFVideo::OpenMediaFile(const std::string& fname,
	int32_t frame_interval,
	bool loop_flag,
	double start_offset)			// use seconds, leave 0.0 for normal play start
{
	StopStream();

	m_media_fname = fname;

	if (frame_interval == 0)
		   m_auto_frame_interval = true;
	else m_auto_frame_interval = false;

	if (frame_interval <= 0)
		   mp_frameMgr->m_frame_interval = 1;
	else mp_frameMgr->m_frame_interval = frame_interval;

	// 0=Media, 1=USB, 2=IP
	mp_frameMgr->m_stream_type = 0;

#ifdef WIN32
	m_vid_src = std::string("file:") + m_media_fname;
#else
	m_vid_src = std::string("file://") + m_media_fname;
#endif

	m_loop_media_file = loop_flag;

	// note parameter 1: not a USB playback, so no USB camera pointer
	// note parameter 2 is the media file's start offset in seconds, leave 0.0 to start at beginning. 
	return StartStream(NULL, start_offset);
}

//////////////////////////////////////////////////////////////////////////////////////
void FFVideo::Pause()
{
	mp_frameMgr->PausePlayback();

	// av_options_report();
}

//////////////////////////////////////////////////////////////////////////////////////
void FFVideo::UnPause()
{
	mp_frameMgr->UnPausePlayback();
}

//////////////////////////////////////////////////////////////////////////////////////
bool FFVideo::Step(FFVIDEO_FRAMESTEP_DIRECTION direction)
{
	if (IsPlaybackPaused())
	{
		// we're paused, but let's check if we want seek refreshes (that are used to finish a previous seek):
		if (mp_frameMgr->AnyPostSeekProcessingActive())
			return false;

		FFVideo_FrameDestination* p_frame_dispatch = mp_frameMgr->mp_frame_dest;

		if (direction == FFVIDEO_FRAMESTEP_DIRECTION::FORWARD)
		{
			p_frame_dispatch->m_scrub_pos--;

			if (p_frame_dispatch->m_scrub_pos < 0)
			{
				// we are at "now", equal with the play position, ask mp_av_player to advance one frame forward:
				p_frame_dispatch->m_scrub_pos = -1;
				//
				mp_frameMgr->m_post_seek_renders++;
				mp_frameMgr->m_post_seek_render_is_really_a_step = true;
			}
			else
			{
				// we are at some frame "back in time", so deliver the frame from the scrub buffer:

				int32_t scrub_max_size = p_frame_dispatch->m_scrub_max_size; // because atomic

				int32_t index = (p_frame_dispatch->m_scrub_index - p_frame_dispatch->m_scrub_pos) % scrub_max_size;

				// calling the process frame callback, delivering the frame to the client: 
				std::shared_lock<std::shared_mutex> frlock(mp_frameMgr->m_cb_lock);
				if (p_frame_dispatch->mp_process_frame)
					(p_frame_dispatch->mp_process_frame)(p_frame_dispatch->mp_process_frame_object,
						p_frame_dispatch->m_scrub_frames[index].m_im,
						p_frame_dispatch->m_scrub_frames[index].m_frame_num);
				frlock.unlock();
			}
			return true;
		}
		else // backward
		{
			// going backwards in time is forwards in the scrub buffer

			// if just starting to step backwards, go back one extra to skip the frame just put in, which user already sees:
			if (p_frame_dispatch->m_scrub_pos == -1)
				p_frame_dispatch->m_scrub_pos++;
			p_frame_dispatch->m_scrub_pos++;

			int32_t scrub_max_size = p_frame_dispatch->m_scrub_max_size; // because atomic

			if (p_frame_dispatch->m_scrub_pos < scrub_max_size)
			{
				int32_t index = (p_frame_dispatch->m_scrub_index - p_frame_dispatch->m_scrub_pos) % scrub_max_size;

				// if present, call the frame callback:
				std::shared_lock<std::shared_mutex> frlock(mp_frameMgr->m_cb_lock);
				if (p_frame_dispatch->mp_process_frame)
					(p_frame_dispatch->mp_process_frame)(p_frame_dispatch->mp_process_frame_object,
						p_frame_dispatch->m_scrub_frames[index].m_im,
						p_frame_dispatch->m_scrub_frames[index].m_frame_num);
				frlock.unlock();
				return true;
			}

			// no more frames to go backwards to: 
			p_frame_dispatch->m_scrub_pos = (int32_t)(p_frame_dispatch->m_scrub_frames.size()) - 1;
			return false;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////
void FFVideo::StopStream()
{
	bool was_playing(false);
	//
	if (mp_frameMgr && mp_frameMgr->m_is_playing)
	{
		was_playing = true;
		mp_frameMgr->m_is_playing = false;
		mp_frameMgr->m_paused = false;

		if (!mp_frameMgr->m_drain_mode)
		{
			// the file has not reached it's EOF so we "artificially" enter drain mode:
			mp_frameMgr->m_drain_mode = true;
			mp_frameMgr->m_drain_complete = (mp_frameMgr->m_frames_received > 0) ? false : true;
		}
		// else the stream already ended or was never played
	}

	// only if the VideoProcessLoop() loop is running:
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
		m_stop_video_processing_loop = false; // reset for next use
		m_video_processing_loop_ended = false;
	}

	if (mp_frameMgr && mp_frameMgr->mp_frame_dest->m_frame_exporter.IsRunning())
	{
		// if the export queue's not empty, let it continue
		if (mp_frameMgr->mp_frame_dest->m_frame_exporter.Size() == 0)
			 mp_frameMgr->mp_frame_dest->m_frame_exporter.StopExporter();
	}

	CloseStream(); // this deletes libav structs 
}

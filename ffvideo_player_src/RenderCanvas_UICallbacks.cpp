///////////////////////////////////////////////////////////////////////////////
// Name:        RenderCanvas_UICallbacks.cpp
// Author:			Blake Senftner
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#if !wxUSE_GLCANVAS
#error "OpenGL required: set wxUSE_GLCANVAS to 1 and rebuild the library"
#endif

#include "ffvideo_player_app.h"



////////////////////////////////////////////////////////////////////////////////////
// installed into ffvideo's frame display callback, this receives the video frames
// according to the frame interval setting; i.e. frame interval of 1 is every frame
void RenderCanvas::FrameCallBack(void* p_object, FFVideo_Image& im, int frame_num)
{
	if (p_object)
		((RenderCanvas*)p_object)->FrameCallBack(im, frame_num);
}
void RenderCanvas::FrameCallBack(FFVideo_Image& im, int frame_num)
{
	if (mp_videoWindow && mp_videoWindow->m_terminating)
		return;

	// if the face detector is both initialized and enbled, send the frame for face detection: 
	if (m_faceDetectMgr.m_faceDetectorInitialized && m_faceDetectMgr.m_faceDetectorEnabled)
	{
		m_faceDetectMgr.Add( im, frame_num );
	}
	else // no face detection, forward to rendering prep:
	{
		CommonFrameHandling( im, frame_num );
	}
}

////////////////////////////////////////////////////////////////////////
// when face detection completes, the face detected frame is sent here
void RenderCanvas::FrameFaceDetectionCallBack(void* p_object, FaceDetectionFrame& fdf)
{
	if (p_object)
		((RenderCanvas*)p_object)->FrameFaceDetectionCallBack(fdf);
}
void RenderCanvas::FrameFaceDetectionCallBack(FaceDetectionFrame& fdf)
{
	if (IsFaceDetectionEnabled())
	{
		// we use the image size of the face detection image to normalize the 
		// face points, so that size is acquired here. It's acquired here so
		// the face detection code is free to muck around with detection image
		// resolution for speed-wise optimization of face detections:
		m_faceDetectMgr.mp_faceDetector->GetDlibImageSize( m_detectImSize );

		m_detections = fdf.m_detections;

		if (IsFaceLandmarksEnabled())
		{
			m_facesLandmarkSets = fdf.m_facesLandmarkSets;
		}
	}

	// pass frame to rendering prep:
	CommonFrameHandling( fdf.m_im, fdf.m_frame_num );
}

////////////////////////////////////////////////////////////////////////
void RenderCanvas::CommonFrameHandling(FFVideo_Image& im, int frame_num)
{
	if (mp_videoWindow && mp_videoWindow->m_terminating)
		return;

	// update frame counters
	m_current_frame_num = frame_num;

	// number of frames FFVideo has delivered; unrelated to any fps calculations
	m_frame_count++;  

	m_frame_lock.lock();
	m_frame.Clone(im.mp_pixels, im.m_height, im.m_width);
	m_frame_lock.unlock();

	if (m_status == VIDEO_STATUS::WAITING_FOR_FIRST_FRAME)
	{
		std::string bsjnk = mp_app->FormatStr("win%d: got first frame\n", mp_videoWindow->m_id);
		mp_app->ReportLog(ReportLogOp::flush, bsjnk);
	}

	m_status = VIDEO_STATUS::RECEIVING_FRAMES;
	m_frame_loaded = true;

	// remember this runs in a non-wxWidgets thread, 
	// so we must send a render event rather than rendering directly: 
	mp_videoWindow->SendVideoRefreshEvent();
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// the "export frame callback" is called with every frame export (writing of a frame to disk)
// typedef void(*EXPORT_FRAME_CALLBACK_CB)(void* p_object, int32_t frame_num, const char* filepath, bool status);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderCanvas::FrameExportCallBack(void* p_object, int32_t frame_num, int32_t export_num, const char* filepath, bool status)
{
	if (p_object)
		((RenderCanvas*)p_object)->FrameExportCallBack(frame_num, export_num, filepath, status);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// installed as the video library's frame export callback, this is called after each frame's write to disk; the parameters:
//		frame_num			this is the frame number according to the media
//		export_num		this tells us how many frame exports have occured since Play began, including this one
//		filepath			where the file is saved
//		status				if false, there's been a write error on this file. It may not exist, due to a write error. Exporting is terminated. 
//
// Note: this is only installed if we're encoding exported frames into movies.
// Note: due to slow disks, this could be called long after the same frame is displayed. 
void RenderCanvas::FrameExportCallBack(int32_t frame_num, int32_t export_num, const char* filepath, bool status)
{
	// the filenames are put into a vector, but one of two, double buffered to prevent thread collision:
	const int32_t bufferId = m_next_encoder_id % 2;

	m_exportedFramePaths[bufferId].push_back( filepath );

	int32_t encode_interval = mp_videoWindow->mp_streamConfig->m_encode_interval.load(std::memory_order::memory_order_relaxed);

	int32_t export_queue_size = mp_ffvideo->GetFrameExportingQueueSize();

	bool end_of_this_streams_export_frames(false);
	//
	if ((m_status == VIDEO_STATUS::STOPPED) || (m_status == VIDEO_STATUS::UNEXPECTED_TERMINATION) || (m_status == VIDEO_STATUS::MEDIA_ENDED))
	{
		if (export_queue_size == 0)
		{
			if (!status)
				end_of_this_streams_export_frames = true;
		}
	}

	if ((m_exportedFramePaths[bufferId].size() >= encode_interval) || end_of_this_streams_export_frames)
	{
		m_next_encoder_id++;
		SendVideoEncodeEvent();
	}
}



///////////////////////////////////////////////////////////////////
// installed into ffvideo, this is called upon stream terminations
void RenderCanvas::UnexpectedTerminationCallBack(void* p_object)
{
	if (p_object)
		((RenderCanvas*)p_object)->UnexpectedTerminationCallBack();
}

void RenderCanvas::UnexpectedTerminationCallBack(void)
{
	m_is_paused = false;
	m_is_playing = false;
	m_status = VIDEO_STATUS::UNEXPECTED_TERMINATION;

	std::string msg = mp_app->FormatStr("win%d: Unexpected stream termination, delivered %d frames\n", mp_videoWindow->m_id, m_frame_count);

	WindowStatus(wxString(msg.c_str()));

	// if the stream is an IP stream, it could be a security camera that simply crapped out or something temporary like that,
	// so if an IP stream we'll try re-playing the stream in 5 seconds by putting a "replay" function into the delayedCallbackMgr:
	if (mp_videoWindow->mp_streamConfig->m_type == STREAM_TYPE::IP)
		m_delayedCallbacks.Add(UnexpectedTerminationReplayCallBack, this, 5.0f);
}

//////////////////////////////////////////////////////////////////////////////////
// installed into the delayed callback manager, this is called upon stream replays
void RenderCanvas::UnexpectedTerminationReplayCallBack(void* p_object)
{
	if (p_object)
		((RenderCanvas*)p_object)->UnexpectedTerminationReplayCallBack();
}

void RenderCanvas::UnexpectedTerminationReplayCallBack(void)
{
	m_is_paused = false;
	m_is_playing = false;
	m_status = VIDEO_STATUS::UNEXPECTED_TERMINATION;

	std::string msg("Unexpected stream termination replay");
	WindowStatus(wxString(msg.c_str()));
	mp_app->ReportLog(ReportLogOp::flush, std::string("Attempting replay..."));

	Play();

	m_ip_restarts++;
}

///////////////////////////////////////////////////////////////////////////
// installed into ffvideo, this is called when a media stream ends normally
void RenderCanvas::MediaEndedCallBack(uint32_t frame_num, void* p_object)
{
	if (p_object)
		((RenderCanvas*)p_object)->MediaEndedCallBack(frame_num);
}

void RenderCanvas::MediaEndedCallBack(uint32_t frame_num)
{
	m_is_paused = false;
	m_is_playing = false;
	m_status = VIDEO_STATUS::MEDIA_ENDED;

	std::string msg = mp_app->FormatStr("win%d: media ended, delivering %d frames\n", mp_videoWindow->m_id, m_frame_count);

	WindowStatus(wxString(msg.c_str()));
}

//////////////////////////////////////////////////////////////////////
// installed into ffvideo, this is called when a log msg is generated
void RenderCanvas::AVLibLoggingCallBack(const char* msg, void* p_object)
{
	if (p_object)
		((RenderCanvas*)p_object)->AVLibLoggingCallBack(msg);
}

void RenderCanvas::AVLibLoggingCallBack(const char* msg)
{
	std::string cmsg(msg);
	mp_noticeMgr->AddNotice(cmsg, 3.0f);
	cmsg += "\n";
	mp_app->ReportLog(ReportLogOp::flush, cmsg);
}



////////////////////////////////////////////////////////////////////////
// installed into a GLButton this is called when the user clicks that button
void RenderCanvas::PlayButton_cb(void* p_object, GLButton* button)
{
	if (p_object)
	{
		((RenderCanvas*)p_object)->PlayButton_cb(button);
	}
}
void RenderCanvas::PlayButton_cb(GLButton* button)
{
	Play();
	mp_noticeMgr->AddNotice(std::string("Playing"), 1.0f);
}

////////////////////////////////////////////////////////////////////////
// installed into a GLButton this is called when the user clicks that button
void RenderCanvas::PauseButton_cb(void* p_object, GLButton* button)
{
	if (p_object)
	{
		((RenderCanvas*)p_object)->PauseButton_cb(button);
	}
}
void RenderCanvas::PauseButton_cb(GLButton* button)
{
	Pause();
	mp_noticeMgr->AddNotice(std::string("Paused"), 1.0f);
}

////////////////////////////////////////////////////////////////////////
// installed into a GLButton this is called when the user clicks that button
void RenderCanvas::StepBackButton_cb(void* p_object, GLButton* button)
{
	if (p_object)
	{
		((RenderCanvas*)p_object)->StepBackButton_cb(button);
	}
}
void RenderCanvas::StepBackButton_cb(GLButton* button)
{
	if (!mp_ffvideo)
		return;

	if (mp_ffvideo->IsRunning())
	{
		// video thread is spinning...

		VideoStreamConfig* vsc = mp_videoWindow->mp_streamConfig;

		if (vsc->m_type == STREAM_TYPE::FILE)
		{
			if (mp_ffvideo->IsPlaybackPaused())
			{
				WindowStatus(wxString("Stepping backward 1 frame."));
				mp_noticeMgr->AddNotice(std::string("Stepping backward 1 frame."), 1.0f);
				mp_ffvideo->Step(FFVIDEO_FRAMESTEP_DIRECTION::BACKWARD);
			}
			else
			{
				Pause();
			}
		}
	}
	else
	{
		mp_noticeMgr->AddNotice(std::string("Playback is not active!"), 1.0f);
	}
}

////////////////////////////////////////////////////////////////////////
// installed into a GLButton this is called when the user clicks that button
void RenderCanvas::StopButton_cb(void* p_object, GLButton* button)
{
	if (p_object)
	{
		((RenderCanvas*)p_object)->StopButton_cb(button);
	}
}
void RenderCanvas::StopButton_cb(GLButton* button)
{
	Stop();
	mp_noticeMgr->AddNotice(std::string("Stopped."), 1.0f);
}

////////////////////////////////////////////////////////////////////////
// installed into a GLButton this is called when the user clicks that button
void RenderCanvas::StepForwardButton_cb(void* p_object, GLButton* button)
{
	if (p_object)
	{
		((RenderCanvas*)p_object)->StepForwardButton_cb(button);
	}
}
void RenderCanvas::StepForwardButton_cb(GLButton* button)
{
	if (!mp_ffvideo)
		return;

	VideoStreamConfig* vsc = mp_videoWindow->mp_streamConfig;

	if (mp_ffvideo->IsRunning() && (vsc->m_type == STREAM_TYPE::FILE))
	{
		// video thread is spinning...

		if (mp_ffvideo->IsPlaybackPaused())
		{
			WindowStatus(wxString("Stepping forwward 1 frame."));
			mp_noticeMgr->AddNotice(std::string("Stepping forwward 1 frame."), 1.0f);
			mp_ffvideo->Step(FFVIDEO_FRAMESTEP_DIRECTION::FORWARD);
		}
		else
		{
			Pause();
		}
	}
	else
	{
		mp_noticeMgr->AddNotice(std::string("Playback is not active!"), 1.0f);
	}
}



/////////////////////////////////////////////////////////////////////////
// a custom wxWidgets event handler for adding video overlay notices
// generated by debug logic inside code running in non-wxWidgets threads
void RenderCanvas::OnVideoNotice(wxCommandEvent& event)
{
	wxString msg = event.GetString();
	int32_t  milliseconds = event.GetInt();

	mp_noticeMgr->AddNotice(std::string(msg.c_str()), (float)milliseconds / 1000.0f);
}




///////////////////////////////////////////////////////////////////
// called by a wxWidgets menu handler in a VideoWindow
bool RenderCanvas::EnableFaceDetection( bool enable )
{
	if (!mp_app) 
		return false;
	if (!mp_app->m_we_are_launched || m_terminating)
		return false;

	if (!IsFaceDetectionInitialized())
	{
		wxMessageBox( "Sorry, the Face Detector is still initializing. Try again in a bit..." );
		return false; 
	}

	std::string msg;

	// "enable" could have a value of true or false here:
	m_faceDetectMgr.m_faceDetectorEnabled = enable;

	if (!IsFaceDetectionEnabled())
	{
		msg = "Face detection is disabled";

		// we're off, these should be empty if we're still receiving frames:
		if (m_status == VIDEO_STATUS::RECEIVING_FRAMES)
		{
			m_detections.clear();
			m_facesLandmarkSets.clear();
		}
	}
	else msg = "Face detection is enabled";

	SendOverlayNoticeEvent( msg, 5000 );

	return true;
}

///////////////////////////////////////////////////////////////////
// called by a wxWidgets menu handler in a VideoWindow
bool RenderCanvas::EnableFaceLandmarks( bool enable )
{
	if (!mp_app) 
		return false;
	if (!mp_app->m_we_are_launched || m_terminating)
		return false;

	m_faceDetectMgr.m_faceFeaturesEnabled = enable;
	return true;
}



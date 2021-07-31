///////////////////////////////////////////////////////////////////////////////
// Name:        RenderCanvas.cpp
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

// These custom wxWidgets events need data defined, so here that is:
//
wxDEFINE_EVENT(wxEVT_VideoEncodeEvent, wxCommandEvent);
//
wxDEFINE_EVENT(wxEVT_VideoEncodeDoneEvent, wxCommandEvent);

const long ID_DELAYEDCALLBACKS_TIMER = wxNewId();

wxBEGIN_EVENT_TABLE(RenderCanvas, wxGLCanvas)
EVT_PAINT(RenderCanvas::OnPaint)
EVT_KEY_DOWN(RenderCanvas::OnKeyDown)
EVT_MOUSE_EVENTS(RenderCanvas::OnMouseEvent)
EVT_TIMER(ID_DELAYEDCALLBACKS_TIMER, RenderCanvas::DelayeCallbacksMonitor)
wxEND_EVENT_TABLE()

int32_t gl_attr_args[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_DEPTH_SIZE, 16, 0 };

////////////////////////////////////////////////////////////////////////
RenderCanvas::RenderCanvas(wxWindow* parent, VideoStreamConfig* streamConfig)
// With OpenGL graphics, the wxFULL_REPAINT_ON_RESIZE style flag should
// always be set, because making the canvas smaller should be followed by a 
// paint event that updates the canvas with new viewport settings.
	: wxGLCanvas(parent, wxID_ANY, NULL, // gl_attr_args,
								wxDefaultPosition, wxDefaultSize,
								wxFULL_REPAINT_ON_RESIZE | wxCLIP_CHILDREN | wxCLIP_SIBLINGS),
	mp_videoWindow((VideoWindow*)parent),
	m_faceDetectMgr(mp_videoWindow->mp_app),
	m_delayedCallbacks(100, this, ID_DELAYEDCALLBACKS_TIMER), // times per second delayed callbacks are checked for expiration
	m_status(VIDEO_STATUS::NEVER_PLAYED),
	mp_playThread(NULL),
	m_playThreadStarted(false),
	m_playThreadExited(false),
	m_playThreadJoined(false),
	m_next_encoder_id(0),
	m_frame_loaded(false),
	m_is_paused(false),
	m_is_playing(false),
	m_expected_width(0),
	m_expected_height(0),
	m_expected_fps(0.0f),
	m_expected_duration(0.0f),
	m_expected_frames(0),
	m_terminating(false),
	m_current_frame_num(0),
	m_frame_count(0),
	m_zoom(1.0f),
	m_trans(0.0f, 0.0f),
	m_mpos(0, 0),
	m_rel_mpos(0, 0),
	m_text_pos(10, 10),
	m_ip_restarts(0),
	m_theme(false)
{
	mp_app = mp_videoWindow->mp_app;

	// passing false here means do not try to end a previous play (there ain't one)
	InitFFMPEG(false);

	// events generated outside the GUI thread that are caught by this class and executed here:
	Bind(wxEVT_VideoNoticeEvent,     &RenderCanvas::OnVideoNotice,     this);
	Bind(wxEVT_VideoEncodeEvent,     &RenderCanvas::OnVideoEncode,     this);
	Bind(wxEVT_VideoEncodeDoneEvent, &RenderCanvas::OnVideoEncodeDone, this);

	// load our logo as an initial video frame:
	LoadLogoFrame();

	// manages "notices", text messages overlaid over the video frame that expire in time.
	mp_noticeMgr = new NoticeMgr(mp_app);


	// Explicitly create a new rendering context instance for this canvas.
	mp_glRC = new wxGLContext(this);
	SetCurrent(*mp_glRC);
	//
	mp_app->ReportLog(ReportLogOp::log, mp_app->FormatStr("Init'ed OpenGL, getting '%s'\n", glGetString(GL_VERSION)));

	const char* vid_lib_version = mp_ffvideo->GetVersion();
	mp_app->ReportLog(ReportLogOp::flush, mp_app->FormatStr("we are running %s\n", vid_lib_version));

	// initialize openGL
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Black Background

	glClearDepth(1.0f);				// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);	// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);		// The Type Of Depth Testing To Do

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// the VCR style semi-transparent buttons:
	m_videoCtls.Initialize(mp_app->m_data_dir, this);
	//
	m_videoCtls.m_ctrls[0].SetCallback(PlayButton_cb, this);
	m_videoCtls.m_ctrls[1].SetCallback(PauseButton_cb, this);
	m_videoCtls.m_ctrls[2].SetCallback(StepBackButton_cb, this);
	m_videoCtls.m_ctrls[3].SetCallback(StopButton_cb, this);
	m_videoCtls.m_ctrls[4].SetCallback(StepForwardButton_cb, this);
	//
	m_videoCtls.SetStreamType(streamConfig->m_type);

	GenerateRenderFont(streamConfig);


	// if used or not, we'll get a faceDetector initialized and spinning waiting for frames:
	m_faceDetectMgr.mp_frame_cb     = FrameFaceDetectionCallBack;
	m_faceDetectMgr.mp_frame_object = this;
	//
	// select the face landmarks model here:
	m_faceDetectMgr.SetFaceModel( FACE_MODEL::eightyone );
	//
	m_faceDetectMgr.StartFaceDetectionThread();


	wglMakeCurrent(NULL, NULL); // releasing the OpenGL context
}

////////////////////////////////////////////////////////////////////////
RenderCanvas::~RenderCanvas()
{
	if (m_is_playing)
	{
		m_terminating = true;

		mp_ffvideo->KillStream();
		wxMilliSleep(500);
		mp_ffvideo->SetDisplayFrameCallback(NULL, NULL);
		mp_ffvideo->SetStreamTerminatedCallBack(NULL, NULL);
		mp_ffvideo->SetStreamFinishedCallBack(NULL, NULL);
		wxMilliSleep(500);
	}

	m_faceDetectMgr.StopFaceDetectionThread();

	if (mp_glRC)
	{
		if (IsShown())
			SetCurrent(*mp_glRC);
		delete mp_glRC;
	}
}

////////////////////////////////////////////////////////////////////////
void RenderCanvas::GenerateRenderFont(VideoStreamConfig* vsc)
{
	// setup a wgl font for opengl text:
	wxClientDC dc(this);
	//HFONT font = CreateFont(-24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE | DEFAULT_PITCH, L"Ariel Black");
	HFONT font = CreateFont(-vsc->m_font_point_size, 0, 0, 0,
													 FW_BOLD, vsc->m_font_italic, vsc->m_font_underlined, vsc->m_font_strike_thru,
													 ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE | DEFAULT_PITCH, 
													 wxString(vsc->m_font_face_name.c_str()).wc_str());
	wglMakeCurrent(dc.GetHDC(), mp_glRC->GetGLRC());
	HFONT	oldfont = (HFONT)SelectObject(dc.GetHDC(), font);
	wglUseFontBitmaps(dc.GetHDC(), 0, 256, 1000);
	SelectObject(dc.GetHDC(), oldfont);

	m_line_height = vsc->m_font_point_size;
	if (vsc->m_font_point_size > 24)
		m_line_height += vsc->m_font_point_size / 4;
	else m_line_height += vsc->m_font_point_size / 2;

	m_OGL_font_dirty = false;
}

////////////////////////////////////////////////////////////////////////
bool RenderCanvas::InitFFMPEG(bool deleteFirst)
{
	if (deleteFirst)
	{
		Stop();

		mp_ffvideo->SetDisplayFrameCallback(NULL, NULL);
		mp_ffvideo->SetStreamTerminatedCallBack(NULL, NULL);
		mp_ffvideo->SetStreamFinishedCallBack(NULL, NULL); 
		mp_ffvideo->SetStreamLoggingCallback(NULL, NULL);
		wxMilliSleep(500);
		mp_ffvideo->KillStream();
		//
		delete mp_ffvideo;
		mp_ffvideo = NULL;
	}

	mp_ffvideo = new FFVideo();
	//
	bool vflip_flag(true), debug_flag(false);
#if defined(_DEBUG)
	debug_flag = true;
#endif
	mp_ffvideo->Initialize(vflip_flag, debug_flag);
	//
	// ffvideo callbacks for information, video frame delivery, and key events: 
	mp_ffvideo->SetDisplayFrameCallback(FrameCallBack, this);
	mp_ffvideo->SetStreamTerminatedCallBack(UnexpectedTerminationCallBack, this);
	mp_ffvideo->SetStreamFinishedCallBack(MediaEndedCallBack, this);
	mp_ffvideo->SetStreamLoggingCallback(AVLibLoggingCallBack, this);
	//
	mp_ffvideo->SetScrubBufferSize(30 * 10);	// how many frames to retain for the scrub buffer

	return true;
}

////////////////////////////////////////////////////////////////////////
void RenderCanvas::WindowStatus(wxString msg)
{
	std::string stdmsg(msg.c_str());
	stdmsg += std::string("\n");
	mp_app->ReportLog(ReportLogOp::flush, stdmsg.c_str());
	mp_videoWindow->mp_statusBar->SetStatusText(msg, 1);
}

////////////////////////////////////////////////////////////////////////
bool RenderCanvas::LoadLogoFrame(void)
{
	std::string windowBg = mp_app->m_data_dir + "\\gui\\ffvideo.jpg";
	m_frame_loaded = m_frame.Load(windowBg.c_str());
	return m_frame_loaded;
}

////////////////////////////////////////////////////////////////////////
bool RenderCanvas::LoadPleaseWaitFrame(void)
{
	std::string windowBg = mp_app->m_data_dir + "\\gui\\ffvideo_pwait.jpg";
	m_frame_loaded = m_frame.Load(windowBg.c_str());
	return m_frame_loaded;
}

////////////////////////////////////////////////////////////////////////
void RenderCanvas::OnKeyDown(wxKeyEvent& event)
{
	wxString play_msg("Playing ");

	VideoStreamConfig* vsc = mp_videoWindow->mp_streamConfig;

	switch (event.GetKeyCode())
	{
	case WXK_RIGHT:
	{
		if (!mp_ffvideo)
			return;
		if (!m_is_playing)
			return;
		if (vsc->m_type != STREAM_TYPE::FILE)
			return;

		// this works while the video is playing or paused or stopped after having been played
		int64_t ts(0);
		if (mp_ffvideo->GetDuration(ts))
		{
			int hours, minutes, seconds;
			if (mp_ffvideo->CalcTimestampParts(ts, hours, minutes, seconds))
			{
				WindowStatus(wxString::Format("Duration %02d:%02d:%02d", hours, minutes, seconds));
			}
		}
		WindowStatus(wxString("Seeking forward 10 seconds."));
		mp_ffvideo->SeekRelative(10.0);
	}
	break;

	case WXK_LEFT:
		if (!mp_ffvideo)
			return;
		if (!m_is_playing)
			return;
		if (vsc->m_type != STREAM_TYPE::FILE)
			return;
		WindowStatus(wxString("Seeking backwards 10 seconds."));
		mp_ffvideo->SeekRelative(-10.0);
		break;

	case WXK_DOWN:
		if (!mp_ffvideo)
			return;
		if (!m_is_paused)
			return;
		if (vsc->m_type != STREAM_TYPE::FILE)
			return;
		WindowStatus(wxString("Stepping backward 1 frame."));
		mp_ffvideo->Step(FFVIDEO_FRAMESTEP_DIRECTION::BACKWARD);
		break;

	case WXK_UP:
		if (!mp_ffvideo)
			return;
		if (!m_is_paused)
			return;
		if (vsc->m_type != STREAM_TYPE::FILE)
			return;
		WindowStatus(wxString("Stepping forward 1 frame."));
		mp_ffvideo->Step(FFVIDEO_FRAMESTEP_DIRECTION::FORWARD);
		break;

	case 'S':
	case 's':
		if (m_status == VIDEO_STATUS::WAITING_FOR_FIRST_FRAME ||
				 m_status == VIDEO_STATUS::PAUSED ||
				 m_status == VIDEO_STATUS::RECEIVING_FRAMES)
		{
			Stop();
			WindowStatus(wxString("Stopped"));
		}
		break;

	case WXK_SPACE:
		switch (m_status)
		{
		case VIDEO_STATUS::NEVER_PLAYED:
		case VIDEO_STATUS::STOPPED:
		case VIDEO_STATUS::MEDIA_ENDED:
		case VIDEO_STATUS::UNEXPECTED_TERMINATION:
		case VIDEO_STATUS::OPENING_MEDIA_FILE_FAILED:
		case VIDEO_STATUS::OPENING_USB_FAILED:
		case VIDEO_STATUS::OPENING_IP_FAILED:
			play_msg = vsc->StreamTypeString() + ": " + vsc->m_info;
			Play();
			WindowStatus(play_msg);
			break;

		case VIDEO_STATUS::RECEIVING_FRAMES:
		case VIDEO_STATUS::PAUSED:
			if (vsc->m_type == STREAM_TYPE::FILE)
			{
				Pause();
				if (m_is_paused)
					WindowStatus(wxString("Paused"));
				else WindowStatus(wxString("Unpaused"));
			}
			else // IP and USB type streams do not support pause
			{
				Stop();
				WindowStatus(wxString("Stopped"));
			}
			break;

		default:
			break;
		}
		break;
		/* */
	default:
		event.Skip();
		return;
	}
}

////////////////////////////////////////////////////////////////////////////////
void RenderCanvas::OnMouseEvent(wxMouseEvent& event)
{
	// Allow default processing to happen, or else the canvas cannot gain focus
	// (for key events).
	event.Skip();

	bool update_display = false;

	FF_Coord2D point(event.GetX(), event.GetY());

	m_mouse_moving = event.Moving();   // true if moving and no button is down
	m_mouse_dragging = event.Dragging(); // true if moving with a button down

	m_mouse_left_down = event.LeftIsDown();
	m_mouse_right_down = event.RightIsDown();
	m_mouse_both_down = m_mouse_left_down && m_mouse_right_down;
	m_mouse_ctrl_down = event.ControlDown();
	m_mouse_shft_down = event.ShiftDown();

	// calc mouse normalized coords

	FF_Vector2D p((float)point.x, (float)(m_winSize.y - point.y));
	if (ff_fabs(m_zoom) > 0.0f)
		p.Scale(1.0f / m_zoom);

	CalcVideoPlacement();

	FF_Vector2D trans = m_trans;
	if (ff_fabs(m_zoom) > 0.0f)
	{
		if (trans.x > 0.0f) trans.x /= m_zoom;
		if (trans.y > 0.0f) trans.y /= m_zoom;
	}

	p.x -= trans.x;
	p.y -= trans.y;

	if (m_frame_loaded)
	{
		p.x /= (float)m_frame.m_width;
		p.y /= (float)m_frame.m_height;
	}

	m_npos = p;

	FF_Coord2D old_mpos(m_mpos);
	m_mpos = point;
	m_rel_mpos.Set(m_mpos.x - old_mpos.x, m_mpos.y - old_mpos.y);

	if (m_rel_mpos.x != 0 || m_rel_mpos.y != 0)
		update_display = true;

	if (event.LeftDClick())
	{
		m_text_pos.Set(10, 10);
		update_display = true;
	}
	if (event.LeftUp())
	{
		// mp_noticeMgr->AddNotice(std::string("Peep!"), 5.0f);
		update_display = true;
	}
	if (m_mouse_dragging && m_mouse_left_down)
	{
		m_text_pos.x += m_rel_mpos.x;
		m_text_pos.y -= m_rel_mpos.y; // minus because y is flipped
	}

	if (update_display)
		Refresh();
}

///////////////////////////////////////////////////////////////////
void RenderCanvas::Stop(void)
{
	if (!mp_ffvideo)
		return;

	mp_ffvideo->StopStream();

	m_is_paused = false;
	m_is_playing = false;

	m_status = VIDEO_STATUS::STOPPED;

	// check if we were encoding exported video frames:
	VideoStreamConfig* vsc = mp_videoWindow->mp_streamConfig;
	if (vsc->m_export_interval > 0 && vsc->m_encode_interval > 0)
	{
		// check if there are un-encoded exported images waiting to be encoded:
		const int32_t bufferId = m_next_encoder_id % 2;
		//
		if (m_exportedFramePaths[bufferId].size() > 0)
		{
			m_next_encoder_id++;
			SendVideoEncodeEvent();
		}
	}
}

///////////////////////////////////////////////////////////////////
void RenderCanvas::Pause(void)
{
	if (!mp_ffvideo)
		return;

	if (!m_is_playing)
		return;

	if (m_is_paused)
	{
		HandleUnPausing();
	}
	else
	{
		HandlePausing();
	}
}

bool RenderCanvas::HandlePausing(void)
{
	if (mp_videoWindow->mp_streamConfig->m_type != STREAM_TYPE::FILE)
	{
		Stop();
		return true;
	}

	mp_ffvideo->Pause();		// needs toolbar button and timer attention!

	m_is_paused = true;
	m_status = VIDEO_STATUS::PAUSED;

	return true;
}

bool RenderCanvas::HandleUnPausing(void)
{
	if (mp_videoWindow->mp_streamConfig->m_type != STREAM_TYPE::FILE)
	{
		return Play();
	}

	mp_ffvideo->UnPause();		// needs toolbar button and timer attention!

	m_is_paused = false;
	m_status = VIDEO_STATUS::RECEIVING_FRAMES;

	return true;
}

//////////////////////////////////////////////////////////////////////////////
// because starting playback takes time, time that would block the GUI thread
// from updating other video windows, this Play() actually creates and calls
// a thread whose sole purpose is to start playback in a non-blocking manner.
//
bool RenderCanvas::Play(void)
{
	if (!mp_ffvideo)
		return false;

	// if m_is_playing is true and Play() is called, behave like a "pause" button:
	if (m_is_playing)
	{
		Pause();
		return (!m_is_paused);
	}

	bool please_wait(false);

	// make sure a previous play's thread exited normally, 
	// if so join it, delete it, so we can do it again: 
	if (mp_playThread)
	{
		if (m_playThreadExited)
		{
			if (!m_playThreadJoined)
			{
				mp_playThread->join();
				delete mp_playThread;
				mp_playThread = NULL;
				m_playThreadJoined = true;
			}
			// else joined already
		}
		else please_wait = true;
	}
	if (mp_ffvideo->GetFrameExportingQueueSize() > 0)
		please_wait = true;
	//
	// want the size of the active encoders, but it is used in another thread, so we shared_lock to read it:
	std::shared_lock lock(m_encodersSharedMutex);	
	size_t active_encoders = m_encoders.size();
	lock.unlock();
	if (active_encoders > 0)
		please_wait = true;

	if (please_wait)
	{
		wxMessageBox( "Previous play has not completed cleaning up yet, try again." );
		return false;
	}

	// if a previous playThread exists, we have a problem:
	if (!mp_playThread)
	{
		LoadPleaseWaitFrame();

		VideoStreamConfig* vsc = mp_videoWindow->mp_streamConfig;

		if (vsc->m_post_process.size() > 0)
		{
			mp_ffvideo->SetPostProcessFilter( vsc->m_post_process );
		}

		std::string              export_dir, export_base;
		EXPORT_FRAME_CALLBACK_CB p_export_cb(NULL);
		void*                    p_export_data(NULL);
		//
		// check if we're encoding, not just exporting:
		if ((vsc->m_export_interval > 0) && (vsc->m_encode_interval > 0))
		{
			// we're exporting so tell the lib about the frame export directory:
			export_dir  = vsc->m_export_dir;
			export_base = vsc->m_export_base;

			// we only care to be notified about frame exports if we're encoding those frames:
			p_export_cb = FrameExportCallBack;
			p_export_data = this;
			//
			// because we're encoding, the encode id resets and we clear any old m_exportedFramePaths[2]
			m_next_encoder_id = 0;
			m_exportedFramePaths[0].clear();
			m_exportedFramePaths[1].clear();
		}
		else if (vsc->m_export_interval > 0)  
		{
			// we're exporting so tell the lib about the frame export directory:
			export_dir  = vsc->m_export_dir;
			export_base = vsc->m_export_base;
		}

		if (vsc->m_frame_interval == 0)
		{
			std::vector<uint32_t> widths, intervals;
			std::string profile = vsc->m_frame_interval_profile, err;
			char* buff = (char*)profile.c_str();
			//
			if (vsc->ParseAutoFrameInterval( mp_app, buff, widths, intervals, err ))
			{
				mp_ffvideo->SetAutoFrameIntervalProfile( widths, intervals );
			}
			else
			{
				wxMessageBox( wxString::Format("Error parsing custom Frame Interval Profile '%s',\nreverting to default.",
											vsc->m_frame_interval_profile.c_str()) );
				widths.clear();
				intervals.clear();
				//
				vsc->m_frame_interval_profile = "7680, 3, 1920, 2, 320, 1";
				//
				widths.push_back(7680); intervals.push_back(3);
				widths.push_back(1920); intervals.push_back(2);
				widths.push_back(320);  intervals.push_back(1);
			}
		}

		// if vsc->m_export_interval is > 0 encoding is enabled, if vsc->m_export_interval == 0 encoding is disabled: 
		mp_ffvideo->SetFrameExportingParams(vsc->m_export_interval, export_dir, export_base, vsc->m_export_scale, vsc->m_export_quality, p_export_cb, p_export_data);

		if (vsc->m_type == STREAM_TYPE::FILE)
		{
			// if we happen to be exporting frames, exporting turns off on reloop. Likewise for encoding.  
			mp_ffvideo->SetMediaFileLoopFlag( vsc->m_loop_media_files );
		}

		mp_playThread = new std::thread(&RenderCanvas::ThreadedPlay, this);
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////
void RenderCanvas::ThreadedPlay(void)
{
	m_playThreadStarted = true;
	m_playThreadExited  = false;
	m_playThreadJoined  = false;

	VideoWindow*			 ovw = mp_videoWindow;

	VideoStreamConfig* vsc = mp_videoWindow->mp_streamConfig;

	bool good = false;
	switch (vsc->m_type)
	{
	default:
		break;

	case STREAM_TYPE::FILE:
		good = PlayMediaFile();
		break;

	case STREAM_TYPE::USB:
		good = PlayUSBCamera();
		break;

	case STREAM_TYPE::IP:
		good = PlayIPCamera();
		break;
	}
	if (ovw != mp_videoWindow)
	{
		// video window closed while inside video library!!
		return;
	}
	if (good)
	{
		// because play was successful, we write the current settings to persistent memory:
		vsc->SaveStreamInfo( mp_app->mp_config );
	}
	else
	{
		std::string msg = mp_app->FormatStr("win%d: Play request failed.", mp_videoWindow->m_id );

		SendOverlayNoticeEvent( msg, 5000 );

		mp_app->ReportLog(ReportLogOp::flush, msg); // flush so the msg is written immediately
	}

	m_playThreadExited = true;
}

///////////////////////////////////////////////////////////////////
bool RenderCanvas::PlayMediaFile(void)
{
	VideoStreamConfig* vsc = mp_videoWindow->mp_streamConfig;

	if (vsc->m_info.length() == 0)
	{
		return false;
	}

	// if we are playing, the Play button acts like a pause button
	if (m_is_playing) //  && m_is_paused)
	{
		Pause();
		return (!m_is_paused);
	}

	Stop();
	m_current_frame_num = 0;
	m_frame_count = 0;
	m_status = VIDEO_STATUS::WAITING_FOR_FIRST_FRAME;

	// loop media files:
	if (!mp_ffvideo->OpenMediaFile(vsc->m_info, vsc->m_frame_interval, vsc->m_loop_media_files))
	{
		m_status = VIDEO_STATUS::OPENING_MEDIA_FILE_FAILED;
		return false;
	}
	m_is_playing = true;
	return true;
}

///////////////////////////////////////////////////////////////////
bool RenderCanvas::PlayIPCamera(void)
{
	// if we are playing, the Play button acts like a pause button
	if (m_is_playing)
	{
		Pause();
		return (!m_is_paused);
	}

	Stop();
	m_current_frame_num = 0;
	m_frame_count = 0;
	m_status = VIDEO_STATUS::WAITING_FOR_FIRST_FRAME;

	VideoStreamConfig* vsc = mp_videoWindow->mp_streamConfig;

	if (!mp_ffvideo->OpenIPCamera( vsc->m_info, vsc->m_frame_interval ))
	{
		m_status = VIDEO_STATUS::OPENING_IP_FAILED;
		return false;
	}
	m_is_playing = true;
	mp_app->RememberThisURL(vsc->m_info);
	return true;
}

///////////////////////////////////////////////////////////////////
bool RenderCanvas::PlayUSBCamera(void)
{
	// if we are playing, the Play button acts like a pause button
	if (m_is_playing)
	{
		Pause();
		return (!m_is_paused);
	}

	Stop();
	m_current_frame_num = 0;
	m_frame_count = 0;
	m_status = VIDEO_STATUS::WAITING_FOR_FIRST_FRAME;

	VideoStreamConfig* vsc = mp_videoWindow->mp_streamConfig;

	FFVIDEO_USB_Camera_Format usb_format;

	std::vector<std::string> parts;
	parts = mp_app->split( vsc->m_info, std::string(" : ") );
	//
	int32_t usb_pin = (parts.size() > 0) ? std::atoi( parts[0].c_str() ) : 0;

	int32_t cam_fmt = (parts.size() > 1) ? std::atoi( parts[1].c_str() ) : 0;

	mp_ffvideo->GetUSB_CameraNames( mp_videoWindow->m_usb_names );
	if (usb_pin >= mp_videoWindow->m_usb_names.size())
		usb_pin = 0;

	bool good(true);
	std::vector<FFVIDEO_USB_Camera_Format> usb_formats;
	if (!mp_ffvideo->GetCameraPixelFormats( mp_videoWindow->m_usb_names[usb_pin].c_str(), usb_formats ))
		good = false;
	//
	if (cam_fmt >= usb_formats.size())
		cam_fmt = 0; 
	//
	if (!good)
	{
		usb_format.m_pixelFormat = "";
		usb_format.m_vcodec = "mjpeg";
		usb_format.m_min_width = 320;
		usb_format.m_max_width = 1920;
		usb_format.m_min_height = 240;
		usb_format.m_max_height = 1080;
		usb_format.m_min_fps = 5.0f;
		usb_format.m_max_fps = 30.0f;
	}
	else
	{
		usb_format = usb_formats[cam_fmt];
	}
	usb_format.m_name = mp_videoWindow->m_usb_names[usb_pin];

	if (!mp_ffvideo->OpenUSBCamera(usb_pin, vsc->m_frame_interval, &usb_format))
	{
		m_status = VIDEO_STATUS::OPENING_USB_FAILED;
		return false;
	}
	m_is_playing = true;
	return true;
}


////////////////////////////////////////////////////////////////////////////
// this runs 100 times per second and monitors delayed callbacks being due
void RenderCanvas::DelayeCallbacksMonitor(wxTimerEvent& event)
{
	if (!mp_app) 
		return;
	if (!mp_app->m_we_are_launched || m_terminating)
		return;

	m_delayedCallbacks.CallReadyCallbacks();

	// is the playback thread active and the video player paused? 
	if (mp_ffvideo->IsRunning() == false || mp_ffvideo->IsPlaybackActive() == false)
		Refresh();	// this, being called many times per second, causes the video frame to refresh when there is no video playing
}

///////////////////////////////////////////////////////////////////
void RenderCanvas::SendOverlayNoticeEvent( std::string& msg, int32_t milliseconds  )
{
	if (!mp_app) 
		return;
	if (!mp_app->m_we_are_launched || m_terminating)
		return;

	wxCommandEvent cmdEvt(wxEVT_VideoNoticeEvent);
	cmdEvt.SetString(wxString(msg.c_str()));
	cmdEvt.SetInt(milliseconds);	// milliseconds to display notice
	//
	AddPendingEvent(cmdEvt);
}

// the "export frame callback" is called with every frame export
// typedef void(*EXPORT_FRAME_CALLBACK_CB)(void* p_object, int32_t frame_num, const char* filepath, bool status);
//
///////////////////////////////////////////////////////////////////
void RenderCanvas::SendVideoEncodeEvent(void)
{
	if (!mp_app) 
		return;
	if (!mp_app->m_we_are_launched || m_terminating)
		return;

	wxCommandEvent evt(wxEVT_VideoEncodeEvent);
	AddPendingEvent(evt);
}

///////////////////////////////////////////////////////////////////
void RenderCanvas::SendVideoEncodeDoneEvent(void)
{
	if (!mp_app) 
		return;
	if (!mp_app->m_we_are_launched || m_terminating)
		return;

	wxCommandEvent evt(wxEVT_VideoEncodeDoneEvent);
	AddPendingEvent(evt);
}


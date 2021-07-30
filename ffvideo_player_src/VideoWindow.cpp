///////////////////////////////////////////////////////////////////////////////
// Name:        VideoWindow.cpp
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

#include <wx/aboutdlg.h>
#include "wx/display.h"
#include <wx/numdlg.h> 

#include "ffvideo_player_app.h"

#include <boost/date_time/posix_time/posix_time.hpp>

// These custom wxWidgets events need data defined, so here that is:
//
wxDEFINE_EVENT(wxEVT_VideoRefreshEvent, wxCommandEvent);
//
wxDEFINE_EVENT(wxEVT_VideoNoticeEvent, wxCommandEvent);
//
wxDEFINE_EVENT(wxEVT_QuitAppEvent, wxCommandEvent);


const long ID_STREAMCONFIG_MENU = wxNewId();
const long ID_NEWVIDEOWINDOW_MENU = wxNewId();
const long ID_RESETVIDEOLIB_MENU = wxNewId();

const long ID_REQUESTURL_DIALOG = wxNewId();
const long ID_REQUESTURL_USERTEXT = wxNewId();

const long ID_OPENFILE_MENU = wxNewId();
const long ID_SPECIFYURL_MENU = wxNewId();
const long ID_CHOOSEUSB_MENU = wxNewId();

const long ID_SAVEWINDOWCONFIG_MENU = wxNewId();
const long ID_RESTOREWINDOWCONFIG_MENU = wxNewId();

const long ID_PLAYALL_MENU = wxNewId();
const long ID_STOPALL_MENU = wxNewId();
const long ID_CLOSEALL_MENU = wxNewId();

const long ID_ABOUT_MENU = wxNewId();

const long ID_ENABLE_FACE_DETECTION_MENU = wxNewId();
const long ID_SET_FACE_DETECTION_PRECISION = wxNewId();
const long ID_ENABLE_FACE_FEATURE_DISPLAY_MENU = wxNewId();
const long ID_ENABLE_FACE_IMAGES_DISPLAY_MENU = wxNewId();
const long ID_ENABLE_FACE_IMAGE_STANDARDIZATION_MENU = wxNewId();

const long ID_TOGGLE_THEME_MENU = wxNewId();
const long ID_TILE_WINDOWS_MENU = wxNewId();
const long ID_HELP_MENU = wxNewId();

// this must be the last wxId because this id is used as index 0 for the USB camera names.
// Which means as many USB cameras as are connected to the host system will be used.
// So, make sure this is the last ID created, and those values greater are available. 
const long ID_CHOOSEUSBCAM0_MENU = wxNewId();

wxBEGIN_EVENT_TABLE(VideoWindow, wxFrame)
EVT_SIZE(VideoWindow::OnSize)
EVT_MOVE(VideoWindow::OnMove)
EVT_SCROLL(VideoWindow::OnScroll)
EVT_MENU(wxID_CLOSE, VideoWindow::OnCloseMenu)
wxEND_EVENT_TABLE()

////////////////////////////////////////////////////////////////////////
VideoWindow::VideoWindow(TheApp* app, int32_t id)
	: wxFrame(NULL, wxID_ANY, "FFVideo_player running ffmpeg 4.2.3"),
	mp_app(app), mp_streamConfig(NULL), m_id(id), m_terminating(false), m_usb_selection(-1), m_visible(false)
{
	// needs to be before the RenderCanvas() creation because that uses the ReportLog, so we need to init it first:
	mp_app->ReportLog(ReportLogOp::init, mp_app->FormatStr("\n\nhello world from window %d\n", m_id ));

	// if the stream id does not exist, a configuration is created with default values: 
	mp_streamConfig = new VideoStreamConfig(mp_app, mp_app->mp_config, m_id);

	// the RenderCanvas hosts the video library and is an OpenGL frame:
	mp_renderCanvas = new RenderCanvas(this, mp_streamConfig);

	// Make a menubar
	mp_menuBar = new wxMenuBar;
	SetMenuBar(mp_menuBar);

	// make a menu for beneath:
	mp_menu = new wxMenu;
	/*
	mp_menu->Append(ID_OPENFILE_MENU, "Open Media File...");
	//
	mp_menu->Append(ID_CHOOSEUSB_MENU, "Select USB WebCam...");
	mp_USBWebCamMenu = NULL;
	//
	mp_menu->Append(ID_SPECIFYURL_MENU, "Provide IP Camera playback URL...");
	//
	mp_menu->AppendSeparator(); 
	//
	Connect(ID_OPENFILE_MENU, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&VideoWindow::OnMediaFileOpen);
	Connect(ID_CHOOSEUSB_MENU, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&VideoWindow::OnChooseUSBWebCam);
	Connect(ID_SPECIFYURL_MENU, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&VideoWindow::OnSpecifyIPVideoURL);
	*/
	//
	mp_menu->Append(ID_STREAMCONFIG_MENU, "Video Stream Configuration...");
	//
	mp_menu->AppendSeparator();
	//
	mp_menu->Append(ID_NEWVIDEOWINDOW_MENU, "New Video Window");
	//
	// mp_menu->Append(ID_RESETVIDEOLIB_MENU, "Reset this Video Window's FFMPEG instance");
	//
	mp_menu->AppendSeparator();
	//
	mp_menu->Append(ID_RESTOREWINDOWCONFIG_MENU, "Restore saved multi-window configuration");
	//
	mp_menu->Append(ID_SAVEWINDOWCONFIG_MENU, "Save multi-window configuration");
	//
	mp_menu->AppendSeparator();
	//
	mp_menu->Append(ID_PLAYALL_MENU, "Play All (not playing)");
	//
	mp_menu->Append(ID_STOPALL_MENU, "Stop All");
	//
	mp_menu->AppendSeparator();
	//
	mp_menu->Append(ID_CLOSEALL_MENU, "Close all windows and quit");
	//
	// add menu to menuBar:
	mp_menuBar->Append(mp_menu, wxString::Format("&FFVideo_player Win%d", m_id));


	mp_optionsMenu = new wxMenu;
	//
	mp_faceDetectItem = mp_optionsMenu->Append(ID_ENABLE_FACE_DETECTION_MENU, "Enable face detection");
	//
	mp_optionsMenu->Append(ID_SET_FACE_DETECTION_PRECISION, "Set face detection precision...");
	//
	mp_faceFeaturesItem = mp_optionsMenu->Append(ID_ENABLE_FACE_FEATURE_DISPLAY_MENU, "Enable face feature display");
	//
	mp_faceImagesItem = mp_optionsMenu->Append(ID_ENABLE_FACE_IMAGES_DISPLAY_MENU, "Enable face images display");
	//
	mp_faceImagesStandarizedItem = mp_optionsMenu->Append(ID_ENABLE_FACE_IMAGE_STANDARDIZATION_MENU, "Enable face image standardization");
	//
	mp_optionsMenu->AppendSeparator();
	//
	mp_themeItem = mp_optionsMenu->Append(ID_TOGGLE_THEME_MENU, "Switch to \"Light Theme\"");
	//
	mp_tileWindowsItem = mp_optionsMenu->Append(ID_TILE_WINDOWS_MENU, "Tile video windows");
	//
	mp_optionsMenu->AppendSeparator();
	//
	mp_optionsMenu->Append(ID_HELP_MENU, "Help...");
	//
	mp_optionsMenu->Append(ID_ABOUT_MENU, "About FFVideo Player...");
	//
	// add menu to menuBar:
	mp_menuBar->Append(mp_optionsMenu, wxString("&Options"));



	Connect(ID_STREAMCONFIG_MENU, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&VideoWindow::OnStreamConfig);

	Connect(ID_NEWVIDEOWINDOW_MENU, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&VideoWindow::OnNewVideoWindow);

	Connect(ID_RESETVIDEOLIB_MENU, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&VideoWindow::OnResetVideoWindow);

	Connect(ID_SAVEWINDOWCONFIG_MENU,    wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&VideoWindow::OnSaveWindowConfig);
	Connect(ID_RESTOREWINDOWCONFIG_MENU, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&VideoWindow::OnRestoreWindowConfig);

	Connect(ID_PLAYALL_MENU, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&VideoWindow::OnPlayAll);
	Connect(ID_STOPALL_MENU, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&VideoWindow::OnStopAll);
	Connect(ID_CLOSEALL_MENU, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&VideoWindow::OnCloseAllAndQuit);

	Connect(ID_ABOUT_MENU, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&VideoWindow::OnAbout);

	Connect(wxEVT_CLOSE_WINDOW, (wxObjectEventFunction)&VideoWindow::OnCloseVideoWindow);



	Connect(ID_ENABLE_FACE_DETECTION_MENU, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&VideoWindow::OnToggleFaceDetection);
	Connect(ID_SET_FACE_DETECTION_PRECISION, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&VideoWindow::OnSetFaceDetectionPrecision);
	Connect(ID_ENABLE_FACE_FEATURE_DISPLAY_MENU, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&VideoWindow::OnToggleFaceFeatures);
	Connect(ID_ENABLE_FACE_IMAGES_DISPLAY_MENU, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&VideoWindow::OnToggleFaceImages);
	Connect(ID_ENABLE_FACE_IMAGE_STANDARDIZATION_MENU, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&VideoWindow::OnToggleFaceImageStandarization);
	

	Connect(ID_TOGGLE_THEME_MENU, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&VideoWindow::OnToggleTheme);
	Connect(ID_TILE_WINDOWS_MENU, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&VideoWindow::OnTileWindows);
	Connect(ID_HELP_MENU, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&VideoWindow::OnHelp);



	// a custom to event to end the application:
	Connect(wxEVT_QuitAppEvent, (wxObjectEventFunction)&VideoWindow::OnQuitApp);

	mp_statusBar = new wxStatusBar(this, wxID_ANY, wxNO_BORDER | wxSTB_ELLIPSIZE_END | wxFULL_REPAINT_ON_RESIZE);
	mp_statusBar->SetFieldsCount(2);
	mp_statusBar->SetStatusText(wxString::Format( "Win%d hosting video stream '%s'", m_id, mp_streamConfig->m_name.c_str() ), 0);

	// events generated outside this thread that are caught by this class and executed here:
	Bind(wxEVT_VideoRefreshEvent, &VideoWindow::OnVideoRefresh, this);

	mp_scrollbar = new wxScrollBar(this, wxID_ANY);
	mp_scrollbar->Disable();
	m_scroll_thumb_drag = false;	// track interactive use of scroll thumb

	SetClientSize(mp_streamConfig->m_win_width, 
								mp_streamConfig->m_win_height + mp_scrollbar->GetSize().GetHeight() + mp_statusBar->GetSize().GetHeight());
	SetPosition(wxPoint(mp_streamConfig->m_win_left, mp_streamConfig->m_win_top));

	// display our window:
	Show();
}

////////////////////////////////////////////////////////////////////////
VideoWindow::~VideoWindow() 
{ 
	if (mp_app)
	{
		if (mp_streamConfig)
		{ 
			delete mp_streamConfig;
			mp_streamConfig = NULL;
		}

		mp_app->RemoveVideoWindow(this);

		mp_app = NULL;
	}
};

///////////////////////////////////////////////////////////////////
ComboTextDlg::ComboTextDlg(wxWindow* parent, wxWindowID id, const wxString& title, int dialog_width, ComboTextParams& params)
	: wxDialog(parent, id, title, wxDefaultPosition, wxSize(740, 290)) 
{
	m_params = params; // remember the params

	const int fieldWidth = dialog_width - 20;
	const int textHeight = 25;

	wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);	// the outter container 

	wxStaticText* userPrompt_staticText = new wxStaticText(	// create static text 
																													this,
																													wxID_ANY,
																													m_params.m_prompt,
																													wxDefaultPosition, wxDefaultSize);
	//
	topSizer->Add(userPrompt_staticText, 0, wxALIGN_LEFT | wxALL, 10);

	wxComboBox* userText_comboBox = new wxComboBox(this,
																									ID_REQUESTURL_USERTEXT,
																									wxEmptyString,
																									wxDefaultPosition,
																									wxSize(fieldWidth, textHeight),
																									m_params.m_choices,
																									wxCB_DROPDOWN);
	//
	topSizer->Add(userText_comboBox, 0, wxALIGN_LEFT | wxALL, 10);

	wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	buttonSizer->Add(new wxButton(this, wxID_OK, wxT("Ok")), 0, wxALL, 10);
	buttonSizer->Add(new wxButton(this, wxID_CANCEL, wxT("Cancel")), 0, wxALL, 10);
	topSizer->Add(buttonSizer, 0, wxALIGN_CENTER);

	Connect(ID_REQUESTURL_USERTEXT, wxEVT_TEXT, (wxObjectEventFunction)&ComboTextDlg::OnTextEdit);
	Connect(ID_REQUESTURL_USERTEXT, wxEVT_TEXT_ENTER, (wxObjectEventFunction)&ComboTextDlg::OnTextEdit);
	Connect(ID_REQUESTURL_USERTEXT, wxEVT_COMBOBOX, (wxObjectEventFunction)&ComboTextDlg::OnTextEdit);

	SetSizer(topSizer);
	topSizer->Fit(this);
	topSizer->SetSizeHints(this);

	CenterOnParent();
}

///////////////////////////////////////////////////////////////////
ComboTextDlg::~ComboTextDlg() { }

///////////////////////////////////////////////////////////////////
void ComboTextDlg::OnTextEdit(wxCommandEvent& event)
{
	wxComboBox* c = (wxComboBox*)this->FindWindow(ID_REQUESTURL_USERTEXT);

	// regardless of this being a EVT_TEXT or a EVT_COMBOBOX event, this returns the current text:
	wxString s = c->GetValue();

	// verify there is a string there, and it's not too big:
	size_t l = wxStrlen(s);
	if (l > 0 && l < 1024)
	{
		m_params.m_usertext = s;
	}
}

/*
////////////////////////////////////////////////////////////////////////
void VideoWindow::OnMediaFileOpen(wxCommandEvent& WXUNUSED(event))
{
	if (m_terminating)
		return;

	if (mp_renderCanvas->m_is_playing)
		 mp_renderCanvas->Stop();

	std::string fname;
	if (GetOpenFilename(fname, NULL))
	{
		VideoStreamConfig* vsc = mp_streamConfig;

		mp_renderCanvas->m_status = VIDEO_STATUS::NEVER_PLAYED;
		vsc->m_type = STREAM_TYPE::FILE;
		if (mp_scrollbar)
			 mp_scrollbar->Enable();
		mp_renderCanvas->m_videoCtls.SetStreamType(vsc->m_type);
		vsc->m_info = fname;

		mp_renderCanvas->LoadLogoFrame();

		mp_renderCanvas->WindowStatus(wxString::Format("Set to %s", vsc->m_info.c_str()));

		mp_streamConfig->SaveStreamType( mp_app->mp_config );
	}
}

////////////////////////////////////////////////////////////////////////
void VideoWindow::OnChooseUSBWebCam(wxCommandEvent& WXUNUSED(event))
{
	if (m_terminating)
		return;

	if (mp_renderCanvas->m_is_playing)
		 mp_renderCanvas->Stop();

	if (!mp_USBWebCamMenu)
	{
		mp_renderCanvas->mp_ffvideo->GetUSB_CameraNames(m_usb_names);

		if (m_usb_names.size() > 0)
		{
			// create the submenu for selecting different webcams
			mp_USBWebCamMenu = new wxMenu(wxString("Select USB WebCam"));
			//
			for (int32_t i = 0; i < m_usb_names.size(); i++)
			{
				mp_USBWebCamMenu->Append(ID_CHOOSEUSBCAM0_MENU + i, wxString(m_usb_names[i].c_str()), wxEmptyString, wxITEM_CHECK);
				mp_USBWebCamMenu->Check(ID_CHOOSEUSBCAM0_MENU + i, (i == m_usb_selection));

				Connect(ID_CHOOSEUSBCAM0_MENU + i, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&VideoWindow::OnChosenUSBWebCam);
			}
		}
	}
	else
	{
		if (m_usb_names.size() > 0)
		{
			for (int32_t i = 0; i < m_usb_names.size(); i++)
			{
				mp_USBWebCamMenu->Check(ID_CHOOSEUSBCAM0_MENU + i, (i == m_usb_selection));
			}
		}
	}

	if (mp_USBWebCamMenu)
	{
		// Show menu at mouse position:
		wxPoint mouse_screen_pos = wxGetMousePosition();
		wxPoint window_pos = GetScreenPosition();
		wxPoint mouse_window_pos;
		mouse_window_pos.x = mouse_screen_pos.x - window_pos.x;
		mouse_window_pos.y = mouse_screen_pos.y - window_pos.y;

		PopupMenu(mp_USBWebCamMenu, mouse_window_pos);
	}
}

////////////////////////////////////////////////////////////////////////
void VideoWindow::OnChosenUSBWebCam(wxCommandEvent& event)
{
	if (m_terminating)
		return;

	int id = event.GetId();
	id -= ID_CHOOSEUSBCAM0_MENU;
	if (id >= 0 && id < m_usb_names.size())
	{
		m_usb_selection = id;

		VideoStreamConfig* vsc = mp_streamConfig;

		mp_renderCanvas->m_status = VIDEO_STATUS::NEVER_PLAYED;
		vsc->m_type = STREAM_TYPE::USB;
		if (mp_scrollbar) mp_scrollbar->Disable();
		mp_renderCanvas->m_videoCtls.SetStreamType(vsc->m_type);
		vsc->m_info = std::to_string(m_usb_selection);

		mp_renderCanvas->LoadLogoFrame();

		mp_renderCanvas->WindowStatus(wxString::Format("Set to USB Pin %d, %s", 
																	m_usb_selection, m_usb_names[m_usb_selection] ));

		mp_streamConfig->SaveStreamType( mp_app->mp_config );
	}
}

////////////////////////////////////////////////////////////////////////
void VideoWindow::OnSpecifyIPVideoURL(wxCommandEvent& WXUNUSED(event))
{
	if (m_terminating)
		return;

	if (mp_renderCanvas->m_is_playing)
		 mp_renderCanvas->Stop();

	std::string default_text("<playback URL>");
	if (mp_app->mp_config)
	{
		std::string data_key = mp_app->FormatStr("app/win%d_last_good_url", m_id);
		default_text = mp_app->mp_config->ReadString(data_key, "rtsp://192.168.0.17:554/cam/realmonitor?channel=1&subtype=0&unicast=true&proto=Onvif");
	}


	// wxString res = wxGetTextFromUser(
	// 	wxT("Enter the playback URL for the IP camera, including user/pass:"),
	// 	wxT("IP Camera playback URL"), default_text, this);
	// if (res == wxEmptyString)
	//  	return;

	// make sure last good url is first selection:
	mp_app->RememberThisURL(default_text);
	//
	std::vector<std::string> history;
	mp_app->GetURLHistory(history);
	//
	wxArrayString wxHistory;
	for (auto it = history.begin(); it != history.end(); ++it)
	{
		wxString wxUrl = it->c_str();
		wxHistory.Add(wxUrl);
	}
	//
	ComboTextParams params;
	params.m_prompt = wxString("Please enter an IP Camera / Video URL:");
	params.m_choices = wxHistory;
	params.m_usertext = wxEmptyString;
	//
	ComboTextDlg dialog(this, wxID_ANY, wxString("IP Video Playback URL"), 740, params );
	if (dialog.ShowModal() == wxID_OK)
	{
		std::string result = dialog.m_params.m_usertext.c_str().AsString();
		//
		// being nice to ONVIF Device Manager copied URLs:
		// (these URLs have underbar characters where ampersand characters should be)
	  //
		mp_app->stdReplaceAll(result, std::string("_password="), std::string("&password="));
		mp_app->stdReplaceAll(result, std::string("_channel="), std::string("&channel="));
		mp_app->stdReplaceAll(result, std::string("_stream="), std::string("&stream="));


		mp_app->RememberThisURL(result);
		
		VideoStreamConfig* vsc = mp_streamConfig;

		mp_renderCanvas->m_status = VIDEO_STATUS::NEVER_PLAYED;
		vsc->m_type = STREAM_TYPE::IP;
		if (mp_scrollbar) mp_scrollbar->Disable();
		mp_renderCanvas->m_videoCtls.SetStreamType(vsc->m_type);
		vsc->m_info = result;

		mp_renderCanvas->LoadLogoFrame();

		mp_renderCanvas->WindowStatus(wxString::Format("IP Camera URL set to %s", vsc->m_info.c_str()));

		mp_streamConfig->SaveStreamType( mp_app->mp_config );
	}
}
*/

////////////////////////////////////////////////////////////////////////
void VideoWindow::OnStreamConfig(wxCommandEvent& WXUNUSED(event))
{
	if (m_terminating)
		return;

	wxString bsjnk = wxString::Format( "Win%d hosting: \"%s\", View/Edit Video Stream Configuration Dialog", m_id, mp_streamConfig->m_name.c_str() );

	VideoStreamConfigDlg* p_dlg = new VideoStreamConfigDlg(mp_app, this, wxID_ANY, bsjnk, mp_streamConfig);

	if (p_dlg->ShowModal() == wxID_CANCEL) // everything important happens inside the dialog's event handlers
	{
		delete p_dlg;
		p_dlg = NULL;
	}
	else
	{
		if (!p_dlg->m_ok)
		{
			wxMessageBox("Stream configuration changes aborted.", "Video Stream Configuration Dialog" );
			return;
		}

		VideoStreamConfig* vsc = mp_streamConfig;

		if ((vsc->m_type != p_dlg->m_vsc.m_type) || (vsc->m_info.compare( p_dlg->m_vsc.m_info ) != 0))
		{
			mp_renderCanvas->Stop();
			mp_renderCanvas->mp_noticeMgr->AddNotice(std::string("Stopped."), 1.0f);
		}

		bool generate_render_font(false);
		if ((vsc->m_font_point_size  != p_dlg->m_vsc.m_font_point_size)  ||
				(vsc->m_font_italic      != p_dlg->m_vsc.m_font_italic)      ||
				(vsc->m_font_underlined  != p_dlg->m_vsc.m_font_underlined)  ||
				(vsc->m_font_strike_thru != p_dlg->m_vsc.m_font_strike_thru) ||
				(vsc->m_font_face_name.compare( p_dlg->m_vsc.m_font_face_name ) != 0))
				{
					generate_render_font = true;
				}

		// copy dialog's altered stream configuration to here:
		(*vsc) = (p_dlg->m_vsc);

		// no longer need the addstream dialog:
		delete p_dlg;
		p_dlg = NULL;

		mp_renderCanvas->m_OGL_font_dirty = generate_render_font;

		if (vsc->m_post_process.size() > 0)
		{
			mp_renderCanvas->mp_ffvideo->SetPostProcessFilter( vsc->m_post_process );
		}

		mp_renderCanvas->m_status = VIDEO_STATUS::NEVER_PLAYED;

		mp_statusBar->SetStatusText(wxString::Format( "Win%d hosting video stream '%s'", vsc->m_id, vsc->m_name.c_str() ), 0);

		if (mp_scrollbar)
		{
			if (vsc->m_type == STREAM_TYPE::FILE)
				   mp_scrollbar->Enable();
			else mp_scrollbar->Disable();
		}

		mp_renderCanvas->m_videoCtls.SetStreamType(vsc->m_type);

		// mp_renderCanvas->LoadLogoFrame();

		mp_renderCanvas->WindowStatus(wxString::Format("%s : %s", vsc->StreamTypeString().c_str(), vsc->m_info.c_str()));

		mp_streamConfig->Save( mp_app );
	}
}

////////////////////////////////////////////////////////////////////////
void VideoWindow::OnNewVideoWindow(wxCommandEvent& WXUNUSED(event))
{
	if (m_terminating)
		return;

	mp_app->NewVideoWindow();
}

////////////////////////////////////////////////////////////////////////
void VideoWindow::OnResetVideoWindow(wxCommandEvent& WXUNUSED(event))
{
	if (m_terminating)
		return;

	mp_renderCanvas->InitFFMPEG(true);
}


////////////////////////////////////////////////////////////////////////
void VideoWindow::OnSaveWindowConfig(wxCommandEvent& WXUNUSED(event))
{
	if (m_terminating)
		return;

	if (mp_app->mp_config)
	{
		std::string dataKey = "app/total_vw";
		mp_app->mp_config->WriteInt(dataKey, mp_app->m_videoWindowPtrs.size() );
	}
}


////////////////////////////////////////////////////////////////////////
void VideoWindow::OnRestoreWindowConfig(wxCommandEvent& WXUNUSED(event))
{
	if (m_terminating)
		return;

	mp_app->RestoreVideoWindows();
}



////////////////////////////////////////////////////////////////////////
void VideoWindow::OnPlayAll(wxCommandEvent& WXUNUSED(event))
{
	if (m_terminating)
		return;

	// experimental logic:
	// when a USB stream's play begins, an FFmpeg query mechanism is used
	// that is not thread safe. So this is me being lazy about preventing
	// collisions: simply play USB streams first, with a delay between them
	bool played_any(false);
	for (size_t i = 0; i < mp_app->m_videoWindowPtrs.size(); i++)
	{
		VideoWindow* vw = mp_app->m_videoWindowPtrs[i];

		if ((vw->mp_renderCanvas->m_is_playing == false) &&
		    (vw->mp_streamConfig->m_type == STREAM_TYPE::USB))
		{
			vw->mp_renderCanvas->Play();
			played_any = true;
			wxMilliSleep(400);
		}
	}
	
	// now play all the other streams:
	for (size_t i = 0; i < mp_app->m_videoWindowPtrs.size(); i++)
	{
		VideoWindow* vw = mp_app->m_videoWindowPtrs[i];

		if ((vw->mp_renderCanvas->m_is_playing == false) &&
				(vw->mp_streamConfig->m_type != STREAM_TYPE::USB))
			 vw->mp_renderCanvas->Play();
	}
}

////////////////////////////////////////////////////////////////////////
void VideoWindow::OnStopAll(wxCommandEvent& WXUNUSED(event))
{
	if (m_terminating)
		return;

	for (size_t i = 0; i < mp_app->m_videoWindowPtrs.size(); i++)
	{
		VideoWindow* vw = mp_app->m_videoWindowPtrs[i];

		if (vw->mp_renderCanvas->m_is_playing)
			vw->mp_renderCanvas->Stop();
	}
}

////////////////////////////////////////////////////////////////////////
void VideoWindow::OnCloseAllAndQuit(wxCommandEvent& WXUNUSED(event))
{
	if (m_terminating)
		return;

	// because video windows could have sub-threads decoding video 
	// // and/or exporting frames, each video window needs to be told 
	// to shutdown it's subthreads. The wxCloseEvent does that:  
	for (size_t i = 0; i < mp_app->m_videoWindowPtrs.size(); i++)
	{
		wxCloseEvent evt;
		mp_app->m_videoWindowPtrs[i]->AddPendingEvent(evt);
	}

	if (mp_app->mp_helpWindow)
	{
		wxCloseEvent e;
		mp_app->mp_helpWindow->OnCloseBox(e);
	}

	wxCommandEvent event(wxEVT_QuitAppEvent);
	AddPendingEvent(event);
}

////////////////////////////////////////////////////////////////////////
void VideoWindow::OnQuitApp(wxCommandEvent& WXUNUSED(event))
{
	wxExit(); // calls our OnExit() that writes the app config to the sqlite3 db
}


////////////////////////////////////////////////////////////////////////
void VideoWindow::OnCloseVideoWindow(wxCloseEvent& event)
{
	m_terminating = true;
	if (mp_renderCanvas)
	{
		mp_renderCanvas->m_terminating = true;
	
		if (mp_renderCanvas->m_status != VIDEO_STATUS::NEVER_PLAYED)
			 mp_renderCanvas->mp_ffvideo->KillStream();
	}

  // tell app this window no long exists:
	mp_app->RemoveVideoWindow(this);

	Destroy();
}

////////////////////////////////////////////////////////////////////////
void VideoWindow::OnCloseMenu(wxCommandEvent& event)
{
	m_terminating = true;
	if (mp_renderCanvas)
	{
		mp_renderCanvas->m_terminating = true;
	}

	// true is to force the frame to close
	// Close(true);
	mp_app->RemoveVideoWindow(this);
}

///////////////////////////////////////////////////////////////////
void VideoWindow::CalcLayout(
	int32_t* window_width,
	int32_t* window_height,
	int32_t* content_top,
	int32_t* content_height,
	int32_t* video_width,
	int32_t* video_height,
	int32_t* scroll_bar_height,
	int32_t* status_bar_height)
{
	*window_width = 0;
	*window_height = 0;
	*content_top = 0;
	*content_height = 0;
	*video_width = 0;
	*video_height = 0;
	*scroll_bar_height = 0;
	*status_bar_height = 0;

	GetClientSize(window_width, window_height);		// start with the window dimensions

	// wxWidgets possible bug: IsIconized() returns false, yet the window is minimized...
	if (((*window_width) <= 0) || ((*window_height) <= 0))
		return;

	*content_top = 0;
	*content_height = *window_height - *content_top;

	*status_bar_height = 0;  // status bar height
	if (mp_statusBar)
	{
		*status_bar_height = mp_statusBar->GetSize().y;
		(*content_height) -= (*status_bar_height);
	}

	if (mp_scrollbar && mp_renderCanvas) //  && mp_renderCanvas->m_stream_type == STREAM_TYPE_FILE)
	{
		*scroll_bar_height = mp_scrollbar->GetSize().y;
		(*content_height) -= (*scroll_bar_height);
	}

	*video_height = *content_height;

	*video_width = 640;
	if (mp_renderCanvas && (mp_renderCanvas->m_frame_loaded))
	{
		float raw_width = mp_renderCanvas->m_frame.m_width;
		float raw_height = mp_renderCanvas->m_frame.m_height;

		float frame_aspect = raw_width / raw_height;

		(*video_width) = (int32_t)((*video_height) * frame_aspect + 0.5f);

		if ((*window_width) - (*video_width) < 40)
		{
			(*video_width) = (*window_width) - 40;
			(*video_height) = (int32_t)((float)(*video_width) / frame_aspect + 0.5f);
		}

	}
}

///////////////////////////////////////////////////////////////////
void VideoWindow::LayoutSubwindows(void)
{
	int32_t window_width;
	int32_t window_height;
	int32_t content_top;
	int32_t content_height;
	int32_t video_width;
	int32_t video_height;
	int32_t scroll_bar_height;
	int32_t status_bar_height;


	CalcLayout(&window_width, &window_height, &content_top, &content_height,
		&video_width, &video_height, &scroll_bar_height, &status_bar_height);

	if (mp_renderCanvas)
	{
		mp_renderCanvas->SetSize(0, content_top, window_width, content_height);
	}

	if (mp_scrollbar)
	{
		mp_scrollbar->SetSize(0, content_top + content_height, window_width, scroll_bar_height);
	}

	if (mp_statusBar)
	{
		mp_statusBar->SetSize(0, content_top + content_height + scroll_bar_height, window_width, status_bar_height);
	}
}

///////////////////////////////////////////////////////////////////
void VideoWindow::SyncWindowSizeToConfig(int32_t w, int32_t h)
{
	if (!mp_app->m_we_are_launched)
		return;

	if (mp_streamConfig->m_win_width != w || mp_streamConfig->m_win_height != h)
	{
		mp_streamConfig->m_win_width = w;
		mp_streamConfig->m_win_height = h;

		mp_streamConfig->SaveWindowSize( mp_app->mp_config );
	}
}

///////////////////////////////////////////////////////////////////
void VideoWindow::OnSize(wxSizeEvent& event)
{
	bool old_visible = m_visible;

	// due to the nature of wxWidgets, this below case is "almost" never true:
	if (!IsShownOnScreen())
	{
		if (old_visible)
		{
			m_visible = false;
		}
		return;
	}

	// odd, should not link to this version, started after adding dlib to project...!
	// if (!IsMaximized())
	// {
		wxSize newSize = event.GetSize();
		SyncWindowSizeToConfig(newSize.x, newSize.y);
	// }

	if (!old_visible)
	{
		m_visible = true;
	}
	LayoutSubwindows();
}

///////////////////////////////////////////////////////////////////
void VideoWindow::OnMove(wxMoveEvent& event)
{
	if (!mp_app->m_we_are_launched)
		return;

	if (!IsShownOnScreen())
		return;

	// if (!IsMaximized())
	// {
		// all we need to do is remember this new position in the configuration, 
		// so it's there next time we startup: (Note: event's position is offset 
		// by window chrome, so GetScreenPosition() is used.) 
		GetScreenPosition(&mp_streamConfig->m_win_left, &mp_streamConfig->m_win_top);

		mp_streamConfig->SaveWindowPosition( mp_app->mp_config );
	// }
}

///////////////////////////////////////////////////////////////////
void VideoWindow::OnScroll(wxScrollEvent& event)
{
	if (!mp_scrollbar) return;

	VideoStreamConfig* vsc = mp_streamConfig;

	if (mp_renderCanvas)
	{
		if (vsc->m_type == STREAM_TYPE::FILE)
		{
			wxEventType eType = event.GetEventType();

			int32_t eCode = 0;
			if (eType == wxEVT_SCROLL_TOP)
				eCode = 1;
			else if (eType == wxEVT_SCROLL_BOTTOM)
				eCode = 2;
			else if (eType == wxEVT_SCROLL_THUMBTRACK)
				eCode = 3;
			else if (eType == wxEVT_SCROLL_THUMBRELEASE)
				eCode = 4;
			else if (eType == wxEVT_SCROLL_CHANGED)	 // anytime the scroll changes (including hot-key mapped events)
				eCode = 5;
			else if (eType == wxEVT_SCROLL_PAGEUP)	 // space between thumb and backwards arrow
				eCode = 6;
			else if (eType == wxEVT_SCROLL_PAGEDOWN) // space between thumb and forward arrow
				eCode = 7;
			else if (eType == wxEVT_SCROLL_LINEUP)   // backwards arrow
				eCode = 8;
			else if (eType == wxEVT_SCROLL_LINEDOWN) // forward arrow
				eCode = 9;
			if (!eCode)
				return;

			if (mp_renderCanvas->m_status == VIDEO_STATUS::RECEIVING_FRAMES && (eType == wxEVT_SCROLL_THUMBTRACK))
			{
				m_scroll_thumb_drag = true;
				mp_renderCanvas->Pause();
			}
			if (mp_renderCanvas->m_status == VIDEO_STATUS::RECEIVING_FRAMES && (eType == wxEVT_SCROLL_LINEUP || eType == wxEVT_SCROLL_LINEDOWN))
			{
				mp_renderCanvas->Pause();
			}

			int32_t  curr_frame = mp_renderCanvas->m_current_frame_num;
			int32_t  expected_frames = (int32_t)mp_renderCanvas->m_expected_frames;

			if (expected_frames > 0)
			{
				// because looping video file's m_current_frame_num does not reset when looping:
				curr_frame = curr_frame % expected_frames;

				bool good = ((eType == wxEVT_SCROLL_THUMBRELEASE) ||
					(eType == wxEVT_SCROLL_PAGEUP) ||
					(eType == wxEVT_SCROLL_PAGEDOWN));
				if (good)
				{
					int32_t new_pos = mp_scrollbar->GetThumbPosition();

					if ((eType == wxEVT_SCROLL_PAGEUP) || (eType == wxEVT_SCROLL_PAGEDOWN))
					{
						int32_t w, h;
						mp_scrollbar->GetClientSize(&w, &h);									// get scrollbar width

						const wxPoint pt0 = wxGetMousePosition();
						const wxPoint pt1 = mp_scrollbar->GetScreenPosition();
						int mouseX = pt0.x - pt1.x;															// get mouseX inside scrollbar

						float npos(mouseX);
						//
						npos /= w;
						if (npos > 1.0f)
							npos = 1.0f;
						else if (npos < 0.0f)
							npos = 0.0f;
						//
						new_pos = npos * expected_frames;
					}

					if (new_pos < 0)
						new_pos = 0;
					else if (new_pos > expected_frames)
						new_pos = expected_frames;

					int64_t ts;
					if (mp_renderCanvas->mp_ffvideo->CalcTimestampFromNormalizedPosition(ts, (float)new_pos / (float)expected_frames))
					{
						if (mp_renderCanvas->mp_ffvideo->Seek(ts, 0, false))
						{
							// seek sucess:
							// mp_scrollbar->SetThumbPosition( new_pos );

							mp_scrollbar->SetScrollbar(new_pos, 1, expected_frames, expected_frames / 8);
						}
					}
				}
				else if (eType == wxEVT_SCROLL_LINEUP)
				{
					mp_renderCanvas->mp_ffvideo->Step(FFVIDEO_FRAMESTEP_DIRECTION::BACKWARD);
				}
				else if (eType == wxEVT_SCROLL_LINEDOWN)
				{
					mp_renderCanvas->mp_ffvideo->Step(FFVIDEO_FRAMESTEP_DIRECTION::FORWARD);
				}
			}

			if (m_scroll_thumb_drag && (eType == wxEVT_SCROLL_THUMBRELEASE))
			{
				m_scroll_thumb_drag = false;
				mp_renderCanvas->Play();
			}
		}
	}

	event.Skip();
}

///////////////////////////////////////////////////////////////////
void VideoWindow::UpdateScrollbar(void)
{
	if (!mp_scrollbar) return;

	VideoStreamConfig* vsc = mp_streamConfig;

	if (mp_renderCanvas)
	{
		if (vsc->m_type == STREAM_TYPE::FILE)
		{
			if (mp_renderCanvas->m_current_frame_num != 0 && mp_renderCanvas->m_expected_frames > 0)
			{
				double duration(0.0);
				if (mp_renderCanvas->mp_ffvideo->GetDurationSeconds(duration))
				{
					mp_renderCanvas->m_expected_duration = (float)duration;
					mp_renderCanvas->m_expected_frames = mp_renderCanvas->m_expected_fps * mp_renderCanvas->m_expected_duration;
				}

				// because m_current_frame_num continues incrementing after relooping, rather than resetting to 0,
				// a modulus is needed to make sure the thumb position is set correctly:
				int thumb_pos = mp_renderCanvas->m_current_frame_num % mp_renderCanvas->m_expected_frames;
				mp_scrollbar->SetScrollbar(thumb_pos, 1, mp_renderCanvas->m_expected_frames, mp_renderCanvas->m_expected_frames / 8);
				mp_scrollbar->Refresh();
			}
		}
	}
}

///////////////////////////////////////////////////////////////////
bool VideoWindow::GetOpenFilename(std::string& fname, const char* szFilter)
{
	wxFileDialog openFileDialog(NULL,
		_("Open file"),
		wxEmptyString,
		wxEmptyString,
		szFilter,
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if (openFileDialog.ShowModal() == wxID_CANCEL)
		return false; // the user changed idea...

	// proceed loading the file chosen by the user;
	fname = openFileDialog.GetPath().c_str();
	return true;
}

///////////////////////////////////////////////////////////////////
void VideoWindow::SendVideoRefreshEvent(void)
{
	if (!mp_app) 
		return;
	if (!mp_app->m_we_are_launched || m_terminating)
		return;

	wxCommandEvent evt(wxEVT_VideoRefreshEvent);
	AddPendingEvent(evt);
}

///////////////////////////////////////////////////////////////////
void VideoWindow::OnVideoRefresh(wxCommandEvent& event)
{
	if (!mp_app->m_we_are_launched || m_terminating)
		return;

	if (mp_streamConfig->m_type == STREAM_TYPE::FILE)
		 UpdateScrollbar();
	//
	// this causes wxWidgets to call RenderCanvas::OnPaint()
	mp_renderCanvas->Refresh(false);
}

////////////////////////////////////////////////////////////////////////
void VideoWindow::OnAbout(wxCommandEvent& WXUNUSED(event))
{
	if (m_terminating)
		return;

	wxAboutDialogInfo info;
	info.SetName(_("FFVideo"));
	info.SetVersion(_("0.93 Beta, July 28, 2021"));

	wxString desc( "\nThis program is a series of things:\n\n" );
	desc += "* a video player supporting IP video, USB Camera and Media File sourced video,\n";
	desc += "* a C++ example FFmpeg based, multi-threaded, multi-window video player in wxWidgets,\n";
	desc += "* a framework for learning, video analysis, experiments, and model training,\n";
	desc += "* examples of integration with sqlite3, OpenGL, libjpeg-turbo, and Dlib, plus\n";
	desc += "* face detection, face feature identification, and general mad computer science.\n\n";
	//
	info.SetDescription(desc);

	info.SetCopyright(wxT("(C) 2021 Blake Senftner https://github.com/bsenftner/ffvideo"));
	
	wxAboutBox(info);
}

////////////////////////////////////////////////////////////////////////
void VideoWindow::OnToggleFaceDetection(wxCommandEvent& WXUNUSED(event))
{
	if (m_terminating)
		return;

	bool just_enabled(false);
	if (!mp_renderCanvas->IsFaceDetectionInitialized())
	{
		if (!mp_renderCanvas->EnableFaceDetection( true ))
    {
			wxMessageBox( "Error initializing face detector!" );
			return;
		}
		just_enabled = true;
	}

	if (!just_enabled)
	{
		mp_renderCanvas->EnableFaceDetection( !mp_renderCanvas->IsFaceDetectionEnabled() );
	}

	if (mp_faceDetectItem)
	{
		wxString msg;
		if (mp_renderCanvas->IsFaceDetectionEnabled())
		{
			msg = "Disable face detection";
		}
		else
		{
			msg = "Enable face detection";
		}
		mp_faceDetectItem->SetItemLabel(msg);
	}
}

////////////////////////////////////////////////////////////////////////
void VideoWindow::OnSetFaceDetectionPrecision(wxCommandEvent& WXUNUSED(event))
{
	if (m_terminating)
		return;

	wxString msg("Values close to 0 process faster, but distant faces are not detected.\nValues close to 200 detect distant faces, but process much slower."); 
	msg += "\nValues close to 25 correspond to a minimum head height of 1/2 the frame height,\n";
	msg += "\nValues close to 200 correspond to a minimum head height of 1/16th the frame height.";

	wxString prompt("Please enter a value between 20 and 200."); 
	wxString caption("Face Detection Precision"); 
	//
	wxPoint pos(  mp_renderCanvas->m_mpos.x,  mp_renderCanvas->m_mpos.y );

	long ret = wxGetNumberFromUser( msg, prompt, caption, (long)(mp_renderCanvas->m_faceDetectMgr.GetFaceDetectionScale() * 100.0f), 20, 200, this, pos );
	
	if (ret < 0)
	   return;

	float new_detection_scale = (float)ret / 100.0f;

	mp_renderCanvas->m_faceDetectMgr.SetFaceDetectionScale(new_detection_scale);
}

////////////////////////////////////////////////////////////////////////
void VideoWindow::OnToggleFaceFeatures(wxCommandEvent& WXUNUSED(event))
{
	if (m_terminating)
		return;

	mp_renderCanvas->EnableFaceLandmarks( !mp_renderCanvas->IsFaceLandmarksEnabled() );

	if (mp_faceFeaturesItem)
	{
		wxString msg;
		if (mp_renderCanvas->IsFaceLandmarksEnabled())
		{
			msg = "Disable face feature display";
		}
		else
		{
			msg = "Enable face feature display";
		}
		mp_faceFeaturesItem->SetItemLabel(msg);
	}
}

////////////////////////////////////////////////////////////////////////
void VideoWindow::OnToggleFaceImages(wxCommandEvent& WXUNUSED(event))
{
	if (m_terminating)
		return;

	mp_renderCanvas->EnableFaceImages( !mp_renderCanvas->IsFaceImagesEnabled() );

	if (mp_faceImagesItem)
	{
		wxString msg;
		if (mp_renderCanvas->IsFaceImagesEnabled())
		{
			msg = "Disable face images display";
		}
		else
		{
			msg = "Enable face images display";
		}
		mp_faceImagesItem->SetItemLabel(msg);
	}
}

////////////////////////////////////////////////////////////////////////
void VideoWindow::OnToggleFaceImageStandarization(wxCommandEvent& WXUNUSED(event))
{
	if (m_terminating)
		return;

	mp_renderCanvas->EnableFaceImageStandardization( !mp_renderCanvas->IsFaceImagesStandardized() );

	if (mp_faceImagesStandarizedItem)
	{
		wxString msg;
		if (mp_renderCanvas->IsFaceImagesEnabled())
		{
			msg = "Disable face image standardization";
		}
		else
		{
			msg = "Enable face image standardization";

			// this requires face landmarks, so enable them if not already enabled: 
			if (mp_renderCanvas->IsFaceLandmarksEnabled() == false)
			{
				wxCommandEvent dummy;
				OnToggleFaceFeatures( dummy );
			}
		}
		mp_faceImagesStandarizedItem->SetItemLabel(msg);
	}
}

////////////////////////////////////////////////////////////////////////
void VideoWindow::OnToggleTheme(wxCommandEvent& WXUNUSED(event))
{
	if (m_terminating)
		return;

	mp_renderCanvas->m_theme = !mp_renderCanvas->m_theme;

	SendVideoRefreshEvent();
}


/////////////////////////////////////////////////////////////////////////////////////
void VideoWindow::OnTileWindows( wxCommandEvent& event ) 
{
	wxDisplay primaryDisplay;

	// will be a vector of the monitor rectangles:
	std::vector<wxRect>  monitorRects;	
	//
	std::vector<VideoWindow*>& windows = mp_app->m_videoWindowPtrs;
	//
	uint32_t display_count = primaryDisplay.GetCount();
	for (size_t i = 0; i < display_count; i++)
	{
		wxDisplay monitor(i);
		wxRect clientArea = monitor.GetGeometry(); 

		// do accomidate windows bottom of the monitor toolbars:
		clientArea.SetBottom( clientArea.GetBottom() - 40 );

		monitorRects.push_back(clientArea);
	}

	// will be for each video window, which monitor it is on:
	std::vector<int32_t> win_indexes;		
	//
	uint32_t window_count = windows.size();
	for (size_t i = 0; i < window_count; i++)
	{
		int32_t display_index = primaryDisplay.GetFromWindow( windows[i] );
		win_indexes.push_back( display_index );
	}

	// looping over each monitor:
	for (int32_t i = 0; i < monitorRects.size(); i++)
	{
		std::vector<int32_t> wins;		// will be indexes of windows in this monitor

		// looping over each window:
		for (size_t j = 0; j < window_count; j++)
		{
			// is this window on the current monitor?
			if (win_indexes[j] == i) 
				wins.push_back(j);			// yes
		}

		if (wins.size() > 0)
		{
			// we have windows on this monitor

			int32_t m_l = monitorRects[i].GetLeft();
			int32_t m_t = monitorRects[i].GetTop();

			int32_t m_w = monitorRects[i].GetWidth();
			int32_t m_h = monitorRects[i].GetHeight();

			if (wins.size() == 1)
			{
				// we have one window this monitor
				wxFrame* p_win = windows[wins[0]];
				p_win->SetSize(m_l, m_t, m_w, m_h);
			}
			else if (wins.size() <= 4)
			{
				int32_t w_w = m_w / 2;
				int32_t w_h = m_h / ((wins.size() + 1) / 2);

				bool is_odd = false;
				if (wins.size() % 2 != 0) is_odd = true;

				for (uint32_t j = 0; j < wins.size(); j++)
				{
					int32_t left = m_l;
					if ((j % 2) != 0) left += w_w;
					int32_t top = m_t + ((j / 2) * w_h);

					wxFrame* p_win = windows[wins[j]];

					p_win->SetSize(left, top, w_w, w_h);
					wxYield();
					wxMilliSleep(50);
				}
			}
			else if (wins.size() <= 9)
			{
				int32_t w_w = m_w / 3;
				int32_t w_h = m_h / ((1 + wins.size()) / 3);

				for (uint32_t j = 0; j < wins.size(); j++)
				{
					int32_t left = m_l + ((j % 3) * w_w);
					int32_t top = m_t;
					if (j >= 3) top += w_h;
					if (j >= 6) top += w_h;

					wxFrame* p_win = windows[wins[j]];

					p_win->SetSize(left, top, w_w, w_h);
					wxYield();
					wxMilliSleep(50);
				}
			}
			else if (wins.size() <= 12)
			{
				int32_t w_w = m_w / 4;
				int32_t d = ff_round(ff_ceil((float)wins.size() / 4.0f));
				int32_t w_h = m_h / d;

				for (uint32_t j = 0; j < wins.size(); j++)
				{
					int32_t left = m_l + ((j % 4) * w_w);
					int32_t top = m_t + ((j / 4) * w_h);

					wxFrame* p_win = windows[wins[j]];

					p_win->SetSize(left, top, w_w, w_h);
					wxYield();
					wxMilliSleep(50);
				}
			}
			else 
			{
				int32_t w_w = m_w / 5;
				int32_t d = ff_round(ff_ceil((float)wins.size() / 5.0f));
				int32_t w_h = m_h / d;

				for (uint32_t j = 0; j < wins.size(); j++)
				{
					int32_t left = m_l + ((j % 5) * w_w);
					int32_t top = m_t + ((j / 5) * w_h);

					wxFrame* p_win = windows[wins[j]];

					p_win->SetSize(left, top, w_w, w_h);
					wxYield();
					wxMilliSleep(50);
				}
			}
		}
	}
}


////////////////////////////////////////////////////////////////////////
void VideoWindow::OnHelp(wxCommandEvent& WXUNUSED(event))
{
	if (m_terminating)
		return;

	mp_app->ShowHelp( this );
}


#pragma once
#ifndef _VIDEOWINDOW_H_
#define _VIDEOWINDOW_H_

// expiring messages overlaid over the video frame:
#include "NoticeMgr.h"

// call function after a delay:
#include "DelayedCallbackMgr.h"

// call function over duration with interpolated 't' value:
#include "TweenedCallbackMgr.h"

// defines config class for video configurations:
#include "VideoStreamConfig.h"

// an experiment:
#include "FaceDetect.h"



// wxWidgets custom events:
// 
// define an event to be used outside the GUI thread to trigger a refresh
wxDECLARE_EVENT(wxEVT_VideoRefreshEvent, wxCommandEvent);
//
// define an event to be used outside the GUI thread to add a Notice to the video window
wxDECLARE_EVENT(wxEVT_VideoNoticeEvent, wxCommandEvent);
//
// define an event to close/quit the application, because we have window
// (video decompression and exporting) threads that need to die and that takes a beat
wxDECLARE_EVENT(wxEVT_QuitAppEvent, wxCommandEvent);



///////////////////////////////////////////////////////////
// our VCR style buttons for controlling the video stream
#include "VideoCtrls.h"

////////////////////////////////////////////////////////////////////////
// predeclarations:
class TheApp;
class RenderCanvas;

///////////////////////////////////////////////////////////////////////////////
// Define a new frame type for use as a floating window hosting a video player
class VideoWindow : public wxFrame
{
public:
	VideoWindow(TheApp* app, int32_t ordinal);

	~VideoWindow();

	bool GetOpenFilename(std::string& fname, const char* szFilter);

	TheApp*							mp_app;
	VideoStreamConfig*	mp_streamConfig;
	int32_t							m_id;									// which video window are we? 
	RenderCanvas*				mp_renderCanvas;			// OpenGL frame host video library instance
	bool								m_visible;						// shown on screen/desktop
	bool								m_terminating;

	wxMenu*							mp_menu;
	wxMenu*							mp_optionsMenu;
	wxMenuItem*					mp_faceDetectItem;
	wxMenuItem*					mp_faceFeaturesItem;

	wxMenuItem*					mp_themeItem;
	wxMenuItem*					mp_tileWindowsItem;

	std::vector<std::string>	m_usb_names;
	int32_t										m_usb_selection;

	wxMenuBar*		mp_menuBar;
	wxScrollBar*	mp_scrollbar;					// if a media file, used for seeks, otherwise disabled
	bool					m_scroll_thumb_drag;	// manipulation state for scroll thrumb
	wxStatusBar*	mp_statusBar;

	void CalcLayout(
		int32_t* window_width,
		int32_t* window_height,
		int32_t* content_top,
		int32_t* content_height,
		int32_t* video_width,
		int32_t* video_height,
		int32_t* scroll_bar_height,
		int32_t* status_bar_height);
	void LayoutSubwindows(void);

	void SyncWindowSizeToConfig(int32_t w, int32_t h);

	void OnSize(wxSizeEvent& event);
	void OnMove(wxMoveEvent& event);
	void OnScroll(wxScrollEvent& event);

	void OnStreamConfig(wxCommandEvent& event);
	void OnNewVideoWindow(wxCommandEvent& event);
	void OnResetVideoWindow(wxCommandEvent& event);

	void OnPlayAll(wxCommandEvent& event);
	void OnCloseAllAndQuit(wxCommandEvent& event);

	void OnAbout(wxCommandEvent& event);

	void OnSaveWindowConfig(wxCommandEvent& event);
	void OnRestoreWindowConfig(wxCommandEvent& event);
	/*
	void OnMediaFileOpen(wxCommandEvent& event);
	void OnChooseUSBWebCam(wxCommandEvent& event);
	void OnChosenUSBWebCam(wxCommandEvent& event);
	void OnSpecifyIPVideoURL(wxCommandEvent& event);
	*/
	void OnCloseMenu(wxCommandEvent& event);
	void OnCloseVideoWindow(wxCloseEvent& event);
	void OnQuitApp(wxCommandEvent& event);

	void SendVideoRefreshEvent(void);
	void OnVideoRefresh(wxCommandEvent& event);

	void UpdateScrollbar(void);

	void OnToggleFaceDetection(wxCommandEvent& event);
	void OnToggleFaceFeatures(wxCommandEvent& event);

	void OnToggleTheme(wxCommandEvent& event);
	void OnTileWindows(wxCommandEvent& event);
	void OnHelp(wxCommandEvent& event);

	wxDECLARE_EVENT_TABLE();
};




#endif // _VIDEOWINDOW_H_

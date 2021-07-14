#pragma once
#ifndef _HELPWINDOW_H_
#define _HELPWINDOW_H_

#include "ffvideo_player_app.h"

#include <wx/wxhtml.h>

#include "wx/webview.h"
#include "wx/webviewarchivehandler.h"
#include "wx/webviewfshandler.h"

#include <wx/fs_mem.h>

////////////////////////////////////////////////////////////////////////
// predeclarations:
class TheApp;

///////////////////////////////////////////////////////////////////////////////
// Define a new frame type for use as a floating window hosting a video player
class HelpWindow : public wxFrame
{
public:
	HelpWindow(TheApp* app, wxWindow* parent );

	~HelpWindow();

	void BuildFeatureBullets( void );
	void BuildHTMLBody( void );							
	void ReplaceHTML( wxString& body );
	wxString GetWebContentCSS(void);
	wxString GetWebContentJavaScriptCallbacks(void);

	void RefreshWebView( void );


	TheApp*							mp_app;
	wxWebView*					mp_help_view;

	wxString						m_html_filename;		
	wxString						m_url;					// our page, used to check if we're still on that page
	wxString						m_feature_bullet0;
	wxString						m_feature_bullet1;
	wxString						m_feature_bullet2;
	wxString            m_html_body;		// the current body, set by BuildHTMLBody()

	bool								m_include_A;		// these 3 bools are used to demo C++/webView communications
	bool								m_include_B;
	bool								m_include_C;

	void OnWebViewTitleChangedCB(wxWebViewEvent& event);
	void OnWebViewNavigationCB(wxWebViewEvent& event);

	void OnSize(wxSizeEvent& event);
	void OnMove(wxMoveEvent& event);
	void OnCloseBox(wxCloseEvent& event);

	wxDECLARE_EVENT_TABLE();
};




#endif // _HELPWINDOW_H_

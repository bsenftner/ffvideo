/////////////////////////////////////////////////////////////////////////////
// Name:        ffvideo_player.h
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_FFVIDEO_PLAYER_H_
#define _WX_FFVIDEO_PLAYER_H_

#include "windows.h"
#include <math.h>
#include <stdint.h>
#include <inttypes.h>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <string>
#include <array>
#include <vector>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>
#include <map>

#include <GL/glew.h>

#ifdef __cplusplus
extern "C" {
#include "../../jpeg/jpeglib.h"
#include "../../jpeg/jerror.h"
}
#endif

// C++ jpeg encode/decode
#include "..\..\jpgcpp\jpge.h"
#include "..\..\jpgcpp\jpgd.h"

/////////////////////////////////////////////////////////////////////////////
// wxWidgets & OpenGL stuff
// we need OpenGL headers for GLfloat/GLint types used below
#include "wx/wx.h"
#include "wx/glcanvas.h"

// ffmpeg wrapper library:
#include "ffvideo.h"

// common structures, classes and utilities:
#include "util.h"

// an sqlite3 key/value store:
#include "kvs.h"

// defines a wxFrame that hosts an upper level video player:
#include "VideoWindow.h"

// defines a frame encoding thread for generating .mp4 and .264 streams:
#include "FrameEncoder.h"

// defines a wxGLCanvas that hosts lower level video player:
#include "RenderCanvas.h"

// defines a wxDialog for video configuration:
#include "VideoConfigDialog.h"

// defines a wxDialog for USB camera configuration:
#include "USBCamConfigDlg.h"

#include "HelpWindow.h"

////////////////////////////////////////////////////////////////////////////////
// The "report log" takes an operator because this logger retains log messages
// until flushed. Useful when logging from multiple threads. 
enum class ReportLogOp
{
	init = 0,
	log,
	flush
};


////////////////////////////////////////////////////////////////////////
// predeclarations:
// class RenderCanvas;
// class VideoWindow;
class HelpWindow;


////////////////////////////////////////////////////////////////////////
struct ComboTextParams
{
	wxString      m_prompt;		  // text vertically above combo box
	wxArrayString m_choices;      // other text choices (use history?)
	wxString      m_usertext;     // user's response to dialog
};

class ComboTextDlg : public wxDialog
{
public:
	ComboTextDlg(wxWindow* parent, wxWindowID id, const wxString& title, int32_t dialog_width, ComboTextParams& ctp);
	~ComboTextDlg();

	void OnTextEdit(wxCommandEvent& event);

	ComboTextParams m_params;
};


////////////////////////////////////////////////////////////////////////
// Define a new application type
class TheApp : public wxApp
{
public:

		TheApp() : mp_config(NULL), mp_helpWindow(NULL), m_last_video_window_id(-1), m_we_are_launched(false) {	}

    // virtual wxApp methods
    virtual bool OnInit() wxOVERRIDE;
    virtual int  OnExit() wxOVERRIDE;

		void RecoverAppConfig(void);

		int32_t GenNextVideoWindowId(void);		

		void InitVideoWindow(VideoWindow* vw);

		bool NewVideoWindow(void);						// generates an unused id
		bool NewVideoWindow(int32_t id);			// returns false if id already in use
		bool RemoveVideoWindow(VideoWindow*);

		void ShowHelp(wxWindow* parent);	// opens an HTML window containing application help

		VideoWindow* GetVideoWindow( int32_t id ); // returns NULL if not in m_videoWindowPtrs

		void RestoreVideoWindows( void );

		// adds url to history as first url in history:
		bool RememberThisURL(std::string& url);
		bool GetURLHistory(std::vector<std::string>& history);
		bool SetURLHistory(std::vector<std::string>& history);
		bool SetURLHistoryLimit(uint32_t limit);


		// adds post_process_filter to history as first post_process_filter in history:
		bool RememberThisPostProcessFilter(std::string& url);
		bool GetPostProcessFilterHistory(std::vector<std::string>& history);
		bool SetPostProcessFilterHistory(std::vector<std::string>& history);
		bool SetPostProcessFilterHistoryLimit(uint32_t limit);


		// application logging:
		std::string FormatStr(const char* formatStr, ...);
private:
		void _ReportLog(std::string& log_path, std::string msg);
public:
		// use one of the ReportLogOps defined above. 
		// implemented like this so msgs from multiple threads don't get interleaved
		void ReportLog(ReportLogOp op, std::string msg);

		// utility
		int32_t ParseInteger( char *param, int32_t *ret, int32_t min, int32_t max );
		int32_t VerifySomeNumber( char *param );
		int32_t ParseFloat( char *param, float *ret, float min, float max );
		void stdReplaceAll(std::string& str, const std::string& from, const std::string& to);
		std::string GetPath(std::string& str);
		std::string GetFilename(std::string& str);
		std::string GetExtension(std::string& str);
		bool CompareIgnoringCase(const std::string& a, const std::string& b);
		std::vector<std::string> split(const std::string& text, const std::string& delims);
		bool FindFiles(std::vector<std::string>& fnames, const char* path, const char* wildCards);
		int32_t IsDirectory(const char* path);
		bool IsFile(const char* path);

		CKeyValueStore* mp_config;
		std::mutex	    m_mutex;			// multi-threaded security for access to mp_config writes

		bool						m_we_are_launched;
		int32_t					m_last_video_window_id;
		std::string			m_data_dir;
		std::string			m_retainedLog;
		std::string			m_logPath;
		BCTime					m_clock;				// maintains time since app launch

		std::vector<VideoWindow*> m_videoWindowPtrs;

		HelpWindow*			mp_helpWindow;
};



#endif // _WX_FFVIDEO_PLAYER_H_

#pragma once

#ifndef _VIDEOCONFIGDIALOG_H_
#define _VIDEOCONFIGDIALOG_H_


// #include "ffvideo_player_app.h"

#include <ctype.h>

///////////////////////////////////////////////////////////////////
class VideoStreamConfigDlg : public wxDialog
{
public:
	VideoStreamConfigDlg(TheApp* app, 
										wxWindow* parent, 
										wxWindowID id, 
										const wxString& title, 
										VideoStreamConfig* params);
	~VideoStreamConfigDlg();

	void OnNameEdit(wxCommandEvent& event);

	void OnVideoType(wxCommandEvent& event);
	void UpdateVideoType(void);

	void OnLoopMediaFiles(wxCommandEvent& event);

	void OnInfoEdit(wxCommandEvent& event);

	void OnPostProcessFilterEdit(wxCommandEvent& event);

	// void OnFontEdit(wxFontPickerEvent& event);
	void OnFontButton(wxCommandEvent& event);
	//
	void OnConfigButton(wxCommandEvent& event);
	void OnMediaFileBrowse(wxCommandEvent& event);
	void OnUSBWebCamConfig(wxCommandEvent& event);
	void OnIPStreamUrlConfig(wxCommandEvent& event);

	void DoMediaFileBrowse(void);
	void DoUSBWebCamConfig(void);
	void DoIPStreamUrlConfig(void);

	void OnFrameInterval(wxCommandEvent& event);
	void OnAutoFrameIntervalButton(wxCommandEvent& event);

	void OnExportInterval(wxCommandEvent& event);
	void OnExportDirButton(wxCommandEvent& event);
	void OnExportBasenameButton(wxCommandEvent& event);
	void OnExportScaleEdit(wxCommandEvent& event);
	void OnExportQuality(wxCommandEvent& event);

	void OnEncodeDirButton(wxCommandEvent& event);
	void OnEncodeBasenameButton(wxCommandEvent& event);

	void OnEncodeInterval(wxCommandEvent& event);
	void OnFFmpegPathButton(wxCommandEvent& event);
	void OnPostEncodeAction(wxCommandEvent& event);
	void OnMovePathButton(wxCommandEvent& event);
	void OnEncodeFPS(wxCommandEvent& event);
	void OnEncodeType(wxCommandEvent& event);
	void OnEncodeWH(wxCommandEvent& event);

	// validate all input before accepting: 
	void OnOkayButton(wxCommandEvent& event);

	bool HasInvalidFilenameChars(std::string& path)
	{
		std::string::iterator it;
		for (it = path.begin(); it != path.end(); it++) 
		{
			if (isspace( *it )) return true;
			if (iscntrl( *it )) return true;
			if (ispunct( *it )) return true;
		}
		return false;
	}

	void UpdateExampleFrameExportPath( void );
	void UpdateExampleFrameEncodePath( void );
	void UpdateFFmpegPath( void );
	void UpdatePostEncodeMovePath( void );

	TheApp*										mp_app;
	VideoWindow*							mp_parent;

	VideoStreamConfig					m_vsc;
	bool											m_ok;

	// all streams and their ids, with ours first (whom ever "ours" is)
	std::map<std::string,int32_t>	m_namesMap;
			
	wxCheckBox*								mp_loopMediaCtrl;
	wxComboBox*								mp_streamTypeCtrl;
	wxButton*									mp_config_button;
	wxTextCtrl*								mp_infoCtrl;
	wxSpinCtrl*								mp_frameIntervalCtrl;
	wxButton*									mp_autoFrameInterval_button;

	wxSpinCtrl*								mp_exportIntervalCtrl;
	wxButton*									mp_exportDir_button;
	wxButton*									mp_exportBasename_button;
	wxStaticText*							mp_exampleFrameExportPath;
	wxTextCtrl*								mp_exportScaleCtrl;
	wxSpinCtrl*								mp_exportQualityCtrl;

	wxComboBox*								mp_encodeTypeCtrl;
	wxSpinCtrl*								mp_encodeIntervalCtrl;
	wxButton*									mp_ffmpegPath_button;
	wxStaticText*							mp_ffmpegPath;
	wxStaticText*							mp_postEncodeMovePath;
	wxComboBox*								mp_postEncodeCtrl;
	wxButton*									mp_movePath_button;
	wxButton*									mp_encodeDir_button;
	wxButton*									mp_encodeBasename_button;
	wxStaticText*							mp_exampleFrameEncodePath;
	wxTextCtrl*								mp_encodeFPSCtrl;
	wxTextCtrl*								mp_encodeWHCtrl;

	// wxFont* 									mp_overlayFont;
	wxButton*									mp_font_button;

};



#endif // _VIDEOCONFIGDIALOG_H_

#pragma once
#pragma once

#ifndef _USBCAMCONFIGDIALOG_H_
#define _USBCAMCONFIGDIALOG_H_


#include "ffvideo_player_app.h"


///////////////////////////////////////////////////////////////////
class USBCamConfigDlg : public wxDialog
{
public:
	USBCamConfigDlg(TheApp* app, 
									wxWindow* parent, 
									wxWindowID id, 
									const wxString& title, 
									VideoStreamConfig* params,
									std::vector<std::string>& usb_camNames );
	~USBCamConfigDlg(){};


	void GenCamFormatInfo( const char* cam_name );
	std::string CameraFormatAsString( FFVIDEO_USB_Camera_Format& format );

	// validate all input before accepting: 
	void OnOkayButton(wxCommandEvent& event);


	TheApp*										mp_app;
	VideoStreamConfigDlg*			mp_parent;

	VideoStreamConfig					m_vsc;
	std::vector<std::string>  m_camNames;

	wxListBox*								mp_cameraCtrl;
	wxListBox*								mp_formatCtrl;
	wxButton*									mp_okButton;
	wxButton*									mp_cancelButton;

	int32_t										m_curr_cam, m_curr_format;

	void OnSize(wxSizeEvent& event);
	void OnCameraSelection(wxCommandEvent& event);
	void OnFormatSelection(wxCommandEvent& event);

	wxDECLARE_EVENT_TABLE();
};



#endif // _USBCAMCONFIGDIALOG_H_

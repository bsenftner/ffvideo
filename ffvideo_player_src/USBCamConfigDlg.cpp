///////////////////////////////////////////////////////////////////////////////
// Name:        USBCamConfigDlg.cpp
// Author:      Blake Senftner
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
// include main wxWidgets
///////////////////////////////////////////////////////////////////
#include "wx/wx.h"
#include "wx/choice.h"
#include "wx/dialog.h"
#include <wx/textdlg.h>
#include "wx/spinctrl.h"

#include "ffvideo_player_app.h"

static const long ID_USBCAM_CAMERAS  = wxNewId();
static const long ID_USBCAM_FORMATS = wxNewId();

wxBEGIN_EVENT_TABLE(USBCamConfigDlg, wxDialog)
EVT_SIZE(USBCamConfigDlg::OnSize)
wxEND_EVENT_TABLE()

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USBCamConfigDlg::USBCamConfigDlg(TheApp* app, wxWindow* parent, wxWindowID id, const wxString& title, 
																 VideoStreamConfig* params, std::vector<std::string>& usb_camNames)
		: mp_app(app), mp_parent((VideoStreamConfigDlg*)parent), m_vsc(*params), m_camNames(usb_camNames), 
	    mp_cameraCtrl(NULL), mp_formatCtrl(NULL), m_curr_cam(0), m_curr_format(0), 
			wxDialog( parent, id, title, wxDefaultPosition, wxSize(600, 300), wxRESIZE_BORDER | wxDEFAULT_DIALOG_STYLE)
{
	wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);    // the outer container is a vertical sizer
	wxPanel    *panel = new wxPanel(this, -1);				// vbox will hold panel and hbox
	wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);

	mp_cameraCtrl = new wxListBox(this, ID_USBCAM_CAMERAS, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE | wxLB_NEEDED_SB );

	mp_formatCtrl = new wxListBox(this, ID_USBCAM_FORMATS, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE | wxLB_NEEDED_SB );

	m_camNames = usb_camNames;
	for (int32_t i = 0; i < m_camNames.size(); i++)
	{
		mp_cameraCtrl->Append( wxString( m_camNames[i].c_str() ) );
	}

	m_curr_cam = std::atoi( m_vsc.m_info.c_str() );
	if (m_curr_cam >= m_camNames.size())
		 m_curr_cam = 0;

	m_curr_format = 0;

	GenCamFormatInfo( m_camNames[ m_curr_cam ].c_str() );

	mp_cameraCtrl->SetSelection( m_curr_cam );
	mp_formatCtrl->SetSelection( m_curr_format );

	Connect(ID_USBCAM_CAMERAS, wxEVT_LISTBOX, (wxObjectEventFunction)&USBCamConfigDlg::OnCameraSelection);
	Connect(ID_USBCAM_FORMATS, wxEVT_LISTBOX, (wxObjectEventFunction)&USBCamConfigDlg::OnFormatSelection);


	mp_okButton     = new wxButton(this, wxID_OK,     wxT("Ok"));
	mp_cancelButton = new wxButton(this, wxID_CANCEL, wxT("Cancel"));

	hbox->Add(mp_okButton,     1, wxGROW | wxALL, 10);
	hbox->Add(mp_cancelButton, 0, wxGROW | wxALL, 10);

	vbox->Add(panel, 1);
	vbox->Add(hbox,  1, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);


	SetSizer(vbox);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::string USBCamConfigDlg::CameraFormatAsString( FFVIDEO_USB_Camera_Format& format )
{
	char* desc = (format.m_pixelFormat.size()>0) ? format.m_pixelFormat.c_str() : 
							 (format.m_vcodec.size()>0)      ? format.m_vcodec.c_str() : "?";

	std::string bsjnk = mp_app->FormatStr( "%s (%dx%d) fps-min:%1.1f fps-max:%1.15f", desc,
																					format.m_min_width, format.m_min_height, 
																					format.m_min_fps, format.m_max_fps );
	return bsjnk;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USBCamConfigDlg::GenCamFormatInfo( const char* cam_name )
{
	// clear out old info:
	mp_formatCtrl->Clear();

	std::vector<FFVIDEO_USB_Camera_Format> usb_formats;
	if (mp_parent->mp_parent->mp_renderCanvas->mp_ffvideo->GetCameraPixelFormats( cam_name, usb_formats ))
	{
	  for (size_t i = 0; i < usb_formats.size(); i++)
		{
			FFVIDEO_USB_Camera_Format& format = usb_formats[i];

		  mp_formatCtrl->Append( wxString( CameraFormatAsString( format ).c_str() ) );
		}
	}
	else
	{
		wxMessageBox("Error getting camera formats!" );
	}
}

///////////////////////////////////////////////////////////////////
void USBCamConfigDlg::OnSize(wxSizeEvent& event)
{
	if (!IsShownOnScreen()) return;
	
	int32_t winWidth, winHeight;
	GetClientSize(&winWidth, &winHeight);
	if (winWidth <= 0 || winHeight <= 0) 
	   return;

	int32_t border = 10;
	int32_t bttnHeight = 25;
	int32_t bttnWidth = winWidth / 2 - border * 2;

	int32_t leftPart = ff_round(0.25f * (winWidth - 3 * border));
	int32_t ritePart = ff_round(0.75f * (winWidth - 3 * border));
	int32_t ctrlsHeight = winHeight - bttnHeight - 3*border;

	if (mp_cameraCtrl) mp_cameraCtrl->SetSize(border, border, leftPart, ctrlsHeight);
	if (mp_formatCtrl) mp_formatCtrl->SetSize(border + leftPart + border, border, ritePart, ctrlsHeight);

	if (mp_okButton)     mp_okButton->SetSize( border, winHeight - border - bttnHeight, bttnWidth, bttnHeight);
	if (mp_cancelButton) mp_cancelButton->SetSize( border + bttnWidth + border, winHeight - border - bttnHeight, bttnWidth, bttnHeight);
}

///////////////////////////////////////////////////////////////////
void USBCamConfigDlg::OnCameraSelection(wxCommandEvent& event)
{
	if (!mp_cameraCtrl) 
	   return;

	int32_t sel = mp_cameraCtrl->GetSelection();
	if (wxNOT_FOUND != sel)
	{
		m_curr_cam = sel;
		GenCamFormatInfo( m_camNames[m_curr_cam].c_str() );
	}
}

///////////////////////////////////////////////////////////////////
void USBCamConfigDlg::OnFormatSelection(wxCommandEvent& event)
{
	if (!mp_formatCtrl) 
	   return;

	int32_t sel = mp_formatCtrl->GetSelection();
	if (wxNOT_FOUND != sel)
	{
		m_curr_format = sel;
	}
}
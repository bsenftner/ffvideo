///////////////////////////////////////////////////////////////////////////////
// Name:        VideoStreamDlg.cpp
// Author:      Blake Senftner
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
// include main wxWidgets
///////////////////////////////////////////////////////////////////
#include "wx/wx.h"
// #include "wx/cmdline.h"
// #include "wx/bmpcbox.h"
#include "wx/choice.h"
#include "wx/dialog.h"
#include <wx/textdlg.h>
#include "wx/spinctrl.h"

#include <wx/fontpicker.h>
#include <wx/fontdata.h>
#include <wx/fontdlg.h>
///////////////////////////////////////////////////////////////////

#include "ffvideo_player_app.h"


///////////////////////////////////////////////////////////////////
static const long ID_VIDEOSTREAMDLG_NAMEEDIT = wxNewId();
static const long ID_VIDEOSTREAMDLG_LOOPCTRL = wxNewId();
static const long ID_VIDEOSTREAMDLG_TYPECTRL = wxNewId();
static const long ID_VIDEOSTREAMDLG_USBBCTRL = wxNewId();
static const long ID_VIDEOSTREAMDLG_INFOEDIT = wxNewId();

static const long ID_VIDEOSTREAMDLG_FRAMEINTERVAL = wxNewId();
static const long ID_VIDEOSTREAMDLG_AUTOFRAMEINTERVAL = wxNewId();
static const long ID_VIDEOSTREAMDLG_FILTEREDIT = wxNewId();

static const long ID_VIDEOSTREAMDLG_EXPORTINTERVAL = wxNewId();
static const long ID_VIDEOSTREAMDLG_EXPORTDIR = wxNewId();
static const long ID_VIDEOSTREAMDLG_EXPORTBASE = wxNewId();
static const long ID_VIDEOSTREAMDLG_EXPORTSCALE = wxNewId();
static const long ID_VIDEOSTREAMDLG_EXPORTQUALITY = wxNewId();
static const long ID_VIDEOSTREAMDLG_EXPORTFAILLIMIT = wxNewId();

static const long ID_VIDEOSTREAMDLG_ENCODEINTERVAL = wxNewId();
static const long ID_VIDEOSTREAMDLG_ENCODEDIR = wxNewId();
static const long ID_VIDEOSTREAMDLG_ENCODEBASE = wxNewId();
static const long ID_VIDEOSTREAMDLG_FFMPEGPATH = wxNewId();
static const long ID_VIDEOSTREAMDLG_POSTENCODE = wxNewId();
static const long ID_VIDEOSTREAMDLG_POSTMOVEPATH = wxNewId();
static const long ID_VIDEOSTREAMDLG_ENCODEFPS = wxNewId();
static const long ID_VIDEOSTREAMDLG_ENCODETYPE = wxNewId();
static const long ID_VIDEOSTREAMDLG_ENCODEWH = wxNewId();

static const long ID_VIDEOSTREAMDLG_FONTPICK = wxNewId();
static const long ID_VIDEOSTREAMDLG_FONTBTTN = wxNewId();


///////////////////////////////////////////////////////////////////
VideoStreamConfigDlg::VideoStreamConfigDlg(TheApp* app, wxWindow* parent, wxWindowID id, const wxString& title, VideoStreamConfig* params)
	: mp_app(app), mp_parent((VideoWindow*)parent), m_vsc(*params), m_ok(false), mp_loopMediaCtrl(NULL), mp_streamTypeCtrl(NULL), mp_config_button(NULL),
	  mp_infoCtrl(NULL), mp_frameIntervalCtrl(NULL), mp_autoFrameInterval_button(NULL), mp_exportIntervalCtrl(NULL), mp_exportDir_button(NULL),
	  mp_exportBasename_button(NULL), mp_exampleFrameExportPath(NULL), mp_exportScaleCtrl(NULL), mp_exportQualityCtrl(NULL), mp_encodeTypeCtrl(NULL),
	  mp_encodeIntervalCtrl(NULL), mp_ffmpegPath_button(NULL), mp_ffmpegPath(NULL), mp_postEncodeMovePath(NULL), mp_postEncodeCtrl(NULL), 
		mp_movePath_button(NULL), mp_encodeDir_button(NULL),
	  mp_encodeBasename_button(NULL), mp_exampleFrameEncodePath(NULL), mp_encodeFPSCtrl(NULL), mp_encodeWHCtrl(NULL), mp_font_button(NULL),
	  wxDialog(parent, id, title, wxDefaultPosition, wxSize(800, 816) ) 
{
	// first build m_namesMap:

	// placing ourselves first in the map:
	m_namesMap[params->m_name] = params->m_id;

	// now get all other video stream names and add them to the map:
	std::string dataKey = "app/total_vw";
	int32_t max_stream_names = mp_app->mp_config->ReadInt(dataKey, 0 );
	//
	std::string data_key;
	for (int32_t i = 0; i < max_stream_names; i++)
	{
		data_key = app->FormatStr( "win%d/name", i );
		std::string stream_name = mp_app->mp_config->ReadString( data_key, "unnamed" );
		
		// if not ourselves (because we're already in the map)
		if ((stream_name.size() > 0) && (stream_name.compare( params->m_name ) != 0))
			m_namesMap[stream_name] = i;	
	}

	// build the GUI controls:

	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);   // the outter container is a vertical sizer
	wxPanel*    panel = new wxPanel(this, -1);			 // vbox will hold panel and hbox
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

	const int vOff = -6;  // vertical offset used by ctrls but not static text, to better align them visually
	const int ctrlH = 32; // vertical height of our controls
	const int lh = 25;		// text height of one line
	const int ctrlLeft = 150;
	const int textEditWidth = 600;

	const int pheight = 22 * ctrlH;

	wxStaticBox* st = new wxStaticBox(panel, -1, wxT("Video Stream Configuration"), wxPoint(5, 5), wxSize(ctrlLeft + textEditWidth + 15, pheight));

	wxStaticText* nameLabel_txt = new wxStaticText(panel, -1, "Video Stream Name:",      wxPoint(15, 30 + 0 * ctrlH));
	wxStaticText* typeLabel_txt = new wxStaticText(panel, -1, "Video Stream Type:",      wxPoint(15, 30 + 1 * ctrlH));
	wxStaticText* connLabel_txt = new wxStaticText(panel, -1, "Path / USB PIN / URL:",   wxPoint(15, 30 + 2 * ctrlH));
	wxStaticText* fdinLabel_txt = new wxStaticText(panel, -1, "Frame Display Interval:", wxPoint(15, 30 + 3 * ctrlH));
	wxStaticText* opffLabel_txt = new wxStaticText(panel, -1, "Optional Frame Filter:",  wxPoint(15, 30 + 4 * ctrlH));
	//
	wxStaticText* opt1Addtl_txt = new wxStaticText(panel, -1, "--- Optional Frame Exporting Parameters --------------------------------------",
																								 wxPoint(ctrlLeft, 30 + 5 * ctrlH) );
	//
	wxStaticText* feinLabel_txt = new wxStaticText(panel, -1, "Frame Export Interval:",  wxPoint(15, 30 + 6 * ctrlH));
	wxStaticText* exefLabel_txt = new wxStaticText(panel, -1, "Example Export File:",    wxPoint(15, 30 + 7 * ctrlH));
	wxStaticText* exmoLabel_txt = new wxStaticText(panel, -1, "Frame Export Scale:",     wxPoint(15, 30 + 8 * ctrlH));
	wxStaticText* exquLabel_txt = new wxStaticText(panel, -1, "Frame Export Quality:",   wxPoint(15, 30 + 9 * ctrlH));
	//
	wxStaticText* opt2Addtl_txt = new wxStaticText(panel, -1, "--- Optional Exported Frame Encoding Parameters ------------------------------", 
																								wxPoint(ctrlLeft, 30 + 10 * ctrlH) );
	//
	wxStaticText* eninLabel_txt = new wxStaticText(panel, -1, "Encoding Interval:",      wxPoint(15, 30 + 11 * ctrlH));
	wxStaticText* exenLabel_txt = new wxStaticText(panel, -1, "Example Encode File:",    wxPoint(15, 30 + 12 * ctrlH));

	wxStaticText* ffmpLabel_txt = new wxStaticText(panel, -1, "FFmpeg.exe Path:",        wxPoint(15, 30 + 13 * ctrlH));
	wxStaticText* pempLabel_txt = new wxStaticText(panel, -1, "Post Encode Move:",       wxPoint(15, 30 + 15 * ctrlH));
	wxStaticText* enfsLabel_txt = new wxStaticText(panel, -1, "Encoding FPS:",           wxPoint(15, 30 + 16 * ctrlH));
	wxStaticText* entyLabel_txt = new wxStaticText(panel, -1, "Encoding Type:",					 wxPoint(15, 30 + 17 * ctrlH));
	wxStaticText* enwhLabel_txt = new wxStaticText(panel, -1, "Encoding Width, Height:", wxPoint(15, 30 + 18 * ctrlH));
	//
	wxStaticText* fontLabel_txt = new wxStaticText(panel, -1, "Video Overlay Font:",     wxPoint(15, 30 + 20 * ctrlH));

	wxSize textEditSize(textEditWidth, 24);

	int32_t ctrlsLine(0);	// which line of controls a given bit of code is building

	wxComboBox* nameCtrl = new wxComboBox(panel, ID_VIDEOSTREAMDLG_NAMEEDIT, m_vsc.m_name.c_str(), wxPoint(ctrlLeft, 30 + ctrlsLine * ctrlH + vOff), textEditSize);
	//
	std::map<std::string,int32_t>::iterator it;
	for (it = m_namesMap.begin(); it != m_namesMap.end(); ++it)
	{
		nameCtrl->Append(wxString(it->first.c_str()));
	}

	ctrlsLine++;

	// stream type combo box:
	mp_streamTypeCtrl = new wxComboBox(panel, ID_VIDEOSTREAMDLG_TYPECTRL, wxEmptyString, wxPoint(ctrlLeft, 30 + ctrlsLine * ctrlH + vOff), wxSize(120, -1), 0, 0, wxCB_READONLY);
	mp_streamTypeCtrl->Append("Video File");
	mp_streamTypeCtrl->Append("USB Cam");
	mp_streamTypeCtrl->Append("IP Camera/Stream");
	mp_streamTypeCtrl->SetSelection((int32_t)m_vsc.StreamTypeInt());

	bool is_media_file(false);
	wxString labelStr;
	if (m_vsc.m_type == STREAM_TYPE::FILE)
	{
		labelStr = "Browse";
		is_media_file = true;
	}
	else if (m_vsc.m_type == STREAM_TYPE::USB)
	{
		labelStr = "Webcam Config";
	}
	else
	{
		labelStr = "Previous URLs";
	}
	//
	mp_config_button = new wxButton(panel, ID_VIDEOSTREAMDLG_USBBCTRL, labelStr, wxPoint(ctrlLeft + 130, 30 + ctrlsLine * ctrlH + vOff), wxSize(150, lh));


	mp_loopMediaCtrl = new wxCheckBox( panel, ID_VIDEOSTREAMDLG_LOOPCTRL, wxT("Loop Media files (frame exports turn off on loop)"), wxPoint(ctrlLeft + 290, 30 + ctrlsLine * ctrlH) );
	//
	mp_loopMediaCtrl->SetValue(m_vsc.m_loop_media_files);
	mp_loopMediaCtrl->Enable(is_media_file);

	ctrlsLine++;

	mp_infoCtrl = new wxTextCtrl(panel, ID_VIDEOSTREAMDLG_INFOEDIT, m_vsc.m_info.c_str(), wxPoint(ctrlLeft, 30 + ctrlsLine * ctrlH + vOff), textEditSize);


	ctrlsLine++;

	mp_frameIntervalCtrl = new wxSpinCtrl(panel, ID_VIDEOSTREAMDLG_FRAMEINTERVAL,
																				wxString::Format("%d", m_vsc.m_frame_interval),
																				wxPoint(ctrlLeft + 2, 30 + ctrlsLine * ctrlH + vOff), wxSize(60, lh),
																				wxSP_ARROW_KEYS, 0, 10, m_vsc.m_frame_interval);

	wxStaticText* fdinAddtl_txt = new wxStaticText(panel, -1, " 0 = auto, 1 = every frame, 2 = every other frame, ...up to 10", wxPoint(ctrlLeft + 72, 30 + ctrlsLine * ctrlH) );

	mp_autoFrameInterval_button = new wxButton(panel, ID_VIDEOSTREAMDLG_AUTOFRAMEINTERVAL, wxString("Auto Frame Interval Profile"), 
																						 wxPoint(ctrlLeft + 430, 30 + ctrlsLine * ctrlH + vOff), wxSize(170, lh));


	ctrlsLine++;

	wxComboBox* filterCtrl = new wxComboBox(panel, ID_VIDEOSTREAMDLG_FILTEREDIT, m_vsc.m_post_process.c_str(), wxPoint(ctrlLeft, 30 + ctrlsLine * ctrlH + vOff), textEditSize);
	//
	std::vector<std::string> post_process_history;
	mp_app->GetPostProcessFilterHistory( post_process_history );
	//
	for (size_t i = 0; i < post_process_history.size(); i++)
	{
		filterCtrl->Append(wxString(post_process_history[i].c_str()));
	}


	ctrlsLine+=2;

	mp_exportIntervalCtrl = new wxSpinCtrl(panel, ID_VIDEOSTREAMDLG_EXPORTINTERVAL,
																				 wxString::Format("%d", m_vsc.m_export_interval),
																				 wxPoint(ctrlLeft + 2, 30 + ctrlsLine * ctrlH + vOff), wxSize(60, lh),
																				 wxSP_ARROW_KEYS, 0, 10, m_vsc.m_export_interval);

	wxStaticText* exinAddtl_txt = new wxStaticText(panel, -1, " 0 = off, 1 = every frame, 2 = every other, ...", wxPoint(ctrlLeft + 72, 30 + ctrlsLine * ctrlH) );

	mp_exportDir_button = new wxButton(panel, ID_VIDEOSTREAMDLG_EXPORTDIR, wxString("Frame Export Directory"), 
																		 wxPoint(ctrlLeft + 310, 30 + ctrlsLine * ctrlH + vOff), wxSize(160, lh));

	mp_exportBasename_button = new wxButton(panel, ID_VIDEOSTREAMDLG_EXPORTBASE, wxString("Export Basename"), 
																			           wxPoint(ctrlLeft + 480, 30 + ctrlsLine * ctrlH + vOff), wxSize(120, lh));


	ctrlsLine++;

	mp_exampleFrameExportPath = new wxStaticText(panel, -1, "Set with 'Frame Export Directory' and 'Export Basename' buttons", wxPoint(ctrlLeft, 30 + ctrlsLine * ctrlH) );
	if (m_vsc.m_export_dir.compare("unknown")!=0)
	{
		UpdateExampleFrameExportPath();
	}

	ctrlsLine++;

	std::string bsjnk = mp_app->FormatStr("%1.4f", m_vsc.m_export_scale );
	mp_exportScaleCtrl = new wxTextCtrl(panel, ID_VIDEOSTREAMDLG_EXPORTSCALE, bsjnk.c_str(), wxPoint(ctrlLeft, 30 + ctrlsLine * ctrlH + vOff), wxSize(60, lh));

	wxStaticText* exscAddtl_txt = new wxStaticText(panel, -1, "A normalized > 0.0 and <= 1.0 value", wxPoint(ctrlLeft + 72, 30 + ctrlsLine * ctrlH) );


	ctrlsLine++;

	mp_exportQualityCtrl = new wxSpinCtrl(panel, ID_VIDEOSTREAMDLG_EXPORTQUALITY,
																				wxString::Format("%d", m_vsc.m_export_quality),
																				wxPoint(ctrlLeft + 2, 30 + ctrlsLine * ctrlH + vOff), wxSize(60, lh),
																				wxSP_ARROW_KEYS, 0, 100, m_vsc.m_export_quality);

	wxStaticText* exquAddtl_txt = new wxStaticText(panel, -1, "frame exports are JPEG format, quality varies from 0 to 100", wxPoint(ctrlLeft + 72, 30 + ctrlsLine * ctrlH) );



	ctrlsLine+=2;

	int32_t encode_interval = m_vsc.m_encode_interval.load(std::memory_order::memory_order_relaxed);
	//
	mp_encodeIntervalCtrl = new wxSpinCtrl(panel, ID_VIDEOSTREAMDLG_ENCODEINTERVAL,
																				 wxString::Format("%d", encode_interval),
																				 wxPoint(ctrlLeft + 2, 30 + ctrlsLine * ctrlH + vOff), wxSize(70, lh),
																				 wxSP_ARROW_KEYS, 0, 120000, encode_interval);

	wxStaticText* eninAddtl_txt = new wxStaticText(panel, -1, "Encode every N exported frames, 0 = off", wxPoint(ctrlLeft + 82, 30 + ctrlsLine * ctrlH) );

	mp_encodeDir_button = new wxButton(panel, ID_VIDEOSTREAMDLG_ENCODEDIR, wxString("Encoding Directory"), 
																		 wxPoint(ctrlLeft + 320, 30 + ctrlsLine * ctrlH + vOff), wxSize(150, lh));

	mp_exportBasename_button = new wxButton(panel, ID_VIDEOSTREAMDLG_ENCODEBASE, wxString("Encoding Basename"), 
																					 wxPoint(ctrlLeft + 480, 30 + ctrlsLine * ctrlH + vOff), wxSize(120, lh));

	ctrlsLine++;

	mp_exampleFrameEncodePath = new wxStaticText(panel, -1, "Set with 'Encoding Directory' and 'Encoding Basename' buttons", wxPoint(ctrlLeft, 30 + ctrlsLine * ctrlH) );
	if (m_vsc.m_encode_dir.compare("unknown")!=0)
	{
		UpdateExampleFrameEncodePath();
	}


	ctrlsLine++;

	wxString ffmpeg_button_text("Set path to FFmpeg.exe");
	mp_ffmpegPath_button = new wxButton(panel, ID_VIDEOSTREAMDLG_FFMPEGPATH, ffmpeg_button_text, wxPoint(ctrlLeft, 30 + ctrlsLine * ctrlH + vOff), wxSize(170, lh));


	// post encode action combo box:
	mp_postEncodeCtrl = new wxComboBox(panel, ID_VIDEOSTREAMDLG_POSTENCODE, wxEmptyString, wxPoint(ctrlLeft + 180, 30 + ctrlsLine * ctrlH + vOff + 1), wxSize(230, -1), 0, 0, wxCB_READONLY);
	mp_postEncodeCtrl->Append("Delete exported frames after encoding");
	mp_postEncodeCtrl->Append("Move exported frames after encoding");
	mp_postEncodeCtrl->SetSelection(m_vsc.m_post_encode);


	wxString move_button_text("Post encode move path");
	if (m_vsc.m_ffmpeg_path.compare("unknown")==0)
	{
		move_button_text += " is unset";
	}
	mp_movePath_button = new wxButton(panel, ID_VIDEOSTREAMDLG_POSTMOVEPATH, move_button_text, wxPoint(ctrlLeft + 420, 30 + ctrlsLine * ctrlH + vOff), wxSize(190, lh));

	ctrlsLine++;

	mp_ffmpegPath = new wxStaticText(panel, -1, "Set with 'Path to FFmpeg.exe' button", wxPoint(ctrlLeft, 30 + ctrlsLine * ctrlH) );
	if (m_vsc.m_ffmpeg_path.compare("unknown")!=0)
	{
		UpdateFFmpegPath();
	}

	ctrlsLine++;

	mp_postEncodeMovePath = new wxStaticText(panel, -1, "Set with 'Post encode move path' button", wxPoint(ctrlLeft, 30 + ctrlsLine * ctrlH) );
	if (m_vsc.m_post_move_path.compare("unknown")!=0)
	{
		UpdatePostEncodeMovePath();
	}

	ctrlsLine++;

	if (m_vsc.m_encode_fps < 0.0f)
		 m_vsc.m_encode_fps = 0.0f;
  //
	bsjnk = mp_app->FormatStr("%1.2f", m_vsc.m_encode_fps );
	//
	mp_encodeFPSCtrl = new wxTextCtrl(panel, ID_VIDEOSTREAMDLG_ENCODEFPS, bsjnk.c_str(), wxPoint(ctrlLeft, 30 + ctrlsLine * ctrlH + vOff), wxSize(60, lh));

	wxStaticText* enfsAddtl_txt = new wxStaticText(panel, -1, "0 = current playback fps, else is movie encoding fps", wxPoint(ctrlLeft + 72, 30 + ctrlsLine * ctrlH) );


	ctrlsLine++;

	if (m_vsc.m_encode_type < 0)
		m_vsc.m_encode_type = 0;
	else if (m_vsc.m_encode_type > 1)
		m_vsc.m_encode_type = 1;
	//
	// encoding type combo box:
	mp_encodeTypeCtrl = new wxComboBox(panel, ID_VIDEOSTREAMDLG_ENCODETYPE, wxEmptyString, wxPoint(ctrlLeft, 30 + ctrlsLine * ctrlH + vOff), wxSize(300, -1), 0, 0, wxCB_READONLY);
	mp_encodeTypeCtrl->Append("Encode standard media player H.264 MP4 files");
	mp_encodeTypeCtrl->Append("Encode H.264 MP4 and H.264 Elementary Stream files");
	mp_encodeTypeCtrl->SetSelection(m_vsc.m_encode_type);


	ctrlsLine++;

	if (m_vsc.m_encode_width < 0)
		 m_vsc.m_encode_width = -1;	
	if (m_vsc.m_encode_height < 0)
		 m_vsc.m_encode_height = -1;
	//
	bsjnk = mp_app->FormatStr("%d, %d", m_vsc.m_encode_width, m_vsc.m_encode_height );
	//
	mp_encodeWHCtrl = new wxTextCtrl(panel, ID_VIDEOSTREAMDLG_ENCODEWH, bsjnk.c_str(), wxPoint(ctrlLeft, 30 + ctrlsLine * ctrlH + vOff), wxSize(80, lh));

	wxStaticText* enwhAddtl_txt = new wxStaticText(panel, -1, "Encode width, height; set either to -1 for auto-fitting, or both to -1 for original resolution", wxPoint(ctrlLeft + 92, 30 + ctrlsLine * ctrlH) );


	ctrlsLine+=2;

	mp_font_button = new wxButton(panel, ID_VIDEOSTREAMDLG_FONTBTTN, wxString("Change"), wxPoint(ctrlLeft, 30 + ctrlsLine * ctrlH + vOff), wxSize(150, lh));

	/* 
	wxFontStyle style = (m_vsc.m_font_italic) ? wxFONTSTYLE_ITALIC : wxFONTSTYLE_NORMAL;

	mp_overlayFont = new wxFont(m_vsc.m_font_point_size, wxFONTFAMILY_DEFAULT, style, wxFONTWEIGHT_BOLD, 
															m_vsc.m_font_underlined,  wxString( m_vsc.m_font_face_name.c_str() ));

	wxFontPickerCtrl* font_ctrl = new wxFontPickerCtrl( (wxWindow *)this, wxID_ANY, *mp_overlayFont, 
	wxPoint(ctrlLeft + 160, 30 + 16 * ctrlH + vOff), wxDefaultSize, wxFNTP_DEFAULT_STYLE );

	wxColour fontColor;
	fontColor.Set( (uint8_t)(m_vsc.m_font_color.x * 255.0f), (uint8_t)(m_vsc.m_font_color.y * 255.0f), (uint8_t)(m_vsc.m_font_color.z * 255.0f));
	font_ctrl->SetSelectedColour( fontColor );  */


	wxButton* okButton = new wxButton(this, wxID_OK, wxT("Ok"));
	wxButton* cancelButton = new wxButton(this, wxID_CANCEL, wxT("Cancel"));

	hbox->Add(okButton, 1, wxGROW | wxALL, 10);
	hbox->Add(cancelButton, 0, wxGROW | wxALL, 10);

	vbox->Add(panel, 1);
	vbox->Add(hbox, 1, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);



	Connect(ID_VIDEOSTREAMDLG_NAMEEDIT, wxEVT_COMMAND_TEXT_UPDATED, (wxObjectEventFunction)&VideoStreamConfigDlg::OnNameEdit);
	Connect(ID_VIDEOSTREAMDLG_TYPECTRL, wxEVT_COMMAND_COMBOBOX_SELECTED, (wxObjectEventFunction)&VideoStreamConfigDlg::OnVideoType);
	Connect(ID_VIDEOSTREAMDLG_USBBCTRL, wxEVT_BUTTON, (wxObjectEventFunction)&VideoStreamConfigDlg::OnConfigButton);

	Connect(ID_VIDEOSTREAMDLG_LOOPCTRL, wxEVT_CHECKBOX, (wxObjectEventFunction)&VideoStreamConfigDlg::OnLoopMediaFiles);

	Connect(ID_VIDEOSTREAMDLG_INFOEDIT, wxEVT_COMMAND_TEXT_UPDATED, (wxObjectEventFunction)&VideoStreamConfigDlg::OnInfoEdit);

	Connect(ID_VIDEOSTREAMDLG_FRAMEINTERVAL, wxEVT_SPINCTRL,  (wxObjectEventFunction)&VideoStreamConfigDlg::OnFrameInterval);
	Connect(ID_VIDEOSTREAMDLG_FRAMEINTERVAL, wxEVT_TEXT,      (wxObjectEventFunction)&VideoStreamConfigDlg::OnFrameInterval);
	//
	Connect(ID_VIDEOSTREAMDLG_AUTOFRAMEINTERVAL, wxEVT_BUTTON,(wxObjectEventFunction)&VideoStreamConfigDlg::OnAutoFrameIntervalButton);

	Connect(ID_VIDEOSTREAMDLG_FILTEREDIT, wxEVT_COMMAND_TEXT_UPDATED, (wxObjectEventFunction)&VideoStreamConfigDlg::OnPostProcessFilterEdit);

	Connect(ID_VIDEOSTREAMDLG_EXPORTINTERVAL, wxEVT_SPINCTRL,  (wxObjectEventFunction)&VideoStreamConfigDlg::OnExportInterval);
	Connect(ID_VIDEOSTREAMDLG_EXPORTINTERVAL, wxEVT_TEXT,      (wxObjectEventFunction)&VideoStreamConfigDlg::OnExportInterval);

	Connect(ID_VIDEOSTREAMDLG_EXPORTDIR, wxEVT_BUTTON,(wxObjectEventFunction)&VideoStreamConfigDlg::OnExportDirButton);
	Connect(ID_VIDEOSTREAMDLG_EXPORTBASE, wxEVT_BUTTON,(wxObjectEventFunction)&VideoStreamConfigDlg::OnExportBasenameButton);

	Connect(ID_VIDEOSTREAMDLG_EXPORTSCALE,			wxEVT_COMMAND_TEXT_UPDATED, (wxObjectEventFunction)&VideoStreamConfigDlg::OnExportScaleEdit);

	Connect(ID_VIDEOSTREAMDLG_EXPORTQUALITY, wxEVT_SPINCTRL,	(wxObjectEventFunction)&VideoStreamConfigDlg::OnExportQuality);
	Connect(ID_VIDEOSTREAMDLG_EXPORTQUALITY, wxEVT_TEXT,      (wxObjectEventFunction)&VideoStreamConfigDlg::OnExportQuality);
	
	Connect(ID_VIDEOSTREAMDLG_ENCODEINTERVAL, wxEVT_SPINCTRL,  (wxObjectEventFunction)&VideoStreamConfigDlg::OnEncodeInterval);
	Connect(ID_VIDEOSTREAMDLG_ENCODEINTERVAL, wxEVT_TEXT,      (wxObjectEventFunction)&VideoStreamConfigDlg::OnEncodeInterval);

	Connect(ID_VIDEOSTREAMDLG_ENCODEDIR,  wxEVT_BUTTON,							(wxObjectEventFunction)&VideoStreamConfigDlg::OnEncodeDirButton);
	Connect(ID_VIDEOSTREAMDLG_ENCODEBASE, wxEVT_BUTTON,							(wxObjectEventFunction)&VideoStreamConfigDlg::OnEncodeBasenameButton);

	Connect(ID_VIDEOSTREAMDLG_FFMPEGPATH,   wxEVT_BUTTON,										 (wxObjectEventFunction)&VideoStreamConfigDlg::OnFFmpegPathButton);
	Connect(ID_VIDEOSTREAMDLG_POSTENCODE,   wxEVT_COMMAND_COMBOBOX_SELECTED, (wxObjectEventFunction)&VideoStreamConfigDlg::OnPostEncodeAction);
	Connect(ID_VIDEOSTREAMDLG_POSTMOVEPATH, wxEVT_BUTTON,										 (wxObjectEventFunction)&VideoStreamConfigDlg::OnMovePathButton);

	Connect(ID_VIDEOSTREAMDLG_ENCODEFPS, wxEVT_COMMAND_TEXT_UPDATED,(wxObjectEventFunction)&VideoStreamConfigDlg::OnEncodeFPS);

	Connect(ID_VIDEOSTREAMDLG_ENCODETYPE, wxEVT_COMMAND_COMBOBOX_SELECTED, (wxObjectEventFunction)&VideoStreamConfigDlg::OnEncodeType);

	Connect(ID_VIDEOSTREAMDLG_ENCODEWH, wxEVT_COMMAND_TEXT_UPDATED,(wxObjectEventFunction)&VideoStreamConfigDlg::OnEncodeWH);


	// Connect(ID_VIDEOSTREAMDLG_FONTPICK, wxEVT_FONTPICKER_CHANGED, (wxObjectEventFunction)&VideoStreamConfigDlg::OnFontEdit);
	Connect(ID_VIDEOSTREAMDLG_FONTBTTN, wxEVT_BUTTON, (wxObjectEventFunction)&VideoStreamConfigDlg::OnFontButton);


	Connect(wxID_OK, wxEVT_BUTTON, (wxObjectEventFunction)&VideoStreamConfigDlg::OnOkayButton);

	SetSizer(vbox);
}

///////////////////////////////////////////////////////////////////
VideoStreamConfigDlg::~VideoStreamConfigDlg() { }


///////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::OnNameEdit(wxCommandEvent& event)
{
	wxComboBox* c = (wxComboBox*)this->FindWindow(ID_VIDEOSTREAMDLG_NAMEEDIT);
	if (c)
	{
		wxString s = c->GetValue();

		// verify there is a string there, and it's not too big:
		uint32_t l = wxStrlen(s);
		if (l > 0 && l < 1024)
		{
			m_vsc.m_name = s.ToUTF8();

			// check to see if this is one of the available stream names:
			std::map<std::string,int32_t>::iterator it;
			for (it = m_namesMap.begin(); it != m_namesMap.end(); ++it)
			{
				if (it->first.compare( m_vsc.m_name ) == 0)
				{
					// yes, this is one of the available streams. populate our ctrls with it's data:

					// load the selected stream config (ssc)
					VideoStreamConfig ssc( mp_app, mp_app->mp_config, it->second ); 

					mp_loopMediaCtrl->SetValue(ssc.m_loop_media_files);
					bool is_media_file = (ssc.m_type == STREAM_TYPE::FILE);
					mp_loopMediaCtrl->Enable( is_media_file );

					mp_streamTypeCtrl->SetSelection( ssc.StreamTypeInt() );

					mp_infoCtrl->SetValue(ssc.m_info.c_str());
					mp_infoCtrl->SetModified(true);

					mp_frameIntervalCtrl->SetValue( wxString::Format("%d", ssc.m_frame_interval) );

					mp_exportIntervalCtrl->SetValue(  wxString::Format("%d", ssc.m_export_interval) );

					mp_exportScaleCtrl->SetValue( wxString::Format("%1.4f", ssc.m_export_scale) );
					mp_exportScaleCtrl->SetModified(true);

					mp_exportQualityCtrl->SetValue(  wxString::Format("%d", ssc.m_export_quality) );

					mp_encodeIntervalCtrl->SetValue(  wxString::Format("%d", ssc.m_encode_interval.load(std::memory_order::memory_order_relaxed)) );

					mp_encodeFPSCtrl->SetValue( wxString::Format("%1.4f", ssc.m_encode_fps) );
					mp_encodeFPSCtrl->SetModified(true);

					mp_encodeTypeCtrl->SetSelection( ssc.m_encode_type );

					mp_encodeWHCtrl->SetValue( wxString::Format("%d, %d", ssc.m_encode_width, ssc.m_encode_height ) );
					mp_encodeWHCtrl->SetModified(true);

					m_vsc = ssc;	// copy everything else across

					UpdateVideoType();
					UpdateExampleFrameExportPath();
					UpdateExampleFrameEncodePath();
					UpdateFFmpegPath();
					UpdatePostEncodeMovePath();

					wxString bsjnk = wxString::Format( "Win%d hosting: \"%s\", View/Edit Video Stream Configuration Dialog", 
																						 m_vsc.m_id, m_vsc.m_name.c_str() );
					SetTitle( bsjnk );
				}
			}
/* */
		}
	}
}

///////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::UpdateVideoType(void)
{
	bool is_media_file(false);
	std::string bsjnk;
	switch (m_vsc.m_type)
	{
	case STREAM_TYPE::FILE:
		mp_config_button->SetLabel(wxString("Browse"));
		is_media_file = true;
		break;
	case STREAM_TYPE::USB:
		mp_config_button->SetLabel(wxString("Webcam Config"));
		break;
	case STREAM_TYPE::IP:
	default:
		mp_config_button->SetLabel(wxString("Previous URLs"));
		break;
	}
	wxTextCtrl* c = (wxTextCtrl*)this->FindWindow(ID_VIDEOSTREAMDLG_INFOEDIT);
	if (c)
	{
		c->SetValue(m_vsc.m_info.c_str());
		c->SetModified(true);
	}
	mp_loopMediaCtrl->Enable( is_media_file );
}

///////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::OnVideoType(wxCommandEvent& event)
{
	wxComboBox* cbox = (wxComboBox*)this->FindWindow(ID_VIDEOSTREAMDLG_TYPECTRL);
	if (cbox)
	{
		int32_t sel = cbox->GetSelection();
		if (sel != wxNOT_FOUND)
		{
			m_vsc.m_type = (sel == 0) ? STREAM_TYPE::FILE :
				             (sel == 1) ? STREAM_TYPE::USB : STREAM_TYPE::IP;

			UpdateVideoType();
		}
	}
}



///////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::OnLoopMediaFiles(wxCommandEvent& event)
{
	wxCheckBox* c = (wxCheckBox*)this->FindWindow(ID_VIDEOSTREAMDLG_LOOPCTRL);
	if (!c)
		return;
	
	m_vsc.m_loop_media_files = c->GetValue();
}


///////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::OnInfoEdit(wxCommandEvent& event)
{
	wxTextCtrl* c = (wxTextCtrl*)this->FindWindow(ID_VIDEOSTREAMDLG_INFOEDIT);
	if (c->IsModified())
	{
		wxString s = c->GetLineText(0);

		// verify there is a string there, and it's not too big:
		uint32_t l = wxStrlen(s);
		if (l >= 0 && l < 1024)
		{
			m_vsc.m_info = s.c_str();
		}
	}
}


///////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::OnPostProcessFilterEdit(wxCommandEvent& event)
{
	wxComboBox* c = (wxComboBox*)this->FindWindow(ID_VIDEOSTREAMDLG_FILTEREDIT);
	if (c)
	{
		wxString s = c->GetValue();

		// verify there is a string there, and it's not too big:
		uint32_t l = wxStrlen(s);
		if (l > 0 && l < 1024)
		{
			m_vsc.m_post_process = s.ToUTF8();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::OnFontButton(wxCommandEvent& event)
{
	wxFontData fontData;
	// fontData.EnableEffects(false);
	fontData.SetAllowSymbols(false);

	wxColour fontColor;
	fontColor.Set( (uint8_t)(m_vsc.m_font_color.x * 255.0f), (uint8_t)(m_vsc.m_font_color.y * 255.0f), (uint8_t)(m_vsc.m_font_color.z * 255.0f));
	fontData.SetColour( fontColor );

	wxFontStyle style = (m_vsc.m_font_italic) ? wxFONTSTYLE_ITALIC : wxFONTSTYLE_NORMAL;

	wxFont font(m_vsc.m_font_point_size, wxFONTFAMILY_DEFAULT, style, wxFONTWEIGHT_BOLD, m_vsc.m_font_underlined, wxString( m_vsc.m_font_face_name.c_str() ));

	fontData.SetInitialFont( font );

	wxFontDialog* fontDialog = new wxFontDialog(this, fontData);

	if (fontDialog->ShowModal() == wxID_OK) 
	{
		wxFontData& usrFontData = fontDialog->GetFontData();

		wxFont font = usrFontData.GetChosenFont();
		m_vsc.m_font_face_name = font.GetFaceName();
		m_vsc.m_font_point_size = font.GetPointSize();
		m_vsc.m_font_italic = !(font.GetStyle() != wxFONTSTYLE_ITALIC);
		m_vsc.m_font_underlined = font.GetUnderlined();
		m_vsc.m_font_strike_thru = font.GetStrikethrough();

		wxColour usrFontColor = usrFontData.GetColour();
		m_vsc.m_font_color.Set( usrFontColor.Red() / 255.0f, usrFontColor.Green() / 255.0f, usrFontColor.Blue() / 255.0f );
	}

}

////////////////////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::OnConfigButton(wxCommandEvent& event)
{
	switch (m_vsc.m_type)
	{
	case STREAM_TYPE::FILE:
		DoMediaFileBrowse();
		break;
	case STREAM_TYPE::USB:
		DoUSBWebCamConfig();
		break;
	case STREAM_TYPE::IP:
	default:
		DoIPStreamUrlConfig();
		break;
	}
}

////////////////////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::OnMediaFileBrowse(wxCommandEvent& event)
{
	DoMediaFileBrowse();
}

///////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::DoMediaFileBrowse(void)
{
	wxString caption = wxT("Browse to and select a video file:");
	wxString wildcard = wxT("Files (*.wmv;*.avi;*.mpg;*.mp4;*.mpeg;*.mkv;*.mov)|*.wmv;*.avi;*.mpg;*.mp4;*.mkv;*.mov||");

	wxString defaultDir = "";
	wxString defaultFilename = "";

	wxFileDialog dlg(this, caption, defaultDir, defaultFilename, wildcard, wxFD_OPEN);
	if (dlg.ShowModal() == wxID_OK)
	{
		m_vsc.m_info = dlg.GetPath().c_str();
		wxTextCtrl* c = (wxTextCtrl*)this->FindWindow(ID_VIDEOSTREAMDLG_INFOEDIT);
		if (c)
		{
			c->SetValue(dlg.GetPath());
			c->SetModified(true);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::OnUSBWebCamConfig(wxCommandEvent& event)
{
	DoUSBWebCamConfig();
}

////////////////////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::DoUSBWebCamConfig(void)
{
	// wxMessageBox("The USB WebCam Config dialog is not written yet...");

	std::vector<std::string> camNames;
	bool names_are_good = mp_parent->mp_renderCanvas->mp_ffvideo->GetUSB_CameraNames( camNames );
	if (!names_are_good)
	{
		wxMessageBox("The host system is reporting no USB cameras are available.");
		return;
	}

	USBCamConfigDlg* p_dlg = new USBCamConfigDlg( mp_app, this, wxID_ANY, wxString("USB CAM"), 
																								&m_vsc, camNames);
	// everything important happens inside the dialog's event handlers
	if (p_dlg->ShowModal() == wxID_CANCEL) 
	{
		delete p_dlg;
		p_dlg = NULL;
		return;
	}
	else
	{
		m_vsc.m_info = std::to_string( p_dlg->m_curr_cam ) + " : " + std::to_string( p_dlg->m_curr_format );
		if (mp_infoCtrl)
		{
		  mp_infoCtrl->SetValue(m_vsc.m_info.c_str());
	 	  mp_infoCtrl->SetModified(true);
		}
	}

	delete p_dlg;
}

////////////////////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::OnIPStreamUrlConfig(wxCommandEvent& event)
{
	DoIPStreamUrlConfig();
}

////////////////////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::DoIPStreamUrlConfig(void)
{
	wxArrayString wxHistory; // combo text param choices
  std::vector<std::string> history;
	bool success = mp_app->GetURLHistory(history); // from config system

	ComboTextParams ctp;
	ctp.m_prompt = wxString("Please enter an IP Camera playback URL:");
	ctp.m_choices.clear();
  for (size_t i = 0; i < history.size(); i++)
      ctp.m_choices.Add( wxString(history[i].c_str()) );
	ctp.m_usertext = wxEmptyString;

	ComboTextDlg dialog(this,
		wxID_ANY,
		wxString("IP Camera URL"),
		740, ctp);

	if (dialog.ShowModal() == wxID_OK)
	{
		m_vsc.m_info = std::string(dialog.m_params.m_usertext.c_str());

		mp_app->RememberThisURL(m_vsc.m_info);

		wxTextCtrl* c = (wxTextCtrl*)this->FindWindow(ID_VIDEOSTREAMDLG_INFOEDIT);
		if (c)
		{
			c->SetValue(m_vsc.m_info.c_str());
			c->SetModified(true);
		}
	}

}

///////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::OnFrameInterval(wxCommandEvent& event)
{
	wxSpinCtrl* c = (wxSpinCtrl*)this->FindWindow(ID_VIDEOSTREAMDLG_FRAMEINTERVAL);
	if (!c)
		return;
	m_vsc.m_frame_interval = c->GetValue();
}

////////////////////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::OnAutoFrameIntervalButton(wxCommandEvent& event)
{
	wxString more;

	for (;;) // forever
	{
		wxString msg("Sometimes, the requested resolution and received frame resolution of a video stream differ.\n");
		      msg += "(Both USB and IP cameras can deliver an unexpected frame resolution.) To handle this one can\n";
					msg += "provide a series of comma separated values, each pair respresenting one logial handler for all\n";
					msg += "horizontal frame resolutions equal and greater than the first value in the pair. For example:\n\n";
					msg += "    7680, 3, 1920, 2, 320, 1\n\n";
					msg += "The above says to play using a frame interval of 3 for frames >= 7680 wide, play with an\n";
					msg += "interval of 2 for frames >= 1920 and < 7680 wide, and a frame interval of 1 for small frames.\n";
					msg += "Start with the largest frame width and work down, specifying the frame intervals to use.\n";
					msg += "Note: the default Auto Frame Interval Profile is:\n\n";
					msg += "    7680, 3, 1920, 2, 320, 1\n\n";
					msg += "By providing a Auto Frame Interval Profile below you will replace the default profile:\n";
					if (more.size() > 0)
					   msg += more;

		wxString res = wxGetTextFromUser( msg,
																			wxString::Format("Stream '%s' Auto Frame Interval Profile", m_vsc.m_name.c_str()), 
																			m_vsc.m_frame_interval_profile.c_str(), this);
		if (res == wxEmptyString)
			return;

		// make copy because original gets wreaked by parsing: 
		std::string res_copy( res.c_str() );

		// this gets destroyed by the parsing inside ParseAutoFrameInterval():
		char* buff = (char*)res.c_str().AsChar();

		// parse result goes here: 
		std::vector<uint32_t> widths, intervals;

		std::string err;

		if (mp_parent->mp_streamConfig->ParseAutoFrameInterval( mp_app, buff, widths, intervals, err ))
		{
			m_vsc.m_frame_interval_profile = res_copy;
			return; // success!
		}
		else
		{
			more = wxString("\n") + err.c_str();
		}
	}
}


///////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::OnExportInterval(wxCommandEvent& event)
{
	wxSpinCtrl* c = (wxSpinCtrl*)this->FindWindow(ID_VIDEOSTREAMDLG_EXPORTINTERVAL);
	if (!c)
		return;
	m_vsc.m_export_interval = c->GetValue();
}

////////////////////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::UpdateExampleFrameExportPath( void )
{
	if (mp_exampleFrameExportPath)
	{
		mp_exampleFrameExportPath->SetLabelText(	wxString::Format( "%s\\%s_[timecode].jpg", m_vsc.m_export_dir.c_str(), m_vsc.m_export_base.c_str() ) );
	}
}

////////////////////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::OnExportDirButton(wxCommandEvent& event)
{
	wxDirDialog dlg( this, "Select the directory frame exports will be written", wxString(m_vsc.m_export_dir.c_str()) ); 
	
	if (dlg.ShowModal() == wxID_OK)
	{
		std::string new_export_dir = dlg.GetPath().c_str();
		
		m_vsc.m_export_dir = new_export_dir;
	  UpdateExampleFrameExportPath();
	}
}

////////////////////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::OnExportBasenameButton(wxCommandEvent& event)
{
	for (;;) // forever
	{
		wxString res = wxGetTextFromUser( wxT("Enter a basename for this stream's JPEG frame exports:\n"),
			                                wxString::Format("Stream '%s' Frame Export Basename", m_vsc.m_name.c_str()), 
																			m_vsc.m_export_base.c_str(), this);
		if (res == wxEmptyString)
			return;

		std::string new_export_base = res.ToUTF8();

		if (HasInvalidFilenameChars( new_export_base ))
		{
			wxMessageBox( "Please only use alphanumeric characters for the basename", "Frame export basename has illegal characters", wxOK | wxICON_ERROR );
		}
		else
		{
			m_vsc.m_export_base = new_export_base;
			UpdateExampleFrameExportPath();
			return;
		}
	}
}


///////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::OnExportScaleEdit(wxCommandEvent& event)
{
	wxTextCtrl* c = (wxTextCtrl*)this->FindWindow(ID_VIDEOSTREAMDLG_EXPORTSCALE);
	if (c->IsModified())
	{
		wxString s = c->GetLineText(0);

		// verify there is a string there, and it's not too big:
		uint32_t l = wxStrlen(s);
		if (l >= 0 && l < 1024)
		{
		  float   parsed_value(0.0f);

			// reversed min/max means don't enforce min/max limits:
			char* float_buff = (char*)s.c_str().AsChar();
			//
			int32_t ret = mp_app->ParseFloat( float_buff, &parsed_value, 0.0f, -1.0f );	
			if (ret >= 0)
			{
				if (parsed_value > 0.0f && parsed_value <= 1.0)
				{	
					// a normalized value gets just copied over:
					m_vsc.m_export_scale = parsed_value;
				}
			}
		}
	}
}

///////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::OnExportQuality(wxCommandEvent& event)
{
	wxSpinCtrl* c = (wxSpinCtrl*)this->FindWindow(ID_VIDEOSTREAMDLG_EXPORTQUALITY);
	if (!c)
		return;
	m_vsc.m_export_quality = c->GetValue();
}

///////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::OnEncodeInterval(wxCommandEvent& event)
{
	wxSpinCtrl* c = (wxSpinCtrl*)this->FindWindow(ID_VIDEOSTREAMDLG_ENCODEINTERVAL);
	if (!c)
		return;
	m_vsc.m_encode_interval = c->GetValue();
}

////////////////////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::UpdateExampleFrameEncodePath( void )
{
	if (mp_exampleFrameEncodePath)
	{
		mp_exampleFrameEncodePath->SetLabelText(	wxString::Format( "%s\\%s_N_[timecode].mp4", m_vsc.m_encode_dir.c_str(), m_vsc.m_encode_base.c_str() ) );
	}
}

////////////////////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::UpdateFFmpegPath( void )
{
	if (mp_ffmpegPath)
	{
		mp_ffmpegPath->SetLabelText( wxString::Format( "%s", m_vsc.m_ffmpeg_path.c_str() ) );
	}
}

////////////////////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::UpdatePostEncodeMovePath( void )
{
	if (mp_postEncodeMovePath)
	{
		mp_postEncodeMovePath->SetLabelText( wxString::Format( "%s", m_vsc.m_post_move_path.c_str() ) );
	}
}

////////////////////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::OnEncodeDirButton(wxCommandEvent& event)
{
	wxDirDialog dlg( this, "Select the directory encoded streams of exported frames will be written", wxString(m_vsc.m_encode_dir.c_str()) ); 

	if (dlg.ShowModal() == wxID_OK)
	{
		std::string new_encode_dir = dlg.GetPath().c_str();

		m_vsc.m_encode_dir = new_encode_dir;
	  UpdateExampleFrameEncodePath();
	}
}

////////////////////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::OnEncodeBasenameButton(wxCommandEvent& event)
{
	for(;;) // forever
	{
		wxString res = wxGetTextFromUser( wxT("Enter a basename for this stream's encoding of frame exports:"),
																			wxString::Format("Stream '%s' Encoding Basename", m_vsc.m_name.c_str()), 
																			m_vsc.m_encode_base.c_str(), this);
		if (res == wxEmptyString)
			return;

		std::string new_encode_base = res.ToUTF8();

		if (HasInvalidFilenameChars( new_encode_base ))
		{
			wxMessageBox( "Please only use alphanumeric characters for the basename", "Encoding basename has illegal characters", wxOK | wxICON_ERROR );
		}
		else
		{
			m_vsc.m_encode_base = new_encode_base;
			UpdateExampleFrameEncodePath();
			return;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::OnFFmpegPathButton(wxCommandEvent& event)
{
	wxString defaultDir  = wxEmptyString;
	wxString defaultFile = wxEmptyString;
	if (m_vsc.m_ffmpeg_path.compare("unknown")!=0)
	{
		std::string pathOnly = mp_app->GetPath( m_vsc.m_ffmpeg_path );
		std::string filename = mp_app->GetFilename( m_vsc.m_ffmpeg_path );
		std::string ext      = mp_app->GetExtension( m_vsc.m_ffmpeg_path );

		if (pathOnly.size() > 0)
			 defaultDir = wxString( pathOnly.c_str() );

		if (filename.size() > 0 && ext.compare("exe")==0)
		   defaultFile = wxString( filename.c_str() );
	}
	wxFileDialog openFileDialog(this,
															 _("Select the FFmpeg.exe for encoding"),
															 defaultDir,
															 defaultFile,
															 "EXE files (*.exe)|*.exe",
															 wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if (openFileDialog.ShowModal() == wxID_CANCEL)
		return;


	m_vsc.m_ffmpeg_path = openFileDialog.GetPath().c_str();
	UpdateFFmpegPath();
}

///////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::OnPostEncodeAction(wxCommandEvent& event)
{
	wxComboBox* cbox = (wxComboBox*)this->FindWindow(ID_VIDEOSTREAMDLG_POSTENCODE);
	if (cbox)
	{
		int32_t sel = cbox->GetSelection();
		if (sel != wxNOT_FOUND)
		{
			m_vsc.m_post_encode = sel;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::OnMovePathButton(wxCommandEvent& event)
{
	wxDirDialog dlg( this, "Select the directory encoded frames are moved after movie encoding", wxString(m_vsc.m_post_move_path.c_str()) ); 

	if (dlg.ShowModal() == wxID_OK)
	{
		std::string new_move_dir = dlg.GetPath().c_str();

		m_vsc.m_post_move_path = new_move_dir;
	  mp_movePath_button->SetLabelText("Post encode move path");
		UpdatePostEncodeMovePath();
	}
}

///////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::OnEncodeFPS(wxCommandEvent& event)
{
	wxTextCtrl* c = (wxTextCtrl*)this->FindWindow(ID_VIDEOSTREAMDLG_ENCODEFPS);
	if (c->IsModified())
	{
		wxString s = c->GetLineText(0);

		// verify there is a string there, and it's not too big:
		uint32_t l = wxStrlen(s);
		if (l >= 0 && l < 1024)
		{
			float   parsed_value(0.0f);

			// reversed min/max means don't enforce min/max limits:
			char* float_buff = (char*)s.c_str().AsChar();
			//
			int32_t ret = mp_app->ParseFloat( float_buff, &parsed_value, 0.0f, -1.0f );	
			if (ret >= 0 && parsed_value >= 0.0f)
			{
				if (parsed_value > 60.0f)
				{
					int32_t answer = wxMessageBox(wxString::Format("Are you sure you want to encode to %1.2f frames per second?", parsed_value ), 
																				"Confirm high encoding FPS", wxYES_NO | wxCANCEL, this);
					if (answer != wxYES)
						return;
				}
				m_vsc.m_encode_fps = parsed_value;
			}
		}
	}
}

///////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::OnEncodeType(wxCommandEvent& event)
{
	wxComboBox* cbox = (wxComboBox*)this->FindWindow(ID_VIDEOSTREAMDLG_ENCODETYPE);
	if (cbox)
	{
		int32_t sel = cbox->GetSelection();
		if (sel != wxNOT_FOUND)
		{
			m_vsc.m_encode_type = sel;
		}
	}
}

///////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::OnEncodeWH(wxCommandEvent& event)
{
	wxTextCtrl* c = (wxTextCtrl*)this->FindWindow(ID_VIDEOSTREAMDLG_ENCODEWH);
	if (c->IsModified())
	{
		wxString s = c->GetLineText(0);

		// verify there is a string there, and it's not too big:
		uint32_t l = wxStrlen(s);
		if (l >= 0 && l < 1024)
		{
			char* buff = (char*)s.c_str().AsChar();

			int   nTokens(0);
			CHAR  *token, *parseTokens[4], *seps = ", ", *nextToken = NULL;

			nTokens = 0;
			token = strtok_s( buff, seps, &nextToken );
			while (token != NULL) 
			{
				parseTokens[nTokens++] = token;
				token = strtok_s( NULL, seps, &nextToken );
				if (nTokens > 2)
				{
					// assume the user is just editing and let this pass
					return;
				}
			}
			if (nTokens != 2)
			{
				// assume the user is just editing and let this pass
				return;
			}

			int32_t parsed_width(0);
			//
			int32_t ret = mp_app->ParseInteger( parseTokens[0], &parsed_width, 0.0f, -1.0f );	
			if (ret == 0)
			{
				int32_t parsed_height(0);

				ret = mp_app->ParseInteger( parseTokens[1], &parsed_height, 0.0f, -1.0f );
				if (ret == 0)
				{
					m_vsc.m_encode_width  = parsed_width;
					m_vsc.m_encode_height = parsed_height;
				}
			}
		}
	}
}



////////////////////////////////////////////////////////////////////////////////
void VideoStreamConfigDlg::OnOkayButton(wxCommandEvent& event)
{
	m_ok = false;

	if (m_vsc.m_export_interval > 0)
	{
		if (m_vsc.m_encode_interval > 0)
		{
			if (m_vsc.m_encode_width < 0)
				m_vsc.m_encode_width = -1;	// just in case
			if (m_vsc.m_encode_height < 0)
				m_vsc.m_encode_height = -1;	// just in case

			wxString msg = wxString::Format("Video Stream '%s' is configured for frame exporting and encoding:\n", m_vsc.m_name.c_str() );

			std::string missing;
			bool any_missing(false);
			if (!mp_app->IsDirectory(m_vsc.m_export_dir.c_str()))
			{
				if (any_missing)
					missing += std::string(", ");
				missing += "image export directory";
				any_missing = true;
			}
			if (m_vsc.m_export_base.size() < 1)
			{
				if (any_missing)
					missing += std::string(", ");
				missing += "export basename";
				any_missing = true;
			}
			if (!mp_app->IsFile( m_vsc.m_ffmpeg_path.c_str() ))
			{
				if (any_missing)
					missing += std::string(", ");
				missing += std::string("ffmpeg path");
				any_missing = true;
			}
			if (!mp_app->IsDirectory(m_vsc.m_encode_dir.c_str()))
			{
				if (any_missing)
					 missing += std::string(", ");
				missing += "encode directory";
				any_missing = true;
			}
			if (m_vsc.m_encode_base.size() < 1)
			{
				if (any_missing)
					missing += std::string(", ");
				missing += "encode basename";
				any_missing = true;
			}
			if (m_vsc.m_encode_width > 8192)
			{
				if (any_missing)
					missing += std::string(", ");
				missing += "encode width too large";
				any_missing = true;
			}
			if (m_vsc.m_encode_width != -1 && m_vsc.m_encode_width & 1)
			{
				if (any_missing)
					missing += std::string(", ");
				missing += "encode width cannot be an odd number";
				any_missing = true;
			}
			if (m_vsc.m_encode_height > 4320)
			{
				if (any_missing)
					missing += std::string(", ");
				missing += "encode height too large";
				any_missing = true;
			}
			if (m_vsc.m_encode_height != -1 && m_vsc.m_encode_height & 1)
			{
				if (any_missing)
					missing += std::string(", ");
				missing += "encode height cannot be an odd number";
				any_missing = true;
			}
			if (m_vsc.m_post_encode == 1 && !mp_app->IsDirectory( m_vsc.m_post_move_path.c_str() ))
			{
				if (any_missing)
					missing += std::string(", ");
				missing += "post encode directory";
				any_missing = true;
			}
			if (any_missing)
			{
				msg += "however, these fields are missing data:\n" + missing +
							 "\nPlease fix these before continuing.";
				wxMessageBox(msg, "Video Frame Exporting and Encoding Parameter Error:", wxICON_ERROR, this);
				return;
			}

			// temp:
			if (m_vsc.m_encode_fps < 0.01f)
				 m_vsc.m_encode_fps = 25.0f;

			std::string bad_data;
			bool any_bad_data(false);
			if (m_vsc.m_export_scale < 0.01f)
			{
				if (any_bad_data)
					bad_data += std::string(", ");
				bad_data += "image scale questionably small";
				any_bad_data = true;
			}
			if (m_vsc.m_export_quality < 30)
			{
				if (any_bad_data)
					bad_data += std::string(", ");
				bad_data += "image quality very low";
				any_bad_data = true;
			}
			if (any_bad_data)
			{
				msg += "but, these fields might have incorrect values:\n" + bad_data +
					"\nAre you sure you want these settings?";

				int32_t answer = wxMessageBox(msg, // "Are you sure you want to export video frames?", 
																			 "Confirm Unusual Parameters for Video Frame Exporting :", wxYES_NO | wxCANCEL, this);
				if (answer != wxYES)
					return;

				msg = wxString::Format("Video Stream '%s' is configured for frame exporting:\n", m_vsc.m_name.c_str() );
			}



			msg += wxString::Format("Exporting every %d frames, each frame scaled by %1.3f, at a quality of %d,\n", 
															 m_vsc.m_export_interval, m_vsc.m_export_scale, m_vsc.m_export_quality );
			msg +=  wxString::Format("writing frames to directory:\n\n    %s\n\n", m_vsc.m_export_dir.c_str() );
			msg +=  wxString::Format("as JPEGs with basename:\n\n    %s\n\n", m_vsc.m_export_base.c_str() );
			msg +=  wxString::Format("...and when %d frames have exported they are encoded, writing\n", 
															 m_vsc.m_encode_interval.load(std::memory_order::memory_order_relaxed) );
			if (m_vsc.m_encode_type == 0)
			{
				msg +=  wxString::Format(".MP4s files to directory:\n\n    %s\n\n", m_vsc.m_encode_dir.c_str() );
			}
			else // only 2 encode types for now...
			{
				msg +=  wxString::Format(".MP4 and .264 files to directory:\n\n    %s\n\n", m_vsc.m_encode_dir.c_str() );
			}
			msg +=  wxString::Format("writing these with basename:\n\n    %s\n\n", m_vsc.m_encode_base.c_str() );

			msg += wxString::Format("with an encoding fps of %1.2f, and encoding resolution of %d, %d\n",
															 m_vsc.m_encode_fps, m_vsc.m_encode_width, m_vsc.m_encode_height );

			if (m_vsc.m_post_encode == 0)
			{
				msg += wxString("and the original exported frames deleted after encoding.\n");
			}
			else
			{
				msg += wxString::Format("and the exported frames moved after encoding to directory:\n\n    %s\n\n",
																 m_vsc.m_post_move_path.c_str() );
			}

			msg += "\nAre you sure you want all these settings?";

			int32_t answer = wxMessageBox(msg, "Confirm Video Frame Exporting and Encoding:", wxYES_NO | wxCANCEL, this);
			if (answer != wxYES)
				return;
		}
		else // only exporting frames
		{
			wxString msg = wxString::Format("Video Stream '%s' is configured for frame exporting:\n", m_vsc.m_name.c_str() );

			std::string missing;
			bool any_missing(false);
			if (!mp_app->IsDirectory(m_vsc.m_export_dir.c_str()))
			{
				if (any_missing)
					missing += std::string(", ");
				missing += "image export directory";
				any_missing = true;
			}
			if (m_vsc.m_export_base.size() < 1)
			{
				if (any_missing)
					missing += std::string(", ");
				missing += "export basename";
				any_missing = true;
			}
			if (any_missing)
			{
				msg += "however, these fields are missing data:\n" + missing +
					"\nPlease fix these before continuing.";
				wxMessageBox(msg, "Video Frame Exporting Parameter Error:", wxICON_ERROR, this);
				return;
			}

			std::string bad_data;
			bool any_bad_data(false);
			if (m_vsc.m_export_scale < 0.01f)
			{
				if (any_bad_data)
					bad_data += std::string(", ");
				bad_data += "image scale questionably small";
				any_bad_data = true;
			}
			if (m_vsc.m_export_quality < 30)
			{
				if (any_bad_data)
					bad_data += std::string(", ");
				bad_data += "image quality very low";
				any_bad_data = true;
			}
			if (any_bad_data)
			{
				msg += "but, these fields might have incorrect values:\n" + bad_data +
					"\nAre you sure you want these settings?";

				int32_t answer = wxMessageBox(msg, // "Are you sure you want to export video frames?", 
																			 "Confirm Unusual Parameters for Video Frame Exporting :", wxYES_NO | wxCANCEL, this);
				if (answer != wxYES)
					return;

				msg = wxString::Format("Video Stream '%s' is configured for frame exporting:\n", m_vsc.m_name.c_str() );
			}
			
			msg += wxString::Format("Exporting every %d frames, each frame scaled by %1.3f, at a quality of %d,\n", 
															m_vsc.m_export_interval, m_vsc.m_export_scale, m_vsc.m_export_quality );
			msg +=  wxString::Format("writing frames to directory:\n\n    %s\n\n", m_vsc.m_export_dir.c_str() );
			msg +=  wxString::Format("as JPEGs with basename:\n\n    %s\n\n", m_vsc.m_export_base.c_str() );
			msg += "Exported frames will NOT be encoded.\n";
			msg += "\nAre you sure you want all these settings?";

			int32_t answer = wxMessageBox(msg, // "Are you sure you want to export video frames?", 
																	  "Confirm Video Frame Exporting:", wxYES_NO | wxCANCEL, this);
			if (answer != wxYES)
				return;
		}
	}
	else if (m_vsc.m_encode_interval > 0)
	{
		wxMessageBox("Sorry, but one cannot encode without also exporting.", "Video Frame Exporting and Encoding Conflict" );
		return;
	}

	if (m_vsc.m_post_process.size() > 0)
	{
		mp_app->RememberThisPostProcessFilter( m_vsc.m_post_process );
	}

	m_ok = true;
	EndModal(wxID_OK);
}

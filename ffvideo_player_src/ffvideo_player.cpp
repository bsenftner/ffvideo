///////////////////////////////////////////////////////////////////////////////
// Name:        ffvideo_player.cpp
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


#include <wx/stdpaths.h>
#include <wx/app.h>

#include "ffvideo_player_app.h"



// ----------------------------------------------------------------------------
// TheApp: the application object
// ----------------------------------------------------------------------------

wxIMPLEMENT_APP(TheApp);

////////////////////////////////////////////////////////////////////////
bool TheApp::OnInit()
{
    if ( !wxApp::OnInit() )
        return false;

		// initialize the data directory to our execution directory:
		// use wxWidget's StandardPaths facility to get the executable directory:
		m_data_dir = wxPathOnly(::wxStandardPaths::Get().GetExecutablePath()).c_str();
		//
		RecoverAppConfig();

		m_logPath = m_data_dir;
		m_logPath += std::string("\\execute_log.txt");

		// this application uses the wxWidgets virtual file system for a WebView based help.
		// this provides a virtual file system for application wide use:
		wxFileSystem::AddHandler(new wxMemoryFSHandler);

		m_clock.Start();

		// recreate the first video window:
		VideoWindow* p_vw = new VideoWindow(this, 0);
		if (p_vw)
		{
			m_videoWindowPtrs.push_back(p_vw);
			InitVideoWindow( p_vw );
		}
		else
		{
			NewVideoWindow();
		}

		mp_helpWindow = NULL;

		m_we_are_launched = true;

    return true;
}

/////////////////////////////////////////////////////////////////// 
void TheApp::RecoverAppConfig(void)
{
	std::string configPath(m_data_dir);
	configPath += "\\ffvideo_config.data";
	mp_config = new CKeyValueStore(configPath.c_str(), NULL, NULL); 
	mp_config->Init();

	// std::string dataKey( "app/last_video_id");
	// m_last_video_window_id = mp_config->ReadInt( dataKey, -1 );
}

/////////////////////////////////////////////////////////////////// 
void TheApp::InitVideoWindow( VideoWindow* p_vw )
{
	p_vw->mp_renderCanvas->mp_noticeMgr->AddNotice(std::string("Initialized."), 3.0f);
	VideoStreamConfig* vsc = p_vw->mp_streamConfig;
	p_vw->mp_renderCanvas->WindowStatus(wxString::Format("%s : %s", vsc->StreamTypeString().c_str(), vsc->m_info.c_str()));
}

/////////////////////////////////////////////////////////////////// 
int32_t TheApp::GenNextVideoWindowId(void)
{
	// because we allow video window restoration, 
	// which may occur when other windows are already open, 
	// this is used to locate the next available unused window id

	int32_t next_available_id = 0;
	for (int32_t i = 0; i < m_videoWindowPtrs.size(); i++)
	{
		if (next_available_id == m_videoWindowPtrs[i]->m_id)
		{
		  next_available_id++;
			i = -1; // -1 so it reloops to 0 and rechecks everyone with this new value
		}
	}
	m_last_video_window_id = next_available_id;
	//
	// std::string dataKey("app/last_video_id");
	// mp_config->WriteInt(dataKey, m_last_video_window_id);

	return m_last_video_window_id;
}

/////////////////////////////////////////////////////////////////// 
bool TheApp::NewVideoWindow(void)
{
	GenNextVideoWindowId();
	VideoWindow* p_vw = new VideoWindow(this, m_last_video_window_id);
	if (p_vw)
	{
		m_videoWindowPtrs.push_back(p_vw);
		InitVideoWindow( p_vw );
	}

	return true;
}

/////////////////////////////////////////////////////////////////// 
bool TheApp::NewVideoWindow(int32_t id)
{
	// check if the requested id already exists:
	for (int32_t i = 0; i < m_videoWindowPtrs.size(); i++)
	{
		if (id == m_videoWindowPtrs[i]->m_id)
			return false;
	}

	VideoWindow* p_vw = new VideoWindow(this, id);
	if (p_vw)
	{
		m_videoWindowPtrs.push_back(p_vw);
		InitVideoWindow( p_vw );
	}

	return p_vw != NULL;
}

/////////////////////////////////////////////////////////////////// 
bool TheApp::RemoveVideoWindow(VideoWindow* p_vw)
{
	int32_t limit = m_videoWindowPtrs.size();
	for (int32_t i = 0; i < limit; i++)
	{
		if (m_videoWindowPtrs[i] == p_vw)
		{
			if (i < limit - 1)
			{
				using std::swap;
				swap(m_videoWindowPtrs[i], m_videoWindowPtrs.back());
			}
			// p_vw->Close(true);
			m_videoWindowPtrs.pop_back();

			if (m_videoWindowPtrs.size() == 0)
			{
				// no more video windows, user must be quitting, 
				// so delete the help window
				if (mp_helpWindow)
				{
					wxCloseEvent e;
					mp_helpWindow->OnCloseBox(e);
				}
			}

			return true;
		}
	}

	return false;
}

/////////////////////////////////////////////////////////////////// 
VideoWindow* TheApp::GetVideoWindow( int32_t id )
{
	for (int32_t i = 0; i < m_videoWindowPtrs.size(); i++)
		  if (m_videoWindowPtrs[i]->m_id == id)
			   return m_videoWindowPtrs[i];

	return NULL;
}

/////////////////////////////////////////////////////////////////// 
void TheApp::RestoreVideoWindows( void )
{
	std::string dataKey = "app/total_vw";
	int32_t total_vw = mp_config->ReadInt(dataKey, 0 );

	for (int32_t i = 0; i < total_vw; i++)
	{
		bool active(false);
		for (int32_t j = 0; j < m_videoWindowPtrs.size(); j++)
		{
			if (m_videoWindowPtrs[j]->m_id == i)
				active = true;
		}
		if (!active)
		{
			NewVideoWindow(i);
		}
	}
}

/////////////////////////////////////////////////////////////////// 
void TheApp::ShowHelp( wxWindow* parent )
{
	if (!mp_helpWindow)
	{
		mp_helpWindow = new HelpWindow( this, parent );
	}

	if (mp_helpWindow)
	{
		mp_helpWindow->Show();
	}
}


////////////////////////////////////////////////////////////////////////
int TheApp::OnExit()
{
	ReportLog( ReportLogOp::flush, "bye world.\n" );

	if (mp_config)
	{
		mp_config->SyncToDiskStorage(true);

		delete mp_config;
		mp_config = NULL;
	}

  return wxApp::OnExit();
}

////////////////////////////////////////////////////////////////////////////////
void TheApp::_ReportLog(std::string& log_path, std::string msg)
{
	std::ofstream logfile(log_path.c_str(), std::ios_base::app);
	logfile << msg;
	logfile.close();
}

////////////////////////////////////////////////////////////////////////////////
std::string TheApp::FormatStr(const char* formatStr, ...)
{
	size_t bufsize = 1024 * 16;

	char* buffer = (char*)calloc(bufsize, 1); // 16K big enough for anything? in this app, I hope. 
	if (buffer != NULL)
	{
		va_list valist;
		va_start(valist, formatStr);
		vsprintf_s(buffer, bufsize, formatStr, valist);
		va_end(valist);

		std::string timestamp = "";// std::to_string(double(m_timer.s())) + std::string(",");
		std::string finalStr = timestamp + std::string(buffer);

		free(buffer);

		return finalStr;
	}
	std::string nada;
	return nada;
}

////////////////////////////////////////////////////////////////////////////////
void TheApp::ReportLog(ReportLogOp op, std::string msg)
{
	switch (op)
	{
	case ReportLogOp::init:
		m_retainedLog = msg;
		break;
	case ReportLogOp::log:
		m_retainedLog += msg;
		break;
	case ReportLogOp::flush:
		m_retainedLog += msg;
		_ReportLog(m_logPath, m_retainedLog);
		m_retainedLog.clear();
		break;
	}
	
}

////////////////////////////////////////////////////////////////////////////////
// parse the passed string as an integer with possible range checking
// returns 0 on success or -1 when string is not a number
////////////////////////////////////////////////////////////////////////////////
int32_t TheApp::ParseInteger( char *param, int32_t *ret, int32_t min, int32_t max )
{
	int32_t i, len, decimalFlag = false;

	len = strlen( param );
	for (i = 0; i < len; i++)
	{
		// only allow a minus sign on the first character:
		if ((param[i] == '-') && (i == 0))
			continue;

		if (!isdigit( param[i] ))
		{
			// only allow one decimal:
			if (param[i] == '.')
			{
				if (decimalFlag)
					return -1;

				decimalFlag = true;
			}
			else return -1;
		}
	}

	// if a decimal was present, parse as a float and truncate:
	if (decimalFlag)
		*ret = (INT32)atof( param );

	// the expected path for an integer:
	else *ret = atoi( param );

	// if max < min we don't range test
	if (max >= min)
	{ 
		if (*ret < min)
			return -1;

		if (*ret > max)
			return -1;
	}

	return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Return true if the passed string is an integer, a float
// or scientific notation of a float, else return FALSE.
////////////////////////////////////////////////////////////////////////////////
int32_t TheApp::VerifySomeNumber( char *param )
{
	int32_t i, len, parseCode;

	bool	 any_digits = false;

	/*
	The string argument to atof has the following form:

	[whitespace] [sign] [digits] [.digits] [ {d | D | e | E }[sign]digits]

	A whitespace consists of space and/or tab characters, which are ignored; 
	sign is either plus (+) or minus ( – ); and digits are one or more decimal digits. 
	If no digits appear before the decimal point, at least one must appear after the 
	decimal point. The decimal digits may be followed by an exponent, which consists 
	of an introductory letter ( d, D, e, or E) and an optionally signed decimal integer.
	*/

#define PARSECODE_FIRST_SIGN    0
#define PARSECODE_FIRST_DIGITS  1
#define PARSECODE_SECOND_DIGITS 2
#define PARSECODE_EXPONENT0     3
#define PARSECODE_EXPONENT1     4

	i         = 0;
	len       = strlen( param );
	parseCode = PARSECODE_FIRST_SIGN;
	while (i < len)
	{
		switch (parseCode)
		{
		case PARSECODE_FIRST_SIGN:
			if ((param[i] == '+') || (param[i] == '-'))
			{
				parseCode = PARSECODE_FIRST_DIGITS;
				i++;
			}
			else if (isdigit(param[i]))
			{
				parseCode = PARSECODE_FIRST_DIGITS;
				i++;
				any_digits = true;
			}
			else if (param[i] == '.')
			{
				parseCode = PARSECODE_SECOND_DIGITS;
				i++;
			}
			else return false;
			break;

		case PARSECODE_FIRST_DIGITS:
			if (isdigit(param[i]))
			{
				i++;
				any_digits = true;
			}
			else if (param[i] == '.')
			{
				parseCode = PARSECODE_SECOND_DIGITS;
				i++;
			}
			else return false;
			break;

		case PARSECODE_SECOND_DIGITS:
			if (isdigit(param[i]))
			{
				i++;
				any_digits = true;
			}
			else if ((param[i] == 'd') || (param[i] == 'D') || (param[i] == 'e') || (param[i] == 'E'))
			{
				parseCode = PARSECODE_EXPONENT0;
				i++;
			}
			else return false;
			break;

		case PARSECODE_EXPONENT0:
			if ((param[i] == '+') || (param[i] == '-'))
			{
				parseCode = PARSECODE_EXPONENT1;
				i++;
			}
			else if (isdigit( param[i] ))
			{
				parseCode = PARSECODE_EXPONENT1;
				i++;
				any_digits = true;
			}
			else return false;
			break;

		case PARSECODE_EXPONENT1:
			if (isdigit( param[i] ))
			{
				i++;
				any_digits = true;
			}
			else return false;
			break;

		default: return false;
		}
	}

#undef PARSECODE_FIRST_SIGN
#undef PARSECODE_FIRST_DIGITS
#undef PARSECODE_SECOND_DIGITS
#undef PARSECODE_EXPONENT0
#undef PARSECODE_EXPONENT1

	return any_digits;
}

////////////////////////////////////////////////////////////////////////////////
// parse the passed string as a float with possible range checking
// return codes:
//		-1 = not a number
//		-2 = value is less than min
//		-3 = value is greater than max
// Note: in both cases of returning -2 & -3, *ret has the parsed value	
////////////////////////////////////////////////////////////////////////////////
int32_t TheApp::ParseFloat( char *param, float *ret, float min, float max )
{
	if (!VerifySomeNumber( param ))
	{
		return -1;
	}

	*ret = (float)atof( param );

	// if max < min we don't range test

	if (max >= min)
	{ 
		if (*ret < min)
		{ 
			return -2;
		}
		if (*ret > max)
		{ 
			return -3;
		}
	}

	if (*ret == -0.0f)
		*ret = 0.0f;

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////
void TheApp::stdReplaceAll(std::string& str, const std::string& from, const std::string& to)
{
	if (from.empty())
		return;

	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos)
	{
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}

//////////////////////////////////////////////////////////////////////////////////////
std::string TheApp::GetPath(std::string& str)
{
	std::string ret;
	size_t found = str.find_last_of("/\\");
	if (found != std::string::npos)
	{
		ret = str.substr(0,found);
	}
	return ret;
}

//////////////////////////////////////////////////////////////////////////////////////
std::string TheApp::GetFilename(std::string& str)
{
	std::string ret = str;
	size_t last_dot_found = str.find_last_of('.');
	if (last_dot_found != std::string::npos)
	{
		ret = str.substr(0,last_dot_found);
	}
	size_t last_slash_found = ret.find_last_of("/\\");
	if (last_slash_found != std::string::npos)
	{
		ret = ret.substr(last_slash_found+1);
	}

	return ret;
}

//////////////////////////////////////////////////////////////////////////////////////
std::string TheApp::GetExtension(std::string& str)
{
	std::string ret;
	size_t found = str.find_last_of('.');

	if (found != std::string::npos)
	{
		ret = str.substr(found+1);
	}
	return ret;
}

////////////////////////////////////////////////////////////////////////////////
bool TheApp::CompareIgnoringCase(const std::string& a, const std::string& b)
{
	uint32_t sz = (uint32_t)a.size();
	if ((uint32_t)b.size() != sz)
		return false;
	for (uint32_t i = 0; i < sz; ++i)
		if (tolower(a[i]) != tolower(b[i]))
			return false;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
std::vector<std::string> TheApp::split(const std::string& text, const std::string& delims)
{
	std::vector<std::string> tokens;
	std::size_t start = text.find_first_not_of(delims), end = 0;

	while ((end = text.find_first_of(delims, start)) != std::string::npos)
	{
		tokens.push_back(text.substr(start, end - start));
		start = text.find_first_not_of(delims, end);
	}
	if (start != std::string::npos)
		tokens.push_back(text.substr(start));

	return tokens;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
bool TheApp::FindFiles(std::vector<std::string>& fnames, const char* path, const char* wildCards)
{
	struct _finddata_t c_file;
	intptr_t hFile;

	fnames.clear();
	
	std::vector<std::string> files;
	
	std::string stdPath = std::string(path);
	
	if (stdPath.back() != '\\')	
		stdPath += '\\';
	
	std::string pathWithWildCards = stdPath + std::string(wildCards);

	// Find first .c file in current directory 
	if( (hFile = _findfirst( pathWithWildCards.c_str(), &c_file )) == -1L )
	{
		return false;
	}
	else
	{
		std::string kFilename = stdPath + std::string(c_file.name); // first file found
		files.push_back(kFilename);
		while( _findnext( hFile, &c_file ) == 0 )
		{
			kFilename = stdPath + std::string(c_file.name);
			files.push_back(kFilename);
		}
		_findclose( hFile );
	}

	std::string ef(wildCards);
	std::string efextn=GetExtension(ef);
	for (int i=0; i<files.size(); i++)
	{
		std::string fname=files[i];
		std::string extn=GetExtension(fname);
		if (CompareIgnoringCase( extn, efextn ))
			fnames.push_back(fname);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// no trailing slash allowed
int32_t TheApp::IsDirectory(const char* path)
{
	struct stat info;

	int32_t ret = stat(path, &info);

	if (ret < 0)
	{
		if (errno == ENOENT)
		{
			// does not exist:
			return 0;
		}

		// if (errno == EINVAL)
		// bad input?
		return -1;
	}
	else if (info.st_mode & S_IFDIR)  
		return 1; // is a directory
	else
		return 0; // not a directory
}

////////////////////////////////////////////////////////////////////////////////
bool TheApp::IsFile(const char* fname)
{
	int fhandle = _open(fname, _O_BINARY | _O_RDONLY, 0);
	if (fhandle == -1) 
		 return false;
	_close(fhandle);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////
bool TheApp::SetURLHistoryLimit(uint32_t limit)
{
	if (mp_config)
	{
		std::string dataKey = "app/url_history_limit";
		mp_config->WriteInt(dataKey, limit);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////
// only writes out up to the url_history_limit
bool TheApp::SetURLHistory(std::vector<std::string>& history)
{
	if (mp_config)
	{
		std::string dataKey = "app/url_history_limit";
		int32_t url_history_limit = mp_config->ReadInt(dataKey, 10);

		int32_t limit = MIN(url_history_limit, (int32_t)history.size());

		for (int32_t i = 0; i < limit; i++)
		{
			dataKey = FormatStr("app/url%d", i);
			mp_config->WriteString(dataKey, (char*)history[i].c_str());
		}

		dataKey = "app/url_history_count";
		mp_config->WriteInt(dataKey, limit);

		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////
bool TheApp::GetURLHistory(std::vector<std::string>& history)
{
	if (mp_config)
	{
		std::string dataKey = "app/url_history_count";
		int32_t url_history_count = mp_config->ReadInt(dataKey, 10);

		history.clear();
		for (int32_t i = 0; i < url_history_count; i++)
		{
			dataKey = FormatStr("app/url%d", i);
			std::string url = mp_config->ReadString(dataKey, "read failure");
			if ((url.size() > 0) && (url.compare( "read failure" ) != 0))
				 history.push_back(url);
		}

		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////
bool TheApp::RememberThisURL(std::string& url)
{
	if (url.size() < 1)
		return false;

	if (mp_config)
	{
		std::vector<std::string> history;
		if (GetURLHistory(history))
		{
			std::vector<std::string> new_history;
			new_history.push_back(url);
			
			for (auto it = history.begin(); it != history.end(); ++it) 
				if ((it->size() > 0) && (url.compare( *it ) != 0 ))
					 new_history.push_back( *it );

			return SetURLHistory(new_history);
		}
	}

	return false;
}




///////////////////////////////////////////////////////////////////////////////////
bool TheApp::SetPostProcessFilterHistoryLimit(uint32_t limit)
{
	if (mp_config)
	{
		std::string dataKey = "app/postprocess_history_limit";
		mp_config->WriteInt(dataKey, limit);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////
// only writes out up to the postprocess_history_limit
bool TheApp::SetPostProcessFilterHistory(std::vector<std::string>& history)
{
	if (mp_config)
	{
		std::string dataKey = "app/postprocess_history_limit";
		int32_t postprocess_history_limit = mp_config->ReadInt(dataKey, 40);

		int32_t limit = MIN(postprocess_history_limit, (int32_t)history.size());

		for (int32_t i = 0; i < limit; i++)
		{
			dataKey = FormatStr("app/ppf%d", i);
			mp_config->WriteString(dataKey, (char*)history[i].c_str());
		}

		dataKey = "app/postprocess_history_count";
		mp_config->WriteInt(dataKey, limit);

		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////
bool TheApp::GetPostProcessFilterHistory(std::vector<std::string>& history)
{
	if (mp_config)
	{
		std::string dataKey = "app/postprocess_history_count";
		int32_t postprocess_history_count = mp_config->ReadInt(dataKey, 40);

		history.clear();
		for (int32_t i = 0; i < postprocess_history_count; i++)
		{
			dataKey = FormatStr("app/ppf%d", i);
			std::string postprocess = mp_config->ReadString(dataKey, "read failure");
			if ((postprocess.size() > 0) && (postprocess.compare( "read failure" ) != 0))
				history.push_back(postprocess);
		}

		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////
bool TheApp::RememberThisPostProcessFilter(std::string& postprocess)
{
	if (postprocess.size() < 1)
		return false;

	if (mp_config)
	{
		std::vector<std::string> history;
		if (GetPostProcessFilterHistory(history))
		{
			std::vector<std::string> new_history;
			new_history.push_back(postprocess);

			for (auto it = history.begin(); it != history.end(); ++it) 
				if ((it->size() > 0) && (postprocess.compare( *it ) != 0 ))
					new_history.push_back( *it );

			return SetPostProcessFilterHistory(new_history);
		}
	}

	return false;
}



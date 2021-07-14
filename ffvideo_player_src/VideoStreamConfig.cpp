///////////////////////////////////////////////////////////////////////////////
// Name:        VideoStreamConfig.cpp
// Author:      Blake Senftner
///////////////////////////////////////////////////////////////////////////////

#include "ffvideo_player_app.h"

// create new video stream config with id and default settings
VideoStreamConfig::VideoStreamConfig(TheApp* app, CKeyValueStore* keyValueStore)
{
	std::string dataKey("app/last_video_id");
	m_id = keyValueStore->ReadInt(dataKey, -1) + 1;	// +1 because we're creating a new config

	Init(app, keyValueStore);
}



// load video stream config with id, if not found gets default settings
VideoStreamConfig::VideoStreamConfig(TheApp* app, CKeyValueStore* keyValueStore, int32_t stream_id)
{
	m_id = stream_id;
	Init(app, keyValueStore);
}



// load video stream config with id, if not found gets default settings
void VideoStreamConfig::Init(TheApp* app, CKeyValueStore* keyValueStore)
{
	std::string bsjnk;

	std::string data_prefix = "win" + std::to_string(m_id) + "/";

	std::string data_key = data_prefix + "win_width";
	m_win_width = keyValueStore->ReadInt(data_key, 1280);

	data_key = data_prefix + "win_height";
	m_win_height = keyValueStore->ReadInt(data_key, 720);

	data_key = data_prefix + "win_left";
	m_win_left = keyValueStore->ReadInt(data_key, 100);

	data_key = data_prefix + "win_top";
	m_win_top = keyValueStore->ReadInt(data_key, 100);



	data_key = data_prefix + "name";
	bsjnk = app->FormatStr("win%d", m_id );
	m_name = keyValueStore->ReadString(data_key, (char*)bsjnk.c_str());

	data_key = data_prefix + "stream_type";
	int32_t stype = keyValueStore->ReadInt(data_key, 100);
	switch (stype)
	{
	default:
	case 0:
		m_type = STREAM_TYPE::FILE;
		data_key = data_prefix + "last_good_mediafile";
		m_info = keyValueStore->ReadString(data_key, "unknown");
		break;
	case 1:
		m_type = STREAM_TYPE::USB;
		data_key = data_prefix + "last_good_usbpin";
		m_info = keyValueStore->ReadString(data_key, "unknown");
		break;
	case 2:
		m_type = STREAM_TYPE::IP;
		data_key = data_prefix + "last_good_url";
		m_info = keyValueStore->ReadString(data_key, "unknown");
		break;
	}

	data_key = data_prefix + "loop_media_files";
	m_loop_media_files = keyValueStore->ReadBool(data_key, false);

	// AVFilterGraph filter string:
	data_key = data_prefix + "post_process";
	m_post_process = keyValueStore->ReadString(data_key, ""); // defaults to empty 

	// overlay font info:
	data_key = data_prefix + "font_face_name";
	m_font_face_name = keyValueStore->ReadString(data_key, "Ariel Black");

	data_key = data_prefix + "font_point_size";
	m_font_point_size = keyValueStore->ReadInt(data_key, 24);

	data_key = data_prefix + "font_italic";
	m_font_italic = keyValueStore->ReadBool(data_key, false);

	data_key = data_prefix + "font_underlined";
	m_font_underlined = keyValueStore->ReadBool(data_key, false);

	data_key = data_prefix + "font_strike_thru";
	m_font_strike_thru = keyValueStore->ReadBool(data_key, false);

	data_key = data_prefix + "font_color";
	//
	FF_Vector3D default_color( 0.0f, 1.0f, 0.0f );
	std::string fontColorStr = FF_Vector3D_to_string(app, default_color);
	//
	fontColorStr = keyValueStore->ReadString(data_key, (char*)fontColorStr.c_str());
	//
	m_font_color = FF_Vector3D_from_string(app, fontColorStr);


	data_key = data_prefix + "frame_interval";
	m_frame_interval = keyValueStore->ReadInt(data_key, 1);

	data_key = data_prefix + "frame_interval_profile";
	m_frame_interval_profile = keyValueStore->ReadString(data_key, "7680, 3, 1920, 2, 320, 1");


	data_key = data_prefix + "export_interval";
	m_export_interval = keyValueStore->ReadInt(data_key, 0);

	data_key = data_prefix + "export_dir";
	m_export_dir = keyValueStore->ReadString(data_key, "unknown");

	data_key = data_prefix + "export_base";
  bsjnk = app->FormatStr("win%d", m_id );
	m_export_base = keyValueStore->ReadString(data_key, (char*)bsjnk.c_str() ); 

	data_key = data_prefix + "export_scale";
	m_export_scale = keyValueStore->ReadReal(data_key, 0.5f);

	data_key = data_prefix + "export_quality";
	m_export_quality = keyValueStore->ReadInt(data_key, 80);


	data_key = data_prefix + "encode_interval";
	m_encode_interval = keyValueStore->ReadInt(data_key, 0);

	data_key = data_prefix + "post_encode";
	m_post_encode = keyValueStore->ReadInt(data_key, 0);

	data_key = data_prefix + "ffmpeg_path";
	m_ffmpeg_path = keyValueStore->ReadString(data_key, "unknown");

	data_key = data_prefix + "post_move_path";
	m_post_move_path = keyValueStore->ReadString(data_key, "unknown");

	data_key = data_prefix + "encode_dir";
	m_encode_dir = keyValueStore->ReadString(data_key, "unknown");

	data_key = data_prefix + "encode_base";
	bsjnk = app->FormatStr("win%d", m_id );
	m_encode_base = keyValueStore->ReadString(data_key, (char*)bsjnk.c_str());

	data_key = data_prefix + "encode_fps";
	m_encode_fps = keyValueStore->ReadReal(data_key, 0.0f);

	data_key = data_prefix + "encode_type";
	m_encode_type = keyValueStore->ReadInt(data_key, 4);

	data_key = data_prefix + "encode_width";
	m_encode_width = keyValueStore->ReadInt(data_key, 1024);

	data_key = data_prefix + "encode_height";
	m_encode_height = keyValueStore->ReadInt(data_key, -1);
}



VideoStreamConfig::VideoStreamConfig(const VideoStreamConfig& vsc)
{
	m_id = vsc.m_id;

	m_win_width = vsc.m_win_width;
	m_win_height = vsc.m_win_height;
	m_win_left = vsc.m_win_left;
	m_win_top = vsc.m_win_top;

	m_name							= vsc.m_name;
	m_type							= vsc.m_type;
	m_info							= vsc.m_info;
	m_loop_media_files	= vsc.m_loop_media_files;
	m_usb_fmt						= vsc.m_usb_fmt;
	m_post_process      = vsc.m_post_process;

	m_font_face_name   = vsc.m_font_face_name;
	m_font_point_size  = vsc.m_font_point_size;
	m_font_italic      = vsc.m_font_italic;
	m_font_underlined  = vsc.m_font_underlined;
	m_font_strike_thru = vsc.m_font_strike_thru;
	m_font_color       = vsc.m_font_color;

	m_frame_interval         = vsc.m_frame_interval;
	m_frame_interval_profile = vsc.m_frame_interval_profile;

	m_export_interval = vsc.m_export_interval;
	m_export_dir      = vsc.m_export_dir;
	m_export_base     = vsc.m_export_base;
	m_export_scale    = vsc.m_export_scale;
	m_export_quality  = vsc.m_export_quality;

	m_encode_interval = vsc.m_encode_interval.load(std::memory_order::memory_order_relaxed);
	m_post_encode     = vsc.m_post_encode;
	m_ffmpeg_path     = vsc.m_ffmpeg_path;
	m_post_move_path  = vsc.m_post_move_path;
	m_encode_dir      = vsc.m_encode_dir;
	m_encode_base     = vsc.m_encode_base;
	m_encode_fps      = vsc.m_encode_fps;
	m_encode_type     = vsc.m_encode_type;
	m_encode_width    = vsc.m_encode_width;
	m_encode_height   = vsc.m_encode_height;
}

VideoStreamConfig& VideoStreamConfig::operator = (const VideoStreamConfig& vsc)
{
	if (this != &vsc)
	{
		m_id = vsc.m_id;

		m_win_width = vsc.m_win_width;
		m_win_height = vsc.m_win_height;
		m_win_left = vsc.m_win_left;
		m_win_top = vsc.m_win_top;

		m_name							= vsc.m_name;
		m_type							= vsc.m_type;
		m_info							= vsc.m_info;
		m_loop_media_files	= vsc.m_loop_media_files;
		m_usb_fmt						= vsc.m_usb_fmt;
		m_post_process      = vsc.m_post_process;

		m_font_face_name    = vsc.m_font_face_name;
		m_font_point_size   = vsc.m_font_point_size;
		m_font_italic       = vsc.m_font_italic;
		m_font_underlined   = vsc.m_font_underlined;
		m_font_strike_thru  = vsc.m_font_strike_thru;
		m_font_color        = vsc.m_font_color;

		m_frame_interval         = vsc.m_frame_interval;
		m_frame_interval_profile = vsc.m_frame_interval_profile;

		m_export_interval = vsc.m_export_interval;
		m_export_dir      = vsc.m_export_dir;
		m_export_base     = vsc.m_export_base;
		m_export_scale    = vsc.m_export_scale;
		m_export_quality  = vsc.m_export_quality;

		m_encode_interval = vsc.m_encode_interval.load(std::memory_order::memory_order_relaxed);
		m_post_encode     = vsc.m_post_encode;
		m_ffmpeg_path     = vsc.m_ffmpeg_path;
		m_post_move_path  = vsc.m_post_move_path;
		m_encode_dir      = vsc.m_encode_dir;
		m_encode_base     = vsc.m_encode_base;
		m_encode_fps      = vsc.m_encode_fps;
		m_encode_type     = vsc.m_encode_type;
		m_encode_width    = vsc.m_encode_width;
		m_encode_height   = vsc.m_encode_height;
	}
	return (*this);
}



std::string VideoStreamConfig::StreamTypeString(void)
{
	switch (m_type)
	{
	default:
	case STREAM_TYPE::FILE:	return std::string("Media File");
	case STREAM_TYPE::USB:  return std::string("USB WebCam");
	case STREAM_TYPE::IP:		return std::string("Stream URL");
	}
}


std::string VideoStreamConfig::FF_Vector3D_to_string(TheApp* app, FF_Vector3D& v)
{
	return app->FormatStr("%g,%g,%g", v.x, v.y, v.z);
}

FF_Vector3D VideoStreamConfig::FF_Vector3D_from_string(TheApp* app, std::string str)
{
	FF_Vector3D v;
	sscanf( str.c_str(), "%f,%f,%f", &v.x, &v.y, &v.z );

	return v;
}



bool VideoStreamConfig::ParseAutoFrameInterval(TheApp* app, char* buff, std::vector<uint32_t>& widths, std::vector<uint32_t>& intervals, std::string& err )
{
	widths.clear();
	intervals.clear();

	int   nTokens(0);
	CHAR  *token, *parseTokens[24], *seps = ", ", *nextToken = NULL;

	nTokens = 0;
	token = strtok_s( buff, seps, &nextToken );
	while (token != NULL) 
	{
		parseTokens[nTokens++] = token;
		token = strtok_s( NULL, seps, &nextToken );
	}
	if (nTokens & 1)
	{
		err = "An even number of comma separated integers is expected.";
		return false;
	}
	else
	{
		bool good = true;
		for (size_t i = 0; i < nTokens; i += 2)
		{
			int32_t width, interval;

			if (app->ParseInteger( parseTokens[i], &width, 1, 0 ) == 0)
			{
				if (app->ParseInteger( parseTokens[i+1], &interval, 1, 0 ) == 0)
				{
					widths.push_back(width);
					intervals.push_back(interval);
				}
				else good = false;
			}
			else good = false;
		}
		if (!good)
		{
			err = "Input is not composed of comma separated numbers.";
		}
		else
		{
			for (size_t i = 1; i < widths.size(); i++)
			{
				if (widths[i] >= widths[i-1])
					good = false;
			}
			if (!good)
			{
				err = "Input widths do not decend in size.\n";
			}
			else
			{
				return true;
			}
			return false;
		}
	}
	return false;
}



void VideoStreamConfig::Save(TheApp* app)
{
  CKeyValueStore* keyValueStore = app->mp_config;

	SaveWindowSize( keyValueStore );
	SaveWindowPosition( keyValueStore );
	SaveStreamType( keyValueStore );
	SaveStreamInfo( keyValueStore );

	std::string data_prefix = "win" + std::to_string(m_id) + "/";

	std::string data_key = data_prefix + "name";
	keyValueStore->WriteString( data_key, (char*)m_name.c_str() );


	data_key = data_prefix + "loop_media_files";
	keyValueStore->WriteBool( data_key, m_loop_media_files );


	data_key = data_prefix + "post_process";
	keyValueStore->WriteString( data_key, (char*)m_post_process.c_str() );


	data_key = data_prefix + "font_face_name";
	keyValueStore->WriteString( data_key, (char*)m_font_face_name.c_str() );

	data_key = data_prefix + "font_point_size";
	keyValueStore->WriteInt( data_key, m_font_point_size );

	data_key = data_prefix + "font_italic";
	keyValueStore->WriteBool( data_key, m_font_italic );

	data_key = data_prefix + "font_underlined";
	keyValueStore->WriteBool( data_key, m_font_underlined );

	data_key = data_prefix + "font_strike_thru";
	keyValueStore->WriteBool( data_key, m_font_strike_thru );

	data_key = data_prefix + "font_color";
	std::string fontColorStr = FF_Vector3D_to_string(app, m_font_color);
	keyValueStore->WriteString( data_key, (char*)fontColorStr.c_str() );


	data_key = data_prefix + "frame_interval";
	keyValueStore->WriteInt( data_key, m_frame_interval );

	data_key = data_prefix + "frame_interval_profile";
	keyValueStore->WriteString( data_key, (char*)m_frame_interval_profile.c_str() );


	data_key = data_prefix + "export_interval";
	keyValueStore->WriteInt( data_key, m_export_interval );

	data_key = data_prefix + "export_dir";
	keyValueStore->WriteString( data_key, (char*)m_export_dir.c_str() );

	data_key = data_prefix + "export_base";
	keyValueStore->WriteString( data_key, (char*)m_export_base.c_str() );

	data_key = data_prefix + "export_scale";
	keyValueStore->WriteReal( data_key, m_export_scale );

	data_key = data_prefix + "export_quality";
	keyValueStore->WriteInt( data_key, m_export_quality );


	data_key = data_prefix + "encode_interval";
	keyValueStore->WriteInt( data_key, m_encode_interval );

	data_key = data_prefix + "post_encode";
	keyValueStore->WriteInt( data_key, m_post_encode );

	data_key = data_prefix + "ffmpeg_path";
	keyValueStore->WriteString( data_key, (char*)m_ffmpeg_path.c_str() );

	data_key = data_prefix + "post_move_path";
	keyValueStore->WriteString( data_key, (char*)m_post_move_path.c_str() );

	data_key = data_prefix + "encode_dir";
	keyValueStore->WriteString( data_key, (char*)m_encode_dir.c_str() );

	data_key = data_prefix + "encode_base";
	keyValueStore->WriteString( data_key, (char*)m_encode_base.c_str() );

	data_key = data_prefix + "encode_fps";
	keyValueStore->WriteReal( data_key, m_encode_fps );

	data_key = data_prefix + "encode_type";
	keyValueStore->WriteInt( data_key, m_encode_type );

	data_key = data_prefix + "encode_width";
	keyValueStore->WriteInt( data_key, m_encode_width );

	data_key = data_prefix + "encode_height";
	keyValueStore->WriteInt( data_key, m_encode_height );
}



void VideoStreamConfig::SaveWindowSize( CKeyValueStore* keyValueStore )
{
	std::string data_prefix = "win" + std::to_string(m_id) + "/";

	std::string data_key = data_prefix + "win_width";
	keyValueStore->WriteInt(data_key, m_win_width);

	data_key = data_prefix + "win_height";
	keyValueStore->WriteInt(data_key, m_win_height);
}



void VideoStreamConfig::SaveWindowPosition( CKeyValueStore* keyValueStore )
{
	std::string data_prefix = "win" + std::to_string(m_id) + "/";

	std::string data_key = data_prefix + "win_left";
	keyValueStore->WriteInt(data_key, m_win_left);

	data_key = data_prefix + "win_top";
	keyValueStore->WriteInt(data_key, m_win_top);
}



int32_t VideoStreamConfig::StreamTypeInt( void )
{
	switch (m_type)
	{
	default:
	case STREAM_TYPE::FILE:	return 0;
	case STREAM_TYPE::USB:  return 1;
	case STREAM_TYPE::IP:   return 2;
	}
}


void VideoStreamConfig::SaveStreamType( CKeyValueStore* keyValueStore )
{
	std::string data_prefix = "win" + std::to_string(m_id) + "/";
	std::string data_key = data_prefix + "stream_type";

	switch (m_type)
	{
	case STREAM_TYPE::FILE:	
		keyValueStore->WriteInt( data_key, 0 );
		break;
	case STREAM_TYPE::USB:
		keyValueStore->WriteInt( data_key, 1 );
		break;
	case STREAM_TYPE::IP:
		keyValueStore->WriteInt( data_key, 2 );
		break;
	}
}



void VideoStreamConfig::SaveStreamInfo( CKeyValueStore* keyValueStore )
{
	std::string data_prefix = "win" + std::to_string(m_id) + "/";
	std::string data_key;

	switch (m_type)
	{
	case STREAM_TYPE::FILE:	
		data_key = data_prefix + "last_good_mediafile";;
		keyValueStore->WriteString( data_key, (char *)m_info.c_str() );
		break;
	case STREAM_TYPE::USB:
		data_key = data_prefix + "last_good_usbpin";;
		keyValueStore->WriteString( data_key, (char *)m_info.c_str() );
		break;
	case STREAM_TYPE::IP:
		data_key = data_prefix + "last_good_url";;
		keyValueStore->WriteString( data_key, (char *)m_info.c_str() );
		break;
	}
}



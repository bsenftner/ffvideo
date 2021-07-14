#pragma once

#ifndef _VIDEOSTREAMCONFIG_H_
#define _VIDEOSTREAMCONFIG_H_


#include "ffvideo_player_app.h"


/////////////////////////
// our video sources:
enum class STREAM_TYPE { FILE = 0, USB, IP };


class VideoStreamConfig
{
public:
	// create new video stream config with id and default settings
	VideoStreamConfig(TheApp* app, CKeyValueStore* keyValueStore);

	// load video stream config with id, if not found gets default settings
	VideoStreamConfig(TheApp* app, CKeyValueStore* keyValueStore, int32_t stream_id);

	~VideoStreamConfig() {};

	// copy constructor 
	VideoStreamConfig(const VideoStreamConfig& vsc);

	// copy assignement operator
	VideoStreamConfig& operator = (const VideoStreamConfig& vsc);

private:
	void Init(TheApp* app, CKeyValueStore* keyValueStore);
public:

	std::string StreamTypeString(void);

	std::string FF_Vector3D_to_string(TheApp* app, FF_Vector3D& v);
	FF_Vector3D FF_Vector3D_from_string(TheApp* app, std::string str);

	bool ParseAutoFrameInterval(TheApp* app, char* buff, std::vector<uint32_t>& widths, std::vector<uint32_t>& intervals, std::string& err );
	
	void Save(TheApp* app);
	//
	void SaveWindowSize( CKeyValueStore* keyValueStore );
	void SaveWindowPosition( CKeyValueStore* keyValueStore );

	void SaveStreamType( CKeyValueStore* keyValueStore );
	int32_t StreamTypeInt( void );

	void SaveStreamInfo( CKeyValueStore* keyValueStore );


	// fields

	int32_t											m_id;		// stream id, "win" + to_string(id) is the stream's keyValueStore key prefix

	int32_t											m_win_width,		// last window pos and size
															m_win_height, 
															m_win_left, 
															m_win_top;

	std::string									m_name;	// "name" of camera / video stream
	STREAM_TYPE									m_type;
	//
	// depending upon m_type, m_info's value varies, but is always a string:
	//	FILE:	is the file path
	//	USB:	is the PIN number integer
	//	IP:		is the url string with any required user:pass embedded in the url
	//
	std::string									m_info;

	bool												m_loop_media_files;			// only valid for media files

	FFVIDEO_USB_Camera_Format		m_usb_fmt;							// used if m_type is USB

	std::string									m_post_process;					// optional AVFilterGraph filter string

	std::string									m_font_face_name;				// video overlay font characteristics 
	int32_t											m_font_point_size;
	bool												m_font_italic;
	bool												m_font_underlined;
	bool												m_font_strike_thru;
	FF_Vector3D									m_font_color;

	int32_t											m_frame_interval;				// how often the frame callback is called
	std::string									m_frame_interval_profile;	// optional list of frame intervals at different resolutions

	int32_t											m_export_interval;			// how often the frame exporter is called
	std::string									m_export_dir;						// directory frame is exported, if exported
	std::string									m_export_base;					// export filename basename before timecode for frame
	float												m_export_scale;					// some normalized value, 0.0 to 1.0; don't allow larger than delivered
	int32_t											m_export_quality;				// jpeg quality setting

	std::atomic<int32_t>				m_encode_interval;			// 0 = off, if > 0 is number of exported frames before encoding them
	int32_t											m_post_encode;					// an action code; 0 = delete frames, 1 = move to frame store
	std::string									m_ffmpeg_path;					// required path to ffmpeg.exe for encoding
	std::string									m_post_move_path;				// if m_post_encode = 1, is path to move image files
	std::string									m_encode_dir;						// directory encoded movies are saved, if encoding
	std::string									m_encode_base;					// encoding basename of movie
	float												m_encode_fps;						// 0 = last playback fps, else actual encoding fps 
	int32_t											m_encode_type;					// 0 = encode H.264 mp4s, 1 = encode H.264 MP4 and elementary streams
	int32_t											m_encode_width;					// set this or m_encode_height to -1 to retain aspect ratio of original frames			
	int32_t											m_encode_height;				// set this or m_encode_width to -1 to retain aspect ratio of original frames			

										 
};




#endif // _VIDEOSTREAMCONFIG_H_
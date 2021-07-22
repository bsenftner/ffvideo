#pragma once


#ifndef _RENDERCANVAS_H_
#define _RENDERCANVAS_H_

#include "ffvideo_player_app.h"

////////////////////////////////////////////////////////////////////////
enum class VIDEO_STATUS {
	NEVER_PLAYED = 0,
	WAITING_FOR_FIRST_FRAME,
	RECEIVING_FRAMES,
	PAUSED,
	STOPPED,
	UNEXPECTED_TERMINATION,
	MEDIA_ENDED,
	OPENING_MEDIA_FILE_FAILED,
	OPENING_USB_FAILED,
	OPENING_IP_FAILED
};

////////////////////////////////////////////////////////////////////////
class RenderCanvas : public wxGLCanvas
{
public:
	RenderCanvas(wxWindow* parent, VideoStreamConfig* vsc);
	~RenderCanvas();

	void GenerateRenderFont(VideoStreamConfig* vsc);

	bool LoadLogoFrame(void);
	bool LoadPleaseWaitFrame(void);

	// wxWidgets event callbacks
	void OnPaint(wxPaintEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnMouseEvent(wxMouseEvent& event);
	void OnVideoNotice(wxCommandEvent& event);					 // custom event, add notice to video overlay
	void DelayeCallbacksMonitor(wxTimerEvent& event);    // callback for the DelayedCallbacksMgr 
																											 //
	void SendVideoEncodeEvent(void);										 // custom event, do an FFmpeg encoding
	void OnVideoEncode(wxCommandEvent& event);
	//
	void SendVideoEncodeDoneEvent(void);								 // custom event, signal FFmpeg encode done
	void OnVideoEncodeDone(wxCommandEvent& event);

	bool InitFFMPEG(bool deleteFirst);

	// video library callbacks:
	//
	// Static version is called by video lib, non-static is then called for easy object member access:
	//
	static void FrameCallBack(void* p_object, FFVideo_Image& im, int frame_num);
	void FrameCallBack(FFVideo_Image& im, int frame_num);
	//
	static void FrameExportCallBack(void* p_object, int32_t frame_num, int32_t export_num, const char* filepath, bool status);
	void FrameExportCallBack(int32_t frame_num, int32_t export_num, const char* filepath, bool status);
	//
	static void UnexpectedTerminationCallBack(void* p_object);
	void UnexpectedTerminationCallBack(void);
	//
	static void MediaEndedCallBack(uint32_t frame_num, void* p_object);
	void MediaEndedCallBack(uint32_t frame_num);
	//
	static void AVLibLoggingCallBack(const char* msg, void* p_object);
	void AVLibLoggingCallBack(const char* msg);

	// callback setup for local face detection, installed into the m_faceDetectMgr:
	//
	static void FrameFaceDetectionCallBack(void* p_object, FaceDetectionFrame& fdf);
	void FrameFaceDetectionCallBack(FaceDetectionFrame& fdf);

	void CommonFrameHandling( FFVideo_Image& im, int frame_num );

	// video controls (GLButtons) callbacks:
	static void PlayButton_cb(void* p_object, GLButton* button);
	void PlayButton_cb(GLButton* button);
	//
	static void PauseButton_cb(void* p_object, GLButton* button);
	void PauseButton_cb(GLButton* button);
	//
	static void StepBackButton_cb(void* p_object, GLButton* button);
	void StepBackButton_cb(GLButton* button);
	//
	static void StopButton_cb(void* p_object, GLButton* button);
	void StopButton_cb(GLButton* button);
	//
	static void StepForwardButton_cb(void* p_object, GLButton* button);
	void StepForwardButton_cb(GLButton* button);

	std::string GetStatus(void)
	{
		std::string status("Video status: ");

		switch (m_status)
		{
		default:
		case VIDEO_STATUS::NEVER_PLAYED:              return (status + "NEVER_PLAYED");
		case VIDEO_STATUS::WAITING_FOR_FIRST_FRAME:   return (status + "WAITING_FOR_FIRST_FRAME");
		case VIDEO_STATUS::RECEIVING_FRAMES:          return (status + "RECEIVING_FRAMES");
		case VIDEO_STATUS::PAUSED:                    return (status + "PAUSED");
		case VIDEO_STATUS::STOPPED:                   return (status + "STOPPED");
		case VIDEO_STATUS::UNEXPECTED_TERMINATION:    return (status + "UNEXPECTED_TERMINATION");
		case VIDEO_STATUS::MEDIA_ENDED:               return (status + "MEDIA_ENDED");
		case VIDEO_STATUS::OPENING_MEDIA_FILE_FAILED: return (status + "OPENING_MEDIA_FILE_FAILED");
		case VIDEO_STATUS::OPENING_USB_FAILED:        return (status + "OPENING_USB_FAILED");
		case VIDEO_STATUS::OPENING_IP_FAILED:         return (status + "OPENING_IP_FAILED");
		}
	}

	void WindowStatus(wxString msg);

	void SendOverlayNoticeEvent( std::string& msg, int32_t milliseconds );

	// callback installed into the DelayedCallbackMgr
	//
	static void UnexpectedTerminationReplayCallBack(void* p_object);
	void UnexpectedTerminationReplayCallBack(void);


	void Stop(void);		// stop playing video (if playing)
	void Pause(void);
private:
	bool HandleUnPausing(void);
	bool HandlePausing(void);
public:

	// because starting playback can take time, Play() is the public interface to
	// the creation of another thread that actually initiates the playback.
	// Done in this manner, the main thread hosting the GUI is not delayed by video buffering.
	bool Play(void);
private:
	void ThreadedPlay(void);
	// thread variables:
	std::thread* mp_playThread;
	std::atomic<bool> m_playThreadStarted;
	std::atomic<bool> m_playThreadExited;
	std::atomic<bool> m_playThreadJoined;
	//
	bool PlayMediaFile(void);
	bool PlayUSBCamera(void);
	bool PlayIPCamera(void);

	// here's a different thread operation; here is a vector of FrameEncoder, one for each frame  
	// encode spawned by both exporting frames and then encoding them in encode_interval chunks.
	//
	mutable std::shared_mutex	 m_encodersSharedMutex;	
	std::vector<FrameEncoder*> m_encoders;
	std::atomic<int32_t>			 m_next_encoder_id;

	// a double buffer of strings, used to hold exported frame filenames.
	// Double buffered so the logic does not need to worry about threading collision: 
	std::vector<std::string>   m_exportedFramePaths[2];

public:

	bool IsFaceDetectionInitialized( void ) { return m_faceDetectMgr.m_faceDetectorInitialized; }
	bool IsFaceDetectionEnabled( void ) { return m_faceDetectMgr.m_faceDetectorEnabled; }
	bool IsFaceLandmarksEnabled( void ) { return m_faceDetectMgr.m_faceFeaturesEnabled; }
	bool IsFaceImagesEnabled( void ) { return m_faceDetectMgr.m_faceImagesEnabled; }
	bool EnableFaceDetection( bool enable );
	bool EnableFaceLandmarks( bool enable );
	bool EnableFaceImages( bool enable );

	// framebuffer rendering:

	void RenderGradientBg(float minx, float miny, float maxx, float maxy);
	void DrawString(const std::string& s, float x, float y, const FF_Vector3D& rgb);
	void DrawLines(std::vector<FF_Vector2D>& pts, float z, float ff, FF_Vector2D& scale, bool closed );
	void DrawFaces(void);
	void CalcVideoPlacement(void); // calculates m_zoom and m_trans for the current frame
	void Render(void);

	VideoWindow*							mp_videoWindow;		// also parent
	TheApp*										mp_app;						// TheApp hosts the App Clock, App Log, and some string utils
	FFVideo*									mp_ffvideo;				// the ffmpeg based video library

	wxGLContext*							mp_glRC;					// our OpenGL context
	std::atomic<bool>				  m_OGL_font_dirty;	// if true the OpenL font needs to be regenerated


	std::mutex								m_frame_lock;			// thread protection for rendering

	FFVideo_Image							m_frame;					// video framebuffer
	std::atomic<VIDEO_STATUS>	m_status;					// current state of video player 

	bool											m_frame_loaded;
	bool											m_is_paused;
	bool											m_is_playing;

	int32_t										m_current_frame_num;	// "frame number" in media file, not necessary frame count received from library
	int32_t										m_frame_count;				// frames received from video library

	uint32_t									m_expected_width;			// media files get these set by the info callback
	uint32_t									m_expected_height;
	float											m_expected_fps;
	float											m_expected_duration;
	int32_t										m_expected_frames;

	VideoCtrls								m_videoCtls;					// rendered after Tweened callbacks trigered

	std::atomic<bool>					m_terminating;				// signal to all parts of app, we're quitting

	DelayedCallbackMgr				m_delayedCallbacks;		// used to handle replaying terminated IP cameras after a pause


	wxSize										m_winSize;						// window size 
	float											m_zoom;								// scale factor for video image to fit the window
	FF_Vector2D								m_trans;							// translation to center frame

	FF_Vector2D								m_npos;								// normalized position of mouse inside video frame
	FF_Coord2D								m_mpos;								// position of mouse in window
	FF_Coord2D								m_rel_mpos;						// relative position of mouse in window
	bool											m_mouse_moving;				// true if moving and no button is down
	bool											m_mouse_dragging;			// true if moving with a button down
	bool											m_mouse_left_down;
	bool											m_mouse_right_down;
	bool											m_mouse_both_down;
	bool											m_mouse_ctrl_down;
	bool											m_mouse_shft_down;

	std::vector<std::string>	m_text;								// multi-line msg text rendered over video
	FF_Coord2D								m_text_pos;						// position of text (user can drag it)
	int32_t										m_line_height;

	NoticeMgr*								mp_noticeMgr;					// manage "notices" - expiring messages overlaid over the video frame

	int32_t										m_ip_restarts;

	bool											m_theme;							// 0 = dark, 1 = light...

	// face & feature detection:
	FaceDetectionThreadMgr										m_faceDetectMgr;
	FF_Vector2D																m_detectImSize;
	std::vector<dlib::rectangle>							m_detections;
	std::vector<dlib::full_object_detection>  m_facesLandmarkSets;
	std::vector<FFVideo_Image>								m_facesImages;

	wxDECLARE_EVENT_TABLE();
};


#endif // _RENDERCANVAS_H_
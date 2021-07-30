#pragma once

#ifndef _FACEDETECT_H_
#define _FACEDETECT_H_

#include "ffvideo_player_app.h"

#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>

class FaceDetector
{
public:
	FaceDetector(TheApp* app);
	~FaceDetector();

	void SetImage( FFVideo_Image& im, float detection_scale );

	void GetDlibImageSize( FF_Vector2D& size ) { size.Set( m_dlib_real_im.nc(), m_dlib_real_im.nr() ); }

	bool GetFaceBoxes( std::vector<dlib::rectangle>& detections );

	void GetFaceImages( std::vector<dlib::rectangle>& detections, 
											std::vector<dlib::full_object_detection>& faceLandmarkSets,
											std::vector<FFVideo_Image>& face_images,
											bool full_head_flag = false );

	bool GetFaceLandmarkSets( std::vector<dlib::rectangle>& detections, 
	                          std::vector<dlib::full_object_detection>& faceLandmarkSets );

	void GetLandmarks( dlib::full_object_detection& oneFace_landmarkSet,
										 std::vector<FF_Vector2D>& jawline,
										 std::vector<FF_Vector2D>& rtBrow,
										 std::vector<FF_Vector2D>& ltBrow,
										 std::vector<FF_Vector2D>& noseBridge,
										 std::vector<FF_Vector2D>& noseBottom,
										 std::vector<FF_Vector2D>& rtEye,
										 std::vector<FF_Vector2D>& ltEye,
										 std::vector<FF_Vector2D>& outsideLips,
										 std::vector<FF_Vector2D>& insideLips );

private:

	dlib::frontal_face_detector			m_detector;
	dlib::shape_predictor						m_sp;

	FFVideo_Image										m_im;							// video frame as delivered by ffmpeg
	bool									          m_image_set;

	float														m_detect_scale;		// normalized size factor between the two below

	dlib::array2d<uint8_t>					m_dlib_im;				// is same w,h as m_im but greyscale
	dlib::array2d<uint8_t>          m_dlib_real_im;		// potentially scaled to speed up detections or enhance precision
};



//------------------------------------------------------------------------------
class FaceDetectionFrame
{
public:
	FaceDetectionFrame() {};

	// copy constructor 
	FaceDetectionFrame(const FaceDetectionFrame& fdf)
	{
		m_im.Clone(fdf.m_im);
		m_frame_num         = fdf.m_frame_num;
		m_detections        = fdf.m_detections;
		m_facesLandmarkSets = fdf.m_facesLandmarkSets;
		m_facesImages       = fdf.m_facesImages;
	}

	// copy assignement operator
	FaceDetectionFrame& operator = (const FaceDetectionFrame& fdf)
	{
		if (this != &fdf)
		{
			m_im.Clone(fdf.m_im);
			m_frame_num         = fdf.m_frame_num;
			m_detections        = fdf.m_detections;
			m_facesLandmarkSets = fdf.m_facesLandmarkSets;
			m_facesImages       = fdf.m_facesImages;
		}
		return (*this);
	}

	FFVideo_Image															m_im;
	int32_t																		m_frame_num;
	std::vector<dlib::rectangle>							m_detections;
	std::vector<dlib::full_object_detection>  m_facesLandmarkSets;
	std::vector<FFVideo_Image>                m_facesImages;
};

class RenderCanvas;

//------------------------------------------------------------------------------
class FaceDetectionThreadMgr
{
public:
	FaceDetectionThreadMgr(TheApp* app) : mp_app(app), 
	  mp_frameProcessingThread(NULL), m_stop_frame_processing_loop(false), 
		m_frame_processing_loop_ended(false), mp_frame_cb(NULL), mp_frame_object(NULL),
		mp_faceDetector(NULL), m_face_detection_scale(0.25f), 
		m_faceDetectorInitialized(false), m_faceDetectorEnabled(false),
		m_faceFeaturesEnabled(false), m_faceImagesEnabled(false), m_faceImagesStandardized(false) {};

	// 2nd required for for thread constructor
	FaceDetectionThreadMgr(const FFVideo_FrameExporter& obj) {}

	//////////////////////////////////////////////////////////////////////////////////////
	// class sub-thread function that spins waiting for frames to find faces within:
	void FrameProcessingLoop(void);
	//
	// thread variables:
	std::thread*				mp_frameProcessingThread;
	//
	std::atomic<bool>		m_stop_frame_processing_loop;
	std::atomic<bool>		m_frame_processing_loop_ended;

	//////////////////////////////////////////////////////////////////////////////////////
	~FaceDetectionThreadMgr()
	{
		StopFaceDetectionThread();
		std::queue<FaceDetectionFrame> empty;
		std::swap(m_frameQue, empty);
		if (mp_faceDetector)
		{
			delete mp_faceDetector;
			mp_faceDetector = NULL;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	bool IsRunning(void)
	{
		if (!mp_frameProcessingThread)
			return false;

		// set to true entering Process thread, goes false when exiting Process:
		bool ret = !m_frame_processing_loop_ended;

		return ret;
	}

	//////////////////////////////////////////////////////////////////////////////////////
	void StartFaceDetectionThread(void)
	{
		if (!IsRunning())
		{
			mp_frameProcessingThread = new std::thread(&FaceDetectionThreadMgr::FrameProcessingLoop, this);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	void StopFaceDetectionThread(void)
	{
		// only if the ExportProcessLoop() is running:
		if (IsRunning())
		{
			// tell FrameProcessingLoop() (running in it's own thread) to exit:
			m_stop_frame_processing_loop = true;
			//
			uint32_t spins = 0;
			while (!m_frame_processing_loop_ended)
			{
				using namespace std::chrono_literals;
				std::this_thread::sleep_for(200ms);
				//
				spins++;
			}
			mp_frameProcessingThread->join();
			delete mp_frameProcessingThread;
			mp_frameProcessingThread = NULL;
			m_stop_frame_processing_loop = false; // reset for next use
			m_frame_processing_loop_ended = false;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// adds to queue
	static void Add(void* object, FFVideo_Image& im, int32_t frame_num);
	void Add(FFVideo_Image& im, int32_t frame_num);
	

	//////////////////////////////////////////////////////////////////////////////////////
	size_t Size(void) {	
		std::shared_lock<std::shared_mutex> rlock(m_queue_lock);
		size_t work_to_do = m_frameQue.size();
		rlock.unlock();
		return work_to_do;
	}

	//////////////////////////////////////////////////////////////////////////////////////
	void SetFaceDetectionScale(float detection_scale) {	
		m_face_detection_scale = detection_scale;
	}


	//////////////////////////////////////////////////////////////////////////////////////
	float GetFaceDetectionScale(void) {	
		return m_face_detection_scale;
	}


	TheApp*																		mp_app;

	// the "frame callback", called with every frame: 
	typedef void(*FRAME_FACE_DETECTION_CALLBACK_CB)(void* p_object, FaceDetectionFrame& fdf);
	//
	FRAME_FACE_DETECTION_CALLBACK_CB				  mp_frame_cb;
	void*																			mp_frame_object;

	FaceDetector*															mp_faceDetector;
	float																			m_face_detection_scale;
	bool																			m_faceDetectorInitialized;
	bool																			m_faceDetectorEnabled;
	bool																			m_faceFeaturesEnabled;
	bool																			m_faceImagesEnabled;
	bool																			m_faceImagesStandardized;

	mutable std::shared_mutex									m_queue_lock;
	std::queue<FaceDetectionFrame>						m_frameQue;

	std::string																m_err;
};


#endif // _FACEDETECT_H_

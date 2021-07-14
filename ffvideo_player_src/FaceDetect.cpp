///////////////////////////////////////////////////////////////////////////////
// Name:        faceDetect.cpp
// Author:			Blake Senftner
///////////////////////////////////////////////////////////////////////////////


#include "ffvideo_player_app.h"


using namespace dlib;

////////////////////////////////////////////////////////////////////////
FaceDetector::FaceDetector( TheApp* p_app )
{
	m_detector = get_frontal_face_detector();
	//
	std::string model_path = p_app->m_data_dir + "\\shape_predictor_68_face_landmarks.dat";
	//
	// deserialize("c:\\\\dev\\shape_predictor_68_face_landmarks.dat") >> m_sp;
	deserialize( model_path.c_str() ) >> m_sp;

	m_image_set = false;
}


////////////////////////////////////////////////////////////////////////
FaceDetector::~FaceDetector()
{
}

////////////////////////////////////////////////////////////////////////
void FaceDetector::SetImage( FFVideo_Image& im )
{
	if (m_dlib_im.nc() != im.m_width || m_dlib_im.nr() != im.m_height)
	{
		m_dlib_im.set_size( im.m_height, im.m_width );

		// see comment below:
		float shrink_factor = 0.4f;
		//
		m_dlib_real_im.set_size( ((int32_t)(float)im.m_height * shrink_factor + 0.5f), ((int32_t)(float)im.m_width * shrink_factor + 0.5f) );
	}

	for (uint32_t y = 0; y < im.m_height - 1; y++)
	{
		uint32_t iy = im.m_height - y - 1;

		for (uint32_t x = 0; x < im.m_width - 1; x++)
			memcpy((void*)(&m_dlib_im[iy][x]), im.Pixel(x, y), sizeof(uint8_t) * 3); 
	} 

	// because face detection and face feature recovery take time,
	// we're going to actually work with a significantly smaller
	// image for our detections with dlib:
	dlib::resize_image( m_dlib_im, m_dlib_real_im );

	m_image_set = true;
}


////////////////////////////////////////////////////////////////////////
bool FaceDetector::GetFaceBoxes( std::vector<rectangle>& detections )
{
	if (m_image_set)
	{
		detections = m_detector(m_dlib_real_im);
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////
bool FaceDetector::GetFaceLandmarkSets( std::vector<rectangle>& detections, 
																				std::vector<full_object_detection>& faceLandmarkSets )
{
	if (m_image_set)
	{
		faceLandmarkSets.clear();
		for (size_t i = 0; i < detections.size(); i++)
		{
			full_object_detection oneFace_landmarkSet = m_sp( m_dlib_real_im, detections[i] );
			faceLandmarkSets.push_back(oneFace_landmarkSet);
		}

		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////
void FaceDetector::GetLandmarks( dlib::full_object_detection& oneFace_landmarkSet,
																 std::vector<FF_Vector2D>& jawline,
																 std::vector<FF_Vector2D>& rtBrow,
																 std::vector<FF_Vector2D>& ltBrow,
																 std::vector<FF_Vector2D>& noseBridge,
																 std::vector<FF_Vector2D>& noseBottom,
																 std::vector<FF_Vector2D>& rtEye,
																 std::vector<FF_Vector2D>& ltEye,
																 std::vector<FF_Vector2D>& outsideLips,
																 std::vector<FF_Vector2D>& insideLips )
	{
		jawline.clear();
		rtBrow.clear();
		ltBrow.clear();
		noseBridge.clear();
		noseBottom.clear();
		rtEye.clear();
		ltEye.clear();
		outsideLips.clear();
		insideLips.clear();
		size_t limit = oneFace_landmarkSet.num_parts();
		for (size_t i = 0; i < limit; i++)
		{
			FF_Vector2D vec( oneFace_landmarkSet.part(i).x(), oneFace_landmarkSet.part(i).y() );

			if (i < 17)
				jawline.push_back(vec);
			else if (i < 22)
				rtBrow.push_back(vec);
			else if (i < 27)
				ltBrow.push_back(vec);
			else if (i < 31)
				noseBridge.push_back(vec);
			else if (i < 36)
				noseBottom.push_back(vec);
			else if (i < 42)
				rtEye.push_back(vec);
			else if (i < 48)
				ltEye.push_back(vec);
			else if (i < 60)
				outsideLips.push_back(vec);
			else 
				insideLips.push_back(vec);
		}
	}

////////////////////////////////////////////////////////////////////////
void FaceDetectionThreadMgr::Add(void* p_object, FFVideo_Image& im, int32_t frame_num)
{
	if (p_object)
	{
		((FaceDetectionThreadMgr*)p_object)->Add(im, frame_num);
	}
}

////////////////////////////////////////////////////////////////////////
void FaceDetectionThreadMgr::Add(FFVideo_Image& im, int32_t frame_num)
{
	if (Size() < 3)
	{
		FaceDetectionFrame fdf;
		fdf.m_im.Clone(im);
		fdf.m_frame_num = frame_num;

		std::unique_lock<std::shared_mutex> lock(m_queue_lock);
		m_frameQue.push(fdf);
		lock.unlock();
	}
}

////////////////////////////////////////////////////////////////////////
void FaceDetectionThreadMgr::FrameProcessingLoop(void)
{
	// we are inside the thread now, possibly not for the first time...
	// check if the face detector has been allocated (meaning this is our first use)
	//
	bool good(true); // becomes false when bad occurs
	//
	if (!m_faceDetectorInitialized)
	{
		mp_faceDetector = new FaceDetector(mp_app);
		if (mp_faceDetector)
		{
			m_faceDetectorInitialized = true;
		}
		else
		{
			good = false;
			m_err = "failed to allocate FaceDetector!";
		}
	}

	uint64_t milliseconds = 1000 / 120; // the demoninator is how many times per second we will loop

	uint64_t nanosleep_param = milliseconds * 10; // each ms is 10 nanosleep units

	while (good)
	{
		if (m_stop_frame_processing_loop)
		{
			break;
		}
		else
		{
			std::shared_lock<std::shared_mutex> rlock(m_queue_lock);
			bool work_to_do = !m_frameQue.empty();
			rlock.unlock();

			while (work_to_do)
			{
				std::unique_lock<std::shared_mutex> lock(m_queue_lock);
				FaceDetectionFrame fdf = m_frameQue.front();
				m_frameQue.pop();
				work_to_do = !m_frameQue.empty();
				lock.unlock();

				// do the work of this thread:
				if (m_faceDetectorEnabled)
				{
					mp_faceDetector->SetImage( fdf.m_im );

					// this work is time consuming, so check if we're supposed to quit: 
					if (m_stop_frame_processing_loop)
						break;

					mp_faceDetector->GetFaceBoxes( fdf.m_detections );

					// this work is time consuming, so check if we're supposed to quit: 
					if (m_stop_frame_processing_loop)
						break;

					if (m_faceFeaturesEnabled)
					{
						mp_faceDetector->GetFaceLandmarkSets( fdf.m_detections, fdf.m_facesLandmarkSets );
					}
				}
						
				// send results to the client: 
				if (mp_frame_cb)
				{
					(mp_frame_cb)(mp_frame_object, fdf);
				}
			}

			good = !m_stop_frame_processing_loop;
		}
		nanosleep(nanosleep_param);
	}

	m_frame_processing_loop_ended = true;
}
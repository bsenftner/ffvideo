///////////////////////////////////////////////////////////////////////////////
// Name:        faceDetect.cpp
// Author:			Blake Senftner
///////////////////////////////////////////////////////////////////////////////


#include "ffvideo_player_app.h"


// not actually using OpenCV yet...
// #include <opencv2/core/core.hpp>
// #define SIMD_OPENCV_ENABLE
#include "Simd/SimdLib.hpp"


using namespace dlib;

////////////////////////////////////////////////////////////////////////
FaceDetector::FaceDetector( TheApp* p_app, FACE_MODEL face_model )
{
	m_detector = get_frontal_face_detector();
	//
	m_face_model = face_model;
	//
	std::string model_path;
	if (face_model == FACE_MODEL::sixtyeight)
	{
	  model_path = p_app->m_data_dir + "\\shape_predictor_68_face_landmarks.dat";
	}
	else if (face_model == FACE_MODEL::eightyone)
	{
		model_path = p_app->m_data_dir + "\\shape_predictor_81_face_landmarks-master\\shape_predictor_81_face_landmarks.dat";
	}
	else assert(0);

	deserialize( model_path.c_str() ) >> m_sp;

	m_detect_scale = 0.25f;

	m_image_set = false;
}


////////////////////////////////////////////////////////////////////////
FaceDetector::~FaceDetector() {}

////////////////////////////////////////////////////////////////////////
void FaceDetector::SetImage( FFVideo_Image& im, float detection_scale )
{
  // check if our class instance m_dlib_im is not the same w,h as the passed image:
	if (m_dlib_im.nc() != im.m_width || m_dlib_im.nr() != im.m_height)
	{
	  // reallocate m_dlib_im to same size as passed image:
		m_dlib_im.set_size( im.m_height, im.m_width );
	}

	if (m_detect_scale != detection_scale)
	{
		m_detect_scale = detection_scale;

		// set size of reduced scale version used for faster processing:
		m_dlib_real_im.set_size( ((int32_t)(float)im.m_height * m_detect_scale + 0.5f), ((int32_t)(float)im.m_width * m_detect_scale + 0.5f) );
	}

	// remember in this format, incase we're asked for the face rects later:
	m_im.Clone( im );

	FFVideo_Image im_flip( im );
	im_flip.MirrorVertical();

	int32_t rgba_stride = sizeof(uint8_t) * 4 * im_flip.m_width;
	int32_t rgb_stride  = sizeof(uint8_t) * 3 * im_flip.m_width;

	int32_t grey_stride  = sizeof(uint8_t) * im_flip.m_width;

	// copy the RGBA FFvideo_Image format image to m_dlib_im, a greyscale format image:
	SimdBgraToGray( im_flip.Pixel(0, 0), im_flip.m_width, im_flip.m_height, rgba_stride, (uint8_t*)&m_dlib_im[0][0], grey_stride );

	if (m_detect_scale != 1.0f)
	{
		dlib::array2d<unsigned char> img_flip;
		dlib::flip_image_up_down(m_dlib_im, img_flip);

		// because face detection and face feature recovery take time,
		// we're going to actually work with an end-user set scaling of 
		// the image for our detections with dlib:
		//
		dlib::interpolate_nearest_neighbor interp_type; // slightly faster? 
		//
		dlib::resize_image( m_dlib_im, m_dlib_real_im, interp_type );
		//
		// boo! not faster:
		// SimdResizeBilinear( (uint8_t*)&m_dlib_im[0][0], im_flip.m_width, im_flip.m_height, rgb_stride, 
		// 										 (uint8_t*)&m_dlib_real_im[0][0], m_dlib_real_im.nc(), m_dlib_real_im.nr(), rgb_stride, 3 );
	}
	else
	{
		dlib::flip_image_up_down(m_dlib_im, m_dlib_real_im);
	}

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
void FaceDetector::GetFaceImages( std::vector<rectangle>& detections, 
																	std::vector<full_object_detection>& faceLandmarkSets,
																	std::vector<FFVideo_Image>& face_images, 
																	bool full_head_flag )
{
	if (m_image_set)
	{
		face_images.clear();
 
		if (full_head_flag)
		{
			// gives us the details for the entire vector of faceLandMarks:
			std::vector<dlib::chip_details> dets;
			
			if (m_face_model == FACE_MODEL::sixtyeight)
			   dets = get_face_chip_details(faceLandmarkSets, 128, 0.8 );
			else
			{ 
				// get_face_chip_details() requires the 68 landmark face model, 
				// which is the same as FACE_MODEL::eightyone with the added points removed:
				//
				std::vector<full_object_detection> clipped_faceLandmarkSets;
				for (size_t i = 0; i < faceLandmarkSets.size(); i++)
				{
					clipped_faceLandmarkSets.push_back( faceLandmarkSets[i] );

					full_object_detection& r_lmSet = clipped_faceLandmarkSets.back();
					r_lmSet.parts.resize(68);
				}
				dets = get_face_chip_details(clipped_faceLandmarkSets, 128, 0.8 );
			}

			// storage for an array of face images in dlib format:
			dlib::array<array2d<unsigned char>> face_chips;

			// generates all the sub-images in dlib image format:
			extract_image_chips(m_dlib_real_im, dets, face_chips);

			// convert to FFVideo_Image format:
			for (size_t i = 0; i < face_chips.size(); i++)
			{
			  // get reference to face sub-image:
				array2d<unsigned char>& face_chip = face_chips[i];

				// we want it flipped in our format:
				dlib::array2d<unsigned char> chip_flip;
				dlib::flip_image_up_down(face_chip, chip_flip);

				// create new FFVideo_Image in vector and get reference:
				face_images.push_back( FFVideo_Image() );
				FFVideo_Image& im = face_images.back();

				// set dimensions:
				im.Reallocate( chip_flip.nr(), chip_flip.nc() );
				
				// calc bytes per row and copy the face sub-images to FFVideo_Images:
				int32_t grey_stride  = sizeof(uint8_t) * im.m_width;
				int32_t rgba_stride  = sizeof(uint8_t) * 4 * im.m_width;
				//
				SimdGrayToBgra( &chip_flip[0][0], im.m_width, im.m_height, grey_stride, im.Pixel(0,0), rgba_stride, 255 );
			}

		}
		else // this version clips the detected face rects as new images directly from the video frame:
		{
			for (size_t i = 0; i < detections.size(); i++)
			{
				// make copy of video frame:
				face_images.push_back(m_im);

				// get a reference to the copy:
				FFVideo_Image& im = face_images.back();

				// get a face detection rect (note, in a scaled space!):
				rectangle& oneFaceRect = detections[i];

				float left  = (float)oneFaceRect.left() / (float)m_dlib_real_im.nc();			// normalized
				float right = (float)oneFaceRect.right() / (float)m_dlib_real_im.nc(); 
				//
				float bottom = 1.0f - (float)oneFaceRect.bottom() / (float)m_dlib_real_im.nr(); // normalized and y flipped
				float top    = 1.0f - (float)oneFaceRect.top() / (float)m_dlib_real_im.nr();

				// clip our in-vector image to the face rect of that detection:
				im.ClipToRect( (uint32_t)(left*im.m_width), (uint32_t)(bottom*im.m_height), (uint32_t)(right*im.m_width), (uint32_t)(top*im.m_height) );
			}
		}
	}
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
																 std::vector<FF_Vector2D>& insideLips,
																 std::vector<FF_Vector2D>& forehead )
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
		//
		forehead.clear();
		std::vector<FF_Vector2D> forehead_tmp;

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
			else if (i < 68)
				insideLips.push_back(vec);
			else forehead_tmp.push_back(vec);
		}

		if (forehead_tmp.size() > 0)
		{
			assert(m_face_model == FACE_MODEL::eightyone);

			// the forehead points are out of sequential order, fix that:

			forehead.push_back( forehead_tmp[9] );
			forehead.push_back( forehead_tmp[7] );
			forehead.push_back( forehead_tmp[8] );
			forehead.push_back( forehead_tmp[0] );
			forehead.push_back( forehead_tmp[1] );
			forehead.push_back( forehead_tmp[2] );
			forehead.push_back( forehead_tmp[3] );
			forehead.push_back( forehead_tmp[12] );
			forehead.push_back( forehead_tmp[4] );
			forehead.push_back( forehead_tmp[5] );
			forehead.push_back( forehead_tmp[11] );
			forehead.push_back( forehead_tmp[6] );
			forehead.push_back( forehead_tmp[10] );
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
  // if we are not keeping up, meaning as this is called and images are added to m_frameQue,
	// if we are not popping them off equally as fast, we allow frame loss to maintain realtime:
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
		mp_faceDetector = new FaceDetector(mp_app, m_face_model);
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
					mp_faceDetector->SetImage( fdf.m_im, m_face_detection_scale );

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

					// this work is time consuming, so check if we're supposed to quit: 
					if (m_stop_frame_processing_loop)
						break;

					if (m_faceImagesEnabled && m_faceFeaturesEnabled)
					{
						mp_faceDetector->GetFaceImages( fdf.m_detections, fdf.m_facesLandmarkSets, fdf.m_facesImages, m_faceImagesStandardized );
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
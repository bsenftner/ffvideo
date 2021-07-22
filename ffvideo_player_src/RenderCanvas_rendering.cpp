///////////////////////////////////////////////////////////////////////////////
// Name:        RenderCanvas_rendering.cpp
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

#if !wxUSE_GLCANVAS
#error "OpenGL required: set wxUSE_GLCANVAS to 1 and rebuild the library"
#endif

#include "ffvideo_player_app.h"



/////////////////////////////////////////////////////////////////////////////
void RenderCanvas::CalcVideoPlacement(void)
{
	// if window is taller than wide:
	if (m_winSize.x < m_winSize.y)
	{
		// fit image width to fill the window

		// set zoom to windowWidth / imageWidth
		m_zoom = (float)m_winSize.x / (float)m_frame.m_width;
		m_trans.x = 0.0f;

		// y placement is window height less half scaled image height
		// basically centering the image vertically
		m_trans.y = ((float)m_winSize.y - (m_zoom * (float)m_frame.m_height)) / 2.0f;
	}
	else // window is wider than tall
	{
		// fit image height to fill the window

		// set zoom to window height / image height
		m_zoom = (float)m_winSize.y / (float)m_frame.m_height;
		m_trans.y = 0.0f;

		// x placement is window width less half the scaled width
		// basically centering the image horizontally
		// m_trans.x = ((float)m_winSize.x - (m_zoom * (float)m_frame.m_width)) / 2.0f;

		m_trans.x = ((float)m_winSize.x - (m_zoom * (float)m_frame.m_width)) - 1;
	}
}

//////////////////////////////////////////////////////////////////////
void RenderCanvas::RenderGradientBg(float minx, float miny, float maxx, float maxy)
{
	glBegin(GL_TRIANGLES);

	FF_Vector3D dark_colors[16] =
	{ {0.0f, 0.0f, 0.0f},{0.0f, 0.0f, 0.0f},{0.0f, 0.0f, 0.0f},{0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f},{0.0f, 0.0f, 0.0f},{0.0f, 0.0f, 0.0f},{0.0f, 0.0f, 0.0f},
		{0.5f, 0.5f, 0.5f},{0.5f, 0.5f, 0.5f},{0.5f, 0.5f, 0.5f},{0.5f, 0.5f, 0.5f},
		{1.0f, 1.0f, 1.0f},{1.0f, 1.0f, 1.0f},{1.0f, 1.0f, 1.0f},{1.0f, 1.0f, 1.0f}
	}; 
	FF_Vector3D light_colors[16] =
	{ {1.0f, 1.0f, 1.0f},{1.0f, 1.0f, 1.0f},{1.0f, 1.0f, 1.0f},{1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},{1.0f, 1.0f, 1.0f},{1.0f, 1.0f, 1.0f},{1.0f, 1.0f, 1.0f},
		{0.8f, 0.8f, 0.9f},{0.8f, 0.8f, 0.9f},{0.8f, 0.8f, 0.9f},{0.8f, 0.8f, 0.9f},
		{0.9f, 0.9f, 1.0f},{0.9f, 0.9f, 1.0f},{0.9f, 0.9f, 1.0f},{0.9f, 0.9f, 1.0f}
	}; 

	FF_Vector3D* colors = (m_theme) ? light_colors : dark_colors;

	float third1x = (float)m_winSize.x * 0.33f;
	float third2x = (float)m_winSize.x * 0.66f;
	float third3x = (float)m_winSize.x;

	float third1y = (float)m_winSize.y * 0.33f;
	float third2y = (float)m_winSize.y * 0.66f;
	float third3y = (float)m_winSize.y;

	FF_Vector3D verts[16] =
	{ {0, third3y, 0},{third1x, third3y, 0},{third2x, third3y, 0},{third3x, third3y, 0},
		{0, third2y, 0},{third1x, third2y, 0},{third2x, third2y, 0},{third3x, third2y, 0},
		{0, third1y, 0},{third1x, third1y, 0},{third2x, third1y, 0},{third3x, third1y, 0},
		{0,       0, 0},{third1x,       0, 0},{third2x,       0, 0},{third3x,       0, 0}
	};

	int rorder[54] = { 
		4, 0, 1, 1, 5, 4,
		5, 1, 2, 2, 6, 5,
		6, 2, 3, 3, 7, 6,
		8, 4, 5, 5, 9, 8,
		9, 5, 6, 6, 10, 9,
		10, 6, 7, 7, 11, 10,
		12, 8, 9, 9, 13, 12,
		13, 9, 10, 10, 14, 13,
		14, 10, 11, 11, 15, 14

	};

	for (int32_t j = 0; j < 54; j++)
	{
		int32_t i = rorder[j];
		glColor3f(colors[i].x, colors[i].y, colors[i].z);
		glVertex3f(verts[i].x, verts[i].y, verts[i].z);
	}

	glEnd();
}

/////////////////////////////////////////////////////////////////////////////
// Writes text at posn (x,y) origin bot left
void RenderCanvas::DrawString(const std::string& s, float x, float y, const FF_Vector3D& rgb)
{
	glColor3f(rgb.x, rgb.y, rgb.z);
	glRasterPos2f(x, y);
	glListBase(1000);
	glCallLists(strlen(s.c_str()), GL_UNSIGNED_BYTE, s.c_str());
}

/////////////////////////////////////////////////////////////////////////////
void RenderCanvas::DrawLines(std::vector<FF_Vector2D>& pts, float z, float f, FF_Vector2D& scale, bool closed )
{
	if (closed)
		glBegin(GL_LINE_LOOP);
	else glBegin(GL_LINES);

	size_t limit = pts.size();
	for (size_t i = 0; i < limit; i++)
	{
		float x =      pts[i].x * scale.x + m_trans.x;
		float y = f - (pts[i].y * scale.y + m_trans.y);

		glVertex3f( x, y, z );

		if (!closed && (i > 0))
			glVertex3f( x, y, z );
	}
	glEnd();
}

/////////////////////////////////////////////////////////////////////////////
void RenderCanvas::DrawFaces(void)
{
	if (m_status != VIDEO_STATUS::RECEIVING_FRAMES)
		return;

	if (m_detections.size() > 0)
	{
		// m_frames's m_width,m_height is the size of the display image
		// m_detectImSize.x, m_detectImSize.y is the size of the image face detection occured
		// which define the units used by the face detection results. 
		// The face detection results are scaled to to frame's dimensions for ease of use.

		// this is the display size of the video frame:
		FF_Vector2D im_size( (float)m_frame.m_width * m_zoom, (float)m_frame.m_height * m_zoom );

		// std::string msg = mp_app->FormatStr( "also have %d face images", (int32_t)m_facesImages.size() );
		// SendOverlayNoticeEvent( msg, 40 );

		FF_Vector2D landmark_scale( im_size.x / m_detectImSize.x, im_size.y / m_detectImSize.y );

		if (IsFaceLandmarksEnabled())
		{
			if (m_facesLandmarkSets.size() > 0)
			{
				std::vector<FF_Vector2D> jawline;
				std::vector<FF_Vector2D> rtBrow;
				std::vector<FF_Vector2D> ltBrow;
				std::vector<FF_Vector2D> noseBridge;
				std::vector<FF_Vector2D> noseBottom;
				std::vector<FF_Vector2D> rtEye;
				std::vector<FF_Vector2D> ltEye;
				std::vector<FF_Vector2D> outsideLips;
				std::vector<FF_Vector2D> insideLips;

				for (size_t s = 0; s < m_facesLandmarkSets.size(); s++)
				{
					dlib::full_object_detection& oneFace_landmarkSet = m_facesLandmarkSets[s];

					m_faceDetectMgr.mp_faceDetector->GetLandmarks( oneFace_landmarkSet, jawline, rtBrow, ltBrow, 
																												 noseBridge, noseBottom, rtEye, ltEye, outsideLips, insideLips );

					glColor3ub( 0, 255, 0 );
					glLineWidth( 2.0f );
					//
					DrawLines( jawline,     0.1f, im_size.y, landmark_scale, false );
					DrawLines( rtBrow,      0.1f, im_size.y, landmark_scale, false );
					DrawLines( ltBrow,      0.1f, im_size.y, landmark_scale, false );
					DrawLines( noseBridge,  0.1f, im_size.y, landmark_scale, false );
					DrawLines( noseBottom,  0.1f, im_size.y, landmark_scale, false );
					DrawLines( rtEye,       0.1f, im_size.y, landmark_scale, true );
					DrawLines( ltEye,       0.1f, im_size.y, landmark_scale, true );
					DrawLines( outsideLips, 0.1f, im_size.y, landmark_scale, true );
					DrawLines( insideLips,  0.1f, im_size.y, landmark_scale, true );

					glLineWidth( 1.0f );
				}
			}
		}
		else for (int32_t i = 0; i < m_detections.size(); i++)
		{
		  // faceLandmarks is not enabled, so we'll render wireframe rects over the faces:
			dlib::rectangle& rect = m_detections[i];
			float left   = (float)rect.left();
			float top    = (float)rect.top();
			float right  = (float)rect.right();
			float bottom = (float)rect.bottom();

			/*
			FF_Vector2D detect_rect_size( right - left, bottom - top ); 
			std::string msg = mp_app->FormatStr( "head rect %d %1.2f x %1.2f", i, detect_rect_size.x, detect_rect_size.y );
			SendOverlayNoticeEvent( msg, 40 ); /* */

			left   *= landmark_scale.x; left   += m_trans.x;
			right  *= landmark_scale.x; right  += m_trans.x;
			top    *= landmark_scale.y; top    += m_trans.y;
			bottom *= landmark_scale.y; bottom += m_trans.y;

			top    = im_size.y - top;
			bottom = im_size.y - bottom;

			glColor3ub( 255, 0, 0 );
			glLineWidth( 2.0f );

			glBegin(GL_LINE_LOOP);
			glVertex3f( left,  bottom, 0.1f );
			glVertex3f( right, bottom, 0.1f );
			glVertex3f( right, top,    0.1f );
			glVertex3f( left,  top,    0.1f );
			glEnd();

			glLineWidth( 1.0f );
		}

		if (IsFaceImagesEnabled())
		{
			// showing only the tallest one for the moment: 
			int32_t tallest = -1;
			for (size_t i = 0; i < m_facesImages.size(); i++)
			{
				if (i == 0)
				   tallest = 0;
				else if (m_facesImages[i].m_height > m_facesImages[tallest].m_height)
				   tallest = i;
			}
			if (tallest > -1)
			{
				unsigned char* pix = m_facesImages[tallest].mp_pixels;
				int32_t				 w   = m_facesImages[tallest].m_width;
				int32_t				 h   = m_facesImages[tallest].m_height;

				glRasterPos2f(5.0f, 5.0f);
				glPixelZoom(m_zoom, m_zoom);
				// glPixelZoom(1.0f, 1.0f);

				glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)pix);
			}
		}

		// debug logic, turn off after one frame:
		// EnableFaceDetection(false);
	}
}

////////////////////////////////////////////////////////////////////////
void RenderCanvas::OnPaint(wxPaintEvent& WXUNUSED(event))
{
	// This is required even though dc is not used otherwise.
	wxPaintDC dc(this);

	m_winSize = GetClientSize();
	mp_videoWindow->SyncWindowSizeToConfig(m_winSize.x, m_winSize.y);

	m_frame_lock.lock();

	// This is necessary if there is more than one wxGLCanvas
	// or more than one wxGLContext in the application.
	if (!SetCurrent(*mp_glRC))
	{
		m_frame_lock.unlock();
		return;
	}

	// setup a simple orthographic view the size of the window:
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// sets up orthographic framebuffer:
	glViewport(0, 0, m_winSize.x, m_winSize.y);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0, (GLfloat)m_winSize.x, 0.0, (GLfloat)m_winSize.y);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Render the graphics and swap the buffers.
	m_text.clear();

	// earlier reference to the current video stream configuration
	VideoStreamConfig* vsc = mp_videoWindow->mp_streamConfig;

	std::string scratch;

	scratch = mp_videoWindow->mp_app->FormatStr("Name: %s", vsc->m_name.c_str());
	m_text.push_back(scratch);


	scratch = mp_videoWindow->mp_app->FormatStr("video frame scaling %2.2f", m_zoom);
	m_text.push_back(scratch);

	/*
	scratch = mp_videoWindow->mp_app->FormatStr("canvas size %d x %d", m_winSize.x, m_winSize.y);
	m_text.push_back(scratch);

	scratch = mp_videoWindow->mp_app->FormatStr("Mouse pos %d x %d", m_mpos.x, m_mpos.y);
	m_text.push_back(scratch);

	scratch = mp_videoWindow->mp_app->FormatStr("Mouse rel pos %d x %d", m_rel_mpos.x, m_rel_mpos.y);
	m_text.push_back(scratch);

	scratch = mp_videoWindow->mp_app->FormatStr("Mouse norm pos %1.2f x %1.2f", m_npos.x, m_npos.y);
	m_text.push_back(scratch);
	*/

	scratch = GetStatus();			// the video status
	m_text.push_back(scratch);


	if (vsc->m_frame_interval)
	{
		scratch = mp_videoWindow->mp_app->FormatStr("Frame interval: %d", vsc->m_frame_interval );
	}
	else
	{
		scratch = mp_videoWindow->mp_app->FormatStr("Frame interval profile: %s", vsc->m_frame_interval_profile.c_str() );
	}
	m_text.push_back(scratch);


	bool boinker = ((int32_t)mp_app->m_clock.s() % 2);
	//
	if (vsc->m_export_interval > 0)
	{
		int32_t encode_interval = vsc->m_encode_interval.load(std::memory_order_relaxed);
		if (encode_interval)
		{
			if (boinker)
				scratch = mp_videoWindow->mp_app->FormatStr("Frame ENCODING with interval: %d", encode_interval ); 
			else scratch = mp_videoWindow->mp_app->FormatStr("Frame Encoding with interval: %d", encode_interval );
		}
		else
		{
			if (boinker)
				scratch = mp_videoWindow->mp_app->FormatStr("Frame EXPORTING with interval: %d", vsc->m_export_interval );
			else scratch = mp_videoWindow->mp_app->FormatStr("Frame Exporting with interval: %d", vsc->m_export_interval );
		}
		m_text.push_back(scratch);
	}
	else m_text.push_back( "No frame exporting or encoding" );


	if (m_is_playing && m_frame_loaded)
	{
		scratch = mp_videoWindow->mp_app->FormatStr("delivered video size %d x %d", m_frame.m_width, m_frame.m_height);
		m_text.push_back(scratch);

		if (vsc->m_type == STREAM_TYPE::FILE && m_expected_frames > 0)
		{
			scratch = mp_videoWindow->mp_app->FormatStr("frame %d of %d @ %1.1f fps", m_current_frame_num % m_expected_frames, m_expected_frames, mp_ffvideo->GetReceivedFPS());
			m_text.push_back(scratch);
		}
		else
		{
			scratch = mp_videoWindow->mp_app->FormatStr("frame %d @ %1.1f fps", m_current_frame_num, mp_ffvideo->GetReceivedFPS());
			m_text.push_back(scratch);
		}
	}

	if (m_ip_restarts)
	{ 
		scratch = mp_videoWindow->mp_app->FormatStr("IP camera auto-replays %d", m_ip_restarts);
		m_text.push_back(scratch);
	}

	if (mp_ffvideo->GetFrameExportingQueueSize() > 0)
	{ 
		scratch = mp_videoWindow->mp_app->FormatStr("Frame exporting queue length %d", mp_ffvideo->GetFrameExportingQueueSize() );
		m_text.push_back(scratch);
	}




	if (m_detections.size() > 0)
	{
		if (IsFaceImagesEnabled())
		{
			// showing only the tallest one for the moment: 
			int32_t tallest = -1;
			for (size_t i = 0; i < m_facesImages.size(); i++)
			{
				if (i == 0)
					tallest = 0;
				else if (m_facesImages[i].m_height > m_facesImages[tallest].m_height)
					tallest = i;
			}
			if (tallest > -1)
			{
				int32_t left   = (int32_t)m_detections[tallest].left();
				int32_t top    = (int32_t)m_detections[tallest].top();
				int32_t right  = (int32_t)m_detections[tallest].right();
				int32_t bottom = (int32_t)m_detections[tallest].bottom();
				
				scratch = mp_videoWindow->mp_app->FormatStr("left %d, bottom %d  right %d, top %d", left, bottom, right, top );
				m_text.push_back(scratch);
			}
		}
	}





	CalcVideoPlacement();	// of the video frame
	Render();							// the video frame first, video ctrls, finall overlay text

	SwapBuffers();
	wglMakeCurrent(NULL, NULL);

	m_frame_lock.unlock();

	// trigger any video buttons activated by user (currently, just button growth seen next render)
	m_videoCtls.HilightTriggerCallback();	

}

/////////////////////////////////////////////////////////////////////////////
void RenderCanvas::Render(void)
{
	glColor3f(1.0f, 1.0f, 1.0f);

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);

	RenderGradientBg(0.0f, 0.0f, (float)m_winSize.x, (float)m_winSize.y);

	// a video or background frame:
	if (m_frame_loaded)
	{
		unsigned char* pix = m_frame.mp_pixels;
		int32_t				 w   = m_frame.m_width;
		int32_t				 h   = m_frame.m_height;

		// for negative translation we need to change the draw start position
		if (m_trans.x < 0.0f)
		{
			glPixelStoref(GL_UNPACK_SKIP_PIXELS, FF_ABS(m_trans.x));
			m_trans.x = 0.0f;
		}
		else glPixelStoref(GL_UNPACK_SKIP_PIXELS, 0.0f);

		if (m_trans.y < 0.0f)
		{
			m_trans.y = 0.0f;
			uint32_t r = (uint32_t)ff_round(ff_fabs(m_trans.y));
			h = m_frame.m_height - r - 1;
			pix = &pix[m_frame.m_width * r * 4];
		}

		glRasterPos2f(m_trans.x, m_trans.y);
		glPixelZoom(m_zoom, m_zoom);

		glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)pix);
	}

	if (m_OGL_font_dirty) 
		GenerateRenderFont( mp_videoWindow->mp_streamConfig );

	DrawFaces();

	m_videoCtls.Render();

	mp_noticeMgr->DeleteExpiredNotices();
	mp_noticeMgr->ExportNotices(m_text);


	// the video window's overlay text
	FF_Vector3D shdwRGB(0.3f, 0.3f, 0.3f);
	FF_Vector3D textRGB = mp_videoWindow->mp_streamConfig->m_font_color;
	if (m_text.size() > 0)
	{
		for (size_t i = 0; i < m_text.size(); i++)
		{
			float x = m_line_height; 

			float y = (float)m_winSize.y - 50.0f - ((float)i * m_line_height);

			x += (float)m_text_pos.x;
			y += (float)m_text_pos.y;

			DrawString(m_text[i], x+1.0f, y-1.0f, shdwRGB);
			DrawString(m_text[i], x, y, textRGB);
		}
	}

	glFlush();
}







#include "ffvideo_player_app.h"


//////////////////////////////////////////////////////////////////////////////////////
GLButton::GLButton(void)
{
	m_state = GLButtonState::uninit;
	m_pos.Set(0.0f, 0.0f, 0.0f);
	m_scale = 1.0f;
	mp_canvas = NULL;
	mp_callback = NULL;
	mp_callback_data = NULL;
	m_lastTriggerTime = -1.0f;
}

//////////////////////////////////////////////////////////////////////////////////////
bool GLButton::Load(const char* filePath)
{
	bool ret = m_texture.Load(filePath);
	if (ret)
	{
		m_texture.SetAlphaTweak((uint8_t)64);
	}
	return ret;
}

//////////////////////////////////////////////////////////////////////////////////////
void GLButton::GenGLTexParams(void)
{
	// Create one OpenGL texture
	glGenTextures(1, &m_textureID);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, m_textureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_texture.m_width, m_texture.m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_texture.mp_pixels);
}

//////////////////////////////////////////////////////////////////////////////////////
void GLButton::SetPosition(FF_Vector3D& pos)
{
	m_pos = pos;
}

//////////////////////////////////////////////////////////////////////////////////////
void GLButton::SetCanvas(RenderCanvas* canvas)
{
	mp_canvas = canvas;
}

//////////////////////////////////////////////////////////////////////////////////////
void GLButton::SetCallback(GLBUTTON_CALLBACK cb, void* data)
{
	mp_callback = cb;
	mp_callback_data = data;
}

//////////////////////////////////////////////////////////////////////////////////////
void GLButton::RenderSetup(void)
{
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // for semitransparency
	// glBlendFunc(GL_SRC_COLOR, GL_DST_COLOR); //  GL_SRC_ALPHA); // for semitransparency
	// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //  GL_SRC_ALPHA); // for semitransparency
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); //  GL_ONE_MINUS_SRC_COLOR); //  GL_SRC_ALPHA); // for semitransparency

	glEnable(GL_TEXTURE_2D);
}

//////////////////////////////////////////////////////////////////////////////////////
void GLButton::CalcState(void)
{
	if (m_state == GLButtonState::disabled)
		return;

	m_state = GLButtonState::normal;

	if (mp_canvas)
	{
		float mouseX = (float)mp_canvas->m_mpos.x;
		float mouseY = (float)mp_canvas->m_winSize.y - (float)mp_canvas->m_mpos.y;

		if (mouseX > m_pos.x && mouseX < m_pos.x + m_texture.m_width)
		{
			if (mouseY > m_pos.y && mouseY < m_pos.y + m_texture.m_height)
			{
				m_state = GLButtonState::hover;

				if (mp_canvas->m_mouse_left_down)
					 m_state = GLButtonState::hilight;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////
void GLButton::Render(void)
{
	// if (m_state == GLButtonState::disabled)
	//	return;

	// check for mouse being off screen:
	float mouseX = (float)mp_canvas->m_mpos.x;
	float mouseY = (float)mp_canvas->m_winSize.y - (float)mp_canvas->m_mpos.y;
	//
	float win_width = (float)mp_canvas->m_winSize.x;
	float win_height = (float)mp_canvas->m_winSize.y;
	//
	if (mouseX < 0.0f || mouseX >= win_width ||
		  mouseY < 0.0f || mouseY >= win_height)
	{
		return;
	}

	// we're rendering:

	glBindTexture(GL_TEXTURE_2D, m_textureID);

	float width  = (float)m_texture.m_width * m_scale;
	float height = (float)m_texture.m_height * m_scale;

	float thirdwh = win_height * 0.33f;		// one third window height

	float button_top = height + m_pos.y;
	float button_btm = m_pos.y;

	float r, g, b, a, ceiling(0.35f);
	switch (m_state)
	{

	case		GLButtonState::disabled:
		ceiling = 0.1f;
		// intentional fallthru...

	default:
	case		GLButtonState::normal:
		if (mouseY > button_top)
		{
			// above threshold, button is 100% transparent:
			if (mouseY > button_top + thirdwh)
			{
				r = 0.0f;	g = 0.0f;	b = 0.0f;	
			}
			else
			{
				// puts r in range 0.0 to 1.0, based upon vertical within theshold:
				r = (button_top + thirdwh - mouseY) / thirdwh;
				// we want 0.0 to ceiling:
				r *= ceiling;
				g = b = r;
			}
		}
		else if (mouseY < button_btm)
		{
			// remember: mouseY grows with height
			
			// below threshold, button is 100% transparent:
			if (mouseY < button_btm - thirdwh)
			{
				r = 0.0f;	g = 0.0f;	b = 0.0f;
			}
			else
			{
				// puts r in range 0.0 to 1.0, based upon vertical within theshold:
				r = (mouseY - (button_btm - thirdwh)) / thirdwh;
				// we want 0.0 to ceiling:
				r *= ceiling;
				g = b = r;
			}
		}
		else
		{
			r = ceiling;
			g = ceiling;
			b = ceiling;
		}
		break;
	case		GLButtonState::hover:
		r = 1.0f;
		g = 1.0f;
		b = 1.0f;
		break;
	case		GLButtonState::hilight:
		r = 1.0f;
		g = 1.0f;
		b = 1.0f;
		break;
	}
	a = 0.0f;

	glColor4f(r, g, b, a);

	glBegin(GL_QUADS);

	glTexCoord2f(0.0f, 0.0f); glVertex3f( 0.0f + m_pos.x,   0.0f + m_pos.y, m_pos.z);  // Bottom Left Of The Texture and Quad
	glTexCoord2f(1.0f, 0.0f); glVertex3f(width + m_pos.x,   0.0f + m_pos.y, m_pos.z);  // Bottom Right Of The Texture and Quad
	glTexCoord2f(1.0f, 1.0f); glVertex3f(width + m_pos.x, height + m_pos.y, m_pos.z);  // Top Right Of The Texture and Quad
	glTexCoord2f(0.0f, 1.0f); glVertex3f( 0.0f + m_pos.x, height + m_pos.y, m_pos.z);  // Top Left Of The Texture and Quad

	glEnd();
}

//////////////////////////////////////////////////////////////////////////////////////
void GLButton::RenderDone(void)
{
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}

//////////////////////////////////////////////////////////////////////////////////////
bool GLButton::HilightTriggerCallback(void)
{
	if (m_state != GLButtonState::hilight)
		return false;
	if (!mp_callback)
		return false;			// need a callback

	// the app maintains a clock: 
	float now = mp_canvas->mp_videoWindow->mp_app->m_clock.s();

	// do not fire the callback for a period after last time triggered
	float delay_until = m_lastTriggerTime + 0.1f;
	//
	if ((m_lastTriggerTime < 0.0f) || (delay_until < now))
	{
		m_lastTriggerTime = now;
		mp_callback(mp_callback_data, this);
		return true;
	}

	return false;
}


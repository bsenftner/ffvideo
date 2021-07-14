

#include "ffvideo_player_app.h"


////////////////////////////////////////////////////////////////////////
VideoCtrls::VideoCtrls()
{}

////////////////////////////////////////////////////////////////////////
bool VideoCtrls::Initialize(std::string& data_dir, RenderCanvas* canvas)
{
	m_data_dir = data_dir;
	mp_canvas = canvas;

	std::string fnames[5] = { "\\gui\\play_icon.jpg", "\\gui\\pause_icon.jpg", 
														"\\gui\\step_back_icon.jpg", "\\gui\\stop_icon.jpg", 
														"\\gui\\step_forward_icon.jpg" };


	FF_Vector3D position(200.0f, 200.0f, 0.5f);

	bool status(true);
	for (int32_t i = 0; i < 5; i++)
	{
		std::string texturePath = m_data_dir + fnames[i]; 
		if (m_ctrls[i].Load(texturePath.c_str()))
		{
			m_ctrls[i].GenGLTexParams();

			m_ctrls[i].SetPosition(position);

			// move our position over for the next button
			position.x += m_ctrls[i].m_texture.m_width;
		}
	}

	return true;
}


////////////////////////////////////////////////////////////////////////
void VideoCtrls::SetStreamType(STREAM_TYPE sType)
{
	switch (sType)
	{
	case STREAM_TYPE::FILE:
		m_ctrls[0].m_state = GLButtonState::normal; // play
		m_ctrls[1].m_state = GLButtonState::normal; // pause
		m_ctrls[2].m_state = GLButtonState::normal; // step back
		m_ctrls[3].m_state = GLButtonState::normal; // stop
		m_ctrls[4].m_state = GLButtonState::normal; // step forward
		break;
	case STREAM_TYPE::USB:
		m_ctrls[0].m_state = GLButtonState::normal;   // play
		m_ctrls[1].m_state = GLButtonState::disabled; // pause
		m_ctrls[2].m_state = GLButtonState::disabled; // step back
		m_ctrls[3].m_state = GLButtonState::normal;   // stop
		m_ctrls[4].m_state = GLButtonState::disabled; // step forward
		break;
	case STREAM_TYPE::IP:
		m_ctrls[0].m_state = GLButtonState::normal;   // play
		m_ctrls[1].m_state = GLButtonState::disabled; // pause
		m_ctrls[2].m_state = GLButtonState::disabled; // step back
		m_ctrls[3].m_state = GLButtonState::normal;   // stop
		m_ctrls[4].m_state = GLButtonState::disabled; // step forward
		break;
	}
}

////////////////////////////////////////////////////////////////////////
void VideoCtrls::Render(void)
{
	// calc buttons width resetting any scale per button: 
	float win_width    = (float)mp_canvas->m_winSize.x;
  float video_offset = (float)mp_canvas->m_trans.x; 
  float video_width  = win_width - video_offset;
	float win_height   = (float)mp_canvas->m_winSize.y;

	float btns_width(0.0f);
	for (int32_t i = 0; i < 5; i++)
	{
		m_ctrls[i].m_scale = 1.0f;

		btns_width += m_ctrls[i].m_texture.m_width;
	}

	// at this point, give all buttons have same height:
	float btns_height = m_ctrls[0].m_texture.m_height;

	// calc left-most position of 1st button and centered height for buttons
	FF_Vector3D pos((video_width * 0.5f) - (btns_width * 0.5f) + video_offset,
		              win_height * 0.5f - btns_height * 0.5f, 0.5f);

	// give each button it's initial position:
	for (int32_t i = 0; i < 5; i++)
	{
		m_ctrls[i].SetPosition(pos);
		//
		pos.x += (m_ctrls[i].m_texture.m_width * m_ctrls[i].m_scale);
	}

	// run the tweened callbacks for possible button position animation
	m_tweenedCallbackMgr.CallActiveCallbacks();

	// call RenderSetup() on first button only, it turns on OGL blending & textures, turns off depth testing:
	m_ctrls[0].RenderSetup();
	for (int32_t i = 0; i < 5; i++)
	{
		m_ctrls[i].SetCanvas(mp_canvas);	// only needed once, but does not cost to set continually

		m_ctrls[i].CalcState();						// sets button state based on mouse position

		m_ctrls[i].Render();							// if mouse is on-screen, button will render
	}
	m_ctrls[0].RenderDone();						// disables blending, textures and enables depth testing
}

//////////////////////////////////////////////////////////////////////////////////////
// returns true if any hilight callbacks fired
bool VideoCtrls::HilightTriggerCallback(void)
{
	bool status(false);
	for (int32_t i = 0; i < 5; i++)
	{
		if (m_ctrls[i].HilightTriggerCallback())
		{
			status = true;

			// this will cause TweenedCB_growButton() to be called with &m_ctrls[i], a GLButton,
			// with it's 't' parameter varying from 1.0 to 1.0 over a 0.1 second duration. 
			// the frame rendering goes something like 100fps when not playing video, so this
			// should produce a 10 frame growth animation:
			m_tweenedCallbackMgr.Add(TweenedCB_growButton, &m_ctrls[i], 1.0f, 1.5f, 0.1f);
		}
	}

	return status;
}

//////////////////////////////////////////////////////////////////////////////////////
void VideoCtrls::TweenedCB_growButton(void* obj, float t)
{
	GLButton* glb = (GLButton*)obj;

	// this is called when the passed button is in it's initial unscaled position

	// we're tweening the scale of the button, so:
	glb->m_scale = t;

	// calc new width:
	float new_width  = glb->m_texture.m_width  * glb->m_scale;
	float new_height = glb->m_texture.m_height * glb->m_scale;

	// calc width change from unscaled:
	float deltaX = new_width  - glb->m_texture.m_width;
	float deltaY = new_height - glb->m_texture.m_height;

	// move the x position by half this delta so button grows in place:
	glb->m_pos.x -= (deltaX * 0.5f);
	glb->m_pos.y -= (deltaY * 0.5f);
}

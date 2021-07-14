#pragma once

#ifndef _GLBUTTON_H_
#define _GLBUTTON_H_


#include "ffvideo_player_app.h"

////////////////////////////////////////////////////////////////////////////////
enum class GLButtonState { uninit, normal, hover, hilight, disabled };

// predeclaration:
class RenderCanvas;
class GLButton;


typedef void(*GLBUTTON_CALLBACK) (void* p_object, GLButton* button);

////////////////////////////////////////////////////////////////////////////////
class GLButton
{
public:
	GLButton();
	~GLButton() {};

	bool Load(const char* filePath);
	void GenGLTexParams(void);
	void SetPosition(FF_Vector3D& pos);		// the z coord is considered the depth of the button
	void SetCanvas(RenderCanvas* canvas);
	void SetCallback(GLBUTTON_CALLBACK cb, void* data);
	void RenderSetup(void);
	void CalcState(void);
	void Render(void);
	void RenderDone(void);

	// 
	bool HilightTriggerCallback(void);

	FFVideo_Image			m_texture;
	GLuint						m_textureID;
	FF_Vector3D				m_pos;
	float							m_scale;	// scale factor for w,h
	GLButtonState			m_state;
	RenderCanvas*			mp_canvas;

	GLBUTTON_CALLBACK mp_callback;
	void*             mp_callback_data;
	float							m_lastTriggerTime;
};


#endif // _GLBUTTON_H_


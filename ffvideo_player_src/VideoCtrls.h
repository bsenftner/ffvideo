#pragma once
#ifndef _VIDEOCTRLS_H_
#define _VIDEOCTRLS_H_


// a simple OGL texture with supporting logic for use as button
#include "GLButton.h"

////////////////////////////////////////////////////////////////////////
enum class VCR_CTRL { play = 0, pause, step_back, stop, step_forward };

////////////////////////////////////////////////////////////////////////
class VideoCtrls
{
public:
	VideoCtrls();
	~VideoCtrls() {};

	bool Initialize(std::string& data_dir, RenderCanvas* canvas);
	void SetStreamType(STREAM_TYPE sType);
	void Render(void);
	bool HilightTriggerCallback(void);

	std::string					m_data_dir;
	RenderCanvas*				mp_canvas;
	GLButton						m_ctrls[5];

	// has own clock, callbacks triggered by buttons renderer:
	TweenedCallbackMgr	m_tweenedCallbackMgr;	
	//
	static void TweenedCB_growButton(void* obj, float t);
};


#endif // _VIDEOCTRLS_H_
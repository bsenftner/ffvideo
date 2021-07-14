#pragma once

#ifndef _TWEENEDCALLBACKMGR_H_
#define _TWEENEDCALLBACKMGR_H_


#include "ffvideo_player_app.h"


///////////////////////////////////////////////////////////////////////////////////////////
// A TweenedCallback is a callback that is called every render during a time period, with
// the callback receiving 2 params: callback_data and "t" - an interpololated value.

// TweenedCallback callback type
typedef void(*TWEENED_CALLBACK_TYPE)(void* p_object, float t);

class TweenedCallback
{
public:
	TweenedCallback() : mp_tweened_callback(NULL), mp_tweened_callback_object(NULL), 
	m_start_value(0.0f), m_end_value(0.0f), m_start_time(0.0f), m_end_time(0.0f) { };

	TweenedCallback(TWEENED_CALLBACK_TYPE cb, void* cb_object, float start_value, float end_value, float start_time, float end_time) :
		mp_tweened_callback(cb), mp_tweened_callback_object(cb_object), 
		m_start_value(start_value), m_end_value(end_value), 
		m_start_time(start_time), m_end_time(end_time) {};

	// copy constructor 
	TweenedCallback(const TweenedCallback& tcb)
	{
		mp_tweened_callback        = tcb.mp_tweened_callback;
		mp_tweened_callback_object = tcb.mp_tweened_callback_object;
		m_start_value              = tcb.m_start_value;
		m_end_value                = tcb.m_end_value;
		m_start_time               = tcb.m_start_time;
		m_end_time                 = tcb.m_end_time;
	}

	// copy assignement operator
	TweenedCallback& operator = (const TweenedCallback& tcb)
	{
		if (this != &tcb)
		{
			mp_tweened_callback        = tcb.mp_tweened_callback;
			mp_tweened_callback_object = tcb.mp_tweened_callback_object;
			m_start_value              = tcb.m_start_value;
			m_end_value                = tcb.m_end_value;
			m_start_time               = tcb.m_start_time;
			m_end_time                 = tcb.m_end_time;
		}
		return *this;
	}

	TWEENED_CALLBACK_TYPE mp_tweened_callback;
	void*									mp_tweened_callback_object;
	float									m_start_value, m_end_value;
	float				          m_start_time, m_end_time;
};


////////////////////////////////////////////////////////////////////////
class TweenedCallbackMgr
{
public:
	TweenedCallbackMgr();

	// assumes the start_time is now and the end_time is now+duration:
	void Add(TWEENED_CALLBACK_TYPE cb, void* cb_object, float start_value, float end_value, float duration);

	// adds of a callback that will be active in the future. Start_time is now+wait, end_time is now+wait+duration.  
	void Add(TWEENED_CALLBACK_TYPE cb, void* cb_object, float start_value, float end_value, float wait, float duration);

	// when time exceeds a callback's end_time it is auto-removed:
	void CallActiveCallbacks(void);

	std::vector<TweenedCallback> m_callbacks;
	BCTime											 m_clock;								// keeps track of time
};


#endif // _TWEENEDCALLBACKMGR_H_


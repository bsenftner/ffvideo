#pragma once

#ifndef _DELAYEDCALLBACKMGR_H_
#define _DELAYEDCALLBACKMGR_H_


#include "ffvideo_player_app.h"


///////////////////////////////////////////////////////////////////////////////////////////
// A DelayedCallback is a callback with a timeout, 
// They use a float to represent seconds to wait before calling the callback

// DelayedCallback callback type
typedef void(*DELAYED_CALLBACK_TYPE)(void* p_object);

class DelayedCallback
{
public:
	DelayedCallback() {};

	DelayedCallback(DELAYED_CALLBACK_TYPE cb, void* cb_object, float timeout) : mp_delayed_callback(cb), mp_delayed_callback_object(cb_object), m_timeout(timeout) {};

	// copy constructor 
	DelayedCallback(const DelayedCallback& dlcb)
	{
		mp_delayed_callback = dlcb.mp_delayed_callback;
		mp_delayed_callback_object = dlcb.mp_delayed_callback_object;
		m_timeout = dlcb.m_timeout;
	}

	// copy assignement operator
	DelayedCallback& operator = (const DelayedCallback& dlcb)
	{
		if (this != &dlcb)
		{
			mp_delayed_callback = dlcb.mp_delayed_callback;
			mp_delayed_callback_object = dlcb.mp_delayed_callback_object;
			m_timeout = dlcb.m_timeout;
		}
		return *this;
	}
	

	DELAYED_CALLBACK_TYPE mp_delayed_callback;
	void*									mp_delayed_callback_object;
	float				          m_timeout;
};


////////////////////////////////////////////////////////////////////////
class DelayedCallbackMgr
{
public:
	DelayedCallbackMgr(int32_t frequency, wxEvtHandler* owner, int32_t wxTimerId);

	void Add(DELAYED_CALLBACK_TYPE cb, void* cb_object, float secondsTillCallback);

	void CallReadyCallbacks(void);

	std::vector<DelayedCallback> m_delayed_callbacks;		
	BCTime											 m_clock;								// keeps track of time
	wxTimer										   m_timer;								// causes a wxTimer event to fire on a timed cycle
	wxEvtHandler*								 mp_owner;							// wxWindow the event will be delivered
	int32_t											 m_wxTimerId;						// wxWidgets control id of the wxTimer
};


#endif // _DELAYEDCALLBACKMGR_H_


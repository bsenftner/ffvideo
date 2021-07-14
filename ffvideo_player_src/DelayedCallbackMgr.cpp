

#include "ffvideo_player_app.h"


////////////////////////////////////////////////////////////////////////
DelayedCallbackMgr::DelayedCallbackMgr(int32_t frequency, wxEvtHandler* owner, int32_t wxTimerId)
{
	m_clock.Start();										// how this class measures time

	m_timer.SetOwner(owner, wxTimerId);	
	m_timer.Start(1000 / frequency);		// set number of times per second a wxTimer event is emitted to owner
}

////////////////////////////////////////////////////////////////////////
void DelayedCallbackMgr::Add(DELAYED_CALLBACK_TYPE cb, void* cb_object, float secondsTillCallback)
{
	DelayedCallback ltask(cb, cb_object, m_clock.s() + secondsTillCallback);

	m_delayed_callbacks.push_back(ltask);
}

////////////////////////////////////////////////////////////////////////
void DelayedCallbackMgr::CallReadyCallbacks(void)
{
	float now = m_clock.s();
	size_t limit = m_delayed_callbacks.size();
	for (size_t i = 0; i < limit; i++)
	{
		if (m_delayed_callbacks[i].m_timeout < now)
		{
			if (m_delayed_callbacks[i].mp_delayed_callback)
			{
				m_delayed_callbacks[i].mp_delayed_callback(m_delayed_callbacks[i].mp_delayed_callback_object);
			}

			if (i < limit - 1)
			{
				using std::swap;
				swap(m_delayed_callbacks[i], m_delayed_callbacks.back());
			}
			m_delayed_callbacks.pop_back();

			limit--; // one less delayed callback
			i--;     // tail swapped with the index we just did, need to do that index again
		}
	}
}


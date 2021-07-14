

#include "ffvideo_player_app.h"


////////////////////////////////////////////////////////////////////////
TweenedCallbackMgr::TweenedCallbackMgr()
{
	m_clock.Start(); // how this class measures time
}

////////////////////////////////////////////////////////////////////////
void TweenedCallbackMgr::Add(TWEENED_CALLBACK_TYPE cb, void* cb_object, float start_value, float end_value, float duration)
{
	TweenedCallback callback(cb, cb_object, start_value, end_value, m_clock.s(), m_clock.s() + duration);

	m_callbacks.push_back(callback);
}

////////////////////////////////////////////////////////////////////////
void TweenedCallbackMgr::Add(TWEENED_CALLBACK_TYPE cb, void* cb_object, float start_value, float end_value, float start_time, float end_time)
{
	TweenedCallback callback(cb, cb_object, start_value, end_value, start_time, end_time);

	m_callbacks.push_back(callback);
}

////////////////////////////////////////////////////////////////////////
void TweenedCallbackMgr::CallActiveCallbacks(void)
{
	float now = m_clock.s();
	size_t limit = m_callbacks.size();
	for (size_t i = 0; i < limit; i++)
	{
		TweenedCallback& tcb = m_callbacks[i];

		if (tcb.m_start_time <= now && tcb.m_end_time >= now)
		{
			// we be active

			if (tcb.mp_tweened_callback)
			{
				float t = (now - tcb.m_start_time) / (tcb.m_end_time - tcb.m_start_time);

				float delta = tcb.m_end_value - tcb.m_start_value;

				float client_value = tcb.m_start_value + delta * t;

				tcb.mp_tweened_callback(tcb.mp_tweened_callback_object, client_value);
			}
		}

		if (tcb.m_end_time < now)
		{
			using std::swap;
			swap(m_callbacks[i], m_callbacks.back());
			m_callbacks.pop_back();

			limit--; // one less callback
			i--;     // tail swapped with the index we just did, need to do that index again
		}
	}
}




#include "ffvideo_player_app.h"


////////////////////////////////////////////////////////////////////////
void NoticeMgr::AddNotice(std::string& msg, float secondsToDisplay)
{
	Notice note(msg, mp_app->m_clock.s() + secondsToDisplay);

	m_notices.push_back(note);
}

////////////////////////////////////////////////////////////////////////
void NoticeMgr::DeleteExpiredNotices(void)
{
	float now = mp_app->m_clock.s();
	size_t limit = m_notices.size();
	for (size_t i = 0; i < limit; i++)
	{
		if (m_notices[i].m_timeout < now)
		{
			using std::swap;
			swap(m_notices[i], m_notices.back());
			m_notices.pop_back();

			limit--; // one less notification
			i--;     // tail swapped with the index we just did, need to do that index again
		}
	}
}

////////////////////////////////////////////////////////////////////////
void NoticeMgr::ExportNotices(std::vector<std::string>& dest)
{
	size_t limit = m_notices.size();
	for (size_t i = 0; i < limit; i++)
	{
		Notice& note = m_notices[i];
		float   time = note.m_timeout - mp_app->m_clock.s();

	//std::string msg = mp_app->FormatStr("%s, with %1.2f secs left", note.m_text.c_str(), time);

		dest.push_back(note.m_text);
	}
}

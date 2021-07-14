#pragma once
#ifndef _NOTICEMGR_H_
#define _NOTICEMGR_H_

#include <string>
#include <vector>

// predeclarations:
class TheApp;


////////////////////////////////////////////////////////////////////////////////////////////
// A Notice is a string with a timeout, a float representing seconds to display the message. 
class Notice
{
public:
	Notice() : m_timeout(0.0f) {};

	Notice(std::string& text, float timeout) : m_text(text), m_timeout(timeout) {};

	// copy constructor 
	Notice(const Notice& note)
	{
		m_text = note.m_text;
		m_timeout = note.m_timeout;
	}

	// copy assignement operator
	Notice& operator = (const Notice& note)
	{
		if (this != &note)
		{
			m_text = note.m_text;
			m_timeout = note.m_timeout;
		}
		return *this;
	}

	std::string m_text;
	float				m_timeout;
};

////////////////////////////////////////////////////////////////////////
class NoticeMgr
{
public:
	NoticeMgr(TheApp* app) : mp_app(app) {};

	void AddNotice(std::string& msg, float secondsToDisplay);
	void DeleteExpiredNotices(void);
	void ExportNotices(std::vector<std::string>& dest);

	TheApp* mp_app;
	std::vector<Notice> m_notices;
};


#endif // _NOTICEMGR_H_
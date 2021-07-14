// ---------------------------------------------------------------------------------
// BCTime is just a wrapper around the boost chrono timers
//

#pragma once
#ifndef _BCTIME_H_
#define _BCTIME_H_



#include <boost/chrono.hpp>
using namespace boost::chrono;

// ---------------------------------------------------------------------------------
class BCTime {
protected:
	high_resolution_clock::time_point m_start; // saves time point in nanoseconds

public:
	BCTime(void);

	void	  Start(void);
	void	  Reset(void);
	double	nano(void);			 // returns time from Start in nanoseconds
	double	micro(void);     // returns time from Start in microseconds
	float   ms(void);        // returns time from Start in milliseconds
	float   s(void);         // returns time from Start in seconds
};

// ---------------------------------------------------------------------------------
inline BCTime::BCTime(void) { m_start = high_resolution_clock::now(); }

// ---------------------------------------------------------------------------------
inline void BCTime::Start(void) { m_start = high_resolution_clock::now(); }

// ---------------------------------------------------------------------------------
inline void BCTime::Reset(void) { m_start = high_resolution_clock::now(); }

// ---------------------------------------------------------------------------------
inline double BCTime::nano(void)
{
	high_resolution_clock::duration lapsed = high_resolution_clock::now() - m_start;
	return (double)lapsed.count();
}

// ---------------------------------------------------------------------------------
inline double BCTime::micro(void)
{
	return nano() * 0.001; // 1 micro second = 1000 nanoseconds
}

// ---------------------------------------------------------------------------------
inline float BCTime::ms(void)
{
	return (float)(micro() * 0.001); // 1 ms = 1,000 micro seconds
}

// ---------------------------------------------------------------------------------
inline float BCTime::s(void)
{
	high_resolution_clock::duration lapsed = high_resolution_clock::now() - m_start;

	return (float)(micro() * 0.000001); // 1 s = 1,000,000  micro seconds
}



#endif // _BCTIME_H_


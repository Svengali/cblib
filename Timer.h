#pragma once

#include "cblib/Base.h"

START_CB

namespace Timer
{
	typedef unsigned __int64 tsc_type;

	//---------------------------------------------------------------

	void LogInfo(); // call at startup if you like
	
	//---------------------------------------------------------------
	// Sample all three primary timers
	struct Sample
	{
		tsc_type		tsc;
		double			qpc; // seconds
		//unsigned int	millis;
	};
	
	void GetSample(Sample * ptr);
	
	// delta(early,late)
	double DeltaSamples(const Sample & s1,const Sample & s2);
	
	// GetSeconds is seconds from startup using the Sample routines
	double GetSeconds();
	
	//---------------------------------------------------------------
	//	GetMillis uses the windows GetTickCount
	//	it's reliable, but only measures millisecond accuracy
	//	millis wraps 32 bits ever 49 days
	
	unsigned int GetMillis();

	//---------------------------------------------------------------
	// GetQPC uses QPC and converts to seconds
	//	it's a timer based on the system bus, usually around 50 MHz
	//	it can jump forward or back badly, the Sample stuff corrects for that
	double GetQPCSeconds();
	
	//---------------------------------------------------------------
	// tsc counts the number of clocks passed
	//	it's fast and always reliable when measured in clocks
	//	the tsc conversion to seconds does weird things on laptops with speedstep
		
	tsc_type	rdtsc();
	
	int			GetMHZ();
	double		GetSecondsPerTick();
	double		rdtscSeconds();
	
	//---------------------------------------------------------------
	
	inline void SampleAndAdd(const Timer::Sample start,double *pAccum)
	{
		Timer::Sample end;
		Timer::GetSample(&end);
		double delta = Timer::DeltaSamples(start,end);
		*pAccum += delta;
	}
};


/*

DO_ONCE_EVERY

if N seconds have past since last time, run Code

*/

#define DO_ONCE_EVERY(seconds,some_code)	\
do { static double s_last = -99999.9; double now = Timer::GetSeconds(); if ( (now - s_last) > (seconds) ) { s_last = now; { some_code; } } } while(0)


/*

SCOPE_TIMER(var) , use like :

static double s_time = 0;

void Func()
{
	SCOPE_TIMER(s_time);
	
	// do stuff
	
	// measures time spent in this scope
}

*/

class TimeScope
{
public:
	TimeScope(double * pAccum) : m_pAccum(pAccum)
	{
		Timer::GetSample(&m_start);
	}
	
	~TimeScope()
	{
		Timer::Sample end;
		Timer::GetSample(&end);
		double delta = Timer::DeltaSamples(m_start,end);
		*m_pAccum += delta;
	}

	Timer::Sample	m_start;
	double * m_pAccum;
};

#define SCOPE_TIMER(var)	NS_CB::TimeScope _tsc_thingy(&var)

class TimeScopeLog
{
public:
	TimeScopeLog(const char * tag) : m_tag(tag)
	{
		Timer::GetSample(&m_start);
	}
	
	~TimeScopeLog();
	
	Timer::Sample	m_start;
	const char *	m_tag;
};

END_CB

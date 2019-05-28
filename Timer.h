#pragma once

#include "cblib/Base.h"

START_CB

/*******

NOTE :

uint64 tsc and qpc handles wrapping just fine if you use subtract into an unsigned type :

eg.

uint64 delta = end - start;

will be right even if the ticker wrapper

HOWEVER - compares to not work :

ASSERT( end >= start );

if NOT always true.

---------------

Best to just use the QPC() call here which bases things to zero for you, then you can compare easily

*********/

namespace Timer
{
	typedef uint64 tsc_type;
	typedef uint32 tick_count_type;

	//---------------------------------------------------------------

	void LogInfo(); // call at startup if you like
	
	//---------------------------------------------------------------
	// Sample all three primary timers
	
	// #define DO_SAMPLE_TSC
	
	struct Sample
	{
		#ifdef DO_SAMPLE_TSC
		tsc_type	tsc;
		#endif
		
		uint64			qpc;
		tick_count_type	tickCount;
	};
	
	void GetSample(Sample * ptr);
	
	// delta(early,late)
	// DeltaSamples = end - start
	double DeltaSamples(const Sample & start,const Sample & end);
	
	// GetSeconds is seconds from startup using the Sample routines
	double GetSeconds();
	
	// precedence operator :
	inline bool operator < (const Sample & lhs, const Sample & rhs)
	{
		return DeltaSamples(lhs,rhs) > 0;
	}
	
	//---------------------------------------------------------------
	//	GetMillis uses the windows GetTickCount
	//	it's reliable, but only measures millisecond accuracy
	//	millis wraps 32 bits ever 49 days
	
	uint32 GetMillis32();
	uint64 GetMillis64();

	//---------------------------------------------------------------
	// tsc counts the number of clocks passed
	//	it's fast and always reliable when measured in clocks
	//	the tsc conversion to seconds does weird things on laptops with speedstep
		
	tsc_type	rdtsc();
	
	uint64		RawQPC();
	uint64		QPC(); // QPC starts at 0 so you don't have to worry about wraps
	
	int			GetMHZ();
	uint64		GetAbsoluteTicks();
	double		GetSecondsPerTick();
	double		GetSecondsPerQPC();
	
	double		rdtscSeconds();
	double		GetQPCSeconds();
	
	inline double	ConvertTicksToSeconds(uint64 ticks) { return ticks * GetSecondsPerTick(); }
	inline double	GetDeltaTicksSeconds(uint64 start,uint64 end) { return (end - start) * GetSecondsPerTick(); }
	
	//---------------------------------------------------------------
	
	inline void SampleAndAdd(const Timer::Sample start,double *pAccum)
	{
		Timer::Sample end;
		Timer::GetSample(&end);
		double delta = Timer::DeltaSamples(start,end);
		*pAccum += delta;
	}
	
	class AutoSample : public Sample
	{
	public :
		AutoSample() { GetSample(this); }
	};
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

#define SCOPE_TIMER(var)	NS_CB::TimeScope NUMBERNAME(timer)(&var)

class TimeScopeLog
{
public:
	TimeScopeLog(const char * tag) : m_tag(tag), m_count(1)
	{
		Timer::GetSample(&m_start);
	}
	TimeScopeLog(const char * tag,int count) : m_tag(tag), m_count(count)
	{
		Timer::GetSample(&m_start);
	}
	
	~TimeScopeLog();
	
	Timer::Sample	m_start;
	const int		m_count;
	const char *	m_tag;
private:
	FORBID_CLASS_STANDARDS(TimeScopeLog);
};

#define SCOPE_LOG_TIMER(tag)	NS_CB::TimeScopeLog NUMBERNAME(timer)(tag)

class TSCScopeLog
{
public:
	TSCScopeLog(const char * tag) : m_tag(tag), m_count(1), m_start(Timer::rdtsc())
	{
	}
	TSCScopeLog(const char * tag,int count) : m_tag(tag), m_count(count), m_start(Timer::rdtsc())
	{
	}
	
	~TSCScopeLog();
	
	Timer::tsc_type	m_start;
	const int		m_count;
	const char *	m_tag;
private:
	FORBID_CLASS_STANDARDS(TSCScopeLog);
};

#define SCOPE_LOG_TSC(tag)	NS_CB::TSCScopeLog NUMBERNAME(timer)(tag)

//===================================================================	

// @@ now just the first 4 cores :
	
#define TIMING_LOOP_PRE(timing_repeats) do { \
	HANDLE proc = GetCurrentProcess(); \
	HANDLE thread = GetCurrentThread(); \
	DWORD_PTR affProc,affSys; \
	GetProcessAffinityMask(proc,&affProc,&affSys); \
	for(int core=0;core<4;core++) \
	{ \
		DWORD mask = 1UL<<core; \
		if ( mask & affProc ) \
			SetThreadAffinityMask(thread,mask); \
		else \
			continue; \
		for LOOP(r,timing_repeats) \
		{ \
			TrashTheCache();

#define TIMING_LOOP_POST() \
		} \
	} \
	SetThreadAffinityMask(thread,affProc); \
	}while(0)			
	

#define TIMING_LOOP_PRE_SIMPLE(timing_repeats,timer_name) \
	Timer::tsc_type timer_name = (uint64)-1; \
	TIMING_LOOP_PRE(timing_repeats) \
	Timer::tsc_type tsc_start = Timer::rdtsc();

#define TIMING_LOOP_POST_SIMPLE(timer_name) \
	Timer::tsc_type tsc_end = Timer::rdtsc(); \
	timer_name = MIN( timer_name, (tsc_end - tsc_start) ); \
	TIMING_LOOP_POST();

#define TIMING_LOOP_POST_SIMPLE_LOG(timer_name,length) \
	TIMING_LOOP_POST_SIMPLE(timer_name); \
	lprintf("%s : seconds:%.4f | ticks per: %.3f | MB/s : %.2f\n", STRINGIZE(timer_name), Timer::ConvertTicksToSeconds(timer_name), timer_name/(double)length , (double)length/(1000*1000*Timer::ConvertTicksToSeconds(timer_name)) );

//===================================================================	

END_CB

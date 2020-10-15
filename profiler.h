#pragma once

#include "Base.h"
#include "Timer.h"
#include "cblib_config.h"

/*****************

In-game real-time profiler

Reasonably light-weight when in Disabled mode.

Clients should primarily use the PROFILE macro, like :

void func()
{
	PROFILE(func);
	... do stuff ...
}

-------------------------------------

Profiler is "frame" based.  Even if you're not doing like a frame-based realtime app, you can use the "frame" concept.
Basically a "frame" is one processing block, then Profiler will count things as per-processing block.
For example if you're processing database records, you call Profiler::Frame() for each record and the report will then
	give you information in terms of how long per record.

*******************/

// @@ toggle here : ; uses QPC if you don't want TSC
#define PROFILE_TSC // much faster & more accurate, use when possible !

// usually toggle using SetEnabled() ; this compile-time switch turns it off completely
#ifndef CBLIB_PROFILE_ENABLED
#define CBLIB_PROFILE_ENABLED  1
#endif

START_CB

namespace Profiler
{
	//-------------------------------------------------------------------------------------------

	#ifdef PROFILE_TSC
	inline uint64 GetTimer() { return Timer::GetAbsoluteTicks(); }
	inline double GetSeconds() { return Timer::rdtscSeconds(); }
	inline double TimeToSeconds(uint64 time) { return time * Timer::GetSecondsPerTick(); }
	#else
	inline uint64 GetTimer() { return Timer::QPC(); }
	inline double GetSeconds() { return Timer::GetQPCSeconds(); }
	inline double TimeToSeconds(uint64 time) { return time * Timer::GetSecondsPerQPC(); }
	#endif
	
	//-------------------------------------------------------------------------------------------


	//! default is "Disabled"
	//	when you call SetEnabled() it doesn't actually start until you call a Frame()
	void SetEnabled(const bool yesNo);
	bool GetEnabled();

	extern bool g_enabled; // ugly variable tunnel for fast checks

	//! Push & Pop timer blocks
	//	(generally don't call directly, use "PROFILE" macro)
	void Push(const int index,const char * const name, uint64 time);
	void Pop( const int index,const uint64 delta);
	int Index(const char * const name);

	//! clear the counts
	void Reset();

	//! let us know when a frame happens
	void Frame();

	//! FPS queries
	/*
	float GetInstantFPS();
	float GetAveragedFPS(); // over the last some frames
	float GetTotalFPS(); // since startup
	*/
	
	//-------------------------------------------------------------------------------------------
	
	void WriteRecords(const char * fileName);
	void ReadRecords( const char * fileName);
	
	void GetEntry(const int index, uint64 * pTime, int* pCount);
	
	//! Spew it out to a report since the last Reset (you'll usually want to Report and Reset together)
	//! dumpAll == false only reports the currently selected branch
	//! dumpAll == true dumps all reports known to the profiler (up to some internal limit)
	void Report(const bool dumpAll = false);
	void SetReportNodes(bool enable);
	bool GetReportNodes();
	//! for walking the display :
	void ViewAscend();
	void ViewDescend(int which);

	//-------------------------------------------------------------------------------------------

	//! Useful class for profiling code. Since this stuff is for profiling
	//! everything should be inline.
	class AutoTimer
	{
	public:
		__forceinline AutoTimer(int index,const char * name) : m_index(0)
		{
			if ( g_enabled )
			{
				m_index = index;
				m_startTime = Profiler::GetTimer();
				cb::Profiler::Push(index,name,m_startTime);
			}
		}

		__forceinline ~AutoTimer()
		{
			if ( m_index ) // was enabled at Push time
			{
				uint64 endTime = Profiler::GetTimer();
				Profiler::Pop( m_index, endTime - m_startTime );
			}
		}

	private:
		int			m_index;
		uint64 		m_startTime;
	};

	//-------------------------------------------------------------------------------------------

	#pragma pack(push)
	#pragma pack(4)
	struct ProfileRecord
	{
		int32	index;
		uint64	time;
		
		ProfileRecord() { }
		ProfileRecord(int i,uint64 t) : index(i), time(t) { }
	};
	#pragma pack(pop)
	COMPILER_ASSERT( sizeof(ProfileRecord) == 12 );
	
	const ProfileRecord * GetRecords(int * pCount);
	const char * GetEntryName(int index);
	
	//-------------------------------------------------------------------------------------------
	

};

END_CB

#if CBLIB_PROFILE_ENABLED //{

#define PROFILE(Name) \
static int s_index_##Name = cb::Profiler::Index(_Stringize(Name)); \
cb::Profiler::AutoTimer profile_of_##Name(s_index_##Name,_Stringize(Name))

#define PROFILE_FN(Name) \
static int s_index_##Name = cb::Profiler::Index(_Stringize(Name)); \
cb::Profiler::AutoTimer profile_of_##Name(s_index_##Name,_Stringize(Name))


#else //}{

#define PROFILE(Name)

#define PROFILE_FN(Name)

#endif //} FINAL

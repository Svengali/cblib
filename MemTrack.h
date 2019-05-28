#pragma once

#include <cblib/Base.h>
#include <string.h>

/***************************************

CB : non-intrusive MemTrack

called by my allocs

can also be used to track anything else, just a useful tracker of resources generally

***************************************/

// toggle here :
//	just changes the Auto functions here to NOPs
#define CB_DO_AUTO_MEMTRACK

START_CB

enum EMemTrackType
{
	eMemTrack_New,
	eMemTrack_Malloc,
	eMemTrack_Small,
	eMemTrack_SPtrRef,
	eMemTrack_Count
};
extern const char * c_memTrackNames[eMemTrack_Count];

// MemTrack manual calls always work :
void MemTrack_Add(void * handle, size_t size, EMemTrackType type);
void MemTrack_Remove(void * handle);

void MemTrack_SetEnabled(bool able);
void MemTrack_Shutdown();

void MemTrack_LogAll(const char * tag = NULL, uint64 sinceQPC = 0 );

struct MemTrack_Stats
{
	uint32 numAllocs,numFrees;
	uint64 bytesAlloced,bytesFreed;
	
	MemTrack_Stats() { ZERO_PTR(this); }
	
	void SetDifference(const MemTrack_Stats & newer, const MemTrack_Stats & older);
};

void MemTrack_GetStats(MemTrack_Stats * pStats);

// MemTrack_StatsGrabber : grab in constructor
struct MemTrack_StatsGrabber : public MemTrack_Stats
{
	MemTrack_StatsGrabber()
	{
		MemTrack_GetStats(this);
	}
};

inline uint64 MemTrack_DeltaBytesOutstanding(const MemTrack_Stats & newer, const MemTrack_Stats & older)
{
	return (newer.bytesAlloced - newer.bytesFreed) - (older.bytesAlloced - older.bytesFreed);
}

//=========================================================================

#ifdef CB_DO_AUTO_MEMTRACK

// Auto functins are called by my allocators :
#define AutoMemTrack_Add		MemTrack_Add
#define AutoMemTrack_Remove		MemTrack_Remove

#else

inline void AutoMemTrack_Add(void * handle, size_t size, EMemTrackType type)
{
}

inline void AutoMemTrack_Remove(void * handle)
{
}

#endif // CB_DO_AUTO_MEMTRACK

END_CB

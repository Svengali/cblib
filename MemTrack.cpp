// #define this first so I use untrack versions here :
#define StlAlloc(size)		GlobalAlloc(0,size)
#define StlFree(ptr,size)	GlobalFree(ptr)

// don't go through debug memory system, it uses us !!
#define FROM_MEMORY_CPP

#include "stl_basics.h"
#include "hash_table.h"
#include "StackTrace.h"
#include "Timer.h"
#include "Log.h"
#include "Threading.h"
#include "MemTrack.h"

START_CB

#ifdef CB_DO_AUTO_MEMTRACK
#pragma PRAGMA_MESSAGE("AutoMemtrack is ON")
#else
#pragma PRAGMA_MESSAGE("AutoMemtrack is OFF")
#endif

const char * c_memTrackNames[eMemTrack_Count] =
{
	"New",
	"Malloc",
	"Small",
	"Sptr",
};

struct TrackData
{
	EMemTrackType	type;
	size_t			size;
	uint64			qpc;
	StackTrace		trace;
};

// hash_table uses cb::vector which calls StlAlloc to resize
typedef hash_table<intptr_t,TrackData,hash_table_ops_intptr_t> t_memhash;

struct MemTracker
{
	t_memhash		m_hash;
	CriticalSection	m_crit;
	MemTrack_Stats	m_stats;
	
	MemTracker() { }
	~MemTracker() { }
};

//static volatile bool s_memTrackEnabled = true;
static MemTracker * volatile s_pTracker = NULL;
static volatile bool s_memTrackEnabled = true;
static SimpleMutex s_mutex = 0;
		
MemTracker * GetTracker()
{
	// s_pTracker needs to be a "Singleton" so it works with Cinit stuff
	if ( s_pTracker == NULL )
	{
		// thread safe singleton instantiation :
		SimpleLock(&s_mutex);
		
		if ( s_pTracker == NULL )
		{		
			// disable for new on self :
			ScopedSet<volatile bool> ss(&s_memTrackEnabled,false);
			
			MemTracker * pNew = new MemTracker;

			// !! memory barrier to make pNew get done !			
			
			//s_pTracker = pNew;
			//COMPILER_ASSERT( sizeof(pNew) == sizeof(LONG) );
			InterlockedExchangePointer((void **)&s_pTracker,(void *)pNew);
		}
		SimpleUnlock(&s_mutex);
		
	}
	return s_pTracker;
}

void MemTrack_SetEnabled(bool able)
{
	MemTracker * tracker = GetTracker();
	CB_SCOPE_CRITICAL_SECTION( tracker->m_crit );
	
	s_memTrackEnabled = able;
	//InterlockedExchange((LONG *)&s_memTrackEnabled,(LONG)able);
}

void MemTrack_Shutdown()
{
	//ScopedSet<volatile bool> ss(&s_memTrackEnabled,false);
	
	s_memTrackEnabled = false;
	
	if ( s_pTracker )
	{
		// this is NOT really thread safe
		//	I assume Shutdown is only called by the main thread
		
		MemTracker * pTrack = s_pTracker;
		pTrack->m_crit.Lock();
		
		// s_pTracker = NULL;
		InterlockedExchange((LONG *)&s_pTracker,(LONG)NULL);

		pTrack->m_crit.Unlock();

		delete pTrack;
		s_pTracker = NULL;
	}
	
	StackTrace_Shutdown();
}

void MemTrack_LogAll(const char * tag, uint64 sinceQPC)
{
	ScopedSet<volatile bool> ss(&s_memTrackEnabled,false);
	
	StackTrace_Init();
	
	MemTracker * tracker = GetTracker();
	CB_SCOPE_CRITICAL_SECTION( tracker->m_crit );
	
	// @@ warning - does lprintf do allocs ?
	lprintf("MemTrack_LogAll : %s\n",tag ? tag : "");
	
	t_memhash & hash = tracker->m_hash;
	t_memhash::walk_iterator head = hash.begin();
	for(t_memhash::walk_iterator it = head;it != hash.end();++it)
	{
		const TrackData & data = it->data();
		if ( data.qpc >= sinceQPC )
		{
			lprintf("Mem : %s : %I64d \n",
				c_memTrackNames[data.type],(int64)data.size); 
			//lprintf("Stack:\n");
			LogPushTab();
			data.trace.Log();
			LogPopTab();
		}
	}	
}

void MemTrack_Add(void * handle, size_t size, EMemTrackType type)
{
	if ( handle == NULL )
		return;

	if ( ! s_memTrackEnabled )
		return;
					
	MemTracker * tracker = GetTracker();
	CB_SCOPE_CRITICAL_SECTION( tracker->m_crit );
		
	intptr_t ip = (intptr_t) handle;
	TrackData data;
	data.size = size;
	data.type = type;
	data.qpc  = Timer::QPC();
	data.trace.Grab(2);
	tracker->m_hash.insert( ip, data );
	
	tracker->m_stats.numAllocs ++;
	tracker->m_stats.bytesAlloced += size;
}

void MemTrack_Remove(void * handle)
{
	if ( handle == NULL )
		return;
		
	if ( ! s_memTrackEnabled )
		return;
		
	MemTracker * tracker = GetTracker();
	CB_SCOPE_CRITICAL_SECTION( tracker->m_crit );
			
	intptr_t ip = (intptr_t) handle;
	t_memhash::entry_ptrc ep = tracker->m_hash.find(ip);
	if ( ep )
	{
		const TrackData & data = ep->data();
		tracker->m_stats.numFrees ++;
		tracker->m_stats.bytesFreed += data.size;
		
		tracker->m_hash.erase( ep );
	}
}

void MemTrack_Stats::SetDifference(const MemTrack_Stats & newer, const MemTrack_Stats & older)
{
	numAllocs = newer.numAllocs - older.numAllocs;
	numFrees = newer.numFrees - older.numFrees;
	bytesAlloced = newer.bytesAlloced - older.bytesAlloced;
	bytesFreed = newer.bytesFreed - older.bytesFreed;
}

void MemTrack_GetStats(MemTrack_Stats * pStats)
{
	MemTracker * tracker = GetTracker();
	CB_SCOPE_CRITICAL_SECTION( tracker->m_crit );
	
	*pStats = tracker->m_stats;
}

END_CB


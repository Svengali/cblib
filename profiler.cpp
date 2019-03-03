#include "cblib/Base.h"
#include "cblib/Profiler.h"
#include "cblib/Vector.h"
#include "cblib/Vecsortedpair.h"
#include "cblib/Vector_s.h"
#include "cblib/Log.h"

#include <thread>
#include <atomic>
#include <array>
#include <mutex>


/*****************

In-game real-time profiler

Reasonably light-weight when in Disabled
mode.

Clients should primarily use the PROFILE macro, like :

void func()
{
	PROFILE(func);
	... do stuff ...
}

NOTEZ : 
we match strings by pointer !! requires merging of constant strings !!

--------------------------------------------------

This is a hierarchical AND a non-hierarchical , and you want to be able to switch
between them, like how much time does "Vec3::Add" take in *all* calls to
it, not just the call from the current parent

---------------------------------------------------

Descriptions of counters (all counters represent data since last reset)

percent:
The total percentage of total time that this entry was active for.
This figure includes any nested entries also.

millis:
Average millis per frame (since last reset) that this entry and all
nested entries took.

kclcks:
The average amount of 'ticks' that each invocation of this entry took.

counts:
The average amount of times that this entry was invoked per frame.

---------------------------------------------------

The goal will be to later add :

1. Hookup to the XBox system monitor for nice graphs

2. Provide the full stack information to the PC for an interactive
	tree-view descent; so you can get nice views like :

		GameLoop 100%
			GoMgr::Tick	80%
				Player::Tick 20%
					LookAround 5% (8%)
				Musklum::Tick 10%
					LookAround 3% (8%)
			CameraMgr::Tick 10%

*******************/

#define NUM_ENTRIES_TO_SHOW (20)

START_CB

namespace Profiler
{
	bool g_enabled = false;
}

//namespace
//{
	static int s_numNodes = 0;

	/*
	 ProfilerEntry is for counting the total time by Index of
	 a given PROFILE name
	 */
	struct ProfilerEntry
	{
		const char *	m_name;
		double			m_seconds;
		int				m_count;

		ProfilerEntry(const char * name) : m_name(name), m_seconds(0), m_count(0) { }

		void Reset()
		{
			m_seconds = 0;
			m_count = 0;
		}

		void Increment(const double seconds)
		{
			m_seconds += seconds;
			m_count ++;
		}
	};

	typedef std::pair< const char * , int > charptr_int_pair;
	//typedef vecsortedpair< vector< charptr_int_pair >, str_util::str_compare > charptr_int_map;
	typedef vecsortedpair< vector< charptr_int_pair > > charptr_int_map;

	// compare_ProfilerEntry_by_ticks puts larger ticks first
	struct compare_ProfilerEntry_by_seconds :
			public cb::binary_function<ProfilerEntry, ProfilerEntry, bool>
	{
		bool operator()(const ProfilerEntry & e1, const ProfilerEntry & e2) const
		{
			return e1.m_seconds > e2.m_seconds;
		}
	};
	
	using Profiler::ProfileNode;
	using Profiler::ProfileNodeData;

	// compare_ProfileNode_by_seconds puts larger seconds first
	struct compare_ProfileNode_by_seconds :
			public cb::binary_function<ProfileNode, ProfileNode, bool>
	{
		bool operator()(const ProfileNode & e1, const ProfileNode & e2) const
		{
			return e1.Last().m_seconds > e2.Last().m_seconds;
		}
	};

	//static std::vector<ProfilerData *> s_threads(1024);

	struct ProfilerData;


	static std::array< ProfilerData *, 512 > s_threads;
	static std::atomic<unsigned int> s_threadCount = 0;

	struct ProfilerData
	{
	public:

		static ProfilerData & Instance()
		{
			thread_local ProfilerData inst;
			return inst;
		}

		ProfilerData() : m_root("root",-1,NULL)
		{
			{
				static std::mutex s_mut;
				std::lock_guard lock( s_mut );

				const auto cur = s_threadCount.load();

				m_threadIndex = cur;

				s_threads[cur] = this;

				++s_threadCount;
			}

			m_requestEnabled = false;
			m_requestReset = false;

			m_secondsSinceStart = Profiler::GetSeconds();

			m_inspectNode = &m_root;
			m_showNodes = true;


			DoResetWork();
		}

		void ReqReset()
		{
			m_requestReset = true;
		}

		void DoResetWork()
		{
			m_root.Reset();

			//lprintf("%i | ****** Reset\n", m_threadIndex);

			m_curNode = &m_root;

			#ifdef DEBUG_MEMORY
			MemorySystem::GetStatistics(&m_lastResetMemoryStats);
			#endif

			m_lastSeconds = Profiler::GetSeconds();
			m_secondsSinceReset = 0.0;
			m_framesSinceReset = 0;
			
			m_stack.clear();

			const int n = m_entries.size();
			for(int i=0;i<n;i++)
			{
				m_entries[i].Reset();
			}
		}

		//-------------------------------------

		int						m_threadIndex;

		double					m_lastSeconds;
		double					m_secondsSinceReset;
		double					m_secondsSinceStart;
		int						m_framesSinceReset;
		bool					m_showNodes;
		bool					m_requestEnabled;
		bool					m_requestReset;

		ProfileNode *			m_inspectNode;
		ProfileNode *			m_curNode;
		ProfileNode				m_root;

		charptr_int_map			m_nameMap;
		vector< ProfilerEntry > m_entries;
		vector_s< int , 256 >	m_stack;

	#ifdef DEBUG_MEMORY
		MemorySystem::Statistics	m_lastResetMemoryStats;
	#endif
	};

//} // file-only namespace

//! default is "Disabled"
void Profiler::ReqEnabled(const bool yesNo)
{
	ProfilerData::Instance().m_requestEnabled = yesNo;
	// don't do it until next frame
}

bool Profiler::GetEnabled()
{
	return g_enabled;
}

int Profiler::Index(const char * const name)
{
	// have to do this even when we're disabled, or we'll screw up the indexing for the future;
	//  this should only be done in local statics, though, so it doesn't affect our cost

	auto &profiler = ProfilerData::Instance();

	charptr_int_map & map = profiler.m_nameMap;
	const charptr_int_map::const_iterator it = map.find(name);
	if ( it != map.end() )
	{
		return (*it).second;
	}
	else
	{
		const int index = profiler.m_entries.size();
		profiler.m_entries.push_back( ProfilerEntry(name) );
		map.insert( charptr_int_pair(name,index) );
		ASSERT( map.find(name) != map.end() );
		return index;
	}
}

//! Push & Pop timer blocks
void Profiler::Push(const int index,const char * const name)
{
	ASSERT( g_enabled );

	ProfilerData & profiler = ProfilerData::Instance();

	//lprintf("%i | %s | Push\n", profiler.m_threadIndex, name);

	// match strings by pointer !! requires merging of constant strings !!
	if ( name != profiler.m_curNode->m_name )
	{
		profiler.m_curNode = profiler.m_curNode->GetChild(name,index);
	}

	profiler.m_curNode->Enter();

	// m_stack is a vector_s , so no allocation is possible	
	profiler.m_stack.push_back(index);
}

void Profiler::Pop(const double seconds)
{
	if ( ! g_enabled )
		return;

	ProfilerData & profiler = ProfilerData::Instance();

	//lprintf( "%i | %s | Pop\n", profiler.m_threadIndex, profiler.m_curNode->m_name );


	if ( profiler.m_curNode->Leave(seconds) )
	{
		profiler.m_curNode = profiler.m_curNode->m_parent;
		ASSERT( profiler.m_curNode != NULL );
	}
	
	// this can happen due to enable toggles while the stack is active
	if ( ! profiler.m_stack.empty() )
	{
		int top = profiler.m_stack.back();
		profiler.m_stack.pop_back();

		profiler.m_entries[top].Increment(seconds);
	}
}

void Profiler::Reset()
{
	/*
	ProfilerData & profiler = ProfilerData::Instance();
	profiler.ReqReset();
	*/

	const auto profilerCount = s_threadCount.load();

	for( size_t i=0; i < profilerCount; ++i )
	{
		s_threads[i]->ReqReset();
	}
}

void Profiler::Frame()
{
	ProfilerData & profiler = ProfilerData::Instance();

	//lprintf( "%i | ****** Frame\n", profiler.m_threadIndex );


	if ( g_enabled )
	{
		profiler.m_framesSinceReset ++;
		double curSeconds = Profiler::GetSeconds();
		profiler.m_secondsSinceReset += curSeconds - profiler.m_lastSeconds;
		profiler.m_lastSeconds = curSeconds;
	}
		
	if ( profiler.m_requestEnabled != g_enabled )
	{
		g_enabled = profiler.m_requestEnabled;
		if ( g_enabled )
		{
			profiler.m_requestReset = true;
		}
	}

	if( profiler.m_requestReset )
	{
		profiler.DoResetWork();
		profiler.m_requestReset = false;
	}

}

void Profiler::ViewAscend()
{
	ProfilerData & profiler = ProfilerData::Instance();
	if ( profiler.m_showNodes )
	{
		if ( profiler.m_inspectNode->m_parent )
		{
			profiler.m_inspectNode = profiler.m_inspectNode->m_parent;
		}
	}
}

void Profiler::ViewDescend(int which)
{
	ProfilerData & profiler = ProfilerData::Instance();
	if ( profiler.m_showNodes )
	{	
		ProfileNode * child = profiler.m_inspectNode->m_child;

		ASSERT( which >= 0 );
		if ( which < 0 )
			return;

		for(;;)
		{
			if ( child == NULL )
				return;
			if ( which == 0 )
			{
				profiler.m_inspectNode = child;
				return;
			}
			child = child->m_sibling;
			which--;
		}
	}
}


/** Recursively add the given node, and all siblings and children, to
    the given vector. */
/*
static void	AddNodes(vecsorted< vector_s< ProfileNode, NUM_ENTRIES_TO_SHOW >, compare_ProfileNode_by_seconds >* result, const ProfileNode* node)
{
	if (node == NULL)
	{
		return;
	}

	if (result->size() < NUM_ENTRIES_TO_SHOW)
	{
		result->insert(*node);
	}
	else if (result->back().m_seconds < node->m_seconds)
	{
		result->pop_back();
		result->insert(*node);
	}

	// Add siblings
	AddNodes(result, node->m_sibling);

	// Add children.
	AddNodes(result, node->m_child);
}
*/

namespace Profiler
{
	void ReportNodes(const ProfileNode * pNode,bool recurse);
	void ReportEntries(bool dumpAll);
};

void Profiler::SetReportNodes(bool enable)
{
	ProfilerData::Instance().m_showNodes =enable;
}
bool Profiler::GetReportNodes()
{	
	return ProfilerData::Instance().m_showNodes;
}
	
void Profiler::GetAllNodes( std::vector< ProfileNode* > * const pNodes )
{
	const auto threadCount = s_threadCount.load();

	for( size_t i = 0; i < threadCount; ++i )
	{
		pNodes->push_back(&s_threads[i]->m_root);
	}
}

//! Spew it out to a report
void Profiler::Report(bool dumpAll /* = false */)
{
	/*
	if ( ! g_enabled )
	{
		lprintf("Report on disabled profiler!!\n");
		return;
	}
	*/

	ProfilerData & data = ProfilerData::Instance();

	const int frames = data.m_framesSinceReset;

	if ( frames == 0 )
	{
		lprintf("Report on zero frames!! (call Profiler::Frame)\n");
		return;
	}
	
	const double secondsSinceReset = data.m_secondsSinceReset;

	lprintf("----------------------------------------------- Profiler Report Begin\n");
	lprintf("------ elapsed : %1.3f frames : %d fps = %1.3f , millis = %1.3f\n", 
		data.m_secondsSinceReset,data.m_framesSinceReset,
		double(frames)/secondsSinceReset, secondsSinceReset*1000.0/double(frames) );

	if ( data.m_showNodes )
	{
		if ( dumpAll )
		{
			ReportNodes(&data.m_root,true);
		}
		else
		{
			ReportNodes(data.m_inspectNode,false);
		}
	}
	else
	{
		ReportEntries(dumpAll);
	}
	
	//MemorySystem::LogAllAllocations("Profiler Report",data.m_secondsLastReset);

	#ifdef DEBUG_MEMORY
	MemorySystem::Statistics stats;
	MemorySystem::GetStatistics(&stats);

	MemorySystem::Statistics diff;
	diff.SetDifference(stats,data.m_lastResetMemoryStats);

	lprintf("Memory : allocs = %d, frees = %d\n",diff.numAllocs,diff.numFrees);
	lprintf("per frame : allocs = %1.3f, frees = %1.3f\n",float(diff.numAllocs)/frames,float(diff.numFrees)/frames);
	#endif // DEBUG_MEMORY

	lprintf("----------------------------------------------- Profiler Report End\n");
}


void Profiler::ReportNodes(const ProfileNode * pNodeToShow,bool recurse)
{
	// sort and only show the top 20 or so
	ProfilerData & data = ProfilerData::Instance();

	const double secondsSinceReset = data.m_secondsSinceReset;
	const int frames = data.m_framesSinceReset;
	
	if ( frames == 0 || secondsSinceReset <= 0 )
		return;
	
	double secondsParent;
	if ( pNodeToShow == &(data.m_root) )
	{
		secondsParent = secondsSinceReset;
	}
	else
	{
		secondsParent = pNodeToShow->Last().m_seconds;
	}

	if ( recurse )
		LogPushTab();
		
	lprintf("%-30s : parnt : total : millis: kclocks : counts\n",pNodeToShow->m_name);

	if ( secondsParent <= 0 )
	{
		lprintf("No time in parent!!\n");
		
		if ( recurse )
			LogPopTab();
		
		return;
	}	

	//vecsorted< vector_s< ProfileNode, NUM_ENTRIES_TO_SHOW >, compare_ProfileNode_by_seconds > entries;
	//vector_s< ProfileNode *, NUM_ENTRIES_TO_SHOW > entries;
	vector< ProfileNode * > entries;
	entries.reserve(32);

	// Use entries in the currently selected branch.
	ProfileNode * child = pNodeToShow->m_child;
	while( child != NULL )
	{
		// Sorting fucks up the HUD descent, because it indexes by the normal indexing order
		/*
		if ( entries.size() < NUM_ENTRIES_TO_SHOW )
		{
			entries.insert(*child);
		}
		else if ( child->m_seconds > entries.back().m_seconds )
		{
			entries.pop_back();
			entries.insert(*child);
		}
		*/
		entries.push_back(child);
		child = child->m_sibling;
	}
	
	double secondsSum = 0.0;

	// @@ would be nice to know the per-frame max as well
	// @@ can also use m_index to show the total time for a Node

	for(int i=0;i<entries.size();i++)
	{
		const ProfileNode & entry = *(entries[i]);
		double seconds = entry.Last().m_seconds;
		double percentTotal = 100.0 * seconds / secondsSinceReset;
		double percentParent = 100.0 * seconds / secondsParent;
		double millisPerFrame = seconds * 1000.0 / double(frames);
		double countsPerFrame = entry.Last().m_count / double(frames);
		double kclocksPerCount = ( entry.Last().m_seconds / Timer::GetSecondsPerTick() ) / ( 1000.0 * ( entry.Last().m_count == 0 ? 1 : entry.Last().m_count) );

		lprintf("%-30s : %5.1f : %5.1f : %5.2f : %7.1f : %5.1f\n",
			entry.m_name,
			percentParent,
			percentTotal,
			millisPerFrame,
			kclocksPerCount,
			countsPerFrame);

		secondsSum += seconds;
		
		if ( recurse && entry.Last().m_count > 0 )
		{
			if ( entry.m_child != NULL )
			{
				ReportNodes(&entry,true);	
			}
		}
	}

	// and log missing time not counted in parent
	{
		double seconds = secondsParent - secondsSum;
		double percentTotal = 100.0 * seconds / secondsSinceReset;
		double percentParent = 100.0 * seconds / secondsParent;
		double millisPerFrame = seconds * 1000.0 / double(frames);

		lprintf("%-30s : %5.1f : %5.1f : %5.2f : %7.1f : %5.1f\n",
			"missing",
			percentParent,
			percentTotal,
			millisPerFrame,
			0.0,
			0.0);
	}
	
	if ( recurse )
		LogPopTab();
}

void Profiler::ReportEntries(bool dumpAll)
{
	// sort and only show the top 20 or so

	ProfilerData & data = ProfilerData::Instance();
	if ( ! data.m_stack.empty() )
	{
		lprintf("Report on non-empty stack will not be fully accurate!!\n");
	}
	
	const double secondsSinceReset = data.m_secondsSinceReset;
	const int frames = data.m_framesSinceReset;
	
	const int n = data.m_entries.size();
	if ( n == 0 || frames == 0 )
	{
		lprintf("Nothing to report!!\n");
		return;
	}
	
	const ProfilerEntry * pEntries;
	int numEntries = 0;
	
	vecsorted< vector_s< ProfilerEntry, NUM_ENTRIES_TO_SHOW >, compare_ProfilerEntry_by_seconds > entries_s;
	vector< ProfilerEntry > entries_v;

	if ( dumpAll )
	{
		entries_v.assignv(data.m_entries);
		std::sort( entries_v.begin(), entries_v.end(), compare_ProfilerEntry_by_seconds() );
		pEntries = entries_v.data();
		numEntries = n;
	}
	else
	{
		for(int i=0;i<n;i++)
		{
			const ProfilerEntry & entry = data.m_entries[i];
			if ( entries_s.size() < NUM_ENTRIES_TO_SHOW )
			{
				entries_s.insert(entry);
			}
			else if ( entry.m_seconds > entries_s.back().m_seconds )
			{
				entries_s.pop_back();
				entries_s.insert(entry);
			}
		}
		pEntries = &entries_s[0];
		numEntries = entries_s.size();
	}

	lprintf("%-40s : percent : millis: kclocks : counts\n","name");

	for(int i=0;i<numEntries;i++)
	{
		const ProfilerEntry & entry = pEntries[i];
		if ( entry.m_count == 0 )
			continue;
		double seconds = entry.m_seconds;
		double percent = 100.0 * seconds / secondsSinceReset;
		double millisPerFrame = seconds * 1000.0 / double(frames);
		double kclocksPerCount = ( entry.m_seconds / Timer::GetSecondsPerTick() ) / ( 1000.0 * entry.m_count );
		double countsPerFrame = entry.m_count / double(frames);

		lprintf("%-40s : %-5.1f %% : %-5.2f : %-7.1f : %5.1f\n",
			entry.m_name,
			percent,
			millisPerFrame,
			kclocksPerCount,
			countsPerFrame);
	}
}

void Profiler::GetEntry(double* pSeconds, int* pCount, const int index)
{
	ASSERT( pSeconds );
	ASSERT( pCount );

	ProfilerData & data = ProfilerData::Instance();

	ASSERT( 0 <= index );
	ASSERT( index < data.m_entries.size() );

	const ProfilerEntry& entry = data.m_entries[index];
	*pSeconds = entry.m_seconds;
	*pCount = entry.m_count;
}

END_CB
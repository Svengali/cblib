#include "cblib/Base.h"
#include "cblib/Profiler.h"
#include "cblib/Vector.h"
#include "cblib/Vecsortedpair.h"
#include "cblib/Vector_s.h"
#include "cblib/Log.h"
#include "cblib/File.h"

#define PROFILE_RECORDS
//#define PROFILE_NODES

/**

PROFILE_NODES : track heirarchical time information in realtime
	useful for in-game profile HUD
	
PROFILE_RECORDS : just store a big vector of Push/Pops with times
	doesn't provide any realtime feedback
	write it out and then run AllocParser to get nice views
	
either one or both can be enabled

total times for each index are always tracked

**/

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

namespace
{
	static int s_numNodes = 0;

	/*
	 ProfilerEntry is for counting the total time by Index of
	 a given PROFILE name
	 */
	struct ProfilerEntry
	{
		const char *	m_name;
		uint64			m_time;
		int				m_count;
		String			m_string; // only used by ReadRecords

		ProfilerEntry(const char * name) : m_name(name), m_time(0), m_count(0) { }
		ProfilerEntry() : m_name(NULL), m_time(0), m_count(0) { }

		void Reset()
		{
			m_time = 0;
			m_count = 0;
		}

		void Increment(const uint64 time)
		{
			m_time += time;
			m_count ++;
		}
	};

	typedef std::pair< const char * , int > charptr_int_pair;
	//typedef vecsortedpair< vector< charptr_int_pair >, str_util::str_compare > charptr_int_map;
	typedef vecsortedpair< vector< charptr_int_pair > > charptr_int_map;

	// compare_ProfilerEntry_by_ticks puts larger ticks first
	struct compare_ProfilerEntry_by_seconds :
			public std::binary_function<ProfilerEntry, ProfilerEntry, bool>
	{
		bool operator()(const ProfilerEntry & e1, const ProfilerEntry & e2) const
		{
			return e1.m_time > e2.m_time;
		}
	};
	
	/*
	 ProfileNode is for counting the CHILD time by Index of
	 a given PROFILE name
	 */
	struct ProfileNode
	{
		const char *	m_name;
		int				m_index;
		uint64			m_time;
		int				m_count;
		int				m_recursion;
		ProfileNode	*	m_parent;
		ProfileNode	*	m_child;
		ProfileNode *	m_sibling;

		ProfileNode(const char * name,const int index, ProfileNode * parent) : 
				m_index(index), m_name(name), m_parent(parent), m_time(0), m_count(0), m_recursion(0),
				m_sibling(NULL), m_child(NULL)
		{
			s_numNodes++;
		}

		// never goes away
		//~ProfileNode()

		void Reset()
		{
			m_time = 0;
			m_count = 0;
			m_recursion = 0;

			if ( m_sibling )
			{
				m_sibling->Reset();
			}
			if ( m_child )
			{
				m_child->Reset();
			}
		}

		void Enter()
		{
			ASSERT( m_recursion >= 0 );
			m_count ++;
			m_recursion++;
		}

		bool Leave(const uint64 time)
		{
			m_recursion--;
			ASSERT( m_recursion >= 0 );
			if ( m_recursion > 0 )
				return false;
			m_time += time;
			return true;
		}

		ProfileNode * GetChild( const char * const name, const int index )
		{
			// Try to find this sub node
			ProfileNode * child = m_child;
			while ( child ) 
			{
				if ( child->m_name == name )
				{
					return child;
				}
				child = child->m_sibling;
			}

			// We didn't find it, so add it
			ProfileNode * node = new ProfileNode( name,index, this );
			node->m_sibling = m_child;
			m_child = node;
			return node;
		}
		
	};

	// compare_ProfileNode_by_seconds puts larger seconds first
	struct compare_ProfileNode_by_seconds :
			public std::binary_function<ProfileNode, ProfileNode, bool>
	{
		bool operator()(const ProfileNode & e1, const ProfileNode & e2) const
		{
			return e1.m_time > e2.m_time;
		}
	};

	struct ProfilerData
	{
	public:

		static ProfilerData & Instance()
		{
			static ProfilerData inst;
			return inst;
		}

		ProfilerData() : m_root("root",-1,NULL)
		{
			m_requestEnabled = false;

			m_secondsSinceStart = Profiler::GetSeconds();

			m_inspectNode = &m_root;
			m_showNodes = true;

			#ifdef PROFILE_RECORDS
			m_records.reserve(1024);
			#endif

			m_entries.reserve(32);
			m_entries.resize(1); // slot 0 is unused

			Reset();
		}

		void Reset()
		{
			m_root.Reset();

			m_curNode = &m_root;

			#ifdef DEBUG_MEMORY
			MemorySystem::GetStatistics(&m_lastResetMemoryStats);
			#endif

			m_lastSeconds = Profiler::GetSeconds();
			m_secondsSinceReset = 0.0;
			m_framesSinceReset = 0;
			
			//m_stack.clear();

			const int n = m_entries.size32();
			for(int i=0;i<n;i++)
			{
				m_entries[i].Reset();
			}
		}

		//-------------------------------------

		double					m_lastSeconds;
		double					m_secondsSinceReset;
		double					m_secondsSinceStart;
		int						m_framesSinceReset;
		bool					m_showNodes;
		bool					m_requestEnabled;

		ProfileNode *			m_inspectNode;
		ProfileNode *			m_curNode;
		ProfileNode				m_root;

		charptr_int_map			m_nameMap;
		vector< ProfilerEntry > m_entries;
		//vector_s< int , 256 >	m_stack;

		vector< Profiler::ProfileRecord > m_records;

	#ifdef DEBUG_MEMORY
		MemorySystem::Statistics	m_lastResetMemoryStats;
	#endif
	};

} // file-only namespace

//! default is "Disabled"
void Profiler::SetEnabled(const bool yesNo)
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
	charptr_int_map & map = ProfilerData::Instance().m_nameMap;
	const charptr_int_map::const_iterator it = map.find(name);
	if ( it != map.end() )
	{
		return (*it).second;
	}
	else
	{
		const int index = ProfilerData::Instance().m_entries.size32();
		ASSERT( index > 0 );
		ProfilerData::Instance().m_entries.push_back( ProfilerEntry(name) );
		map.insert( charptr_int_pair(name,index) );
		ASSERT( map.find(name) != map.end() );
		return index;
	}
}

//! Push & Pop timer blocks
void Profiler::Push(const int index,const char * const name,uint64 time)
{
	ASSERT( g_enabled );

	ProfilerData & data = ProfilerData::Instance();

	ASSERT( index > 0 );
	#ifdef PROFILE_RECORDS
	data.m_records.push_back( ProfileRecord(index,time) );
	#endif	

	#ifdef PROFILE_NODES
	// match strings by pointer !! requires merging of constant strings !!
	if ( name != data.m_curNode->m_name )
	{
		data.m_curNode = data.m_curNode->GetChild(name,index);
	}

	data.m_curNode->Enter();
	#endif

	// m_stack is a vector_s , so no allocation is possible	
	//ProfilerData::Instance().m_stack.push_back(index);
}

void Profiler::Pop(int index, const uint64 delta)
{
	//ASSERT( g_enabled );
	ASSERT( index > 0 );

	ProfilerData & data = ProfilerData::Instance();

	#ifdef PROFILE_RECORDS
	data.m_records.push_back( ProfileRecord(- index,delta) );
	#endif
	
	#ifdef PROFILE_NODES
	if ( data.m_curNode && data.m_curNode->m_index == index )
	{
		if ( data.m_curNode->Leave(delta) )
		{
			data.m_curNode = data.m_curNode->m_parent;
			ASSERT( data.m_curNode != NULL );
		}
	}
	#endif
	
	data.m_entries[index].Increment(delta);
		
	/*
	// this can happen due to enable toggles while the stack is active
	if ( ! data.m_stack.empty() )
	{
		int top = data.m_stack.back();
		data.m_stack.pop_back();

		data.m_entries[top].Increment(delta);
	}
	*/
}

void Profiler::Reset()
{
	ProfilerData & data = ProfilerData::Instance();
	data.Reset();
}

void Profiler::Frame()
{
	ProfilerData & data = ProfilerData::Instance();
	if ( g_enabled )
	{
		data.m_framesSinceReset ++;
		double curSeconds = Profiler::GetSeconds();
		data.m_secondsSinceReset += curSeconds - data.m_lastSeconds;
		data.m_lastSeconds = curSeconds;
	}
		
	if ( data.m_requestEnabled != g_enabled )
	{
		g_enabled = data.m_requestEnabled;
		if ( g_enabled )
		{
			Reset();
		}
	}

}

void Profiler::ViewAscend()
{
	#ifdef PROFILE_NODES
	ProfilerData & data = ProfilerData::Instance();
	if ( data.m_showNodes )
	{
		if ( data.m_inspectNode->m_parent )
		{
			data.m_inspectNode = data.m_inspectNode->m_parent;
		}
	}
	#endif
}

void Profiler::ViewDescend(int which)
{
	#ifdef PROFILE_NODES
	ProfilerData & data = ProfilerData::Instance();
	if ( data.m_showNodes )
	{	
		ProfileNode * child = data.m_inspectNode->m_child;

		ASSERT( which >= 0 );
		if ( which < 0 )
			return;

		for(;;)
		{
			if ( child == NULL )
				return;
			if ( which == 0 )
			{
				data.m_inspectNode = child;
				return;
			}
			child = child->m_sibling;
			which--;
		}
	}
	#endif
}


/** Recursively add the given node, and all siblings and children, to
    the given vector. */
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
	else if (result->back().m_time < node->m_time)
	{
		result->pop_back();
		result->insert(*node);
	}

	// Add siblings
	AddNodes(result, node->m_sibling);

	// Add children.
	AddNodes(result, node->m_child);
}

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

	#ifdef PROFILE_NODES
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
	#endif
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
		secondsParent = TimeToSeconds( pNodeToShow->m_time );
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
		else if ( child->m_time > entries.back().m_time )
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

	for(int i=0;i<entries.size32();i++)
	{
		const ProfileNode & entry = *(entries[i]);
		double seconds = TimeToSeconds( entry.m_time );
		double percentTotal = 100.0 * seconds / secondsSinceReset;
		double percentParent = 100.0 * seconds / secondsParent;
		double millisPerFrame = seconds * 1000.0 / double(frames);
		double countsPerFrame = entry.m_count / double(frames);
		double kclocksPerCount = ( entry.m_time ) / ( 1000.0 * (entry.m_count == 0 ? 1 : entry.m_count) );

		lprintf("%-30s : %5.1f : %5.1f : %5.2f : %7.1f : %5.1f\n",
			entry.m_name,
			percentParent,
			percentTotal,
			millisPerFrame,
			kclocksPerCount,
			countsPerFrame);

		secondsSum += seconds;
		
		if ( recurse && entry.m_count > 0 )
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
	/*
	if ( ! data.m_stack.empty() )
	{
		lprintf("Report on non-empty stack will not be fully accurate!!\n");
	}
	*/
	
	const double secondsSinceReset = data.m_secondsSinceReset;
	const int frames = data.m_framesSinceReset;
	
	const int n = data.m_entries.size32();
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
			else if ( entry.m_time > entries_s.back().m_time )
			{
				entries_s.pop_back();
				entries_s.insert(entry);
			}
		}
		pEntries = &entries_s[0];
		numEntries = entries_s.size32();
	}

	lprintf("%-40s : percent : millis: kclocks : counts\n","name");

	for(int i=0;i<numEntries;i++)
	{
		const ProfilerEntry & entry = pEntries[i];
		if ( entry.m_count == 0 )
			continue;
		double seconds = TimeToSeconds( entry.m_time );
		double percent = 100.0 * seconds / secondsSinceReset;
		double millisPerFrame = seconds * 1000.0 / double(frames);
		double kclocksPerCount = ( entry.m_time ) / ( 1000.0 * entry.m_count );
		double countsPerFrame = entry.m_count / double(frames);

		lprintf("%-40s : %-5.1f %% : %-5.2f : %-7.1f : %5.1f\n",
			entry.m_name,
			percent,
			millisPerFrame,
			kclocksPerCount,
			countsPerFrame);
	}
}

void Profiler::GetEntry(const int index, uint64 * pTime, int* pCount)
{
	ASSERT( pTime );
	ASSERT( pCount );

	ProfilerData & data = ProfilerData::Instance();

	ASSERT( 0 <= index );
	ASSERT( index < data.m_entries.size32() );

	const ProfilerEntry& entry = data.m_entries[index];
	*pTime = entry.m_time;
	*pCount = entry.m_count;
}

void Profiler::WriteRecords(const char * fileName)
{
	File f;
	if ( ! f.Open(fileName,"wb") )
		return;
	
	ProfilerData & data = ProfilerData::Instance();

	// write entry names :
	
	f.Put32( data.m_entries.size32() );
	
	for(int i=0; i< data.m_entries.size32(); i++)
	{
		if ( data.m_entries[i].m_name )
			f.WriteCString( data.m_entries[i].m_name ); 
		else
			f.WriteCString( "" ); 
	}
	
	f.Put32( data.m_records.size32() );
	
	if ( ! data.m_records.empty() )
	{
		f.Write( data.m_records.data(), data.m_records.size_bytes() );
	}
	
	f.Close();
}

void Profiler::ReadRecords( const char * fileName)
{
	File f;
	if ( ! f.Open(fileName,"rb") )
		return;
		
	ProfilerData & data = ProfilerData::Instance();

	// read entry names :
	
	uint32 numEntries = f.Get32();
	data.m_entries.resize(numEntries);
	
	for(int i=0; i< data.m_entries.size32(); i++)
	{
		data.m_entries[i].m_string = f.ReadString();
		data.m_entries[i].m_name = data.m_entries[i].m_string.CStr(); 
	}
	
	uint32 numRecords = f.Get32();
	data.m_records.resize(numRecords);
	
	f.Read( data.m_records.data(), data.m_records.size_bytes() );
	
	f.Close();
}
	
const Profiler::ProfileRecord * Profiler::GetRecords(int * pCount)
{
	ProfilerData & data = ProfilerData::Instance();
	*pCount = data.m_records.size32();
	return data.m_records.data();
}

const char * Profiler::GetEntryName(int index)
{
	ProfilerData & data = ProfilerData::Instance();
	if ( index < 0 || index >= data.m_entries.size() )
		return NULL; 
	return data.m_entries[index].m_name;
}

END_CB
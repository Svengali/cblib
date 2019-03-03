#pragma once

#include "cblib/Base.h"
#include "cblib/Timer.h"

#include <vector>

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
#define PROFILE_ENABLED

START_CB

namespace Profiler
{


	struct ProfileNodeData
	{
		int				m_count;
		int				m_recursion;
		double			m_seconds;

	};


	/*
	 ProfileNode is for counting the CHILD time by Index of
	 a given PROFILE name
	 */
	struct ProfileNode
	{
		const char* m_name;
		int				m_index;
		int				m_frame;

		ProfileNode* m_parent;
		ProfileNode* m_child;
		ProfileNode* m_sibling;



		ProfileNode( const char* name, const int index, ProfileNode* parent ):
			m_index( index ), m_name( name ), m_parent( parent ), m_data{ 0 }, m_frame( 0 ),
			m_sibling( NULL ), m_child( NULL )
		{
			//s_numNodes++;
		}

		// never goes away
		//~ProfileNode()

		void Reset()
		{
			++m_frame;
			Cur().m_seconds = 0;

			Cur().m_count = 0;
			Cur().m_recursion = 0;

			if( m_sibling )
			{
				m_sibling->Reset();
			}
			if( m_child )
			{
				m_child->Reset();
			}
		}

		void Enter()
		{
			ASSERT( Cur().m_recursion >= 0 );
			Cur().m_count++;
			Cur().m_recursion++;
		}

		bool Leave( const double seconds )
		{
			Cur().m_recursion--;
			ASSERT( Cur().m_recursion >= 0 );
			if( Cur().m_recursion > 0 )
				return false;
			Cur().m_seconds += seconds;
			return true;
		}

		ProfileNode * GetChild( const char* const name, const int index )
		{
			// Try to find this sub node
			ProfileNode* child = m_child;
			while( child )
			{
				if( child->m_name == name )
				{
					return child;
				}
				child = child->m_sibling;
			}

			// We didn't find it, so add it
			ProfileNode* node = new ProfileNode( name, index, this );
			node->m_sibling = m_child;
			m_child = node;
			return node;
		}

		ProfileNodeData& Cur()
		{
			return m_data[m_frame & 1];
		}

		const ProfileNodeData& Cur() const
		{
			return m_data[m_frame & 1];
		}

		const ProfileNodeData& Last() const
		{
			return m_data[( m_frame - 1 ) & 1];
		}

	private:
		ProfileNodeData m_data[2];

	};



	//-------------------------------------------------------------------------------------------

	#ifdef PROFILE_TSC
	inline double GetSeconds() { return Timer::rdtscSeconds(); }
	#else
	inline double GetSeconds() { return Timer::GetQPCSeconds(); }
	#endif
	
	//-------------------------------------------------------------------------------------------


	//! default is "Disabled"
	//	when you call SetEnabled() it doesn't actually start until you call a Frame()
	void ReqEnabled(const bool enabled);
	bool GetEnabled();

	extern bool g_enabled; // ugly variable tunnel for fast checks

	//! Push & Pop timer blocks
	//	(generally don't call directly, use "PROFILE" macro)
	void Push(const int index,const char * const name);
	void Pop(const double ticks);
	int Index(const char * const name);
	void GetEntry(double * pSeconds, int* pCount, const int index);

	//! Spew it out to a report since the last Reset (you'll usually want to Report and Reset together)
	//! dumpAll == false only reports the currently selected branch
	//! dumpAll == true dumps all reports known to the profiler (up to some internal limit)
	void Report(const bool dumpAll = false);
	void SetReportNodes(bool enable);
	bool GetReportNodes();
	//! for walking the display :
	void ViewAscend();
	void ViewDescend(int which);

	void GetAllNodes( std::vector< ProfileNode* > * const pNodex );


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



	//! Useful class for profiling code. Since this stuff is for profiling
	//! everything should be inline.
	class AutoTimer
	{
	public:
		__forceinline AutoTimer() :
			  m_startTime(Profiler::GetSeconds())
		{
		}

		__forceinline ~AutoTimer()
		{
			if ( g_enabled )
			{
				double endTime = Profiler::GetSeconds();
				Profiler::Pop( endTime - m_startTime );
			}
		}

	private:
		double 		m_startTime;
	};

	//-------------------------------------------------------------------------------------------
};

END_CB

#ifdef PROFILE_ENABLED //{

#define PROFILE(Name) \
static int s_index_##Name = cb::Profiler::Index(_Stringize(Name)); \
if ( cb::Profiler::g_enabled ) cb::Profiler::Push(s_index_##Name,_Stringize(Name)); \
cb::Profiler::AutoTimer profile_of_##Name


#define PROFILE_FN(SUFFIX) \
static int s_index_##__func__##SUFFIX = cb::Profiler::Index(__FUNCTION__ _Stringize(SUFFIX)); \
if ( cb::Profiler::g_enabled ) cb::Profiler::Push(s_index_##__func__##SUFFIX,__FUNCTION__ _Stringize(SUFFIX)); \
cb::Profiler::AutoTimer profile_of_##__func__##SUFFIX

#else //}{

#define PROFILE(Name)
#define PROFILE_FN(SUFFIX)

#endif //} FINAL

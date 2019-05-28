#pragma once

#define THREAD_RELACY
#include "cblib/Threading.h"
#include "cblib/LF/NodeAllocator.h"
	
START_CB
	
/*****************

MPSC_FIFOI : (intrusive)

fully lock-free multi-producer multi-consumer MPSCI

Nodes must not be freed immediately or you deref bad
(MPSC_FIFOI_Node_Free handles this in a shitty way - it keeps them around forever)

I'm not convinced this is any better than the two-lock method
  cuz during contention it just loops really badly

Relies heavy on CAS64 with an ABA counter thing

*****************/

LF_ALIGN_8 struct MPSC_FIFOI_Node
{
	ATOMIC_VAR(MPSC_FIFOI_Node *)	m_next;
};
	
#define MPSC_FIFOI_END	((MPSC_FIFOI_Node *)NULL)
//---------------------------------------------------------

#if 0
inline std::ostream& operator << (std::ostream& s, MPSC_FIFOI_Node_And_Count const& right)
{
    return s << "{" << right.m_ptr << "," << right.m_count << "}";
}
#endif

LF_ALIGN_8 struct MPSC_FIFOI
{
	ATOMIC_VAR(MPSC_FIFOI_Node *)	m_pushEnd; // pushEnd is shared
	
	char		pad1[LF_CACHE_LINE_SIZE - sizeof(MPSC_FIFOI_Node *)];
	
	NOT_THREAD_SAFE(MPSC_FIFOI_Node *)	m_popEnd;	// popEnd is owned by the consumer - not atomic !

	char		pad2[LF_CACHE_LINE_SIZE - sizeof(MPSC_FIFOI_Node *)];
	
	// dummy should be on its own cache line :
	MPSC_FIFOI_Node m_dummy;
	
	char		pad3[LF_CACHE_LINE_SIZE - sizeof(MPSC_FIFOI_Node)];
};
		
inline void MPSC_FIFOI_Open(MPSC_FIFOI * MPSCI)
{
	MPSC_FIFOI_Node * dummy = &(MPSCI->m_dummy);
	StoreRelaxed(&(dummy->m_next),MPSC_FIFOI_END);
	
	StoreRelaxed(&(MPSCI->m_pushEnd),dummy);
	StoreRelaxed(&(MPSCI->m_popEnd),dummy);
}

bool MPSC_FIFOI_IsEmpty(const MPSC_FIFOI volatile * MPSCI);

// generally Flush before Close or assert( IsEmpty() )
inline void MPSC_FIFOI_Close(MPSC_FIFOI * MPSCI)
{
	// should just point at dummy now :
	RL_ASSERT(  MPSC_FIFOI_IsEmpty(MPSCI) );
}

inline void MPSC_FIFOI_Push(MPSC_FIFOI volatile * MPSCI,MPSC_FIFOI_Node * node)
{	
	StoreRelaxed(&(node->m_next),MPSC_FIFOI_END);
	
	// m_pushEnd points at the end of the list,
	//	make it now point at me and get the old end
	//	(at this point Pop can not yet see me)
	MPSC_FIFOI_Node * prev = AtomicExchange(&(MPSCI->m_pushEnd),node);
	
	// old end now links me in (now Pop can see new node)
	// Release to publish the contents of node
	StoreRelease(&(prev->m_next),node);
}

inline LFResult MPSC_FIFOI_Pop_NoLoop(MPSC_FIFOI volatile * MPSCI,MPSC_FIFOI_Node ** pNode)
{
	// does the order matter here ? yes, it does
	MPSC_FIFOI_Node * popEnd = LoadRelaxed(&(MPSCI->m_popEnd));
	MPSC_FIFOI_Node * next = LoadAcquire(&(popEnd->m_next));
	
	if ( popEnd == &(MPSCI->m_dummy) )
	{
		if ( next == MPSC_FIFOI_END )
		{
			// empty
			//return LF_Empty;
			// @@ ??
			
			MPSC_FIFOI_Node * pushEnd = LoadAcquire(&(MPSCI->m_pushEnd));
			if ( pushEnd != popEnd )
			{
				return LF_Abort; // retry
			}
			else
			{
				return LF_Empty;
			}
		}
		
		// popEnd was dummy, but next was not end, just step past it :
		StoreRelaxed(&(MPSCI->m_popEnd), next );
		popEnd = next;
		next = LoadAcquire(&(popEnd->m_next));
	}
	
	// if we have a next we can pop the popEnd :
	if ( next != MPSC_FIFOI_END )
	{
		StoreRelaxed(&(MPSCI->m_popEnd), next );
		
		// got popEnd
		*pNode = popEnd;

		return LF_Success;
	}
	
	// next is null, but popEnd is not dummy, wtf ?
	MPSC_FIFOI_Node * pushEnd = LoadAcquire(&(MPSCI->m_pushEnd));
	if ( pushEnd != popEnd )
	{
		return LF_Abort; // retry
	}
	
	// pushEnd == popEnd and next is null, but popEnd is not dummy
	// repush the dummy :
	
	MPSC_FIFOI_Push(MPSCI,(MPSC_FIFOI_Node *)&(MPSCI->m_dummy));
	
	next = LoadAcquire(&(popEnd->m_next));
	if ( next != MPSC_FIFOI_END )
	{
		StoreRelaxed(&(MPSCI->m_popEnd), next );
		
		// got popEnd
		*pNode = popEnd;

		return LF_Success;		
	}
	else
	{
		MPSC_FIFOI_Node * pushEnd = LoadAcquire(&(MPSCI->m_pushEnd));
		if ( pushEnd != popEnd )
		{
			return LF_Abort; // retry
		}
		else
		{
			return LF_Empty;
		}
	}
}

inline MPSC_FIFOI_Node * MPSC_FIFOI_Pop(MPSC_FIFOI volatile * MPSCI)
{
	SpinBackOff backoff;
	MPSC_FIFOI_Node * node = NULL;
	while ( MPSC_FIFOI_Pop_NoLoop(MPSCI,&node) == LF_Abort )
	{
		backoff.BackOffYield();	
	}
	return node;
}

// @@ CB : IS THIS OKAY ?
//	I have little confidence in my IsEmpty
inline bool MPSC_FIFOI_IsEmpty(const MPSC_FIFOI volatile * MPSCI)
{
	MPSC_FIFOI_Node * popEnd = LoadRelaxed(&(MPSCI->m_popEnd));
	MPSC_FIFOI_Node * next = LoadAcquire(&(popEnd->m_next));
	
	if ( popEnd == &(MPSCI->m_dummy) )
	{
		if ( next == MPSC_FIFOI_END )
		{
			// empty
			return true;
		}
	}
	
	return false;
}

//=================================================================================

struct MPSC_FIFOI_Node_Void : public MPSC_FIFOI_Node
{
	// if the Node Allocator is sequencing, then this can be Relaxed :
	//	if it's not (like Vector_Alloc) then this must be atomic too :
	//NOT_THREAD_SAFE(void *)			m_data;
	ATOMIC_VAR(void *)			m_data;
	
	// (*) cache line pad
};

typedef SELECTED_ALLOCATOR<MPSC_FIFOI_Node_Void> MPSC_FIFOI_Node_Allocator;

//=================================================================================

inline void MPSC_FIFOI_PushData(MPSC_FIFOI volatile * MPSCI,MPSC_FIFOI_Node_Allocator * allocator,void * data)
{
	//SequentialFence();
	MPSC_FIFOI_Node_Void * node = allocator->Alloc();
	RL_ASSERT( node != NULL );
	//SequentialFence();
	//StoreRelaxed( &(node->m_data), data );
	StoreRelease( &(node->m_data), data );
	//SequentialFence();
	MPSC_FIFOI_Push(MPSCI,node);
}

inline void * MPSC_FIFOI_PopData(MPSC_FIFOI volatile * MPSCI,MPSC_FIFOI_Node_Allocator * allocator)
{
	MPSC_FIFOI_Node_Void * node = (MPSC_FIFOI_Node_Void *) MPSC_FIFOI_Pop(MPSCI);
	if ( ! node )
		return NULL;
	//void * ret = LoadRelaxed( &(node->m_data) );
	void * ret = LoadAcquire( &(node->m_data) );
	//SequentialFence();
	allocator->Free(node);
	//SequentialFence();
	return ret;
}

// Flush : Pop out remaining nodes and free them
//	(generally do this before Close)
inline void MPSC_FIFOI_Flush(MPSC_FIFOI volatile * MPSCI,MPSC_FIFOI_Node_Allocator * allocator)
{
	while ( MPSC_FIFOI_Node_Void * node = (MPSC_FIFOI_Node_Void *) MPSC_FIFOI_Pop(MPSCI) )
	{
		allocator->Free(node);
	}
}

//=================================================================================

END_CB

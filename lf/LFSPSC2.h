#pragma once

#define THREAD_RELACY
#include "cblib/Threading.h"
#include "cblib/LF/NodeAllocator.h"

START_CB

//=================================================================================

/***

FIFO2 ; uses a "first" and "head"

Pop is very simple : head just steps to next
	nodes left behind are not cleaned up
	
The Producer owns a linked list from first -> tail
	head is just a jump-in point somewhere in there
	
Producer can Pop off the past ones from first-> head and reuse them to Push new nodes

This means the Allocator is wholly owned by Producer - so the Allocator does not need to be thread-safe !
	That might be a pretty big win.
	or not.  Dimitry has a very fast FIFO allocator.
	Or you can just always send the nodes back on a round trip.

	If you actually *want* to send them on round trips then that is the best option.

***/

LF_ALIGN_8 struct SPSC_FIFO2_Node
{
	ATOMIC_VAR(SPSC_FIFO2_Node *)	m_next;
	NOT_THREAD_SAFE(void *)			m_data;
};

/*
bool operator == (const SPSC_FIFO2_Node & lhs, const SPSC_FIFO2_Node & rhs)
{
	return (lhs.m_next($) == rhs.m_next($) && lhs.m_data($) == rhs.m_data($) );
}
*/

typedef SELECTED_ALLOCATOR<SPSC_FIFO2_Node> SPSC_FIFO2_Node_Allocator;

LF_ALIGN_8 struct SPSC_FIFO2
{
	ATOMIC_VAR(SPSC_FIFO2_Node *)	m_head; // head is owned by the consumer
	
	char	pad1[LF_CACHE_LINE_SIZE - sizeof(SPSC_FIFO2_Node *)];
	
	ATOMIC_VAR(SPSC_FIFO2_Node *)	m_first; 
	ATOMIC_VAR(SPSC_FIFO2_Node *)	m_tail;	// tail is owned by the producer
	
	char	pad2[LF_CACHE_LINE_SIZE - sizeof(SPSC_FIFO2_Node *)];
};

inline void SPSC_FIFO2_Open(SPSC_FIFO2 * fifo,SPSC_FIFO2_Node_Allocator * allocator)
{
	SPSC_FIFO2_Node * dummy = allocator->Alloc();
	StoreRelaxed(&(dummy->m_next), (SPSC_FIFO2_Node *)NULL);
	
	StoreRelaxed(&(fifo->m_first), dummy);
	StoreRelaxed(&(fifo->m_head), dummy);
	StoreRelaxed(&(fifo->m_tail), dummy);
}

// generally Flush before Close or assert( IsEmpty() )
inline void SPSC_FIFO2_Close(SPSC_FIFO2 * fifo,SPSC_FIFO2_Node_Allocator * allocator)
{
	// should just point at dummy now :
	SPSC_FIFO2_Node * dummy = LoadAcquire(&(fifo->m_head));
	RL_ASSERT( dummy == LoadAcquire(&(fifo->m_tail)) );
	RL_ASSERT( dummy == LoadAcquire(&(fifo->m_first)) );
	allocator->Free(dummy);
	//rrSetZero(fifo);
}

inline SPSC_FIFO2_Node * SPSC_FIFO2_Producer_PopUsed(SPSC_FIFO2 volatile * fifo,SPSC_FIFO2_Node_Allocator * allocator)
{
	SPSC_FIFO2_Node * node = NULL;
	
	SPSC_FIFO2_Node * first = LoadRelaxed(&fifo->m_first);
	SPSC_FIFO2_Node * head = LoadAcquire(&fifo->m_head); // sync with Consumer
		
	for(;;)
	{
		if ( first == head )	
			break;
		SPSC_FIFO2_Node * next = LoadRelaxed(&first->m_next);
		if ( node == NULL )
			node = first;
		else
			allocator->Free(first);
		
		first = next;
	}
	
	StoreRelaxed(&fifo->m_first,first);
		
	return node;
}

inline void SPSC_FIFO2_PushData(SPSC_FIFO2 volatile * fifo,SPSC_FIFO2_Node_Allocator * allocator,void * data)
{
	// Producer_Pop before I try to push :
	//	this way my mem use is limited by the number of nodes actually pending in the fifo
	SPSC_FIFO2_Node * node = SPSC_FIFO2_Producer_PopUsed(fifo,allocator);
	if ( node == NULL )
	{
		// didn't pop one, push :
		node = allocator->Alloc();
	}
	
	// regular old push now :
	StoreRelaxed(&(node->m_data), data );
	StoreRelaxed(&(node->m_next), (SPSC_FIFO2_Node *)NULL);
	
	//ReadWriteBarrier();
	//SPSC_FIFO2_Node * back = LoadAcquire(&(fifo->m_tail));
	SPSC_FIFO2_Node * back = LoadRelaxed(&(fifo->m_tail));
	RL_ASSERT( LoadRelaxed(&(back->m_next)) == NULL );
	
	StoreRelease(&(back->m_next),node);
	
	StoreRelaxed(&(fifo->m_tail),node);
}

inline bool SPSC_FIFO2_IsEmpty(/*const*/ SPSC_FIFO2 volatile * fifo)
{
	// @@ do I need to do anything more careful here ?
	SPSC_FIFO2_Node * front = LoadAcquire(&(fifo->m_head));
	if( front == LoadAcquire(&(fifo->m_tail)) )
		return true;
		
	return false;
}

inline void * SPSC_FIFO2_PopData(SPSC_FIFO2 volatile * fifo)
{
	SPSC_FIFO2_Node * head = LoadRelaxed(&(fifo->m_head));
	SPSC_FIFO2_Node * next = LoadAcquire(&(head->m_next));
	if( next == NULL )
		return NULL;

	void * data = LoadRelaxed(&(next->m_data));

	// #LoadStore barrier
	//	I must read m_data before I set m_head = next
	// on x86 loadstore is free :
	CompilerReadWriteBarrier();

	// in Relacy I don't know how to place a #LoadStore ?
	SequentialFence();

	StoreRelease(&(fifo->m_head),next);
	
	return data;
}


// Flush : Pop out remaining nodes and free them
//	(generally do this before Close)
inline void SPSC_FIFO2_Flush(SPSC_FIFO2 volatile * fifo,SPSC_FIFO2_Node_Allocator * allocator)
{
	while ( SPSC_FIFO2_PopData(fifo) )
	{
	}
	
	SPSC_FIFO2_Node * node = SPSC_FIFO2_Producer_PopUsed(fifo,allocator);
	if ( node )
	{
		allocator->Free(node);
	}	
}

//=================================================================================

END_CB

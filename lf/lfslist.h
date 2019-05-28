#pragma once

#ifndef RL_CONTEXT_HPP	
#define THREAD_RELACY
#include "cblib/Threading.h"
//#include "cblib/LF/NodeAllocator.h"
#include "cblib/LF/cblibRelacy.h"
#endif

//=================================================================================

/*****

Lock Free Singly Linked List (LIFO = Stack)

Simple standard Intrusive push/pop with CAS

Using a pop count to solve ABA

requires pointers to be 32 bit because we use 64-bit CAS for {head,count}
	(if pointers are 64 bit we need DCAS)

I'm not using the VC2005 + volatile
I do use volatile to make the compiler not fuck me

NOTEZ : LFSNodes must not be freed immediately !!  Freed Nodes may be still dereffed for a little while.
This is intentional and safe, because the CAS will fail and the operations done from the freed memory will
be tossed.

-> the best solution is to always use a recycling Pool for LFSNode allocation


The standard Push & Pop loop until they succeed which is what you usually want
That can cause bad contention in heavy use cases
So you may want to use the _NoLoop variants and do something better when you get Abort

***********/

START_CB

struct LFSNode
{
	ATOMIC_VAR(LFSNode *)	m_next;
	//int			pad;
};

/*
struct LFSNode_And_Count
{
	LFSNode *	p;
	//int			c;
	intptr_t	c;
};

inline bool operator == (const LFSNode_And_Count & lhs,const LFSNode_And_Count &rhs)
{
	return (lhs.p == rhs.p && lhs.c == rhs.c );
}
*/

//#define LFSNode_And_Count	AtomicPointerAndCount

struct LFSNode_And_Count : public AtomicPointerAndCount
{
	LFSNode_And_Count() { }
	~LFSNode_And_Count() { }
	LFSNode_And_Count(LFSNode * p,uint16 c) : AtomicPointerAndCount(p,c) { }

	LFSNode *	p() const { return (LFSNode *) GetPointer(); }
	uint16		c() const { return GetCount(); }
};

#define LFNC_SIZE	AtomicPointerAndCountSize

#ifdef RL_CONTEXT_HPP
inline std::ostream& operator << (std::ostream& s, LFSNode_And_Count const& right)
{
    return s << "{" << right.p() << "," << right.c() << "}";
}
#endif

//LF_ALIGN_8 
__declspec(align(LFNC_SIZE))
struct LFSList
{
	// must be 8 bytes in a row :
	ATOMIC_VAR(LFSNode_And_Count)	m_head;
	
	// (*) you usually want cache line padding here
};

#ifndef RL_CONTEXT_HPP
COMPILER_ASSERT( sizeof(LFSNode_And_Count) == LFNC_SIZE );
COMPILER_ASSERT( sizeof(LFSList) == LFNC_SIZE );
#endif

// Init : single threaded ; setting struct = { 0 }; is fine.
inline void LFSList_Open(LFSList * volatile pList)
{
	//LFSNode_And_Count nc = { 0 };
	LFSNode_And_Count nc(0,0);
	StoreRelaxed(&(pList->m_head),nc);
}

inline void LFSList_Close(LFSList * volatile pList)
{
	// no release needed, but stomp on it for debug :
	//memset(pList,0xCD,sizeof(LFSList));
	pList;
}

inline LFResult LFSList_Push_NoLoop(LFSList volatile * pList,LFSNode * pNode)
{
	// @@ we could actually just work on head pointer here, not the 64-bit {p,c}
	// but that's forbidden by Relacy and C++0x
	// I'm not sure it really matters since if c changed, then head changed too

	//LFSNode_And_Count localHead = LoadAcquire(&(pList->m_head));
	LFSNode_And_Count localHead = LoadRelaxed(&(pList->m_head));
		
	StoreRelaxed(&(pNode->m_next), localHead.p()); // relaxed store
	//StoreRelease(&(pNode->m_next), localHead.p); // relaxed store
	
	// note - the list can be in a totally invalid state here wrt localHead/pNode
	// but if it is, then the CAS will fail
	// (localHead can be freed memory for example)
	// I own pNode, but other people could still be pointing at
	// eg. if pNode was just popped off the list and recycled through the allocator,
	//  other people could still have a localHead == pNode
	
	// no count change
	LFSNode_And_Count newHead(pNode,localHead.c());
	//newHead.p = pNode;
	//newHead.c = localHead.c; 
	
	if ( AtomicCAS(&(pList->m_head),localHead,newHead) )
	{
	
		RL_ASSERT( localHead.p() != pNode );
	
		return LF_Success;
	}
	else
	{
		return LF_Abort;
	}
}

// LFSList_Push_GetPrev returns previous head
inline LFSNode * LFSList_Push_GetPrev(LFSList volatile * pList,LFSNode * pNode)
{
	//SpinBackOff backoff;
	rl::backoff bo;
			
	LFSNode_And_Count localHead = LoadAcquire(&(pList->m_head));	
		
	for(;;)
	{
		StoreRelaxed(&(pNode->m_next), localHead.p()); // relaxed store
		//StoreRelease(&(pNode->m_next), localHead.p); // relaxed store
				
		LFSNode_And_Count newHead(pNode,localHead.c());
		//newHead.p = pNode;
		//newHead.c = localHead.c; // no count change
	
		LFSNode_And_Count oldHead = AtomicCMPX(&(pList->m_head),localHead,newHead);
		
		if ( oldHead == localHead )
			return oldHead.p();
		// else retry
		
		localHead = oldHead;
		
		//backoff.BackOffYield();
		bo.yield($);
	}
}

inline void LFSList_Push(LFSList volatile * pList,LFSNode * pNode)
{	
	/*
	SpinBackOff backoff;
	while( LFSList_Push_NoLoop(pList,pNode) == LF_Abort )
	{			
		backoff.BackOffYield();
	}
	*/
	
	LFSList_Push_GetPrev(pList,pNode);
}

inline bool LFSList_IsEmpty(LFSList volatile * pList)
{
	LFSNode_And_Count head = LoadAcquire(&(pList->m_head));
	
	return ( head.p() == NULL );
}

inline LFResult LFSList_Pop_NoLoop(LFSList volatile * pList,LFSNode ** pNode)
{
	LFSNode_And_Count oldList = LoadAcquire(&(pList->m_head));
	
	if ( oldList.p() == NULL )
	{
		*pNode = NULL;
		return LF_Empty;
	}
		
	LFSNode * pOldNext = LoadRelaxed(&(oldList.p()->m_next));
	LFSNode_And_Count newList(pOldNext,oldList.c()+1); 
	//newList.p = LoadRelaxed(&(oldList.p->m_next));
	//newList.c = oldList.c + 1;
		
	// DCAS :
	if ( AtomicCAS(&(pList->m_head),oldList,newList) )
	{
		RL_ASSERT( newList.p() != oldList.p() );
	
		//localHead->m_next = NULL;
		*pNode = oldList.p();
		return LF_Success;
	}
	else
	{
		return LF_Abort;	
	}
}

/*
inline LFSNode * LFSList_Pop(LFSList volatile * pList)
{
	SpinBackOff backoff;
	for(;;)
	{
		LFSNode * node = NULL;
		LFResult result = LFSList_Pop_NoLoop(pList,&node);
		if ( result != LF_Abort )
			return node;
			
		backoff.BackOffYield();
	}
}
*/

inline LFSNode * LFSList_Pop(LFSList volatile * pList)
{
	SpinBackOff backoff;
	
	LFSNode_And_Count oldList = LoadAcquire(&(pList->m_head));
	
	for(;;)
	{
		if ( oldList.p() == NULL )
		{
			return NULL;
		}
			
		LFSNode * pOldNext = LoadRelaxed(&(oldList.p()->m_next));
		LFSNode_And_Count newList(pOldNext,oldList.c()+1); 
		//newList.p = LoadRelaxed(&(oldList.p->m_next));
		//newList.c = oldList.c + 1;
			
		// DCAS :
		LFSNode_And_Count gotList = AtomicCMPX(&(pList->m_head),oldList,newList);
		if ( gotList == oldList )
		{
			RL_ASSERT( newList.p() != oldList.p() );
		
			return oldList.p();
		}
		
		oldList = gotList;
	}
}

//=====================================================================

// simple MPSC by reversal

struct LFSNode_NonAtomic
{
	NOT_THREAD_SAFE_ATOMIC(LFSNode_NonAtomic *)	m_next;
};

struct MPSC_FIFO
{
	// shared MPMC stack :
	LF_ALIGN_TO_CACHE_LINE LFSList	m_list;
	// single consumer owns FIFO list :
	//LF_ALIGN_TO_CACHE_LINE LFSNode * m_pFIFO;
	LF_ALIGN_TO_CACHE_LINE LFSNode_NonAtomic m_fifo;
};

inline void MPSC_FIFO_Open(MPSC_FIFO * pFIFO)
{
	LFSList_Open(&(pFIFO->m_list));
	pFIFO->m_fifo.m_next($) = (LFSNode_NonAtomic *)NULL;
}

inline void MPSC_FIFO_Close(MPSC_FIFO * pFIFO)
{
	pFIFO;
}

inline bool MPSC_FIFO_IsEmpty(MPSC_FIFO * pFIFO)
{
	if ( pFIFO->m_fifo.m_next($) != NULL ) return false;
	return LFSList_IsEmpty(&(pFIFO->m_list));
}

inline void MPSC_FIFO_Push(MPSC_FIFO * pFIFO, LFSNode * pNode)
{
	LFSList_Push(&(pFIFO->m_list),pNode);
}

// Fetch : grab the shared LIFO stack to my local FIFO list :
inline bool MPSC_FIFO_Fetch(MPSC_FIFO * pFIFO)
{
	// try to grab whole slist :
	LFSNode_And_Count nc = LoadRelaxed(&(pFIFO->m_list.m_head));
	nc.SetPointer(NULL);
	// make a big jump in count so don't have to CAS
	nc.SetCount( nc.GetCount() + 0xABAD );
	LFSNode_And_Count list = AtomicExchange(&(pFIFO->m_list.m_head),nc);

	// I own list now and need no more atomic ops

	LFSNode_NonAtomic * stack = (LFSNode_NonAtomic *) list.p();
	if ( stack == NULL )
		return false; // nothing to pop
	
	// put stack on the end of existing list
	// first find the end of existing list :
	//  (normally this is empty so it's fast)
	LFSNode_NonAtomic * pLast = &(pFIFO->m_fifo);
	while( pLast->m_next($) != NULL )
	{
		pLast = pLast->m_next($);
	}
		
	// reverse the list into FIFO :
	while ( stack )
	{
		LFSNode_NonAtomic * next = stack->m_next($);
		
		stack->m_next($) = pLast->m_next($);
		pLast->m_next($) = stack;
		
		stack = next;
	}
	
	ASSERT( pFIFO->m_fifo.m_next($) != NULL );
	
	return true;
}

inline LFSNode * MPSC_FIFO_Pop(MPSC_FIFO * pFIFO)
{
	if ( pFIFO->m_fifo.m_next($) == NULL )
	{
		// no fifo, try to grab whole slist :

		if ( ! MPSC_FIFO_Fetch(pFIFO) )
			return NULL;
	}
	
	ASSERT( pFIFO->m_fifo.m_next($) != NULL );
	
	// pop off fifo :
	
	LFSNode_NonAtomic * pRet = pFIFO->m_fifo.m_next($);
	pFIFO->m_fifo.m_next($) = pRet->m_next($);
	
	return (LFSNode *) pRet;
}

// normal single-list list removal :
//	call after Fetch
inline void MPSC_FIFO_Remove(MPSC_FIFO * pFIFO, LFSNode * pRemove)
{
	LFSNode_NonAtomic * pLast = &(pFIFO->m_fifo);
	while ( pLast->m_next($) != NULL )
	{
		if ( pLast->m_next($) == (LFSNode_NonAtomic *)pRemove )
		{
			// cut me 
			pLast->m_next($) = pLast->m_next($)->m_next($);
			break;
		}
		else
		{
			pLast = pLast->m_next($);
		}
	}
}

//=====================================================================

END_CB

#if 0

//=================================================================================

struct LFSNode_And_Int : public LFSNode
{
	void *	m_data;
	
	// (*) you usually want cache line padding here
};


typedef SELECTED_ALLOCATOR<LFSNode_And_Int> LFSNode_And_Int_Allocator;


inline void * LFSList_PopData(LFSList volatile * pList,LFSNode_And_Int_Allocator * allocator)
{
	LFSNode_And_Int * pNode = (LFSNode_And_Int *) LFSList_Pop(pList);
	if ( pNode == NULL )
		return NULL;
	void * i = pNode->m_data;
	allocator->Free(pNode);
	
	// @@ something fucked up here	
	//LFSNode_And_Count localHead = pList->m_head.debug_value();
	//RL_ASSERT( localHead.p != pNode );
	
	return i;
}

inline void LFSList_PushData(LFSList volatile * pList,LFSNode_And_Int_Allocator * allocator,void * i)
{
	LFSNode_And_Int * pNode = allocator->Alloc();

	// @@ something fucked up here	
	//LFSNode_And_Count localHead = pList->m_head.debug_value();
	//RL_ASSERT( localHead.p != pNode );
	
	pNode->m_data = i;
	LFSList_Push(pList,pNode);
}

//=================================================================================

#endif
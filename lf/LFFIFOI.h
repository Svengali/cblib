#pragma once

#define THREAD_RELACY
//#include "cblib/Threading.h"
//#include "cblib/LF/NodeAllocator.h"
	
START_CB	
	
/*****************

MPMC_FIFOI : (non-intrusive)

fully lock-free multi-producer multi-consumer FIFOI

Nodes must not be freed immediately or you deref bad
(MPMC_FIFOI_Node_Free handles this in a shitty way - it keeps them around forever)

I'm not convinced this is any better than the two-lock method
  cuz during contention it just loops really badly

Relies heavy on CAS64 with an ABA counter thing

*****************/

LF_ALIGN_8 struct MPMC_FIFOI_Node
{
	ATOMIC_VAR(MPMC_FIFOI_Node *)	m_next;
};
	
#define MPMC_FIFOI_END	((MPMC_FIFOI_Node *)(-1))
//---------------------------------------------------------

//LF_ALIGN_8 
struct MPMC_FIFOI_Node_And_Count
{
	MPMC_FIFOI_Node *	m_ptr;
	LONG				m_count;
};

inline bool operator == ( const MPMC_FIFOI_Node_And_Count & lhs, const MPMC_FIFOI_Node_And_Count & rhs )
{
	return lhs.m_ptr == rhs.m_ptr && lhs.m_count == rhs.m_count;
}

//---------------------------------------------------------

#ifdef RL_CONTEXT_HPP
inline std::ostream& operator << (std::ostream& s, MPMC_FIFOI_Node_And_Count const& right)
{
    return s << "{" << right.m_ptr << "," << right.m_count << "}";
}
#endif

LF_ALIGN_16 struct MPMC_FIFOI
{
	ATOMIC_VAR(MPMC_FIFOI_Node_And_Count)	m_head; // head is owned by the consumer
	
	char		pad1[LF_CACHE_LINE_SIZE - sizeof(MPMC_FIFOI_Node_And_Count)];
	
	ATOMIC_VAR(MPMC_FIFOI_Node_And_Count)	m_tail;	// tail is owned by the producer

	char		pad2[LF_CACHE_LINE_SIZE - sizeof(MPMC_FIFOI_Node_And_Count)];
	
	MPMC_FIFOI_Node m_dummy;
	
	char		pad3[LF_CACHE_LINE_SIZE - sizeof(MPMC_FIFOI_Node)];
};
		
inline void MPMC_FIFOI_Open(MPMC_FIFOI * FIFOI)
{
	MPMC_FIFOI_Node * dummy = &(FIFOI->m_dummy);
	StoreRelaxed(&(dummy->m_next),MPMC_FIFOI_END);
		
	LF_ALIGN_8 MPMC_FIFOI_Node_And_Count null;
	null.m_ptr = dummy;
	null.m_count = 0;
	
	StoreRelaxed(&(FIFOI->m_head),null);
	StoreRelease(&(FIFOI->m_tail),null);
}

//bool MPMC_FIFOI_IsEmpty(/*const*/ MPMC_FIFOI volatile * FIFOI);

// generally Flush before Close or assert( IsEmpty() )
inline void MPMC_FIFOI_Close(MPMC_FIFOI * FIFOI)
{
	FIFOI;
	// should just point at dummy now :
	//RL_ASSERT(  MPMC_FIFOI_IsEmpty(FIFOI) );

}

/**

this is basically

	tail->next = node
	tail = node
	
but we have to ensure that tail is actually the last
because it can get mucked with as we touch

it's a double-check and lazy update

	m_tail is either pointing at the last or the one before the last
	
if its the one before the last then m_tail->next is not END

**/
inline void MPMC_FIFOI_Push(MPMC_FIFOI volatile * FIFOI,MPMC_FIFOI_Node * node)
{	
	StoreRelease(&(node->m_next),MPMC_FIFOI_END);
	
	SpinBackOff backoff;
	
	// tail & tailCount must be read together :
	MPMC_FIFOI_Node_And_Count tail = LoadAcquire(&(FIFOI->m_tail));
		
	for(;;)
	{
		// we can get swapped out here and tail could get popped and deleted, so tail->m_next is unsafe
			
		// make tail->next point at the new node if it was pointing at END		
		//MPMC_FIFOI_Node * oldTailNext = MPMC_FIFOI_END;
		//bool xchg = tail.m_ptr->m_next($).compare_exchange(oldTailNext,node,rl::memory_order_acq_rel);
		//RL_ASSERT( xchg || oldTailNext != MPMC_FIFOI_END );
		
		// this only need memory_order_acq_rel , not seq_cst :
		MPMC_FIFOI_Node * oldTailNext = AtomicCMPX( &(tail.m_ptr->m_next), MPMC_FIFOI_END, node );
			
		if ( oldTailNext == MPMC_FIFOI_END ) // CMPX succeeded
		{
			// got it, adance tail :
			MPMC_FIFOI_Node_And_Count newTail;
			newTail.m_ptr = node;
			newTail.m_count = tail.m_count + 1;
			
			RL_ASSERT( newTail.m_ptr != MPMC_FIFOI_END );
			
			//FIFOI->m_tail($).compare_exchange(tail,newTail,rl::memory_order_seq_cst);
			//AtomicCAS(&(FIFOI->m_tail),tail,newTail);
			AtomicCAS(&(FIFOI->m_tail),tail,newTail);

			return;
		}
		else // CMPX failed
		{		
			// tail was not the last, fix :
			// try to advance tail : (we don't care if this succeeds or not)
			//AtomicCAS64_2((U64 volatile *)&FIFOI->m_tail, (U32)(UINTa)tail,tailCount, (U32)(UINTa)oldTailNext,tailCount+1);

			MPMC_FIFOI_Node_And_Count newTail;
			newTail.m_ptr = oldTailNext;
			newTail.m_count = tail.m_count + 1;
			
			RL_ASSERT( newTail.m_ptr != MPMC_FIFOI_END );
			
			//FIFOI->m_tail($).compare_exchange(tail,newTail,rl::memory_order_seq_cst);
			//AtomicCAS(&(FIFOI->m_tail),tail,newTail);
			tail = AtomicCMPX(&(FIFOI->m_tail),tail,newTail);
		}
		
		backoff.BackOffYield();
	}
}

/*
inline void MPMC_FIFOI_Push(MPMC_FIFOI volatile * FIFOI,MPMC_FIFOI_Node * node)
{
	rl::backoff backoff;
	
	while( MPMC_FIFOI_Push_NoLoop(FIFOI,node) == LF_Abort )
	{
		backoff.BackOffYield();
	}
}
*/

#if 0
inline LFResult MPMC_FIFOI_Pop_NoLoop(MPMC_FIFOI volatile * FIFOI,MPMC_FIFOI_Node ** pNode)
{
	// does the order matter here ? yes, it does
	MPMC_FIFOI_Node_And_Count head = LoadAcquire(&(FIFOI->m_head));	
	MPMC_FIFOI_Node_And_Count tail = LoadAcquire(&(FIFOI->m_tail));
	MPMC_FIFOI_Node * next = LoadAcquire(&(head.m_ptr->m_next));
	
	// if head count changed while we were reading, try again :
	// @@ WTF ?
	MPMC_FIFOI_Node_And_Count head2 = LoadAcquire(&(FIFOI->m_head));
	if ( head.m_count != head2.m_count )
		return LF_Abort;
	//if ( headCount != LoadAcquire(&(FIFOI->m_headCount)) )
	//	return LF_Abort;
	
	if ( head.m_ptr == tail.m_ptr )
	{
		if ( next == MPMC_FIFOI_END )
		{
			*pNode = NULL;
			return LF_Empty; // empty
		}
		else
		{
			// try to advance tail :
			
			MPMC_FIFOI_Node_And_Count oldTail;
			oldTail.m_ptr = tail.m_ptr; // head ptr = tail ptr
			oldTail.m_count = tail.m_count;
			
			MPMC_FIFOI_Node_And_Count newTail;
			newTail.m_ptr = next;
			newTail.m_count = tail.m_count + 1;
		
			RL_ASSERT( newTail.m_ptr != MPMC_FIFOI_END );
		
			AtomicCAS(&(FIFOI->m_tail),oldTail,newTail);
			//FIFOI->m_tail($).compare_exchange(oldTail,newTail,rl::memory_order_seq_cst);
			//AtomicCAS64((U64 volatile *)&FIFOI->m_tail, MPMC_FIFOI_Node_And_Count_As_64(oldTail), MPMC_FIFOI_Node_And_Count_As_64(newTail));
			return LF_Abort;
		}
	}
	else if ( next != MPMC_FIFOI_END )
	{
		// advance head :
		MPMC_FIFOI_Node_And_Count newHead;
		newHead.m_ptr = next;
		newHead.m_count = head.m_count + 1;
			
		//if ( AtomicCAS64((U64 volatile *)&FIFOI->m_head, MPMC_FIFOI_Node_And_Count_As_64(head), MPMC_FIFOI_Node_And_Count_As_64(newHead)) )
		//if ( FIFOI->m_head($).compare_exchange(head,newHead,rl::memory_order_seq_cst) )
		if ( AtomicCAS(&(FIFOI->m_head),head,newHead) )
		{
			// got it

			if ( head.m_ptr == &(FIFOI->m_dummy) )
			{
				// popped the dummy, put it back :
				MPMC_FIFOI_Push(FIFOI,head.m_ptr);

				return LF_Abort;
			}
			else
			{
				*pNode = head.m_ptr;
				return LF_Success; // !! yay
			}
		}
		else
		{
			return LF_Abort;
		}
	}
	else
	{
		// next == END but head != tail
		//	something is amiss
		// loop around and it should get fixed
		
		return LF_Abort;
	}
}

inline MPMC_FIFOI_Node * MPMC_FIFOI_Pop(MPMC_FIFOI volatile * FIFOI)
{
	SpinBackOff backoff;
	MPMC_FIFOI_Node * node = NULL;
	while ( MPMC_FIFOI_Pop_NoLoop(FIFOI,&node) == LF_Abort )
	{
		backoff.BackOffYield();	
	}
	return node;
}
#endif


inline MPMC_FIFOI_Node * MPMC_FIFOI_Pop(MPMC_FIFOI volatile * FIFOI)
{
	SpinBackOff backoff;
	for(;;)
	{
		// does the order matter here ? yes, it does
		MPMC_FIFOI_Node_And_Count head = LoadAcquire(&(FIFOI->m_head));	
		MPMC_FIFOI_Node_And_Count tail = LoadAcquire(&(FIFOI->m_tail));
		MPMC_FIFOI_Node * next = LoadAcquire(&(head.m_ptr->m_next));
		
		// if head count changed while we were reading, try again :
		// @@ WTF ?
		MPMC_FIFOI_Node_And_Count head2 = LoadAcquire(&(FIFOI->m_head));
		if ( head.m_count != head2.m_count )
			continue;

		//if ( headCount != LoadAcquire(&(FIFOI->m_headCount)) )
		//	return LF_Abort;
		
		if ( head.m_ptr == tail.m_ptr )
		{
			if ( next == MPMC_FIFOI_END )
			{
				return NULL; // empty
			}
			else
			{
				// try to advance tail :
				
				MPMC_FIFOI_Node_And_Count oldTail;
				oldTail.m_ptr = tail.m_ptr; // head ptr = tail ptr
				oldTail.m_count = tail.m_count;
				
				MPMC_FIFOI_Node_And_Count newTail;
				newTail.m_ptr = next;
				newTail.m_count = tail.m_count + 1;
			
				RL_ASSERT( newTail.m_ptr != MPMC_FIFOI_END );
			
				AtomicCAS(&(FIFOI->m_tail),oldTail,newTail);
			}
		}
		else if ( next != MPMC_FIFOI_END )
		{
			// advance head :
			MPMC_FIFOI_Node_And_Count newHead;
			newHead.m_ptr = next;
			newHead.m_count = head.m_count + 1;
				
			//if ( AtomicCAS64((U64 volatile *)&FIFOI->m_head, MPMC_FIFOI_Node_And_Count_As_64(head), MPMC_FIFOI_Node_And_Count_As_64(newHead)) )
			//if ( FIFOI->m_head($).compare_exchange(head,newHead,rl::memory_order_seq_cst) )
			if ( AtomicCAS(&(FIFOI->m_head),head,newHead) )
			{
				// got it

				if ( head.m_ptr == &(FIFOI->m_dummy) )
				{
					// popped the dummy, put it back :
					MPMC_FIFOI_Push(FIFOI,head.m_ptr);
				}
				else
				{
					return head.m_ptr;
				}
			}
			else
			{
			}
		}
		else
		{
			// next == END but head != tail
			//	something is amiss
			// loop around and it should get fixed
		}
	
		backoff.BackOffYield();	
	}
}


/*
// @@ CB : IS THIS OKAY ?
//	I have little confidence in my IsEmpty
inline bool MPMC_FIFOI_IsEmpty( MPMC_FIFOI volatile * FIFOI)
{
	MPMC_FIFOI_Node_And_Count head = LoadAcquire(&(FIFOI->m_head));
	MPMC_FIFOI_Node * next = LoadAcquire(&(head.m_ptr->m_next));
	MPMC_FIFOI_Node_And_Count tail = LoadAcquire(&(FIFOI->m_tail));
	
	if ( head.m_ptr == tail.m_ptr )
	{
		if ( next == MPMC_FIFOI_END )
		{
			// we can have head = tail and != dummy when we're in
			//	that state that Pop() has taken off the dummy but
			//	hasn't yet put it back on
			if ( head.m_ptr == &(FIFOI->m_dummy) )
			{
				return true; // empty
			}
		}
		// else contended state, just return false and try again later
	}
	
	return false;
}
*/

//=================================================================================
#if 0

struct MPMC_FIFOI_Node_Void : public MPMC_FIFOI_Node
{
	NOT_THREAD_SAFE(void *)			m_data;
	
	// (*) note : you may want to pad nodes up to cache_line size too
};

typedef SELECTED_ALLOCATOR<MPMC_FIFOI_Node_Void> MPMC_FIFOI_Node_Allocator;

//=================================================================================

inline void MPMC_FIFOI_PushData(MPMC_FIFOI volatile * FIFOI,MPMC_FIFOI_Node_Allocator * allocator,void * data)
{
	MPMC_FIFOI_Node_Void * node = allocator->Alloc();
	// @@ note : data race here if Allocator does not have fences !
	StoreRelaxed( &(node->m_data), data );
	MPMC_FIFOI_Push(FIFOI,node);
}

inline void * MPMC_FIFOI_PopData(MPMC_FIFOI volatile * FIFOI,MPMC_FIFOI_Node_Allocator * allocator)
{
	MPMC_FIFOI_Node_Void * node = (MPMC_FIFOI_Node_Void *) MPMC_FIFOI_Pop(FIFOI);
	if ( ! node )
		return NULL;
	void * ret = LoadRelaxed( &(node->m_data) );
	// @@ note : data race here if Allocator does not have fences !
	allocator->Free(node);
	return ret;
}

// Flush : Pop out remaining nodes and free them
//	(generally do this before Close)
inline void MPMC_FIFOI_Flush(MPMC_FIFOI volatile * FIFOI,MPMC_FIFOI_Node_Allocator * allocator)
{
	while ( MPMC_FIFOI_Node_Void * node = (MPMC_FIFOI_Node_Void *) MPMC_FIFOI_Pop(FIFOI) )
	{
		allocator->Free(node);
	}
}

//=================================================================================
#endif

END_CB

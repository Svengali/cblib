#pragma once

#define THREAD_RELACY
#include "cblib/Threading.h"
#include "cblib/LF/NodeAllocator.h"

START_CB

//=================================================================================

LF_ALIGN_8 struct SPSC_FIFO_Node
{
	ATOMIC_VAR(SPSC_FIFO_Node *)	m_next;
	NOT_THREAD_SAFE(void *)			m_data;
};

/*
bool operator == (const SPSC_FIFO_Node & lhs, const SPSC_FIFO_Node & rhs)
{
	return (lhs.m_next($) == rhs.m_next($) && lhs.m_data($) == rhs.m_data($) );
}
*/

typedef SELECTED_ALLOCATOR<SPSC_FIFO_Node> SPSC_FIFO_Node_Allocator;

LF_ALIGN_8 struct SPSC_FIFO
{
	ATOMIC_VAR(SPSC_FIFO_Node *)	m_head; // head is owned by the consumer
	
	char	pad1[LF_CACHE_LINE_SIZE - sizeof(SPSC_FIFO_Node *)];
	
	ATOMIC_VAR(SPSC_FIFO_Node *)	m_tail;	// tail is owned by the producer
	
	char	pad2[LF_CACHE_LINE_SIZE - sizeof(SPSC_FIFO_Node *)];
};

inline void SPSC_FIFO_Open(SPSC_FIFO * fifo,SPSC_FIFO_Node_Allocator * allocator)
{
	SPSC_FIFO_Node * dummy = allocator->Alloc();
	StoreRelaxed(&(dummy->m_next), (SPSC_FIFO_Node *)NULL);
	
	StoreRelaxed(&(fifo->m_head), dummy);
	StoreRelaxed(&(fifo->m_tail), dummy);
}

// generally Flush before Close or assert( IsEmpty() )
inline void SPSC_FIFO_Close(SPSC_FIFO * fifo,SPSC_FIFO_Node_Allocator * allocator)
{
	// should just point at dummy now :
	SPSC_FIFO_Node * dummy = LoadAcquire(&(fifo->m_head));
	RL_ASSERT( dummy == LoadAcquire(&(fifo->m_tail)) );
	allocator->Free(dummy);
	//rrSetZero(fifo);
}

inline void SPSC_FIFO_Push(SPSC_FIFO volatile * fifo,SPSC_FIFO_Node * node)
{
	StoreRelaxed(&(node->m_next), (SPSC_FIFO_Node *)NULL);
	
	//ReadWriteBarrier();
	//SPSC_FIFO_Node * back = LoadAcquire(&(fifo->m_tail));
	SPSC_FIFO_Node * back = LoadRelaxed(&(fifo->m_tail));
	RL_ASSERT( LoadRelaxed(&(back->m_next)) == NULL );
	
	// @@ ensure node->data is flushed before we expose node to the consumer in back->m_next
	// ; sfence may be needed on future x86
	StoreRelease(&(back->m_next),node);
	
	StoreRelaxed(&(fifo->m_tail),node);
}

/*

AC_SYS_APIEXPORT void AC_CDECL 
ac_i686_queue_spsc_push
( ac_i686_queue_spsc_t*,
  ac_i686_node_t* );

align 16
ac_i686_queue_spsc_push PROC
  mov eax, [esp + 4]  // eax = queue
  mov ecx, [esp + 8]  // ecx = node
  mov edx, [eax + 4]  // edx = queue->back
  ; sfence may be needed on future x86
  mov [edx], ecx      // back->next = node
  mov [eax + 4], ecx  // queue->back = node
  ret
ac_i686_queue_spsc_push ENDP
*/

inline bool SPSC_FIFO_IsEmpty(/*const*/ SPSC_FIFO volatile * fifo)
{
	// @@ do I need to do anything more careful here ?
	SPSC_FIFO_Node * front = LoadAcquire(&(fifo->m_head));
	if( front == LoadAcquire(&(fifo->m_tail)) )
		return true;
		
	return false;
}

inline SPSC_FIFO_Node * SPSC_FIFO_Pop(SPSC_FIFO volatile * fifo)
{
	SPSC_FIFO_Node * front = LoadRelaxed(&(fifo->m_head));
	SPSC_FIFO_Node * next = LoadAcquire(&(front->m_next));
	if( next == NULL )
		return NULL;

	void * data = LoadRelaxed(&(next->m_data));

	//StoreRelease(&(fifo->m_head),next);
	StoreRelaxed(&(fifo->m_head),next);
	
	// front is now totally owned by me, so no worries :
	StoreRelaxed(&(front->m_data),data);
	return front;
}

/*

AC_SYS_APIEXPORT ac_i686_node_t* AC_CDECL 
ac_i686_queue_spsc_pop
( ac_i686_queue_spsc_t* );


align 16
ac_i686_queue_spsc_pop PROC
  push ebx
  mov ecx, [esp + 8]  // ecx = queue
  mov eax, [ecx]      // eax = front
  cmp eax, [ecx + 4]  // front == back ?
  je ac_i686_queue_spsc_pop_failed
  mov edx, [eax]      // edx = front->next
  ; lfence may be needed on future x86
  mov ebx, [edx + 12] // ebx = next->data
  mov [ecx], edx      // queue->front = edx (front->next)
  mov [eax + 12], ebx // front->data = ebx
  pop ebx
  ret				  // return front

ac_i686_queue_spsc_pop_failed:
  xor eax, eax          // return null
  pop ebx
  ret
ac_i686_queue_spsc_pop ENDP

*/


inline void SPSC_FIFO_PushData(SPSC_FIFO volatile * fifo,SPSC_FIFO_Node_Allocator * allocator,void * data)
{
	SPSC_FIFO_Node * node = allocator->Alloc();
	// @@ note : data race here if Allocator does not have fences !
	StoreRelaxed(&(node->m_data),data);
	SPSC_FIFO_Push(fifo,node);
}

inline void * SPSC_FIFO_PopData(SPSC_FIFO volatile * fifo,SPSC_FIFO_Node_Allocator * allocator)
{
	SPSC_FIFO_Node * node = SPSC_FIFO_Pop(fifo);
	if ( ! node )
		return NULL;
	void * ret = LoadRelaxed(&(node->m_data));
	// @@ note : data race here if Allocator does not have fences !
	allocator->Free(node);
	return ret;
}

// Flush : Pop out remaining nodes and free them
//	(generally do this before Close)
inline void SPSC_FIFO_Flush(SPSC_FIFO volatile * fifo,SPSC_FIFO_Node_Allocator * allocator)
{
	while ( SPSC_FIFO_Node * node = SPSC_FIFO_Pop(fifo) )
	{
		allocator->Free(node);
	}
}

//=================================================================================

END_CB

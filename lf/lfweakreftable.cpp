#include "lfweakreftable.h"
#include "cblibLF.h"

// configurable :
//#define handle_guid_bits		(handle_bits/2)
#define handle_guid_bits		(17)

// derived :
#define handle_refcount_bits	(handle_bits - handle_guid_bits)
#define handle_index_bits		handle_refcount_bits
#define handle_guid_shift		handle_refcount_bits
#define handle_bit_mask(nbits)	((((handle_type)1)<<(nbits))-1)

//=================================================================	

static inline handle_type make_handle( handle_type guid, handle_type index )
{
	LF_OS_ASSERT( guid <= handle_bit_mask(handle_guid_bits) );
	LF_OS_ASSERT( index <= handle_bit_mask(handle_index_bits) );
	return ((guid)<<handle_guid_shift) | (index);
}

static inline handle_type handle_get_guid( handle_type h )
{
	return (h >> handle_guid_shift);
}

static inline handle_type handle_get_index( handle_type h )
{
	return h & handle_bit_mask(handle_index_bits);
}

//=================================================================	


namespace ReferenceTable
{
	enum { c_num_slots = 4096 };

	// refcount must be in the low bits, because we just use ++ and -- to modify it
	// guid must be at the top of the word because we want to use ++ on it and have it wrap
	static inline handle_type make_state( handle_type guid, handle_type refcount )
	{
		//LF_OS_ASSERT( guid <= handle_bit_mask(handle_guid_bits) ); // guid wraps and that's fine
		LF_OS_ASSERT( refcount <= handle_bit_mask(handle_refcount_bits) );
		return ((guid)<<handle_guid_shift) | (refcount);
	}

	static inline handle_type state_get_guid( handle_type h )
	{
		return (h >> handle_guid_shift);
	}

	static inline handle_type state_get_refcount( handle_type h )
	{
		return h & handle_bit_mask(handle_refcount_bits);
	}

	static const handle_type state_max_refcount = handle_bit_mask(handle_refcount_bits);

	struct Slot
	{
		// m_ptr must be first; it is overlapped with m_next of slist_node
		Referable *		m_ptr;
		atomic<handle_type>	m_state;
	};

	// lf_slist_node must lie on top of m_ptr and not touch m_state :
	LF_OS_COMPILER_ASSERT( sizeof(lf_slist_node) == sizeof(void *) );

	Slot	s_slots[c_num_slots];

	lf_mpmc_lifo s_freelist;
	
	//-------------------------
	
	// Init is not thread-safe, must be done at cinit or from main before threads are started
	void Init()
	{
		// validate the slots and push them all on the freelist :
		
		// start guid at 1, so that 0 is a null handle :
		handle_type init_state = make_state(1,0);
		
		// build up the list nonatomically, then swap the whole thing in :
		lf_slist_node_nonatomic head;
		lf_slist_node_nonatomic * last = &head;
		// walk the array with a big stride to avoid false sharing :
		int stride = 16;
		for(int base=0;base<stride;base++)
		{
			for(int i=base;i<c_num_slots;i+=stride)
			{
				Slot * s = &s_slots[i];
				s->m_state.store( init_state, mo_relaxed );
				// s->m_ptr overlaps with cur->m_next
				lf_slist_node_nonatomic * cur = (lf_slist_node_nonatomic *) s;
				last->m_next = cur;
				last = cur;
			}
		}
		last->m_next = NULL;
		// freelist provides the release barrier to publish s_slots :
		lf_slist_node * xchg = s_freelist.swap( (lf_slist_node *) head.m_next($) );
		xchg;
		LF_OS_ASSERT( xchg == NULL );
	}
	
	//-------------------------

	// Alloc gives Referable a refcount of 0
	handle_type Alloc( Referable * ptr )
	{
		lf_slist_node * n = s_freelist.pop();
		if ( n == NULL )
		{
			// out of slots; very bad
			return 0;
		}
		// n is a slot :
		Slot * s = (Slot *) n;
		ptrdiff_t tindex = s - s_slots;
		LF_OS_ASSERT( tindex >= 0 && tindex < c_num_slots );
		handle_type index = (handle_type) tindex;
		handle_type state = s->m_state.load(mo_relaxed);
		// guid was inc'ed on free; noone else can be touching us
		handle_type guid = state_get_guid(state);
		LF_OS_ASSERT( state_get_refcount(state) == 0 );
		handle_type handle = make_handle(guid,index);
		// s->m_ptr was used for the freelist, so it's crap; fill it now
		s->m_ptr = ptr;
		// inc refcount : (NO)
		// state ++;
		LF_OS_ASSERT( state_get_refcount(state) == 0 );
		// release to publish s->m_ptr :
		s->m_state.store(state,mo_release);
		return handle;
	}

	void FreeSlot( Slot * s )
	{
		// guid is already inc'ed and refcount is 0
 		//  but slot is not yet in the freelist
		//	so nobody can touch this slot but me
		Referable * ptr = s->m_ptr;
		if ( ptr )
		{
			s->m_ptr = NULL;
			delete ptr;
		}
		// Slot is cast to node for freelist :
		lf_slist_node * n = (lf_slist_node *) s;
		s_freelist.push(n);
	}

	// IncRef looks up the weak reference; returns null if lost
	Referable * IncRef( handle_type h )
	{
		handle_type index = handle_get_index(h);
		LF_OS_ASSERT( index >= 0 && index < c_num_slots );
		Slot * s = &s_slots[index];

		handle_type guid = handle_get_guid(h);

		handle_type state = s->m_state.load(mo_acquire);
		for(;;)
		{
			if ( state_get_guid(state) != guid )
				return NULL;
			// assert refcount isn't hitting max
			LF_OS_ASSERT( state_get_refcount(state) < state_max_refcount );
			handle_type incstate = state+1;
			if ( s->m_state.compare_exchange_weak(state,incstate,mo_acq_rel,mo_acquire) )
			{
				// did the ref inc
				return s->m_ptr;
			}
			// state was reloaded, loop
		}
	}

	// IncRefRelaxed can be used when you know a ref is held
	//	so there's no chance of the object being gone
	void IncRefRelaxed( handle_type h )
	{
		handle_type index = handle_get_index(h);
		LF_OS_ASSERT( index >= 0 && index < c_num_slots );
		Slot * s = &s_slots[index];
		
		handle_type state_prev = s->m_state.fetch_add(1,mo_relaxed);
		state_prev;
		// make sure we were used correctly :
		LF_OS_ASSERT( handle_get_guid(h) == state_get_guid(state_prev) );
		LF_OS_ASSERT( state_get_refcount(state_prev) >= 0 );
		LF_OS_ASSERT( state_get_refcount(state_prev) < state_max_refcount );
	}
	
	#if 0
	// DecRef
	void DecRef( handle_type h )
	{
		handle_type index = handle_get_index(h);
		LF_OS_ASSERT( index >= 0 && index < c_num_slots );
		Slot * s = &s_slots[index];
		
		// no need to check guid because I must own a ref
		//handle_type state_prev = s->m_state($).fetch_add((handle_type)-1,mo_relaxed);
		handle_type state_prev = s->m_state($).fetch_add((handle_type)-1,mo_release);
		LF_OS_ASSERT( handle_get_guid(h) == state_get_guid(state_prev) );
		LF_OS_ASSERT( state_get_refcount(state_prev) >= 1 );

		if ( state_get_refcount(state_prev) == 1 )
		{
			// I took refcount to 0
			//	slot is not actually freed yet; someone else could IncRef right now
			//  the slot becomes inaccessible to weak refs when I inc guid :
			// try to inc guid with refcount at 0 :
			handle_type old_guid  = handle_get_guid(h);
			handle_type old_state = state_prev-1; // == make_state(old_guid,0);
			LF_OS_ASSERT( old_state == make_state(old_guid,0) );
			handle_type new_state = make_state(old_guid+1,0); // == old_state + (1<<handle_guid_shift);

			if ( s->m_state($).compare_exchange_strong(old_state,new_state,mo_acq_rel,mo_relaxed) )
			{
				// I released the slot
				// cmpx provides the acquire barrier for the free :
				FreeSlot(s);
				return;
			}
			// somebody else mucked with me
		}
	}
	#else
	// DecRef
	void DecRef( handle_type h )
	{
		handle_type index = handle_get_index(h);
		LF_OS_ASSERT( index >= 0 && index < c_num_slots );
		Slot * s = &s_slots[index];
		
		// no need to check guid because I must own a ref
		handle_type state_prev = s->m_state($).load(mo_relaxed);
		
		handle_type old_guid  = handle_get_guid(h);

		for(;;)
		{
			// I haven't done my dec yet, guid must still match :
			LF_OS_ASSERT( state_get_guid(state_prev) == old_guid );
			// check current refcount :
			handle_type state_prev_rc = state_get_refcount(state_prev);
			LF_OS_ASSERT( state_prev_rc >= 1 );
			if ( state_prev_rc == 1 )
			{
				// I'm taking refcount to 0
				// also inc guid, which releases the slot :
				handle_type new_state = make_state(old_guid+1,0);

				if ( s->m_state($).compare_exchange_weak(state_prev,new_state,mo_acq_rel,mo_relaxed) )
				{
					// I released the slot
					// cmpx provides the acquire barrier for the free :
					FreeSlot(s);
					return;
				}
			}
			else
			{
				// this is just a decrement
				// but have to do it as a CAS to ensure state_prev_rc doesn't change on us
				handle_type new_state = state_prev-1;
				LF_OS_ASSERT( new_state == make_state(old_guid,  state_prev_rc-1) );
				
				if ( s->m_state($).compare_exchange_weak(state_prev,new_state,mo_release,mo_relaxed) )
				{
					// I dec'ed a ref
					return;
				}
			}
		}
	}
	#endif

	//-------------------------
};

Referable::Referable()
{
	m_self = ReferenceTable::Alloc(this);
	LF_OS_ASSERT( m_self != 0 );
}
	
//=================================================================	

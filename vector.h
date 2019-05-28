#pragma once

/*********************

"vector" and "vector_s"

cb::vector look-alike

there are two leaf-level classes in here;
	"vector" and "vector_s"
the latter is a static-allocation version of vector

these are built with a templated vector_flex which takes
a templated entry and storage mechanism; vector_flex then
derives from vector_base (just for encapsulation of the
conversation with the storage mechanism).

There are then two storage mechanisms defined here :
vector_storage and vector_storage_static
This is sort of like a better version of the std allocator.
It's better because the storage policy owns the pointer, so it can
do anything to it any time, and it can have state.

---------------------------------------------------

There's a big issue with inlining here;  cb::vector heavily
inlines everything it does.  This results in good speed at the
cost of tons of code.  I let my vector member functions be
non-inline, which makes less code, but shows up slower in benchmarks.
Hard to say what's best.

---------------------------------------------------

vector uses StlAlloc and StlFree

there could be some merit to using an allocator that tells you the size you got

*************************/

#include "cblib/Base.h"
#include "cblib/vector_flex.h"
#include "cblib/stl_basics.h"
#include "cblib/Util.h"

// you can define CB_VECTOR_MAX_GROW_BYTES yourself before including vector.h if you like
#ifndef CB_VECTOR_MAX_GROW_BYTES
#define CB_VECTOR_MAX_GROW_BYTES	(8*1024*1024)
#endif CB_VECTOR_MAX_GROW_BYTES

START_CB

#pragma pack(push)
#pragma pack(4)

#pragma warning(push)
#pragma warning(disable : 4328)
 
//typedef ptrdiff_t vecsize_t;
typedef int vecsize_t;

//}{=======================================================================================
// vector_storage

template <typename t_entry,typename t_sizetype> class vector_storage
{
public:

	__forceinline vector_storage()
	{
		init();
	}

	__forceinline ~vector_storage()
	{
		release();
	}

	void release()
	{
		StlFree(m_begin,m_capacity*sizeof(t_entry));
		//CBFREE(m_begin);
		init();
	}
 
 
	void swap(vector_storage<t_entry,t_sizetype> & other,const t_sizetype maxsize)
	{
		Swap(m_begin,other.m_begin);
		Swap(m_capacity,other.m_capacity);
	}
 
	//-----------------------------------------
	// simple accessors :

	t_entry *			begin()				{ return m_begin; }
	const t_entry *		begin() const		{ return m_begin; }
	t_sizetype			capacity() const	{ return m_capacity; }
	t_sizetype			max_size() const	{ return (32UL)<<20; }

	//-------------------------------------------------------

	__forceinline bool needmakefit(const t_sizetype newsize) const
	{
		return (newsize > m_capacity);
	}

	// makefit1
	// returns the *old* pointer for passing into makefit2
	//
	t_entry * makefit1(const t_sizetype newsize,const t_sizetype oldsize)
	{
		ASSERT( needmakefit(newsize) );

		t_entry * pOld = m_begin;

		// Be much more careful about growing the memory conservatively.  This changes
		//  push_back from amortized O(N) to O(N^2), but results in tighter vectors.

		enum { c_maxGrowBytes = CB_VECTOR_MAX_GROW_BYTES };	// 1 MB
		enum { c_maxGrowCount = c_maxGrowBytes/sizeof(t_entry) };
		COMPILER_ASSERT( c_maxGrowCount > 1 );
		// c_maxGrowCount should limit the doubling, but not the passed in newsize
		
		// m_capacity is 0 the first time we're called
		// newsize can be passed in from reserve() so don't put a +1 on it

		// grow by doubling until we get to max grow count, then grow linearly
		t_sizetype newcapacity = MIN( m_capacity * 2 , (m_capacity + c_maxGrowCount) );
		newcapacity = MAX( newcapacity, newsize );

		// if on constant should optimize out
		if ( sizeof(t_entry) == 1 )
		{
			/*
			// @@ make better use of pages ?
			//	really should let the allocator tell me this
			if ( newcapacity >= 1024 )
			{
				// round up newcapacity to be a multiple of 4096
				newcapacity = (newcapacity+4095)&(~4095);
			}
			else
			*/
			
			{
				// round up newcapacity to be a multiple of 8
				newcapacity = (newcapacity+7)&(~7);
			}
		}
		else
		{
			t_sizetype newbytes = newcapacity * sizeof(t_entry);
			
			if ( newbytes > 65536 )
			{
				// align newbytes up :
				newbytes = (newbytes + 65535) & (~65535);
				// align newcapacity down :
				newcapacity = newbytes / sizeof(t_entry);
			}
			else
			{
				if ( newbytes < 512 )
				{
					// don't touch
				}
				else
				{
					// align up to 4096 :
					newbytes = (newbytes + 4095) & (~4095);
					newcapacity = newbytes / sizeof(t_entry);
				}
			}
		}

		ASSERT( newcapacity >= newsize );			
		
		t_entry * pNew = (t_entry *) StlAlloc( newcapacity * sizeof(t_entry) );

		ASSERT_RELEASE_THROW( pNew != NULL );

		// copy existing :
		entry_array::copy_construct(pNew,pOld,oldsize);
		// @@ new : swap existing !
		//	this requires entries to have a default constructor !!
		//entry_array::swap_construct(pNew,pOld,oldsize);

		m_begin = pNew;
		m_capacity = newcapacity;
		// m_size not changed

		return pOld;
	}

	void makefit2(t_entry * pOld, const t_sizetype oldsize, const t_sizetype oldcapacity)
	{
		if ( pOld )
		{
			entry_array::destruct(pOld,oldsize);

			StlFree(pOld,oldcapacity*sizeof(t_entry));
		}
	}

	//-------------------------------------------------------

private:
	t_entry	*	m_begin;
	t_sizetype	m_capacity;	// how many allocated

	void init()
	{
		m_begin = NULL;
		m_capacity = 0;
	}
};

#pragma pack(pop)
#pragma warning(pop)

//}{=======================================================================================
// vector

template <class t_entry> class vector : public vector_flex<t_entry,vector_storage<t_entry,vecsize_t>,vecsize_t >
{
public:
	//----------------------------------------------------------------------
	//typedef parent_type::size_type;				size_type;
	//using typename parent_type::size_type;
	typedef vector<t_entry>						this_type;
	typedef vector_flex<t_entry,vector_storage<t_entry,vecsize_t>,vecsize_t >	parent_type;

	//----------------------------------------------------------------------
	// constructors

	 vector() { }
	~vector() { }

	// I don't have the normal (t_sizetype) constructor, just this (t_sizetype,value) constructor for clarity
	vector(const size_type size,const t_entry & init) : parent_type(size,init)
	{
	}

	vector(const this_type & other) : parent_type(other)
	{
	}

	template <class input_iterator>
	vector(const input_iterator first,const input_iterator last)
		: parent_type(first,last)
	{
	}
};

//}{=======================================================================================

END_CB

START_CB
// partial specialize swap_functor to all vectors
//	for cb::Swap

template<class t_entry> 
struct swap_functor< cb::vector<t_entry> >
{
	void operator () ( cb::vector<t_entry> & _Left, cb::vector<t_entry> & _Right)
	{
		_Left.swap(_Right);
	}
};

END_CB

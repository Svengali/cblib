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

*************************/

#include "cblib/Base.h"
#include "cblib/vector_flex.h"
#include "cblib/stl_basics.h"
#include "cblib/Util.h"

START_CB

//}{=======================================================================================
// vector_storage

template <class t_entry> class vector_storage
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

	void swap(vector_storage<t_entry> & other,const int )
	{
		Swap(m_capacity,other.m_capacity);
		Swap(m_begin,other.m_begin);
	}

	//-----------------------------------------
	// simple accessors :

	t_entry *			begin()				{ return m_begin; }
	const t_entry *		begin() const		{ return m_begin; }
	int					capacity() const	{ return m_capacity; }
	int					max_size() const	{ return (32UL)<<20; }

	//-------------------------------------------------------

	__forceinline bool needmakefit(const int newsize) const
	{
		return (newsize > m_capacity);
	}

	// makefit1
	// returns the *old* pointer for passing into makefit2
	//
	t_entry * makefit1(const int newsize,const int oldsize)
	{
		ASSERT( needmakefit(newsize) );

		t_entry * pOld = m_begin;

		// On _XBOX, be much more careful about growing the memory conservatively.  This changes
		//  push_back from amortized O(N) to O(N^2), but results in tighter vectors.  On Win32,
		//  use the STL behavior of size doubling, which is O(N).
#ifdef _XBOX
		// growing
		int newcapacity;
		t_entry * pNew;

		// if on constant should optimize out
		if ( sizeof(t_entry) == 1 )
		{
			int extra = m_capacity * 2 - newsize;
			if ( extra < 0 )
			{
				// round up newcapacity to be a multiple of 8
				newcapacity = (newsize+7)&(~7);
			}
			else if ( extra >= 2048 )
			{
				// round up newcapacity to be a multiple of 4096
				newcapacity = (newsize+4095)&(~4095);
			}
			else
			{
				// round up newcapacity to be a multiple of 8
				newcapacity = (newsize+extra+7)&(~7);
			}

			ASSERT( newcapacity >= newsize );
			pNew = (t_entry *) StlAlloc( newcapacity );
		}
		else
		{
			enum { c_maxGrowBytes = 32768 };
		
			int extra = m_capacity * 2 - newsize;
			if ( extra < 0 )
			{
				newcapacity = newsize;
			}
			else if ( (extra * sizeof(t_entry)) < c_maxGrowBytes )
			{
				newcapacity = m_capacity * 2; // = newsize + extra
				if ( (newcapacity * sizeof(t_entry)) > 512 )
				{
					// align up to 4096 :
					int newbytes = (newcapacity * sizeof(t_entry) + 4095) & (~4095);
					newcapacity = (newbytes + sizeof(t_entry)-1) / sizeof(t_entry);
				}
			}
			else
			{
				int newbytes = (newsize * sizeof(t_entry) + c_maxGrowBytes-1) & (~(c_maxGrowBytes-1));
				newcapacity = (newbytes + sizeof(t_entry)-1) / sizeof(t_entry);
			}

			ASSERT( newcapacity >= newsize );			
			pNew = (t_entry *) StlAlloc( newcapacity * sizeof(t_entry) );
		}
#else
		// growing
		int newcapacity = MAX( m_capacity * 2, (newsize + 1) );
		t_entry * pNew;

		// if on constant should optimize out
		if ( sizeof(t_entry) == 1 )
		{
			// round up newcapacity to be a multiple of 8
			newcapacity = (newcapacity+7)&(~7);

			pNew = (t_entry *) StlAlloc( newcapacity );
		}
		else
		{
			pNew = (t_entry *) StlAlloc( newcapacity * sizeof(t_entry) );
		}
#endif // _XBOX

		ASSERT_RELEASE_THROW( pNew );

		// copy existing :
		entry_array::copy_construct(pNew,pOld,oldsize);

		m_begin = pNew;
		m_capacity = newcapacity;
		// m_size not changed

		return pOld;
	}

	void makefit2(t_entry * pOld, const int oldsize, const int oldcapacity)
	{
		if ( pOld )
		{
			entry_array::destruct(pOld,oldsize);
			//CBFREE(pOld);
			StlFree(pOld,oldcapacity*sizeof(t_entry));
		}
	}

	//-------------------------------------------------------

private:
	t_entry	*	m_begin;
	int			m_capacity;	// how many allocated

	void init()
	{
		m_begin = NULL;
		m_capacity = 0;
	}
};

//}{=======================================================================================
// vector

//template<typename T>
//using vector = std::vector<T>;

typedef std::size_t size_type;

//*
template <class t_entry> 
class vector 
	: 
	public vector_flex<t_entry,vector_storage<t_entry>>
{
public:
	//----------------------------------------------------------------------
	typedef vector<t_entry>                                this_type;
	typedef vector_flex<t_entry,vector_storage<t_entry> >  parent_type;

	//----------------------------------------------------------------------
	// constructors

	 vector() { }
	~vector() { }

	// I don't have the normal (size_t) constructor, just this (size_t,value) constructor for clarity
	vector(
		const size_type size,
		const t_entry & init) 
		: 
		parent_type(size,init)
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
//*/

//}{=======================================================================================

END_CB

// @@ how do you do this ?
/*
#include "cblib/stl_basics.h"

CB_STL_BEGIN

//template<class t_entry> 
template<> inline
void swap<cb::vector<t_entry> >(cb::vector<t_entry> & _Left, cb::vector<t_entry> & _Right)
{	// exchange values stored at _Left and _Right
	_Left.Swap(&_Right);
}

CB_STL_END
/**/

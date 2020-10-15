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

#include "vector_flex.h"

START_CB

//}{=======================================================================================
// vector_storage_static

typedef int vector_s_size_t;

template <class t_entry,vector_s_size_t t_capacity> class vector_storage_static
{
public:
	typedef vector_storage_static<t_entry,t_capacity>		this_type;

	vector_storage_static()
	{
	}

	~vector_storage_static()
	{
	}

	void swap(this_type & other,const vector_s_size_t maxsize)
	{
		// static arrays must do full member-wise swaps
		//	much slower than non-static swap implementation
		entry_array::swap_array(begin(),other.begin(),maxsize);
	}

	void release()
	{
	}

	//-----------------------------------------
	// simple accessors :

	t_entry *			begin()			{ return (t_entry *) m_data; }
	const t_entry *		begin() const	{ return (const t_entry *) m_data; }
	vector_s_size_t		capacity() const{ return t_capacity; }
	vector_s_size_t			max_size() const{ return t_capacity; }
	//-------------------------------------------------------
	// makefit does nada on vector_storage_static

	__forceinline bool needmakefit(const vector_s_size_t newsize) const
	{
		ASSERT( newsize <= t_capacity );
		return false;
	}

	__forceinline t_entry * makefit1(const vector_s_size_t newsize,const vector_s_size_t oldsize)
	{
		ASSERT( newsize <= t_capacity );
		return NULL;
	}

	__forceinline void makefit2(t_entry * pOld, const vector_s_size_t oldsize, const vector_s_size_t oldcapacity)
	{
		FAIL("makefit2 should never be called on vector_storage_static!");
	}

	//-------------------------------------------------------

private:
	char		m_data[ sizeof(t_entry) * t_capacity ];
};

//}{=======================================================================================
// vector_s

template <class t_entry,vector_s_size_t t_capacity> class vector_s : public vector_flex<t_entry,vector_storage_static<t_entry,t_capacity>,vector_s_size_t >
{
public:
	//----------------------------------------------------------------------
	typedef vector_s<t_entry,t_capacity>						this_type;
	typedef vector_flex<t_entry,vector_storage_static<t_entry,t_capacity>,vector_s_size_t >	parent_type;

	//----------------------------------------------------------------------
	// constructors

	__forceinline  vector_s() { }
	__forceinline ~vector_s() { }

	/* Removed because of ambiguity
	__forceinline explicit vector_s(const size_type size) : parent_type(size)
	{
	}
	*/

	__forceinline vector_s(const this_type & other) : parent_type(other)
	{
	}

	template <class input_iterator>
	__forceinline vector_s(const input_iterator first,const input_iterator last)
		: parent_type(first,last)
	{
	}

	//----------------------------------------------------------------------
};
//}{=======================================================================================

END_CB

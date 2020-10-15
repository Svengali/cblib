#pragma once

/*********************


*************************/

#include "vector_flex.h"

START_CB

//}{=======================================================================================
// you must call vector_arena_provide() to set up the memory before making a vector_a()

void vector_arena_provide(void *pData,size_t bytes);
void vector_arena_acquire(void ** pData,size_t * pSize,size_t elemSize);

typedef int vector_a_size_t;

//}{=======================================================================================
// vector_storage_a

template <class t_entry> class vector_storage_a
{
public:
	typedef vector_storage_a<t_entry>		this_type;

	vector_storage_a()
	{
		vector_arena_acquire(&m_data,&m_capacity,sizeof(t_entry));
	}

	~vector_storage_a()
	{
	}

	void swap(this_type & other,const vector_a_size_t maxsize)
	{
		// static arrays must do full member-wise swaps
		//	much slower than non-static swap implementation
		entry_array::swap(begin(),other.begin(),maxsize);
	}

	void release()
	{
	}

	//-----------------------------------------
	// simple accessors :

	t_entry *			begin()			{ return (t_entry *) m_data; }
	const t_entry *		begin() const	{ return (const t_entry *) m_data; }
	vector_a_size_t		capacity() const{ return m_capacity; }
	vector_a_size_t		max_size() const{ return m_capacity; }
	//-------------------------------------------------------
	// makefit does nada on vector_storage_a

	__forceinline bool needmakefit(const vector_a_size_t newsize) const
	{
		ASSERT( newsize <= m_capacity );
		return false;
	}

	__forceinline t_entry * makefit1(const vector_a_size_t newsize,const vector_a_size_t oldsize)
	{
		ASSERT( newsize <= m_capacity );
		return NULL;
	}

	__forceinline void makefit2(t_entry * pOld, const vector_a_size_t oldsize, const vector_a_size_t oldcapacity)
	{
		FAIL("makefit2 should never be called on vector_storage_a!");
	}

	//-------------------------------------------------------

private:
	void *	m_data;
	vector_a_size_t	m_capacity;
};

//}{=======================================================================================
// vector_a

template <class t_entry> class vector_a : public vector_flex<t_entry,vector_storage_a<t_entry>,vector_a_size_t >
{
public:
	//----------------------------------------------------------------------
	typedef vector_a<t_entry>								this_type;
	typedef vector_flex<t_entry,vector_storage_a<t_entry>,vector_a_size_t >	parent_type;

	//----------------------------------------------------------------------
	// constructors

	__forceinline  vector_a() { }
	__forceinline ~vector_a() { }

	/* Removed because of ambiguity
	__forceinline explicit vector_a(const vector_a_size_t size) : parent_type(size)
	{
	}
	*/

	__forceinline vector_a(const this_type & other) : parent_type(other)
	{
	}

	template <class input_iterator>
	__forceinline vector_a(const input_iterator first,const input_iterator last)
		: parent_type(first,last)
	{
	}

	//----------------------------------------------------------------------
};
//}{=======================================================================================

END_CB

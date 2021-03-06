#pragma once

/*********************

"vector_t"

vector_t is a "tight" vector, which means size is equal to capacity at all times

*************************/

#include "vector_flex.h"
#include "Util.h"

START_CB

//}{=======================================================================================
// vector_storage_tight

template <typename t_entry,typename t_sizetype> class vector_storage_tight
{
public:

	__forceinline vector_storage_tight() : m_begin(NULL)
	{
	}

	__forceinline ~vector_storage_tight()
	{
		release();
	}

	void release()
	{
		CBFREE(m_begin);
		m_begin = NULL;
	}

	void swap(vector_storage_tight<t_entry,t_sizetype> & other,const t_sizetype maxsize)
	{
		Swap(m_begin,other.m_begin);
	}

	//-----------------------------------------
	// simple accessors :

	t_entry *			begin()				{ return m_begin; }
	const t_entry *		begin() const		{ return m_begin; }
	t_sizetype			capacity() const	{ return 0; }
	t_sizetype			max_size() const	{ return (32UL)<<20; }

	//-------------------------------------------------------

	__forceinline bool needmakefit(const t_sizetype newsize) const
	{
		return true;
	}

	// makefit1
	// returns the *old* pointer for passing into makefit2
	//
	t_entry * makefit1(const t_sizetype newsize,const t_sizetype oldsize)
	{
		ASSERT( needmakefit(newsize) );

		if ( newsize == oldsize )
		{
			// no change needed
			return NULL;
		}

		t_entry * pOld = m_begin;

		// growing
		t_entry * pNew = (t_entry *) CBALLOC( newsize * sizeof(t_entry) );

		ASSERT( pNew );

		// copy existing :
		entry_array::copy_construct(pNew,pOld,oldsize);

		m_begin = pNew;
		// m_size not changed

		// pOld will be deleted later

		return pOld;
	}

	void makefit2(t_entry * pOld, const t_sizetype oldsize, const t_sizetype oldcapacity)
	{
		if ( pOld )
		{
			entry_array::destruct(pOld,oldsize);
			CBFREE(pOld);
		}
	}

	//-------------------------------------------------------

private:
	t_entry	*	m_begin;
};

//}{=======================================================================================
// vector

template <typename t_entry> class vector_t : public vector_flex<t_entry,vector_storage_tight<t_entry,vecsize_t>,vecsize_t >
{
public:
	//----------------------------------------------------------------------
	typedef vector_t<t_entry>						this_type;
	typedef vector_flex<t_entry,vector_storage_tight<t_entry,vecsize_t>,vecsize_t >	parent_type;

	//----------------------------------------------------------------------
	// constructors

	 vector_t() { }
	~vector_t() { }

	/* Removed because of ambiguity
	explicit vector(const size_type size) : parent_type(size)
	{
	}
	*/

	vector_t(const this_type & other) : parent_type(other)
	{
	}

	template <class input_iterator>
	vector_t(const input_iterator first,const input_iterator last)
		: parent_type(first,last)
	{
	}
	
	// don't allow reserve :
	void reserve(const vecsize_t newcap)
	{
		// silent no-op
		//FAIL_NOTHROW("no reserve on vector_t");
	}
};

//}{=======================================================================================

END_CB


START_CB
// partial specialize swap_functor to all vectors
//	for cb::Swap

template<class t_entry> 
struct swap_functor< cb::vector_t<t_entry> >
{
	void operator () ( cb::vector_t<t_entry> & _Left, cb::vector_t<t_entry> & _Right)
	{
		_Left.swap(_Right);
	}
};

END_CB

#pragma once

/*********************

vecsorted & multivecsorted

sorted vector
sits on top of an arbitrary underlying vector
implements fast find() by binary search

NO non-const iterators!!  you cannot modify data in-place,
it invalidates the sort order!

see notes about why you want this in the cpp file

*************************/

//! \todo - document this more !

#include "cblib/Base.h"
#include "cblib/equivalent_functor.h"
#include "cblib/vector.h"
#include <utility>
#include <algorithm>

START_CB

//=======================================================================================

namespace vecsorted_type
{
	enum e
	{
		multi,
		unique
	};
	
	class EMulti { };
	class EUnique{ };
	
};

namespace vecsorted_construct
{
	enum EConstructSorted { sorted };
	enum EConstructNonSorted { non_sorted };
	enum EConstructNonUnique { non_unique };
	enum EConstructSortedNonUnique { sorted_non_unique };
}

//=======================================================================================

template <typename t_vector,
	typename t_compare = std::less<t_vector::value_type>,
	int t_multi = 1 >
class vecsorted
{
public:
	//---------------------------------------------------------------------------

	// type definitions
	typedef typename t_vector::value_type		value_type;
	typedef typename t_vector::const_iterator	const_iterator;
	typedef typename t_vector::const_reference	const_reference;
	typedef typename t_vector::size_type			size_type;
	typedef typename t_compare                  t_compare;

	typedef vecsorted<t_vector,t_compare,t_multi> this_type;

	typedef std::pair<const_iterator,const_iterator> iterator_pair;

	//---------------------------------------------------------------------------
	// constructors

	vecsorted()
	{ }

	vecsorted(const this_type & other) : m_vector(other.m_vector)
	{
		ASSERT( is_valid() ); 
	}

	template <class input_iterator>
	vecsorted(const input_iterator first,const input_iterator last,const vecsorted_construct::EConstructNonSorted e) :
			m_vector(first,last)
	{
		std::sort(m_vector.begin(),m_vector.end(),m_compare);
		ASSERT( is_valid() );
	}
	
	template <class input_iterator>
	vecsorted(const input_iterator first,const input_iterator last,const vecsorted_construct::EConstructSorted e) :
			m_vector(first,last)
	{
		ASSERT( is_valid() );
	}

	template <class input_iterator>
	vecsorted(const input_iterator first,const input_iterator last,const vecsorted_construct::EConstructNonUnique e) :
			m_vector(first,last)
	{
		std::sort(m_vector.begin(),m_vector.end(),m_compare);
		if ( t_multi == vecsorted_type::unique )
		{
			typename t_vector::iterator itend = std::unique(m_vector.begin(),m_vector.end(),m_equivalent);
			m_vector.erase(itend,m_vector.end());
		}
		ASSERT( is_valid() );
	}

	template <class input_iterator>
	vecsorted(const input_iterator first,const input_iterator last,const vecsorted_construct::EConstructSortedNonUnique e) :
			m_vector(first,last)
	{
		ASSERT( is_sorted() );
		if ( t_multi == vecsorted_type::unique )
		{
			typename t_vector::iterator itend = std::unique(m_vector.begin(),m_vector.end(),m_equivalent);
			m_vector.erase(itend,m_vector.end());
		}
		ASSERT( is_valid() );
	}

	//---------------------------------------------------------------------------
	// simple accessors :

	// iterator support
	const_iterator	begin() const	{ return m_vector.begin(); }
	const_iterator	end() const		{ return m_vector.end(); }

	// at() with range check
	const_reference at(const size_type i) const	{ return m_vector.at(i); }

	// operator[]
	const_reference operator[](const size_type i) const { return m_vector.at(i); }

	// front() and back()
	const_reference front() const	{ return m_vector.front(); }
	const_reference back() const	{ return m_vector.back(); }

	// size is constant
	size_type	size() const		{ return m_vector.size(); }
	bool		empty() const		{ return m_vector.empty(); }
	size_type	capacity() const	{ return m_vector.capacity(); }

	void pop_back()					{ m_vector.pop_back(); }

	// no resize ; shrink only
	void shrink(const size_type n)
	{
		ASSERT( n <= size() );
		m_vector.resize(n);
	}

	void clear()					{ m_vector.clear(); }

	void reserve(const size_type n)	{ m_vector.reserve(n); }

	// tighten releases memory that's not in use
	void tighten()
	{
		t_vector other(m_vector);
		m_vector.swap(other);
	}

	// release acts just like clear(), but actually gets rid of the memory
	void release()
	{
		t_vector other;
		m_vector.swap(other);
	}

	// pair of iterators support
	iterator_pair beginend() const
	{
		return iterator_pair(begin(),end());
	}

	//---------------------------------------------------------------------------

	const_iterator	find(const value_type & val) const
	{
		const const_iterator b = begin();
		const const_iterator e = end();
		const const_iterator it = std::lower_bound(b,e,val,m_compare);
		
		if ( it == e )
			return e;

		// I know key is <= *it
		ASSERT( ! m_compare(*it,val) );
		// so, equivalent if ! m_compare(key,*it)
		if ( m_compare(val,*it) )
			return e;
		ASSERT( m_equivalent(*it,val) );

		return it;
	}

	template < class _Key, class _Val_Key_Pred, class _Key_Val_Pred >
	const_iterator	find_key(const _Key & val, _Val_Key_Pred vkp, _Key_Val_Pred kvp ) const
	{
		const const_iterator b = begin();
		const const_iterator e = end();
		const const_iterator it = std::lower_bound(b,e,val,vkp);
		
		if ( it == e )
			return e;

		// I know key is <= *it
		ASSERT( ! vkp(*it,val) );
		// so, equivalent if ! m_compare(key,*it)
		if ( kvp(val,*it) )
			return e;
		//ASSERT( m_equivalent(*it,val) );

		return it;
	}
	
	// findrange only makes sense for multi-type vecsorteds
	iterator_pair findrange(const value_type & val) const
	{
		return findrange_sub<t_multi>(val);
	}

	bool exists(const value_type & val) const
	{
		return std::binary_search(begin(),end(),val,m_compare);
	}

	void insert(const value_type & val)
	{
		insert_sub<t_multi>(val);
	}
	
	void push_back_sorted(const value_type & val)
	{
		// assert back() < val
		//	
		ASSERT( empty() || m_compare(back(),val) );
		m_vector.push_back(val);
	}

	void erase(const const_iterator cit)
	{
		if ( cit == end() ) // std container don't tolerate this, but I do
			return;
		// turn the const iterator into non-const on the parent :
		typename t_vector::iterator it = m_vector.begin() + (cit - begin());
		m_vector.erase(it);
	}

	void erase(const const_iterator cfirst,const const_iterator clast)
	{
		if ( cfirst == end() )
			return;
		// turn the const iterator into non-const on the parent :
		typename t_vector::iterator itfirst = m_vector.begin() + (cfirst - begin());
		typename t_vector::iterator itlast  = m_vector.begin() + (clast  - begin());
		m_vector.erase(itfirst,itlast);
	}

	// @@ I'm nore sure if I like this iterator-pair support
	void erase(const iterator_pair & itpair)
	{
		erase(itpair.first,itpair.second);
	}
	
	// erase by value to match std::map
	//	returns the number erased
	int erase(const value_type & val)
	{
		const iterator_pair itpair = findrange(val);
		const int count = itpair.second - itpair.first;
		erase(itpair.first,itpair.second);
		return count;
	}

	//---------------------------------------------------------------------------
	// validators :

	bool is_valid() const
	{
		if ( empty() )
			return true;
		if ( ! is_sorted() )
			return false;

		if ( t_multi == vecsorted_type::unique )
		{
			const size_type n = size();
			for(size_type i = 0;i<(n-1);i++)
			{
				if ( m_equivalent( at(i), at(i+1) ) )
					return false;
			}
		}

		return true;
	}

	bool is_sorted() const
	{
		const size_type n = size();
		for(size_type i = 0;i<(n-1);i++)
		{
			if ( m_compare( at(i+1), at(i) ) )
				return false;
		}
		return true;
	}

	const t_vector & get_vector() const
	{
		return m_vector;
	}
	// if you use get_vector_mutable, then you must manually ensure that it stays sorted!!
	//	you can use sort_vector if you like !!
	t_vector & get_vector_mutable()
	{
		return m_vector;
	}
	void sort_vector()
	{
		// manually sort :
		std::sort( m_vector.begin(), m_vector.end(), m_compare );
	}

	//---------------------------------------------------------------------------

private:
	template <int multi>
	void insert_sub(const value_type & val)
	{
		if( multi == vecsorted_type::multi )
		{
			const t_vector::iterator b = m_vector.begin();
			const t_vector::iterator e = m_vector.end();
			typename t_vector::iterator it = std::lower_bound(b,e,val,m_compare);
			
			// don't insert if found
			
			ASSERT( it == e || ! m_compare(*it,val) );

			if ( it != e && ! m_compare(val,*it) )
				return;

			ASSERT( it == e || ! m_equivalent(*it,val) );

			m_vector.insert(it,val);
		}
		else
		{
			insert_sub_unique( val );
		}
	}

	
	//template <>
	void insert_sub_unique(const value_type & val)
	{
		const t_vector::iterator b = m_vector.begin();
		const t_vector::iterator e = m_vector.end();
		typename t_vector::iterator it = std::lower_bound(b,e,val,m_compare);
		m_vector.insert(it,val);
	}

	/*
	template <int multi>
	void insert_sub_test(const value_type & val)
	{
		if( multi == vecsorted_type::multi )
		{
			const t_vector::iterator b = m_vector.begin();
			const t_vector::iterator e = m_vector.end();
			t_vector::iterator it = std::lower_bound(b,e,val,m_compare);
			
			// don't insert if found
			
			ASSERT( it == e || ! m_compare(*it,val) );

			if ( it != e && ! m_compare(val,*it) )
				return;

			ASSERT( it == e || ! m_equivalent(*it,val) );

			m_vector.insert(it,val);
		}
		else
		{
			insert_sub_unique( val );
		}
	}

	template <>
	void insert_sub_test<1>(const value_type & val)
	{
		const t_vector::iterator b = m_vector.begin();
		const t_vector::iterator e = m_vector.end();
		t_vector::iterator it = std::lower_bound(b,e,val,m_compare);
		m_vector.insert(it,val);
	}
	*/
	
	// @@ findrange is a compile error if you're not multi :
	//	?
	//*
	template <int multi>
	iterator_pair findrange_sub(const value_type & val) const
	{
		if( multi == vecsorted::unique )
		{
			const const_iterator e = end();
			const const_iterator it = find(val);

			if (it == e)
				return iterator_pair(e, e);

			ASSERT(it < e && ((it+1) == e || !m_equivalent(*(it+1),val)));

			return iterator_pair(it, it+1);
		}
		else
		{
			return findrange_sub_multi( val );
		}
	}
	/**/

	//template <>
	iterator_pair findrange_sub_multi(const value_type & val) const
	{
		const const_iterator b = begin();
		const const_iterator e = end();
		const const_iterator it = std::lower_bound(b,e,val,m_compare);
		
		if ( it == e )
			return iterator_pair(e,e);

		// I know val is <= *it
		ASSERT( ! m_compare(*it,val) );
		// so, equivalent if ! m_compare(key,*it)
		if ( m_compare(val,*it) )
			return iterator_pair(e,e);
		ASSERT( m_equivalent(*it,val) );

		const_iterator it_end = it+1;
		// I know val is <= *it_end
		while( it_end != e && ! m_compare(val,*it_end) )
		{
			ASSERT( m_equivalent(*it_end,val) );
			it_end++;
		}
		ASSERT( it_end == e || ! m_equivalent(*it_end,val) );

		return iterator_pair(it,it_end);
	}

	//---------------------------------------------------------------------------
	//	data :

	t_vector						m_vector;
	t_compare						m_compare;
	equivalent_functor<t_compare>	m_equivalent;

}; // vecsorted


//=======================================================================================

template <typename t_vector,
	typename t_compare = std::less<t_vector::value_type> >
class multivecsorted
	: 
	public vecsorted<t_vector,t_compare,vecsorted_type::multi>
{
public:
	typedef typename vecsorted<t_vector,t_compare,vecsorted_type::multi> parent_type;

	//---------------------------------------------------------------------------
	// must redefine constructors

	multivecsorted()
	{ }

	/*
	multivecsorted(const this_type & other) : parent_type(other)
	{
		ASSERT( is_valid() ); 
	}
	*/

	template <typename input_iterator>
	multivecsorted(const input_iterator first,const input_iterator last,const vecsorted_construct::EConstructNonSorted e) :
			parent_type(first,last,e)
	{;
	}
	
	template <typename input_iterator>
	multivecsorted(const input_iterator first,const input_iterator last,const vecsorted_construct::EConstructSorted e) :
			parent_type(first,last,e)
	{
	}

	//---------------------------------------------------------------------------

}; // multivecsorted
			
//=======================================================================================

END_CB

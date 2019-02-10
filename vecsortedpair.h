#pragma once

/*****************

vecsortedpair & multivecsortedpair

a vector of pairs, sorted by a functor on the pair *FIRST*
with helper functions for finding by first

like a map, with key + data

sits on top of any vector, just like vecsorted

declare like :

	vecsortedpair< vector< std::pair<int,int> > > vsp;

*****************/

//! \todo - document this more !

#include "cblib/Base.h"
#include "cblib/vecsorted.h"
#include "cblib/Util.h"
#include "stl_basics.h"
#include <algorithm>

START_CB

//=======================================================================================
// vecsortedpair


//! pair_key_bool_binary_functor
template<typename Pair, typename Functor>
struct pair_key_both_bool_binary_functor 
	: 
	public cb::binary_function<typename Pair,typename Pair::first_type,bool>
{
	bool operator()(const typename Pair & __x, const typename Pair & __y) const
	{
		return Functor()(__x.first,__y.first); 
	}

	bool operator()(const typename Pair & __x, const typename Pair::first_type & __y) const
	{
		return Functor()(__x.first,__y); 
	}

	bool operator()( const typename Pair::first_type & __x, const typename Pair & __y ) const
	{
		return Functor()(__x,__y.first); 
	}
};

template<typename Pair> 
struct pair_off_key_bool_binary_functor : public cb::binary_function< Pair, Pair, bool >
{
	bool operator()(const Pair & __x, const Pair & __y) const
	{
		return (__x.first < __y.first); 
	}
};

template <typename t_vector,
	typename t_comparefirst = std::less<typename t_vector::value_type::first_type>,
	vecsorted_type::e t_multi = vecsorted_type::unique >
class vecsortedpair
	:
	public vecsorted< t_vector, 
	pair_first_bool_binary_functor<typename t_vector::value_type,t_comparefirst> ,
	t_multi >
{
public:
	// lots of typedefs from parent class

	typedef typename t_vector::value_type		value_type;
	typedef typename t_vector::const_iterator	const_iterator;
	typedef typename t_vector::const_reference	const_reference;
	typedef typename t_vector::size_type			size_type;
	//typedef typename t_vector::t_compare		t_compare;

	typedef vecsortedpair<t_vector, t_comparefirst, t_multi> this_type;

	typedef std::pair<const_iterator, const_iterator> iterator_pair;

	typedef typename value_type::first_type		key_type;
	typedef typename value_type::second_type		data_type;

	typedef vecsorted< t_vector , 
				pair_first_bool_binary_functor<value_type,t_comparefirst> ,
				t_multi > parent_type;


	//---------------------------------------------------------------------------
	// all the basics are inherited,
	//	iterators, etc.
	
	//---------------------------------------------------------------------------

	const_iterator	find(const key_type & key) const
	{
		const const_iterator b = parent_type::begin();
		const const_iterator e = parent_type::end();

		const pair_key_both_bool_binary_functor< value_type, t_comparefirst > comparator;

		const const_iterator it = std::lower_bound(b,e,key, comparator);
		
		if ( it == e )
			return e;

		// I know key is <= *it
		ASSERT( ! m_comparekeys(it->first,key) );
		// so, equivalent if ! m_comparekeys(key,it->first)
		if ( m_comparekeys(key,it->first) )
			return e;
		ASSERT( m_equivalent(it->first,key) );
		
		return it;
	}

	// findrange only makes sense for multi-type vecsorteds
	iterator_pair findrange(const key_type & key) const
	{
		return findrange_sub(key, BoolToType<t_multi == vecsorted_type::unique>() );
	}

	bool exists(const key_type & key) const
	{
		// could be slightly more efficient ala binary_search;
		//	currently has redundant checks of "end"
		return ( find(key) != parent_type::end() );
	}

	void insert(const key_type & key,const data_type & data)
	{
		// this does an extra copy on the stack; could make the pair in-place in the vector
		//	in the final insertion location
		parent_type::insert( std::make_pair(key,data) );
	}
	// must include this insert wrapper because the added insert
	//	hides the parent's form :
	void insert(const value_type & val )
	{
		// causes errors in VC ?
		//parent_type::insert(val);
		((parent_type *)this)->insert(val);
	}

	//---------------------------------------------------------------------------
	// must duplicate all the damn constructors

	vecsortedpair()
	{ }

	vecsortedpair(const this_type & other) : parent_type(other)
	{ }

	template <typename input_iterator>
	vecsortedpair(const input_iterator first,const input_iterator last,const vecsorted_construct::EConstructNonSorted e) :
			parent_type(first,last,e)
	{ }
	
	template <typename input_iterator>
	vecsortedpair(const input_iterator first,const input_iterator last,const vecsorted_construct::EConstructSorted e):
			parent_type(first,last,e)
	{ }

	template <typename input_iterator>
	vecsortedpair(const input_iterator first,const input_iterator last,const vecsorted_construct::EConstructNonUnique e):
			parent_type(first,last,e)
	{ }

	template <typename input_iterator>
	vecsortedpair(const input_iterator first,const input_iterator last,const vecsorted_construct::EConstructSortedNonUnique e):
			parent_type(first,last,e)
	{ }

	//---------------------------------------------------------------------------

private:

	// @@ findrange is a compile error if you're not multi :
	//	?
	//*
	iterator_pair findrange_sub(const key_type & key, const BoolAsType_True & is_unique) const
	{
		const const_iterator it = find(key);
		return iterator_pair(it,it);
	}
	/**/

	iterator_pair findrange_sub(const key_type & key, const BoolAsType_False &) const
	{
		const const_iterator b = parent_type::begin();
		const const_iterator e = parent_type::end();
		const pair_key_both_bool_binary_functor< value_type , t_comparefirst > comparator;
		const const_iterator it = std::lower_bound(b,e,key,comparator);
		
		if ( it == e )
			return iterator_pair(e,e);

		// I know key is <= *it
		ASSERT( ! m_comparekeys(it->first,key) );
		// so, equivalent if ! m_comparekeys(key,it->first)
		if ( m_comparekeys(key,it->first) )
			return iterator_pair(e,e);
		ASSERT( m_equivalent(it->first,key) );

		const_iterator it_end = it+1;
		// I know val is <= *it_end
		while( it_end != e && ! m_comparekeys(key,it_end->first) )
		{
			ASSERT( m_equivalent(it_end->first,key) );
			it_end++;
		}
		ASSERT( it_end == e || ! m_equivalent(it_end->first,key) );

		return iterator_pair(it,it_end);
	}

	///////////////////////////////////////////////////////////////////////////
	// functors :

	t_comparefirst						m_comparekeys;
	equivalent_functor<t_comparefirst>	m_equivalent;
};


//=======================================================================================

template <typename t_vector,
	typename t_comparefirst = std::less<t_vector::value_type::first_type> >
	class multivecsortedpair : public vecsortedpair<t_vector,t_comparefirst,vecsorted_type::multi>
{
public:

	typedef vecsortedpair<t_vector,t_comparefirst,vecsorted_type::multi>	parent_type;
	typedef multivecsortedpair<t_vector,t_comparefirst> this_type;

	//---------------------------------------------------------------------------
	// must duplicate all the damn constructors

	multivecsortedpair()
	{ }

	multivecsortedpair(const this_type & other) : parent_type(other)
	{ }

	template <typename input_iterator>
	multivecsortedpair(const input_iterator first,const input_iterator last,const vecsorted_construct::EConstructNonSorted e) :
			parent_type(first,last,e)
	{ }
	
	template <typename input_iterator>
	multivecsortedpair(const input_iterator first,const input_iterator last,const vecsorted_construct::EConstructSorted e):
			parent_type(first,last,e)
	{ }

	template <typename input_iterator>
	multivecsortedpair(const input_iterator first,const input_iterator last,const vecsorted_construct::EConstructNonUnique e):
			parent_type(first,last,e)
	{ }

	template <typename input_iterator>
	multivecsortedpair(const input_iterator first,const input_iterator last,const vecsorted_construct::EConstructSortedNonUnique e):
			parent_type(first,last,e)
	{ }

};
			

//=======================================================================================

END_CB

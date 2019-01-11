#pragma once

// STL BASICS
//============================================================================

#include "cblib/Base.h"
//#include "cblib/Util.h" // need Util for min/max

//! I hack my STL to call StlAlloc & StlFree instead of some
//	damn ass allocator

// NOTEZ : YOU MUST INCLUDE "stl_basics.h" BEFORE ANY STL INCLUDE

START_CB

extern void * MyStlAlloc(size_t size);
extern void MyStlFree(void * ptr,size_t size);

template<typename _Arg1,
	typename _Arg2,
	typename _Result>
struct binary_function
{	// base class for binary functions
	typedef _Arg1 first_argument_type;
	typedef  _Arg2 second_argument_type;
	typedef  _Result result_type;
};


END_CB

#define StlAlloc(size)		NS_CB::MyStlAlloc(size)
#define StlFree(ptr,size)	NS_CB::MyStlFree(ptr,size)

// use my assert in STL :
#ifdef __STL_ASSERT
#undef __STL_ASSERT
#endif
// STL_ASSERT is defined to NOT need a semi-colon after it, so add one here :
#define __STL_ASSERT(ex)	ASSERT(ex);

//#undef new
//#undef delete

#pragma warning(push) //{ STL includes
#pragma warning(disable: 4702) //  unreachable code
#pragma warning(disable: 4245) //  signed/unsigned mismatch 
#pragma warning(disable: 4701) //  used without initialized

// something in the STL does an #undef new
//	so we make sure that our new define gets restored

#include <new>
#include <utility>
#include <algorithm> // std library for sort()
#include <functional>
#include <iterator>

#pragma warning(pop) //} STL includes

#ifdef _STLP_BEGIN_NAMESPACE
#define CB_STL_BEGIN	_STLP_BEGIN_NAMESPACE
#else
#define CB_STL_BEGIN	_STD_BEGIN
#endif

#ifdef _STLP_END_NAMESPACE
#define CB_STL_END	_STLP_END_NAMESPACE
#else
#define CB_STL_END	_STD_END
#endif


/*
#ifdef gNew
#undef new
#define new gNew
#define delete gDelete
#endif
*/

//=======================================================================================

/*! pair_first_bool_binary_functor

	this adapts a Functor which works on "first" to work on a pair
*/
template<typename Pair,typename Functor> struct pair_first_bool_binary_functor : 
	public cb::binary_function<Pair,Pair,bool>
{
	bool operator()(const Pair & __x, const Pair & __y) const
	{
		return Functor()(__x.first,__y.first); 
	}
};

/*! pair_second_bool_binary_functor

	this adapts a Functor which works on "second" to work on a pair
*/
template<typename Pair,typename Functor> struct pair_second_bool_binary_functor : 
	public cb::binary_function<Pair,Pair,bool>
{
	bool operator()(const Pair & __x, const Pair & __y) const
	{
		return Functor()(__x.second,__y.second); 
	}
};

//! pair_key_bool_binary_functor
template<class Pair,class Functor> struct pair_key_bool_binary_functor : 
	public cb::binary_function<Pair,typename Pair::first_type,bool>
{
	bool operator()(const Pair & __x, const typename Pair::first_type & __y) const
	{
		return Functor()(__x.first,__y); 
	}
};

//! pair_data_bool_binary_functor
template<class Pair,class Functor> struct pair_data_bool_binary_functor : 
	public cb::binary_function<Pair,typename Pair::second_type,bool>
{
	bool operator()(const Pair & __x, const typename Pair::second_type & __y) const
	{
		return Functor()(__x.second,__y); 
	}
};

/*! equivalent_functor

 "equivalence" is the right way to build an "equals" based
	 on a strict-weak-ordering like m_compare
  by definition : if two things are equivalent , then m_compare
	 says they are not different
*/
template<class Comparison> struct equivalent_functor : 
	public cb::binary_function<typename Comparison::first_argument_type,typename Comparison::second_argument_type,bool>
{
	bool operator()(const typename Comparison::first_argument_type & __x, const typename Comparison::second_argument_type & __y) const
	{
		return ( ! Comparison()(__x,__y) && ! Comparison()(__y,__x) );
	}
};

//============================================================================

struct str_compare :
		public cb::binary_function<const char *, const char *, bool>
{
	bool operator()(const char *p1, const char *p2) const
	{
		return strcmp(p1,p2) < 0;
	}
};

struct str_compare_i :
		public cb::binary_function<const char *, const char *, bool>
{
	bool operator()(const char *p1, const char *p2) const
	{
		return _stricmp(p1,p2) < 0;
	}
};

//============================================================================

/*
template <class t_iterator>
(typename std::iterator_traits<t_iterator>::value_type)
average(t_iterator begin,t_iterator end)
{
	std::iterator_traits<t_iterator>::value_type ret(0);
	int count = 0;
	for(t_iterator ptr = begin;ptr != end;++ptr)
	{
		ret += *ptr;
		count ++;
	}
	if ( count > 0 )
	{
		return (ret/count);
	}
	return ret;
}
/**/

#pragma once

#include "stl_basics.h"

// GET RID OF ME
#if 0

#include "Base.h"
#include <functional>

START_CB

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

END_CB

#endif

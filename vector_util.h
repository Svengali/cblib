#pragma once

#include "cblib/Base.h"
#include "cblib/vector.h"
#include <algorithm>
//#include "DebugUtil/Profiler.h"

START_CB

namespace vector_util
{
	template <typename vec_class,
			  typename search_class>
	typename vec_class::iterator find(vec_class& rVector,
							 const search_class& rSearch)
	{
		return std::find(rVector.begin(),
						 rVector.end(),
						 rSearch);
	}

	template <typename vec_class,
			  typename search_class>
	typename vec_class::const_iterator find_c(const vec_class&    rVector,
									 const search_class& rSearch)
	{
		return std::find(rVector.begin(),
						 rVector.end(),
						 rSearch);
	}

	template <typename vec_class,
			  typename search_class>
	typename vec_class::iterator find_if(vec_class& rVector,
							 const search_class& rSearch)
	{
		return std::find_if(rVector.begin(),
						 rVector.end(),
						 rSearch);
	}

	template <typename vec_class,
			  typename search_class>
	typename vec_class::const_iterator find_if_c(const vec_class&    rVector,
									 const search_class& rSearch)
	{
		return std::find_if(rVector.begin(),
						 rVector.end(),
						 rSearch);
	}

	template <typename t_iterator1,
				typename t_iterator2,
			  typename search_class>
	t_iterator1 find_from_end(const t_iterator1 & begin,const t_iterator2 & end,const search_class & what)
	{
		ASSERT( end >= begin );
		for(t_iterator1 it = end;;)
		{
			if ( it == begin )
				return end;
			--it;
			if ( *it == what )
				return it;
		}
	}

	template <typename vec_class,
			  typename search_class>
	typename vec_class::iterator find_from_end(vec_class& rVector,
							 const search_class& rSearch)
	{
		return find_from_end(rVector.begin(),
						 rVector.end(),
						 rSearch);
	}

	template <typename vec_class,
			  typename search_class>
	typename vec_class::const_iterator find_from_end_c(const vec_class&    rVector,
									 const search_class& rSearch)
	{
		return find_from_end(rVector.begin(),
						 rVector.end(),
						 rSearch);
	}
	
	// push_back_tight : like a push_back but with tight memory allocation
	template <typename V,typename T>
	void push_back_tight(V * pVector,const T & elem)
	{
		V other;
		other.reserve(pVector->size()+1);
		other = *pVector;
		other.push_back(elem);
		pVector->swap(other);
	}

	template <typename T>
	void random_permute(T * order,int size)
	{
		for(int i=0;i<size;i++)
		{
			int r = i + irandmod(size - i);
			std::swap( order[i], order[r] );
		}
	}	

	template <typename V>
	void random_permute(V * pVec)
	{
		int size = pVec->size32();
		V::iterator order = pVec->begin();
		for(int i=0;i<size;i++)
		{
			int r = i + irandmod(size - i);
			std::swap( order[i], order[r] );
		}
	}	
	
} // namespace VectorUtil

END_CB

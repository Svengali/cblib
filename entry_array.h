#pragma once
// ENTRY ARRAY
//============================================================================

#include "cblib/Base.h"
#include "cblib/Util.h"
#include <new.h> // need this to get placement new

START_CB

#pragma warning(push)
#pragma warning(disable : 4345)
//d:\Exoddus\Code\Engine\Core\entry_array.h(83) : warning C4345: behavior change: an object of POD type constructed with an initializer of the form () will be default-initialized

namespace entry_array
{
	/*******************

	entry_array

	This is a set of helper template functions for arrays of a templated type.
	It's interesting in a few ways.  

	copy(Entry * pTo,const Entry * pFm,const int count);
	move(Entry * pTo,const Entry * pFm,const int count);
	construct(Entry * pArray,const int size);
	destruct(Entry * pArray,const int size)
	construct(Entry * pEntry);
	destruct(Entry * pEntry);
	copy_construct(Entry * pEntry,const Entry & from);
	copy_construct(Entry * pArray,const int size,const Entry * pFrom);
	swap(Entry * pArray1,Entry * pArray2,const int count)

	see notes at bottom

	*******************/

	//-----------------------------------------------------------------------------------------------

	// EntryCopy
	//	like memcpy
	//	pTo and pFm must not overlap
	template <class Entry>
	static inline void copy(Entry * pTo,const Entry * pFm,const int count)
	{
		// assert we don't overlap in a bad way :
		ASSERT( pTo != pFm || count == 0 );
		ASSERT( pTo < pFm || pTo >= pFm + count );

		for(int i=0;i<count;i++)
		{
			pTo[i] = pFm[i];
		}
	}

	//-----------------------------------------------------------------------------------------------

	// EntryMove
	//	like memmove
	//	pTo and pFm may overlap
	template <class Entry>
	static inline void move(Entry * pTo,const Entry * pFm,const int count)
	{
		ASSERT( pTo != pFm || count == 0 );
		
		if ( pTo > pFm )
		{
			// go backwards
			for(int i = count-1; i>= 0;i--)
			{
				pTo[i] = pFm[i];
			}
		}
		else // ( pTo < pFm )
		{
			// go forwards
			copy(pTo,pFm,count);
		}
	}

	//-----------------------------------------------------------------------------------------------

	template <class Entry>
	static inline void construct(Entry * pArray,const int size)
	{
		// placement new an array :
		for(int i=0;i<size;i++)
		{
			ASSERT(pArray);
			//new (ePlacementNew, pArray+i) Entry();
			new (pArray+i) Entry();
		}
	}

	//-----------------------------------------------------------------------------------------------

	template <class Entry>
	static inline void destruct(Entry * pArray,const int size)
	{
		for(int i=0;i<size;i++)
		{
			ASSERT(pArray);
			//destruct(pArray+i);
			pArray[i].~Entry();
		}
	}

	//-----------------------------------------------------------------------------------------------

	template <class Entry>
	static inline void construct(Entry * pEntry)
	{
		new (pEntry) Entry();
	}

	//-----------------------------------------------------------------------------------------------

	template <class Entry>
	static inline void destruct(Entry * pEntry)
	{
		ASSERT(pEntry);

		pEntry->~Entry();
	}

	//-----------------------------------------------------------------------------------------------
	
	template <class Entry>
	static inline void copy_construct(Entry * pTo,const Entry & from)
	{
		ASSERT(pTo);

		//new (ePlacementNew, pTo) Entry(from);
		new (pTo) Entry(from);
	}
		
	//-----------------------------------------------------------------------------------------------
	
	template <class Entry>
	static inline void copy_construct(Entry * pArray,const Entry * pFrom,const int size)
	{
		// placement new an array :
		for(int i=0;i<size;i++)
		{
			ASSERT( pArray && pFrom );
			copy_construct(pArray+i,pFrom[i]);
		}
	}
	
	//-----------------------------------------------------------------------------------------------

	// Entryswap
	//	like memcpy
	//	pArray1 and pArray2 must not overlap
	template <class Entry>
	static inline void swap(Entry * pArray1,Entry * pArray2,const int count)
	{
		// assert we don't overlap in a bad way :
		ASSERT( pArray1 != pArray2 );
		ASSERT( pArray1 < pArray2 || pArray1 >= pArray2 + count );

		for(int i=0;i<count;i++)
		{
			ASSERT( pArray1 && pArray2 );
			Swap(pArray1[i],pArray2[i]);
		}
	}

	//-----------------------------------------------------------------------------------------------
	
	// We need this in vector, but I don't want to include <memory> there.
	//  Note that this is slightly less flexible than the stl version,
	//  it only works with pointer iterators, not general iterators
	// has [FROM,TO] ordering unlike everything else
	template <class InClass, class FwdClass>
	inline FwdClass* uninitialized_copy(const InClass*  first,
											const InClass*  last,
											FwdClass* result)
	{
		while (first != last)
		{
			//new ( ePlacementNew, result ) FwdClass( *first );
			new ( result ) FwdClass( *first );
			first++;
			result++;
		}

		return result;
	}
	
	//-----------------------------------------------------------------------------------------------
};

#pragma warning(pop)

//============================================================================
END_CB

#pragma once

#define THREAD_RELACY
#include "cblib/Threading.h"

START_CB

// Leaky_Allocator is always the same :

template <typename T>
struct New_Allocator
{
	// nada
		
	T * Alloc()
	{
		return new T;
	}
	
	void Free(T * node)
	{
		delete node;
	}
	
	void Finalize()
	{
		// nop
	}
};

END_CB

#ifndef SELECTED_ALLOCATOR
#define SELECTED_ALLOCATOR	New_Allocator
#endif

#pragma once
#ifndef GALAXY_GREEDYALLOCATOR_H
#define GALAXY_GREEDYALLOCATOR_H

#include "Base.h"

START_CB

namespace SmallAllocator_Greedy
{
	void *	Allocate(const size_t size);
	void	Free(void * p,const size_t size);

	void	FreeNoSize(void * p);
	int		GetMemSize(void * p);
	
	void	Destroy();
	void	Log();
};

END_CB

#endif // GALAXY_GREEDYALLOCATOR_H

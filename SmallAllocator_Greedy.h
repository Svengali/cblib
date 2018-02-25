#pragma once
#ifndef GALAXY_GREEDYALLOCATOR_H
#define GALAXY_GREEDYALLOCATOR_H

START_CB

namespace SmallAllocator_Greedy
{
	void *	Allocate(const int size);
	void	Free(void * p,const int size);

	void	Destroy();
	void	Log();
};

END_CB

#endif // GALAXY_GREEDYALLOCATOR_H

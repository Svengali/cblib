#include "cblib/stl_basics.h"
#include "cblib/SmallAllocator_Greedy.h"

START_CB

void * MyStlAlloc(const size_t size)
{
	return SmallAllocator_Greedy::Allocate(size);
}

void MyStlFree(void * ptr,const size_t size)
{
	SmallAllocator_Greedy::Free(ptr,size);
}

END_CB


#ifdef _STLP_BEGIN_NAMESPACE
#pragma PRAGMA_MESSAGE("STLPort")
#endif

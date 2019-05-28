#include "Mem.h"
#include "MemTrack.h"
#include "SmallAllocator_Greedy.h"
#include <windows.h>

USE_CB

// #define this first so I use untrack versions here :
#define LowLevelAlloc(size)		GlobalAlloc(0,size)
#define LowLevelFree(ptr)		GlobalFree(ptr)

#define CBLIB_DO_USE_SMALLALLOC

#ifdef CBLIB_DO_USE_SMALLALLOC

#if CB_DO_REPLACE_OPERATOR_NEW

void * operator new(size_t size)
{
	return SmallAllocator_Greedy::Allocate(size);
}

void   operator delete(void * ptr)
{
	SmallAllocator_Greedy::FreeNoSize(ptr);
}

void * operator new[](size_t size)
{
	return SmallAllocator_Greedy::Allocate(size);
}

void   operator delete[](void * ptr)
{
	SmallAllocator_Greedy::FreeNoSize(ptr);
}

#endif

START_CB

// __declspec(restrict) __declspec(noalias)
void *	cbmalloc(size_t size)
{
	if ( size == 0 )
		return NULL;
	
	return SmallAllocator_Greedy::Allocate(size);
}

//__declspec(noalias) 
void	cbfree(void * ptr)
{
	SmallAllocator_Greedy::FreeNoSize(ptr);
}

END_CB

#else // CBLIB_DO_USE_SMALLALLOC

#if CB_DO_REPLACE_OPERATOR_NEW

void * operator new(size_t size)
{
	void * ret = LowLevelAlloc(size);
	if ( ret == NULL )
	{
		FAIL("alloc failed");
	}
	AutoMemTrack_Add(ret,size,eMemTrack_New);
	return ret;
}

void   operator delete(void * ptr)
{
	AutoMemTrack_Remove(ptr);
	LowLevelFree(ptr);
}

void * operator new[](size_t size)
{
	void * ret = LowLevelAlloc(size);
	if ( ret == NULL )
	{
		FAIL("alloc failed");
	}
	AutoMemTrack_Add(ret,size,eMemTrack_New);
	return ret;
}

void   operator delete[](void * ptr)
{
	AutoMemTrack_Remove(ptr);
	LowLevelFree(ptr);
}

#endif

START_CB

void *	cbmalloc(size_t size)
{
	if ( size == 0 )
		return NULL;
		
	void * ret = LowLevelAlloc(size);
	if ( ret == NULL )
	{
		FAIL("alloc failed");
	}
	AutoMemTrack_Add(ret,size,eMemTrack_Malloc);
	return ret;
}

void	cbfree(void * ptr)
{
	AutoMemTrack_Remove(ptr);
	LowLevelFree(ptr);
}

END_CB

#endif // CBLIB_DO_USE_SMALLALLOC

#if CB_DO_REPLACE_MALLOC
extern "C"
{

__declspec(noalias) __declspec(restrict) void *	malloc(size_t s)
{
	return cb::cbmalloc(s);
}

__declspec(noalias) void	free(void * p)
{
	return cb::cbfree(p);
}

};
#endif


extern "C"
{

void *	cb_malloc(size_t s)
{
	return cb::cbmalloc(s);
}

void	cb_free(void * p)
{
	cb::cbfree(p);
}

};

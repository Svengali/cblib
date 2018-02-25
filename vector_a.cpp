#include "Base.h"
#include "vector_a.h"

START_CB

static void * s_ptr = NULL;
static int s_bytes = 0;

void vector_arena_provide(void *pData,int bytes)
{
	s_ptr = pData;
	s_bytes = bytes;
}

void vector_arena_acquire(void ** pData,int * pSize,int elemSize)
{
	*pData = s_ptr;
	*pSize = s_bytes / elemSize;
	s_ptr = NULL;
	s_bytes = 0;
}

END_CB

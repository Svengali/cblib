#include "Base.h"
#include "vector_a.h"

START_CB

static void * s_ptr = NULL;
static size_t s_bytes = 0;

void vector_arena_provide(void *pData,size_t bytes)
{
	s_ptr = pData;
	s_bytes = bytes;
}

void vector_arena_acquire(void ** pData,size_t * pSize,size_t elemSize)
{
	ASSERT_RELEASE( s_ptr != NULL );

	*pData = s_ptr;
	*pSize = s_bytes / elemSize;
	s_ptr = NULL;
	s_bytes = 0;
}

END_CB

#pragma once

#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

void *	cb_malloc(size_t s);
void	cb_free(void * p);

#ifdef __cplusplus
};
#endif

#define malloc	cb_malloc
#define free	cb_free

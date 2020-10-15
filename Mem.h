#pragma once

#include "Base.h"
#include <stdlib.h>
#include <malloc.h> // for _alloca
// Util includes me
#include "cblib_config.h"

#ifndef CB_DO_REPLACE_OPERATOR_NEW
#define CB_DO_REPLACE_OPERATOR_NEW 0
#endif
#ifndef CB_DO_REPLACE_MALLOC
#define CB_DO_REPLACE_MALLOC	0
#endif

START_CB

//-------------------------
// my mallocs :

void *	cbmalloc(size_t s);
void	cbfree(void * ptr);

//#define CBNEW			new
#define CBALLOC(size)	cb::cbmalloc(size)
#define CBFREE(ptr)		cb::cbfree(ptr)

#define CBALLOCONE(type)	(type *) CBALLOC(sizeof(type))
#define CBALLOCARRAY(type,count)	(type *) CBALLOC(sizeof(type)*(count))

#define STACK_ARRAY(name,type,count)	type * name = (type *) _alloca(sizeof(type)*(count));

END_CB

//=============================================
// global namespace :

#if CB_DO_REPLACE_OPERATOR_NEW

#undef new
#undef delete

void * operator new		(size_t size);
void   operator delete	(void * ptr);
void * operator new[]	(size_t size);
void   operator delete[](void * ptr);

#endif


#if CB_DO_REPLACE_MALLOC
extern "C"
{

__declspec(noalias) __declspec(restrict) void *	malloc(size_t s);
__declspec(noalias) void	free(void * p);

};
#endif

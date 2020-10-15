#pragma once

#include "Base.h"

extern "C"
{

//	returns bool = exchange was done
//	if it was not done then *oldV = *pVal
extern int __cdecl cblib_cmpxchg128( volatile void * pVal, void * pOld, const void * pNew);

extern void __cdecl cblib_cpuidex( int eax, int ecx, int * pInto);


};

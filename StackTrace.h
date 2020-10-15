#pragma once

#include "Base.h"
#include <stdlib.h>
#include <stddef.h>

START_CB

// kill here to turn all stack traces into nops :
#define CB_DO_STACK_TRACE

//typedef uint32	instructionaddress_t;
typedef size_t	instructionaddress_t;

#ifdef _MSC_VER
// get _ReturnAddress intrinsic :
extern "C" void * _ReturnAddress(void);

#pragma intrinsic(_ReturnAddress)
#endif

// GetAddressOfCaller : fast one-step stack back
inline instructionaddress_t GetAddressOfCaller()
{
	#ifdef _MSC_VER
	return (instructionaddress_t) (intptr_t) _ReturnAddress();
	#else
	// GCC :
	return (instructionaddress_t) __builtin_return_address(0);
	#endif
}

struct StackTrace;

#ifdef CB_DO_STACK_TRACE
//-----------------------------------------------------------

#define CB_STACK_TRACE_DEPTH	(12)

// Init/Shutdown loads & frees symbols
//	you don't need to Init to be able to Grab() but you do need it to Log
void StackTrace_Init();
void StackTrace_Shutdown();

void StackTrace_Grab(StackTrace * trace,int skips = 0);
void StackTrace_Log(const StackTrace * trace);

#else 
//-----------------------------------------------------------

#define CB_STACK_TRACE_DEPTH	(0)

void StackTrace_Init() { }
void StackTrace_Shutdown() { }

void StackTrace_Grab(StackTrace * trace,int skips = 0) { }
void StackTrace_Log(const StackTrace * trace) { }

//-----------------------------------------------------------
#endif // CB_DO_STACK_TRACE

struct StackTrace
{
	instructionaddress_t	ips[CB_STACK_TRACE_DEPTH];
	
	void Grab(int skips = 0) { StackTrace_Grab(this,skips); }
	void Log() const { StackTrace_Log(this); }
};

END_CB

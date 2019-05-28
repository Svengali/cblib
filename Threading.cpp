#include "Threading.h"
#include <mmsystem.h>
#include <intrin.h>

#ifdef CB_64
#include <cblib/cblib_x64_asm.h>
#endif

extern "C"
{
#include <Powrprof.h>
}

#pragma comment(lib,"winmm.lib") // timeBeginPeriod
#pragma comment(lib,"powrprof.lib") // timeBeginPeriod

START_CB

//Atomic<LONG> test;

/*static*/ int SpinBackOff::SpinBackOff_Spins = 500;

namespace {
static bool s_init = false;
static int s_numCores = 0;
static bool s_hasSpeedStep = false;
};

void Threading_Init()
{
	// (not thread safe) :
	if ( s_init ) return;
	s_init = true;

	//-----------------------------------------------------------
	// set time slice :
	
	timeBeginPeriod(1);

	//-----------------------------------------------------------
	// get num cores :
	
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	s_numCores = info.dwNumberOfProcessors;
	
	if ( s_numCores == 1 )
	{
		// on single-core systems - don't spin !
		SpinBackOff::SpinBackOff_Spins = 1;
	}
	
	//-----------------------------------------------------------
	// check for hyperthreading :

	/*	
	// GetLogicalProcessorInformation is not in my Windows XP
	// http://msdn.microsoft.com/en-us/library/ms683194(VS.85).aspx
	DWORD procsSize = 0;
	GetLogicalProcessorInformation(NULL,&procsSize);
	ASSERT( procsSize > 0 );
	int procCount = procsSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
	vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> procs;
	procs.resize(procCount);
	GetLogicalProcessorInformation(procs.data(),&procsSize);
	/**/
	
	//-----------------------------------------------------------
	// look for speed step :

	SYSTEM_POWER_CAPABILITIES pwr = { 0 };
	GetPwrCapabilities(&pwr);
	//CallNtPowerInformation(SystemPowerInformation,NULL,0,&pwr,sizeof(pwr));
	
	//int i = 1;
	if ( pwr.ProcessorThrottle && pwr.ProcessorMinThrottle < 100 )
	{
		// @@ do I need to check anything else ?
		s_hasSpeedStep = true;
	}
	else
	{
		s_hasSpeedStep = false;
	}
}	

void SimpleLock(SimpleMutex volatile * mut)
{
	// 0 is unlocked, 1 is locked
	COMPILER_ASSERT( sizeof(SimpleMutex) == sizeof(LONG) );
	
	SpinBackOff backoff;
	
	while( ! AtomicCAS((LONG *)mut,(LONG)0,(LONG)1) )
	{
		backoff.BackOffYield();
	}
	// *ptr should now be 1	
}
	
void SimpleUnlock(SimpleMutex volatile * mut)
{
	// *ptr should now be 1
	AtomicExchange((LONG *)mut,(LONG)0);
	// *ptr should now be 0
	// if there was a higher priority thread stalled on us, wake it up !	
}

//----------------------------------------------------------------
// Yield allows any other available threads to run

void ThreadYieldToAny(void)
{
	SwitchToThread();
}

void ThreadYieldNotLower(void)
{
	Sleep(0);
}

//===========================================

void ThreadSleep(int sleepTime)
{
	// use ThreadYieldNotLower for Sleep(0)
	ASSERT( sleepTime > 0 );
	
	// try to sleep for an accurate period :
	
	DO_ONCE( timeBeginPeriod(1) );
	
	ThreadPriorityScoper pri(THREAD_PRIORITY_TIME_CRITICAL);

	Sleep(sleepTime);

}

//===========================================

// due to: http://www.gamedev.net/community/forums/topic.asp?topic_id=445787&whichpage=2&#2960603
// allows sleep at fine granularity with no stuttering, and not pegging CPU to 100%
void WaitableTimerSleep( float Seconds )
{
    static HANDLE Timer = CreateWaitableTimer( NULL, FALSE, NULL );

    // Determine time to wait.
    LARGE_INTEGER WaitTime;
    WaitTime.QuadPart = (LONGLONG)(Seconds * -10000000);
    if ( WaitTime.QuadPart >= 0 )
        return;

    // Give up the rest of the frame.
    if ( !SetWaitableTimer( Timer, &WaitTime, 0, NULL, NULL, FALSE ) )
        return;

	ThreadPriorityScoper pri(THREAD_PRIORITY_TIME_CRITICAL);

	WaitForSingleObject(Timer,INFINITE);

	/*
	// waits for timer or input :
    DWORD Result = MsgWaitForMultipleObjects
    (
        1,
        &Timer,
        FALSE,
        INFINITE,
        QS_ALLINPUT
    );
    */
}

//-------------------------------------
// run a low priority spinner to stop speed step :
	
volatile BOOL s_spinner_kill = FALSE;
HANDLE s_spinner_thread = 0;

DWORD WINAPI SpinnerThreadRoutine( LPVOID user_data )
{
	while( ! LoadRelaxed(&s_spinner_kill) )
	{
		Sleep(0);
	}

	return 0;
}

void StartSpeedStepSpinnerThread()
{
	if ( s_spinner_thread )
		return;

	// @@ TODO : I should check if this machine has speedstep and
	//		not do this if it doesn't

	StoreRelaxed(&s_spinner_kill,FALSE);
	
	s_spinner_thread = CreateThread(NULL,0,SpinnerThreadRoutine,NULL,CREATE_SUSPENDED,NULL);
	
	SetThreadPriority(s_spinner_thread,THREAD_PRIORITY_LOWEST);
	
	ResumeThread(s_spinner_thread);
}

void KillSpeedStepSpinnerThread()
{
	if ( s_spinner_thread )
	{
		StoreRelaxed(&s_spinner_kill,TRUE);
		
		WaitForSingleObject(s_spinner_thread,INFINITE);
		s_spinner_thread = 0;
	}
}
	
//===========================================

//----------------------------------------------------------------
// Singleton lock :
//	safe single lock made on first use

DECL_ALIGN(16) static volatile BOOL s_singletonCritIsInit = false;
static CRITICAL_SECTION	s_singletonCrit = { 0 };

static CRITICAL_SECTION * Singleton_GetCrit()
{
	// s_pTracker needs to be a "Singleton" so it works with Cinit stuff
	if ( ! LoadAcquire(&s_singletonCritIsInit) )
	{
		// thread safe singleton instantiation :
		static volatile SimpleMutex s_mutex = 0;
		SimpleLock(&s_mutex);
		
		if ( ! s_singletonCritIsInit )
		{
			InitializeCriticalSectionAndSpinCount(&s_singletonCrit,SpinBackOff::SpinBackOff_Spins);

			// !! memory barrier to make sure writes to s_singletonCrit are flushed!
			// InterlockedExchange does that for us
			
			StoreRelease<BOOL>(&s_singletonCritIsInit,TRUE);
		}
		SimpleUnlock(&s_mutex);		
	}
	
	return &s_singletonCrit;
}

void Singleton_Lock()
{
	CRITICAL_SECTION * crit = Singleton_GetCrit();
	EnterCriticalSection(crit);
}

void Singleton_Unlock()
{
	CRITICAL_SECTION * crit = Singleton_GetCrit();
	LeaveCriticalSection(crit);
}

void Singleton_Shutdown()
{
	// @@ unsafe !
	CRITICAL_SECTION * crit = Singleton_GetCrit();
	DeleteCriticalSection(crit);
	s_singletonCritIsInit = false;
}	
	
//----------------------------------------------------------------

CB_THREAD_LOCAL static LONG s_threadIndex = -1;
static volatile LONG s_threadCounter = 0;

int GetThreadIndex()
{
	//InterlockedExchangeAcquire(
	if ( s_threadIndex == -1 )
	{
		// set it up
		// no need for any singleton mumbo jumbos cuz threadInex is local to my thread !!
		s_threadIndex = InterlockedIncrement(&s_threadCounter);
	}
	
	return s_threadIndex;
}
	
//----------------------------------------------------------------
// manual TLS : don't use this, CB_THREAD_LOCAL is awesome

int AllocTLS()
{
	return TlsAlloc();
}

intptr_t GetTLS(int index)
{
	return (intptr_t) TlsGetValue(index);
}

void SetTLS(int index,intptr_t value)
{
	TlsSetValue(index, (LPVOID) value );
}


uint32 AtomicExchange32(uint32 volatile * pInto,uint32 newVal)
{
	return InterlockedExchange((volatile LONG *)pInto,newVal);
}

uint32 AtomicExchangeAdd32(uint32 volatile * pInto,uint32 inc)
{
	// @@@@ TEMP
	//*
	return InterlockedExchangeAdd((volatile LONG *)pInto,(LONG)inc);
	/*/
	CompilerReadWriteBarrier();
	uint32 oldVal,newVal;
	do
	{
		oldVal = LoadAcquire(pInto);
		newVal = oldVal + inc;
	}
	while ( ! AtomicCAS32(pInto,oldVal,newVal) );
	CompilerReadWriteBarrier();
	
	return oldVal;
	/**/
}
	
//=====================================================================
	
#ifndef CB_64

// _InterlockedCompareExchange64 doesn't exist so make my own :

__declspec( naked ) int __cdecl atomic_dwcas_fence( LONGLONG *, LONGLONG *, const LONGLONG * )
{
	__asm
	{
	push esi
	push ebx
	mov esi, [esp + 16]
	mov eax, [esi]
	mov edx, [esi + 4]
	mov esi, [esp + 20]
	mov ebx, [esi]
	mov ecx, [esi + 4]
	mov esi, [esp + 12]
	lock cmpxchg8b qword ptr [esi]
	jne np_ac_i686_atomic_dwcas_fence_fail
	mov eax, 1 // success
	pop ebx
	pop esi
	ret

	np_ac_i686_atomic_dwcas_fence_fail:
	mov esi, [esp + 16]
	mov [esi + 0],  eax;
	mov [esi + 4],  edx;
	xor eax, eax
	pop ebx
	pop esi
	ret
	}
}

__declspec( naked ) int __cdecl cblib_cas64( LONGLONG *, LONGLONG *, const LONGLONG * )
{
	__asm
	{
	push esi
	push ebx
	mov esi, [esp + 16]
	mov eax, [esi]
	mov edx, [esi + 4]
	mov esi, [esp + 20]
	mov ebx, [esi]
	mov ecx, [esi + 4]
	mov esi, [esp + 12]
	lock cmpxchg8b qword ptr [esi]
    sete cl
	mov esi, [esp + 16]
	mov [esi + 0],  eax;
	mov [esi + 4],  edx;
	movzx eax,cl
	pop ebx
	pop esi
	ret
	}
}

#endif

//=====================================================================
// x86 doesn't have 64 atomics for most ops - just CAS
//	so we have to implement the basic ones in terms of loops !
//
// BTW there's a problem with windows.h where
//	InterlockedIncrement64 is implemented as a loop that calls InterlockedCompareExchange64
// but InterlockedCompareExchange64 doesn't exist in kernel32.dll except in Vista !

uint64 AtomicExchange64(uint64 volatile * pInto,uint64 newVal)
{
	#ifdef CB_64
	// InterlockedExchange64 requires Vista
	return InterlockedExchange64((volatile LONGLONG *)pInto,(LONGLONG)newVal);

	#else

	CompilerReadWriteBarrier();
	uint64 oldVal;
	do
	{
		oldVal = LoadAcquire(pInto);
	}
	while ( ! AtomicCAS64(pInto,oldVal,newVal) );
	CompilerReadWriteBarrier();
	
	return oldVal;
	
	#endif
}

uint64 AtomicExchangeAdd64(uint64 volatile * pInto,uint64 inc)
{
	#ifdef CB_64
	// InterlockedExchange64 requires Vista
	// BTW this is just "lock xadd"
	return InterlockedExchangeAdd64((volatile LONGLONG *)pInto,(LONGLONG)inc);

	#else

	CompilerReadWriteBarrier();
	uint64 oldVal,newVal;
	do
	{
		oldVal = LoadAcquire(pInto);
		newVal = oldVal + inc;
	}
	while ( ! AtomicCAS64(pInto,oldVal,newVal) );
	CompilerReadWriteBarrier();
	
	return oldVal;
	#endif
}

/*
uint64 AtomicIncrement64(uint64 volatile * pInto)
{
	CompilerReadWriteBarrier();
	uint64 oldVal,newVal;
	do
	{
		oldVal = LoadAcquire(pInto);
		newVal = oldVal + 1;
	}
	while ( ! AtomicCAS64(pInto,oldVal,newVal) );
	CompilerReadWriteBarrier();
	
	return newVal;
}

uint64 AtomicDecrement64(uint64 volatile * pInto)
{
	CompilerReadWriteBarrier();
	uint64 oldVal,newVal;
	do
	{
		oldVal = LoadAcquire(pInto);
		newVal = oldVal - 1;
	}
	while ( ! AtomicCAS64(pInto,oldVal,newVal) );
	CompilerReadWriteBarrier();
	
	return newVal;
}
*/

//=====================================================================	

#ifdef CB_HAS_ATOMIC_128

#if _MSC_VER > 1400
inline BOOL my_InterlockedCompareExchange128(Atomic128 volatile * pInto,Atomic128 * pOldVal, Atomic128 * newVal)
{
	ASSERT(	(((intptr_t)pInto)&0xF) == 0 );
	ASSERT(	(((intptr_t)pOldVal)&0xF) == 0 );
		
	unsigned char did = _InterlockedCompareExchange128(&pInto->low,newVal->high,newVal->low,&pOldVal->low);
	return did;
}
#else

inline BOOL my_InterlockedCompareExchange128(Atomic128 volatile * pInto,Atomic128 * pOldVal, Atomic128 * pNewVal)
{
	ASSERT(	(((intptr_t)pInto)&0xF) == 0 );
	ASSERT(	(((intptr_t)pOldVal)&0xF) == 0 );
	ASSERT(	(((intptr_t)pNewVal)&0xF) == 0 );
	
	int did = cblib_cmpxchg128(pInto,pOldVal,pNewVal);
	return did;
}
#endif

Atomic128 AtomicCMPX128(Atomic128 volatile * pInto,Atomic128 oldVal, Atomic128 newVal)
{
	my_InterlockedCompareExchange128(pInto,&oldVal,&newVal);
	return oldVal;
}

bool AtomicCAS128(Atomic128 volatile * pInto,Atomic128 oldVal, Atomic128 newVal)
{
	return !! my_InterlockedCompareExchange128(pInto,&oldVal,&newVal);
}
			
Atomic128 AtomicExchange128(Atomic128 volatile * pInto,Atomic128 newVal)
{
	CompilerReadWriteBarrier();
	Atomic128 oldVal;
	do
	{
		oldVal = LoadAcquire(pInto);
	}
	while ( ! AtomicCAS128(pInto,oldVal,newVal) );
	CompilerReadWriteBarrier();
	
	return oldVal;
}	

#endif
	
//=====================================================================	

END_CB

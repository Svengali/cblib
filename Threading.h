#pragma once

#include <cblib/Base.h>
#include <cblib/Win32Util.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <emmintrin.h> // for pause

/*************************************************************************************************/

// SYSTEM_CACHE_ALIGNMENT_SIZE is 64 for most or 128 for Itanium
//#define LF_CACHE_LINE_SIZE	64 // for paddings
#define LF_CACHE_LINE_SIZE		SYSTEM_CACHE_ALIGNMENT_SIZE // for paddings
#define LF_ALIGN_TO_CACHE_LINE	__declspec(align(LF_CACHE_LINE_SIZE))

#ifdef CB_64

#include <intrin.h>
// not true for early AMD64 stuff :
#define CB_HAS_ATOMIC_128

#define LF_SIZEOFPOINTER	8
#define LF_SIZEOF2POINTERS	16

#else

#define LF_SIZEOFPOINTER	4
#define LF_SIZEOF2POINTERS	8

#endif // CB_64

// LF_SIZEOFPOINTER is defined without using sizeof so it can be in align
COMPILER_ASSERT( LF_SIZEOFPOINTER == sizeof(void *) ); 

#define LF_ALIGN_8		__declspec(align(8))
#define LF_ALIGN_16		__declspec(align(16))

#define LF_ALIGN_POINTER		__declspec(align(LF_SIZEOFPOINTER))
#define LF_ALIGN_2POINTERS		__declspec(align(LF_SIZEOF2POINTERS))


#ifdef CB_HAS_ATOMIC_128
#define LF_LARGEST_ATOMIC	16
#else
#define LF_LARGEST_ATOMIC	8
#endif

/*************************************************************************************************/
// get intrinsics in global namespace :

extern "C"
{
   LONG  __cdecl _InterlockedIncrement(LONG volatile *Addend);
   LONG  __cdecl _InterlockedDecrement(LONG volatile *Addend);
   LONG  __cdecl _InterlockedCompareExchange(volatile LONG * Dest, LONG Exchange, LONG Comp);
   LONG  __cdecl _InterlockedExchange(volatile LONG * Target, LONG Value);
   LONG  __cdecl _InterlockedExchangeAdd(volatile LONG * Addend, LONG Value);

	// these are the *optimizer* barriers, not memory barriers :
	#if _MSC_VER >= 1400
	#define HAS_READ_BARRIER
	void _ReadBarrier(void); // ReadBarrier is VC2005
	#endif
	
	void _WriteBarrier(void);
	void _ReadWriteBarrier(void);
}

// _ReadBarrier, _WriteBarrier, and _ReadWriteBarrier
// these are just *compiler* memory operation barriers, they are NOT fences :

#pragma intrinsic(_WriteBarrier)
#pragma intrinsic(_ReadWriteBarrier)

#define CompilerReadWriteBarrier	_ReadWriteBarrier
#define CompilerWriteBarrier	_WriteBarrier

#ifdef HAS_READ_BARRIER
#pragma intrinsic(_ReadBarrier)	// ReadBarrier is VC2005
#define CompilerReadBarrier		_ReadBarrier	
#else
#define CompilerReadBarrier		_ReadWriteBarrier
#endif

//=====================================================================

#pragma intrinsic (_InterlockedCompareExchange)
#define InterlockedCompareExchange _InterlockedCompareExchange

#pragma intrinsic (_InterlockedExchange)
#define InterlockedExchange _InterlockedExchange 

#pragma intrinsic (_InterlockedExchangeAdd)
#define InterlockedExchangeAdd _InterlockedExchangeAdd

#pragma intrinsic (_InterlockedIncrement)
#define InterlockedIncrement _InterlockedIncrement

#pragma intrinsic (_InterlockedDecrement)
#define InterlockedDecrement _InterlockedDecrement

/*
beware :
InterlockedIncrement :
The function returns the resulting incremented value. 

InterlockedExchangeAdd :
The function returns the initial value of the Addend parameter.
*/

/*
#ifdef CB_64
#pragma intrinsic (_InterlockedExchangeAdd64)
#define InterlockedExchangeAdd64 _InterlockedExchangeAdd64
#endif
*/

// there's also sort of Acquire/Release semantic versions of some of these, but only available on Itanium , yuck yuck

#ifndef InterlockedExchangeRelease
#define InterlockedExchangeRelease InterlockedExchange
#endif

#ifndef InterlockedExchangeAcquire
#define InterlockedExchangeAcquire InterlockedExchange
#endif

#ifndef InterlockedExchangeAddAcquire
#define InterlockedExchangeAddAcquire InterlockedExchangeAdd
#endif

/*************************************************************************************************/

// stupid windows defines Yield :
#ifdef Yield
#undef Yield
#define Yield _dont_use_Yield_cuz_of_fucking_Windows_
#endif

START_CB

	void Threading_Init();

	//-------------------------------------
	// SimpleMutex is really shitty but we need something that can be static initialized
	//	initialize with zero :
	//	static SimpleMutex s_mymut = 0;
	typedef uint32	SimpleMutex;
	
	void SimpleLock(SimpleMutex volatile * mut);
	void SimpleUnlock(SimpleMutex volatile * mut);

	//-------------------------------------
	// Singleton lock :
	//	safe single lock made on first use
	//  useful for instantiating your singletons
	
	void Singleton_Lock();
	void Singleton_Unlock();
	void Singleton_Shutdown(); // <- @@ shutdown is not thread safe! make sure all threads are dead
	
	//-------------------------------------

	void ThreadYieldToAny(void);
	void ThreadYieldNotLower(void);
	void ThreadSleep(int millis);

	// allows sleep at fine granularity with no stuttering, and not pegging CPU to 100%
	void WaitableTimerSleep( float Seconds );

	//-------------------------------------
	// run a low priority spinner to stop speed step :
	
	void StartSpeedStepSpinnerThread();
	void KillSpeedStepSpinnerThread();
	
	//-------------------------------------
	// GetThreadIndex
	
	int GetThreadIndex();
	
	//-------------------------------------
	// get a TLS index and use it :
	
	// use CB_THREAD_LOCAL instead :
	#define CB_THREAD_LOCAL	__declspec(thread)
	
	int AllocTLS();
	intptr_t GetTLS(int index);
	void SetTLS(int index,intptr_t value);
	
	//-------------------------------------
	
	// LockFree trinary result :
	enum LFResult
	{
		LF_Empty = 0,
		LF_Success = 1,
		LF_Abort = 2
	};

	
	//-------------------------------------
	
	inline void HyperYieldProcessor()
	{
		// on hyperthreaded procs need this :
		//	pause is still beneficial on non-HT
		#ifdef YieldProcessor
		YieldProcessor();
		#else
		_mm_pause();
		#endif
	}

	//-------------------------------------
	// SpinBackOff is a little helper for all your yielding needs
	//	spins CPU
	//	then sleeps progressive
	//	then warns about infinite loop
	
	class SpinBackOff
	{
	public:
		
		SpinBackOff() : m_count(0) { }
	
		//static const int SpinBackOff_Spins = 500;
		static int SpinBackOff_Spins;
		static const int SpinBackOff_MaxSleep = 100;
		//static const int SpinBackOff_MaxSleep = 10;
		static const int SpinBackOff_Yields = 10;
	
		void ResetCount()
		{
			m_count = 0;
		}
	
		bool WillYieldSleep() const
		{
			return ( m_count >= SpinBackOff_Spins );
		}
	
		void BackOffYield()
		{
			// if Spins is 1 I want to spin once
			if ( m_count < SpinBackOff_Spins )
			{
				// spinning CPU
				HyperYieldProcessor();
			}
			else
			{
				int sleepTime = m_count - SpinBackOff_Spins;
				// sleepTime starts at 0 and counts up linearly
			
				if ( sleepTime < SpinBackOff_Yields )
				{
					ThreadYieldToAny();
				}
				else
				{
					sleepTime = sleepTime - SpinBackOff_Yields + 1;
							
					if ( sleepTime > SpinBackOff_MaxSleep )
					{
						FAIL("Likely infinite loop in SpinBackOff\n");	
						m_count = 0; // for debugging, restart the count
					}
					
					ThreadSleep(sleepTime);
				}
			}
			
			m_count++;
		}
		
	private:
		int m_count;
	};
	
	//---------------------------------------------
	
	class ThreadPriorityScoper
	{
	public :
		ThreadPriorityScoper(int newpri)
		{
			m_oldpri = GetThreadPriority( GetCurrentThread() );
			SetThreadPriority( GetCurrentThread(), newpri );
		}
		~ThreadPriorityScoper()
		{
			SetThreadPriority( GetCurrentThread(), m_oldpri );
		}
		
		int m_oldpri;
	};
	
	// CB_SCOPE_THREAD_CRITICAL_PRIORITY : make a scope where you want as much CPU as possible
	//	USE THIS SPARINGLY! should mainly only be used for Wait() calls
	#define CB_SCOPE_THREAD_CRITICAL_PRIORITY() ThreadPriorityScoper NUMBERNAME(scoper)(THREAD_PRIORITY_TIME_CRITICAL)
	
	//-------------------------------------
	// full fence with sequential consistency :
	
	inline void SequentialFence()
	{
		CompilerReadWriteBarrier(); // compiler barrier
		//__asm mfence;
		_mm_mfence();
		CompilerReadWriteBarrier(); // compiler barrier
	}
	
	//-------------------------------------

	template <typename T>
	inline void StoreRelease32( volatile T * pTo, T from )
	{
		COMPILER_ASSERT( sizeof(T) == sizeof(uint32) );
		ASSERT(	(((intptr_t)pTo)&3) == 0 );
		ASSERT(	(((intptr_t)&from)&3) == 0 );
		// on x86, stores are Release :
		*((volatile uint32 *)pTo) = cb::same_size_bit_cast_p<uint32>( from );
		CompilerWriteBarrier();
	}
	
	template <typename T>
	inline T LoadAcquire32( volatile T * ptr )
	{
		COMPILER_ASSERT( sizeof(T) == sizeof(uint32) );
		ASSERT(	(((intptr_t)ptr)&3) == 0 );
		CompilerReadBarrier();
		// on x86, loads are Acquire :
		uint32 ret = *((volatile uint32 *)ptr);
		return cb::same_size_bit_cast_p<T>( ret );
	} 
	
	template <typename T>
	inline void StoreRelease64( volatile T * pTo, T from )
	{
		COMPILER_ASSERT( sizeof(T) == sizeof(uint64) );
		ASSERT(	(((intptr_t)pTo)&7) == 0 );
		//ASSERT(	(((intptr_t)&from)&7) == 0 );
		CompilerWriteBarrier();
		#ifdef CB_64
		// on x86, stores are Release :
		*((volatile uint64 *)pTo) = cb::same_size_bit_cast_p<uint64>( from );
		#else
		// on 32 bit windows
		// trick from Intel TBB : (atomic_support.asm)
		//	see http://www.niallryan.com/node/137
		// FPU can store 64 bits atomically
		__asm
		{
			mov eax, pTo
			fild qword ptr [from]
			fistp qword ptr [eax]
		}
		#endif
	}
	
	template <typename T>
	inline T LoadAcquire64( volatile T * src )
	{
		COMPILER_ASSERT( sizeof(T) == sizeof(uint64) );
		ASSERT(	(((intptr_t)src)&7) == 0 );
		#ifdef CB_64
		// on x86, loads are Acquire :
		uint64 ret = *((volatile uint64 *)src);
		CompilerReadBarrier();
		return cb::same_size_bit_cast_p<T>( ret );
		#else
		uint64 outv;
		__asm
		{
		mov eax,src
		fild qword ptr [eax]
		fistp qword ptr [outv]
		}
		return cb::same_size_bit_cast_p<T>( outv );
		#endif
	} 

	#ifdef CB_HAS_ATOMIC_128
	
	// Windows has an M128A	
	// intrin has __m128i
	
	//typedef __m128i Atomic128;

	LF_ALIGN_16 struct Atomic128
	{
		__int64	low;
		__int64	high;
	};
	
	inline bool operator == (const Atomic128 & lhs,const Atomic128 & rhs)
	{
		return lhs.high == rhs.high && lhs.low == rhs.low;
	}

	inline bool operator != (const Atomic128 & lhs,const Atomic128 & rhs)
	{
		return ! ( lhs == rhs );
	}
	
	COMPILER_ASSERT( sizeof(Atomic128) == 16 );
	
	inline void StoreReleaseAtomic128( volatile Atomic128 * pTo, const Atomic128 * from )
	{
		ASSERT(	(((intptr_t)pTo)&0xF) == 0 );
		ASSERT(	(((intptr_t)&from)&0xF) == 0 );
		CompilerWriteBarrier();

		#if 0 // _MSC_VER >= 1500
		__store128_rel(pTo,from->high,from->low);
		#else
		_mm_sfence(); // release fence
		_mm_store_si128((__m128i *)pTo,*((__m128i *)from));
		#endif
	}
	
	inline Atomic128 LoadAcquireAtomic128( const Atomic128 * ptr )
	{		
		ASSERT(	(((intptr_t)ptr)&0xF) == 0 );
		
		#if 0 // _MSC_VER >= 1500
		Atomic128 ret;
		ASSERT(	(((intptr_t)&ret)&0xF) == 0 );
		__load128_acq((int64 *)ptr,&ret.low);
		#else
		// use SSE
		__m128i ret = _mm_load_si128((const __m128i *)ptr);
		_mm_lfence(); // acquire fence
		#endif		
		
		CompilerReadBarrier();
		
		return cb::same_size_bit_cast_p<Atomic128>(ret);
	}
	
	template <typename T>
	inline void StoreRelease128( volatile T * pTo, T from )
	{
		COMPILER_ASSERT( sizeof(T) == 16 );
		
		StoreReleaseAtomic128(pTo,&from);
	}
		
	template <typename T>
	inline T LoadAcquire128( volatile T * ptr )
	{
		COMPILER_ASSERT( sizeof(T) == 16 );
		
		Atomic128 ret = LoadAcquireAtomic128((Atomic128 *)ptr);
		
		return cb::same_size_bit_cast_p<T>( ret );
	} 
	
	template <typename T>
	inline void StoreReleaseT( cb::Bytes<16> obj, volatile T * pTo, T from ) { StoreRelease128(pTo,from); }

	template <typename T>
	inline T LoadAcquireT( cb::Bytes<16> obj, volatile T * ptr ) { return LoadAcquire128(ptr); }
	
	#endif // CB_HAS_ATOMIC_128
	
	//template<size_t N> struct SizeToType { char m[N] };

	template <typename T>
	inline void StoreReleaseT( cb::Bytes<4> obj, volatile T * pTo, T from ) { StoreRelease32(pTo,from); }
	
	template <typename T>
	inline void StoreReleaseT( cb::Bytes<8> obj, volatile T * pTo, T from ) { StoreRelease64(pTo,from); }

	template <typename T>
	inline void StoreRelease( volatile T * pTo, T from )
	{
		StoreReleaseT(cb::Bytes<sizeof(T)>(),pTo,from);
	}
	
	template <typename T>
	inline T LoadAcquireT( cb::Bytes<4> obj, volatile T * ptr ) { return LoadAcquire32(ptr); }
	
	template <typename T>
	inline T LoadAcquireT( cb::Bytes<8> obj, volatile T * ptr ) { return LoadAcquire64(ptr); }
	
	template <typename T>
	inline T LoadAcquire( volatile T * ptr )
	{
		return LoadAcquireT<T>(cb::Bytes<sizeof(T)>(),ptr);
	}
	
	//=====================================================
	
	template <typename T>
	inline void StoreRelaxed( T * pTo, T from )
	{
		*pTo = from;
	}
	
	template <typename T>
	inline T LoadRelaxed( T * ptr )
	{
		return *ptr;
	}
	
	template <typename T>
	inline void StoreRelaxed( volatile T * pTo, T from )
	{
		*pTo = from;
	}
	
	template <typename T>
	inline T LoadRelaxed( volatile T * ptr )
	{
		return *ptr;
	}
	
	//=====================================================
	
	template <typename T>
	inline void StoreReleasePointer( volatile T * pTo, T from )
	{
		COMPILER_ASSERT( sizeof(T) == sizeof(void *) );
		ASSERT(	(((UINTa)pTo) % sizeof(void *)) == 0 );
		ASSERT(	(((UINTa)&from) % sizeof(void *)) == 0 );
		CompilerWriteBarrier();
		// on x86, stores are Release :
		*pTo = from;
	}
	
	template <typename T>
	inline T LoadAcquirePointer( volatile T * ptr )
	{
		COMPILER_ASSERT( sizeof(T) == sizeof(UINTa) );
		ASSERT(	(((UINTa)ptr) % sizeof(UINTa)) == 0 );
		// on x86, loads are Acquire :
		T ret = *ptr;
		CompilerReadBarrier();
		return ret;
	} 
	
	//=====================================================
	
	// the way most people write CAS in the literature has
	//	the *opposite* argument order from InterlockedCompareExchange, so provide that here :
	// bool means it was swapped (matched old val)
	// NOTEZ :
	//	C++0x and Relacey and such have a CAS that mutates oldVal, like this :
	/*
	bool C++0xCAS ( LONG * pInto , LONG & oldVal, LONG newVal )
	{
		LONG got = InterlockedCompareExchange(pInto,newVal,oldVal);
		bool ret = (got == oldVal);
		oldVal = got; // @@ mutate
		return ret;
	}	
	*/
	
	// version that mutates oldVal would be better :
	inline bool AtomicCAS32(uint32 volatile * pInto,uint32 oldVal, uint32 newVal)
	{
		return InterlockedCompareExchange((LONG *)pInto,(LONG)newVal,(LONG)oldVal) == (LONG)oldVal;
	}
	
	#ifdef CB_64
	inline bool AtomicCAS64(uint64 volatile * pInto,uint64 oldVal, uint64 newVal)
	{
		return _InterlockedCompareExchange64((__int64 *)pInto,(__int64)newVal,(__int64)oldVal) == (__int64)oldVal;
	}
	
	#else
	
	// atomic_dwcas_fence returns bool = copy was done
	//	if not, returns *pDest in pOld
	extern int __cdecl atomic_dwcas_fence( LONGLONG * pDest, LONGLONG * pOld, const LONGLONG * pNew);
  	
	inline bool AtomicCAS64(uint64 volatile * pInto,uint64 oldVal, uint64 newVal)
	{
		// I believe asm is automatically a compiler barrier, but WTF be sure :
		CompilerReadWriteBarrier();
		// InterlockedCompareExchange64 requires Vista
		//return InterlockedCompareExchange64((volatile LONGLONG *)pInto,(LONGLONG)newVal,(LONGLONG)oldVal) == (LONGLONG)oldVal;
		int ret = atomic_dwcas_fence((LONGLONG *)pInto,(LONGLONG *)&oldVal,(LONGLONG *)&newVal);
		CompilerReadWriteBarrier();
		return !! ret;
	}
	
	#endif // CB_64
		
	// CASPointer calls CAS32 or CAS64 depending on pointer size
	inline bool AtomicCASPointer(void ** pInto,void * oldVal, void * newVal)
	{
		return InterlockedCompareExchangePointer(pInto,newVal,oldVal) == oldVal;
	}
	
	template <typename T>
	inline bool AtomicCAS_T( cb::Bytes<4> obj, volatile T * pTo, T oldVal, T newVal ) 
	{
		return AtomicCAS32((volatile uint32 *)pTo,cb::same_size_bit_cast_p<uint32>(oldVal),cb::same_size_bit_cast_p<uint32>(newVal));
	}
	
	template <typename T>
	inline bool AtomicCAS_T( cb::Bytes<8> obj, volatile T * pTo, T oldVal, T newVal ) 
	{
		return AtomicCAS64((volatile uint64 *)pTo,cb::same_size_bit_cast_p<uint64>(oldVal),cb::same_size_bit_cast_p<uint64>(newVal));
	}
		
	#ifdef CB_HAS_ATOMIC_128
		
	extern bool AtomicCAS128(Atomic128 volatile * pInto,Atomic128 oldVal, Atomic128 newVal);
	
	template <typename T>
	inline bool AtomicCAS_T( cb::Bytes<16> obj, volatile T * pTo, T oldVal, T newVal ) 
	{
		return AtomicCAS128((volatile Atomic128 *)pTo,cb::same_size_bit_cast_p<Atomic128>(oldVal),cb::same_size_bit_cast_p<Atomic128>(newVal));
	}
	
	#endif // CB_HAS_ATOMIC_128
		
	template <typename T>
	inline bool AtomicCAS( volatile T * pTo, T oldVal, T newVal ) 
	{
		return AtomicCAS_T( cb::Bytes<sizeof(T)>(), pTo, oldVal, newVal );
	}
	
	//=====================================================
	// CPMX	
	// arg order swap compared to Interlocked :
	
	inline uint32 AtomicCMPX32(uint32 volatile * pInto,uint32 oldVal, uint32 newVal)
	{
		return InterlockedCompareExchange((volatile LONG *)pInto,newVal,oldVal);
	}
	
	inline uint64 AtomicCMPX64(uint64 volatile * pInto,uint64 oldVal, uint64 newVal)
	{
		#ifdef CB_64
		
		return _InterlockedCompareExchange64((volatile __int64 *)pInto,(__int64)newVal,(__int64)oldVal);
		
		#else
		// I believe asm is automatically a compiler barrier, but WTF be sure :
		CompilerReadWriteBarrier();
		atomic_dwcas_fence((LONGLONG *)pInto,(LONGLONG *)&oldVal,(LONGLONG *)&newVal);
		CompilerReadWriteBarrier();
		return oldVal;
		#endif		
	}
	
	#ifdef CB_HAS_ATOMIC_128
		
	extern Atomic128 AtomicCMPX128(Atomic128 volatile * pInto,Atomic128 oldVal, Atomic128 newVal);
	
	template <typename T>
	inline T AtomicCMPX_T( cb::Bytes<16> obj, volatile T * pTo, T oldVal, T newVal ) 
	{
		Atomic128 ret = AtomicCMPX128((volatile Atomic128 *)pTo,cb::same_size_bit_cast_p<Atomic128>(oldVal),cb::same_size_bit_cast_p<Atomic128>(newVal));
		return cb::same_size_bit_cast_p<T>(ret);
	}
	
	#endif // CB_HAS_ATOMIC_128
		
		
	template <typename T>
	inline T AtomicCMPX_T( cb::Bytes<4> obj, volatile T * pTo, T oldVal, T newVal ) 
	{
		uint32 ret = AtomicCMPX32((volatile uint32 *)pTo,cb::same_size_bit_cast_p<uint32>(oldVal),cb::same_size_bit_cast_p<uint32>(newVal));
		return cb::same_size_bit_cast_p<T>(ret);
	}
	
	template <typename T>
	inline T AtomicCMPX_T( cb::Bytes<8> obj, volatile T * pTo, T oldVal, T newVal ) 
	{
		uint64 ret = AtomicCMPX64((volatile uint64 *)pTo,cb::same_size_bit_cast_p<uint64>(oldVal),cb::same_size_bit_cast_p<uint64>(newVal));
		return cb::same_size_bit_cast_p<T>(ret);
	}
		
	template <typename T>
	inline T AtomicCMPX( volatile T * pTo, T oldVal, T newVal ) 
	{
		return AtomicCMPX_T( cb::Bytes<sizeof(T)>(), pTo, oldVal, newVal );
	}
	
	//=====================================================
	
	extern uint32 AtomicExchange32(uint32 volatile * pInto,uint32 newVal);
	extern uint64 AtomicExchange64(uint64 volatile * pInto,uint64 newVal);
	
	extern uint32 AtomicExchangeAdd32(uint32 volatile * pInto,uint32 newVal);
	extern uint64 AtomicExchangeAdd64(uint64 volatile * pInto,uint64 newVal);
	
	
	#ifdef CB_HAS_ATOMIC_128
		
	extern Atomic128 AtomicExchange128(Atomic128 volatile * pInto,Atomic128 newVal);
	
	template <typename T>
	inline T AtomicExchange_T( cb::Bytes<16> obj, volatile T * pTo, T newVal ) 
	{
		Atomic128 ret = AtomicExchange128((volatile Atomic128 *)pTo,cb::same_size_bit_cast_p<Atomic128>(newVal));
		return cb::same_size_bit_cast_p<T>(ret);
	}
	
	#endif // CB_HAS_ATOMIC_128
	
	// CASPointer calls CAS32 or CAS64 depending on pointer size
	inline void * AtomicExchangePointer(void * volatile * pInto,void * newVal)
	{
		#ifdef CB_64
		return (void *) AtomicExchange64((uint64 volatile *)pInto,(uint64)newVal);
		#else
		return (void *) (intptr_t) AtomicExchange32((uint32 volatile *)pInto,cb::same_size_bit_cast_p<uint32>(newVal));
		#endif
	}
	
	template <typename T>
	inline T AtomicExchange_T( cb::Bytes<4> obj, volatile T * pTo, T newVal ) 
	{
		uint32 ret = AtomicExchange32((volatile uint32 *)pTo,cb::same_size_bit_cast_p<uint32>(newVal));
		return cb::same_size_bit_cast_p<T>(ret);
	}
	
	template <typename T>
	inline T AtomicExchange_T( cb::Bytes<8> obj, volatile T * pTo, T newVal ) 
	{
		uint64 ret = AtomicExchange64((volatile uint64 *)pTo,cb::same_size_bit_cast_p<uint64>(newVal));
		return cb::same_size_bit_cast_p<T>(ret);
	}
		
	template <typename T>
	inline T AtomicExchange( volatile T * pTo, T newVal ) 
	{
		return AtomicExchange_T( cb::Bytes<sizeof(T)>(), pTo, newVal );
	}
	
	template <typename T>
	inline T AtomicExchangeAdd_T( cb::Bytes<4> obj, volatile T * pTo, T newVal ) 
	{
		uint32 ret = AtomicExchangeAdd32((volatile uint32 *)pTo,cb::same_size_bit_cast_p<uint32>(newVal));
		return cb::same_size_bit_cast_p<T>(ret);
	}
	
	template <typename T>
	inline T AtomicExchangeAdd_T( cb::Bytes<8> obj, volatile T * pTo, T newVal ) 
	{
		uint64 ret = AtomicExchangeAdd64((volatile uint64 *)pTo,cb::same_size_bit_cast_p<uint64>(newVal));
		return cb::same_size_bit_cast_p<T>(ret);
	}
		
	template <typename T>
	inline T AtomicExchangeAdd( volatile T * pTo, T newVal ) 
	{
		return AtomicExchangeAdd_T( cb::Bytes<sizeof(T)>(), pTo, newVal );
	}
	
	//=====================================================
		
	// Atomic_t is the fundamental type that can be used atomically
	typedef LONG Atomic_t;
	COMPILER_ASSERT( sizeof(Atomic_t) == 4 );
	
	class AtomicVal
	{
	public:
		explicit AtomicVal()  : m_value(0) { }
		explicit AtomicVal( Atomic_t val ) : m_value(val) { }
		~AtomicVal() { }
		
		Atomic_t LoadAcquire() const { return cb::LoadAcquire(&m_value); }
		void StoreRelease(Atomic_t val) { cb::StoreRelease(&m_value,val); }
	
		bool AtomicCAS(Atomic_t oldVal, Atomic_t newVal)
		{
			return InterlockedCompareExchange(&m_value,newVal,oldVal) == oldVal;
		}
				
		//void AtomicIncrement() { InterlockedIncrement(&m_value); }
		//void AtomicDecrement() { InterlockedDecrement(&m_value); }
	
		Atomic_t AtomicExchange(Atomic_t newVal) { return InterlockedExchange(&m_value,newVal); }

		Atomic_t AtomicExchangeAdd(Atomic_t newVal) { return InterlockedExchangeAdd(&m_value,newVal); }
	
	private:
		// don't provide operator = or == ; make the client write Store( Load() )
		FORBID_CLASS_STANDARDS(AtomicVal);
		
		volatile Atomic_t	m_value;
	};
	
	//-------------------------------------
	
	template <typename T>
	struct Atomic
	{
	//public:
		//explicit Atomic()  : m_value() { }
		//explicit Atomic( T val ) : m_value(val) { }
		//~Atomic() { }
		
		T LoadAcquire() const volatile { return cb::LoadAcquire(&m_value); }
		void StoreRelease(T val) volatile { cb::StoreRelease(&m_value,val); }
		
		T LoadRelaxed()  const { return m_value; }
		void StoreRelaxed(T val) { m_value = val; }
	
		bool CAS(T oldVal, T newVal) volatile { return cb::AtomicCAS(&m_value,oldVal,newVal); }

		T CMPX(T oldVal, T newVal) volatile { return cb::AtomicCMPX(&m_value,oldVal,newVal); }
		
		T Exchange(T newVal) volatile { return cb::AtomicExchange(&m_value,newVal); }

		T ExchangeAdd(T newVal) volatile { return cb::AtomicExchangeAdd(&m_value,newVal); }
		
		/*		
		void AtomicIncrement() { InterlockedIncrement(&m_value); }
		void AtomicDecrement() { InterlockedDecrement(&m_value); }
	
		T AtomicExchange(T newVal) { return InterlockedExchange(&m_value,newVal); }
		*/
		
	//private:
		// don't provide operator = or == ; make the client write Store( Load() )
		//OODLE_FORBID_CLASS_STANDARDS(Atomic);
				
		#ifdef CB_HAS_ATOMIC_128
		COMPILER_ASSERT( sizeof(T) == 4 || sizeof(T) == 8 || sizeof(T) == 16 );
		#else
		COMPILER_ASSERT( sizeof(T) == sizeof(uint32) || sizeof(T) == sizeof(uint64) );
		#endif
		
		T	m_value;
	};
	
	//=====================================================
	
	template <typename T>
	inline void StoreRelease( volatile Atomic<T> * pTo, T from )
	{
		pTo->StoreRelease(from);
	}
	
	template <typename T>
	inline T LoadAcquire( volatile Atomic<T> * ptr )
	{
		return ptr->LoadAcquire();
	}

	template <typename T>
	inline void StoreRelaxed( volatile Atomic<T> * pTo, T from )
	{
		((Atomic<T> *)pTo)->StoreRelaxed(from);
	}
	
	template <typename T>
	inline T LoadRelaxed( volatile Atomic<T> * ptr )
	{
		return ((Atomic<T> *)ptr)->LoadRelaxed();
	}
	
	template <typename T>
	inline void StoreRelaxed( Atomic<T> * pTo, T from )
	{
		pTo->StoreRelaxed(from);
	}
	
	template <typename T>
	inline T LoadRelaxed( Atomic<T> * ptr )
	{
		return ptr->LoadRelaxed();
	}
	
	template <typename T>
	inline T AtomicExchange( volatile Atomic<T> * pTo, T newVal ) 
	{
		return pTo->Exchange(newVal);
	}

	template <typename T>
	inline T AtomicExchangeAdd( volatile Atomic<T> * pTo, T newVal ) 
	{
		return pTo->ExchangeAdd(newVal);
	}
	
	template <typename T>
	inline T AtomicCMPX( volatile Atomic<T> * pTo, T oldVal, T newVal ) 
	{
		return pTo->CMPX(oldVal,newVal);
	}
	
	template <typename T>
	inline bool AtomicCAS( volatile Atomic<T> * pTo, T oldVal, T newVal ) 
	{
		return pTo->CAS(oldVal,newVal);
	}
		
	//-------------------------------------
	
	template <typename T>
	struct SingletonThreadSafe
	{
		T *		m_ptr;
		SimpleMutex m_mutex;

		// MUST initialize with C static initializer list = { 0 };
		// do NOT put constructors here

		// thread safe Get/create :
		T * Get()
		{
			// no Acquire needed because reads of T will be dependent
			if ( m_ptr )
				return m_ptr;

			// lock mutex :
			SimpleLock(&m_mutex);

			if ( m_ptr == NULL )
			{
				T * pNew = new T;
				// pNew is flushed befure we write m_ptr
				StoreRelease(&m_ptr,pNew);
			}

			SimpleUnlock(&m_mutex);

			return m_ptr;
		}

		// thread-safe release
		void Release()
		{
			// atomically swap in NULL :
			T * ptr = AtomicExchange(&m_ptr,NULL);
			
			if ( ptr )
			{
				delete ptr;
			}
		}
		
		/*
		// for single threaded use :
		
		void CreateNonThreadSafe()
		{
			if ( ! m_ptr )
			{
				m_ptr = new T;
			}		
		}

		void ReleaseNonThreadSafe()
		{
			if ( m_ptr )
			{
				delete m_ptr;
				m_ptr = NULL;
			}
		}
		*/
	};

	/*
	// this is risky
	#define DO_ONCE_THREAD_SAFE( exp ) do { \
		static LONG s_once = 0; 			\
		if ( s_once == 0 ) { 				\
			if ( InterlockedExchange(&s_once,1) == 0 ) { \
			CompilerReadWriteBarrier();  	\
			exp;							\
			CompilerReadWriteBarrier(); 	\
			}								\
		} } while(0)
	*/
	
	//-------------------------------------
	/***
	
	two easy ways to make a safe singleton critsec :
	
	SingletonThreadSafe<CriticalSection> g_critSec = { 0 };
	
	CB_SCOPE_CRITICAL_SECTION( *g_critSec.Get() );
	
	or :
	
	CRITICAL_SECTION g_critSec = { 0 };
	DO_ONCE_THREAD_SAFE( InitializeCriticalSection(&g_critSec) );
	
	CB_SCOPE_CRITICAL_SECTION( g_critSec );
	
	***/
	
	//-------------------------------------
		
	// PointerAndCount is either 64 of 128 bits in size
	//	(size of 2 pointers)
	//__declspec(align(LF_SIZEOF2POINTERS))
	class PointerAndCount
	{
	public:
		PointerAndCount() { }
		~PointerAndCount() { }
		
		PointerAndCount(void * ptr,uint16 count)
		{
			SetPointer(ptr);
			SetCount(count);
		}
		
		void SetPointer(void * ptr)
		{
			m_pointer = ptr;
		}
		void * GetPointer() const
		{
			return m_pointer;
		}
		void SetCount(uint16 count)
		{
			m_count = count;
		}
		uint16 GetCount() const
		{
			return (uint16)m_count;	
		}
	
	private:
		intptr_t	m_count	  ;
		void *		m_pointer ;
	};
	COMPILER_ASSERT( sizeof(PointerAndCount) == 2*sizeof(void *) );
	
	#ifdef CB_64
	//LF_ALIGN_8
	class PackedPointerAndCount64
	{
	public:
		PackedPointerAndCount64() { }
		~PackedPointerAndCount64() { }
		
		PackedPointerAndCount64(void * ptr,uint16 count)
		{
			SetPointer(ptr);
			SetCount(count);
		}
		
		void SetPointer(void * ptr)
		{
			//uint64 p = (uint64)ptr;
			// simpler to just set it and then verify the Get :
			//ASSERT( (p&0xF) == 0 );
			//p >>= 4;
			//ASSERT( (p&0xFFFFFFFFFFFFi64) == p );
			m_pointer = (uint64)ptr >>4;
			ASSERT( GetPointer() == ptr );
		}
		void * GetPointer() const
		{
			return (void *)(m_pointer<<4);
		}
		void SetCount(uint16 count)
		{
			m_count = count;
		}
		uint16 GetCount() const
		{
			return m_count;	
		}
	
	private:
		uint64 m_count   : 16;
		uint64 m_pointer : 48; // actually a 52 bit pointer
	};
	#else
	// on 32 bit, PackedPointerAndCount64 can just be PointerAndCount
	#define PackedPointerAndCount64 PointerAndCount
	#endif
	
	COMPILER_ASSERT( sizeof(PackedPointerAndCount64) == sizeof(uint64) );
	
	
	#ifdef CB_64
	#ifdef CB_HAS_ATOMIC_128
	
	// AtomicPointerAndCountSize cannot use sizeof cuz its in aligns
	
	#define AtomicPointerAndCount		PointerAndCount
	#define AtomicPointerAndCountSize	16
	
	#else
	
	#define AtomicPointerAndCount		PackedPointerAndCount64
	#define AtomicPointerAndCountSize	8
	
	#endif
	#else
	
	#define AtomicPointerAndCount		PointerAndCount
	#define AtomicPointerAndCountSize	8
	
	#endif
	
	COMPILER_ASSERT( sizeof(AtomicPointerAndCount) == AtomicPointerAndCountSize );
	
	inline bool operator == (const AtomicPointerAndCount & lhs,const AtomicPointerAndCount &rhs)
	{
		return ByteEqual(lhs,rhs);
	}

	//-------------------------------------
	
END_CB

//=======================================================================
#if 1 //def THREAD_RELACY

// stuff for compatibility with Relacy :

#define VAR_T(T)					T
#define ATOMIC_VAR(T)				cb::Atomic<T>
#define NOT_THREAD_SAFE(T)			VAR_T(T)
#define RL_ATOMIC_VAR(T)			cb::Atomic<T>
#define RL_NOT_THREAD_SAFE(T)		VAR_T(T)
#define RL_ASSERT(exp)				ASSERT(exp)
#define NOT_THREAD_SAFE_ATOMIC(T)	VAR_T(T)

#pragma warning(disable : 4324)
//c:\devel\projects\oodle\Core\LFSList.h(41) : warning C4324: 'cb::LFSNode' : structure was padded due to __declspec(align())

#endif
//=======================================================================
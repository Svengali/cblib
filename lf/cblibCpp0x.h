#pragma once

//===============================================

#define _WIN32_WINNT 0x0600 // Vista needed if you want condition var

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>
#include <emmintrin.h>

extern "C"
{
	void _ReadWriteBarrier(void);
}
#pragma intrinsic(_ReadWriteBarrier)

#define LF_OS_COMPILER_BARRIER	_ReadWriteBarrier

#define LF_OS_FORBID_CLASS_STANDARDS(x)	private: void operator=(const x&); x(const x&);
	
#define LF_OS_COMPILER_ASSERT(exp)	extern char _dummy_array[ (exp) ? 1 : -1 ]

#define LF_OS_ASSERT(exp)	if ( ! (exp) ) DebugBreak()

#define LF_OS_ASSERT_NATURALLY_ALIGNED_POINTER( ptr )	LF_OS_ASSERT( (((intptr_t)ptr) & (sizeof(*ptr)-1)) == 0 )

#ifdef _WIN64
#define LF_OS_64 1
//#if _MSC_VER >= 1500
#define LF_OS_HAS_ATOMIC_128
//#endif
#define LF_OS_SIZEOFPOINTER 8
#define LF_OS_SIZEOF2POINTERS 16
#else
#define LF_OS_64 0
#define LF_OS_SIZEOFPOINTER 4
#define LF_OS_SIZEOF2POINTERS 8
#endif

// LF_OS_SIZEOFPOINTER is defined without using sizeof so it can be in align
LF_OS_COMPILER_ASSERT( LF_OS_SIZEOFPOINTER == sizeof(void *) ); 
LF_OS_COMPILER_ASSERT( LF_OS_SIZEOF2POINTERS == 2*sizeof(void *) ); 

#define LF_OS_ALIGN_8		__declspec(align(8))
#define LF_OS_ALIGN_16		__declspec(align(16))

#define LF_OS_ALIGN_POINTER		__declspec(align(LF_OS_SIZEOFPOINTER))
#define LF_OS_ALIGN_2POINTERS		__declspec(align(LF_OS_SIZEOF2POINTERS))

#define LF_OS_CACHE_LINE_SIZE	SYSTEM_CACHE_ALIGNMENT_SIZE

#define LF_OS_ALIGN_CACHE_LINE		__declspec(align(LF_OS_CACHE_LINE_SIZE))

#ifdef LF_OS_HAS_ATOMIC_128
extern "C" unsigned char _InterlockedCompareExchange128(
   __int64 volatile * Destination,
   __int64 ExchangeHigh,
   __int64 ExchangeLow,
   __int64 * ComparandResult
);
#endif

namespace lf_os
{


	typedef unsigned char   	uint8;
	typedef unsigned short		uint16;
	typedef unsigned int		uint32;
	typedef unsigned __int64	uint64;

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

	inline void ThreadYieldToAny(void)
	{
		SwitchToThread();
	}

	inline void ThreadYieldNotLower(void)
	{
		Sleep(0);
	}

	inline void ThreadSleep(int sleepTime)
	{
		Sleep(sleepTime);
	}

	//-----------------------------------------------
	
	template <int n_count>
	class Bytes
	{
	public:
		char bytes[n_count];
	};
	
	// same_size_bit_cast casts the bits in memory
	//	eg. it's not a value cast
	template <typename t_to, typename t_fm>
	t_to & same_size_bit_cast_p( t_fm & from )
	{
		LF_OS_COMPILER_ASSERT( sizeof(t_to) == sizeof(t_fm) );
		// cast through char * to make aliasing work ?
		char * ptr = (char *) &from;
		return *( (t_to *) ptr );
	}

	//-------------------------------------

	template <typename T>
	inline void StoreRelease32( volatile T * pTo, T from )
	{
		LF_OS_COMPILER_ASSERT( sizeof(T) == sizeof(uint32) );
		LF_OS_ASSERT_NATURALLY_ALIGNED_POINTER(pTo);
		LF_OS_ASSERT_NATURALLY_ALIGNED_POINTER(&from);
		// on x86, stores are Release :
		*((volatile uint32 *)pTo) = lf_os::same_size_bit_cast_p<uint32>( from );
		LF_OS_COMPILER_BARRIER();
	}
	
	template <typename T>
	inline T LoadAcquire32( volatile T * ptr )
	{
		LF_OS_COMPILER_ASSERT( sizeof(T) == sizeof(uint32) );
		LF_OS_ASSERT_NATURALLY_ALIGNED_POINTER(ptr);
		// on x86, loads are Acquire :
		uint32 ret = *((volatile uint32 *)ptr);
		LF_OS_COMPILER_BARRIER();
		return lf_os::same_size_bit_cast_p<T>( ret );
	} 
	
	template <typename T>
	inline void StoreRelease64( volatile T * pTo, T from )
	{
		LF_OS_COMPILER_ASSERT( sizeof(T) == sizeof(uint64) );
		LF_OS_ASSERT_NATURALLY_ALIGNED_POINTER(pTo);
		//LF_OS_ASSERT(	(((intptr_t)&from)&7) == 0 );
		LF_OS_COMPILER_BARRIER();
		#if LF_OS_64
		// on x86, stores are Release :
		*((volatile uint64 *)pTo) = lf_os::same_size_bit_cast_p<uint64>( from );
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
		LF_OS_COMPILER_ASSERT( sizeof(T) == sizeof(uint64) );
		LF_OS_ASSERT_NATURALLY_ALIGNED_POINTER(src);
		#if LF_OS_64
		// on x86, loads are Acquire :
		uint64 ret = *((volatile uint64 *)src);
		LF_OS_COMPILER_BARRIER();
		return lf_os::same_size_bit_cast_p<T>( ret );
		#else
		uint64 outv;
		__asm
		{
		mov eax,src
		fild qword ptr [eax]
		fistp qword ptr [outv]
		}
		return lf_os::same_size_bit_cast_p<T>( outv );
		#endif
	} 

	#ifdef LF_OS_HAS_ATOMIC_128
	
	// Windows has an M128A	
	// intrin has __m128i
	
	//typedef __m128i Atomic128;

	LF_OS_ALIGN_16 struct Atomic128
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
	
	LF_OS_COMPILER_ASSERT( sizeof(Atomic128) == 16 );
	
	inline void StoreReleaseAtomic128( volatile Atomic128 * pTo, const Atomic128 * from )
	{
		LF_OS_ASSERT_NATURALLY_ALIGNED_POINTER(pTo);
		LF_OS_ASSERT_NATURALLY_ALIGNED_POINTER(from);
		LF_OS_COMPILER_BARRIER();

        #if false // #if _MSC_VER >= 1500
		__store128_rel(pTo,from->high,from->low);
		#else
		_mm_sfence(); // release fence
		_mm_store_si128((__m128i *)pTo,*((__m128i *)from));
		#endif
	}
	
	inline Atomic128 LoadAcquireAtomic128( volatile const Atomic128 * ptr )
	{		
		LF_OS_ASSERT_NATURALLY_ALIGNED_POINTER(ptr);
		
        #if false // #if _MSC_VER >= 1500
        Atomic128 ret;
		LF_OS_ASSERT(	(((intptr_t)&ret)&0xF) == 0 );
		__load128_acq((int64 *)ptr,&ret.low);
		#else
		// use SSE
		__m128i ret = _mm_load_si128((const __m128i *)ptr);
		_mm_lfence(); // acquire fence
		#endif		
		
		LF_OS_COMPILER_BARRIER();
		
		return lf_os::same_size_bit_cast_p<Atomic128>(ret);
	}
	
	template <typename T>
	inline void StoreRelease128( volatile T * pTo, T from )
	{
		LF_OS_COMPILER_ASSERT( sizeof(T) == 16 );
		
		StoreReleaseAtomic128((volatile Atomic128 *)pTo,(const Atomic128 *)&from);
	}
		
	template <typename T>
	inline T LoadAcquire128( volatile T * ptr )
	{
		LF_OS_COMPILER_ASSERT( sizeof(T) == 16 );
		
		Atomic128 ret = LoadAcquireAtomic128((volatile Atomic128 *)ptr);
		
		return lf_os::same_size_bit_cast_p<T>( ret );
	} 
	
	template <typename T>
	inline void StoreReleaseT( lf_os::Bytes<16> obj, volatile T * pTo, T from ) { obj; StoreRelease128(pTo,from); }

	template <typename T>
	inline T LoadAcquireT( lf_os::Bytes<16> obj, volatile T * ptr ) { obj; return LoadAcquire128(ptr); }
	
	#endif // LF_OS_HAS_ATOMIC_128
	
	template <typename T>
	inline void StoreReleaseT( lf_os::Bytes<4> obj, volatile T * pTo, T from ) { obj; StoreRelease32(pTo,from); }
	
	template <typename T>
	inline void StoreReleaseT( lf_os::Bytes<8> obj, volatile T * pTo, T from ) { obj; StoreRelease64(pTo,from); }

	template <typename T>
	inline void StoreRelease( volatile T * pTo, T from )
	{
		StoreReleaseT(lf_os::Bytes<sizeof(T)>(),pTo,from);
	}
	
	template <typename T>
	inline T LoadAcquireT( lf_os::Bytes<4> obj, volatile T * ptr ) { obj; return LoadAcquire32(ptr); }
	
	template <typename T>
	inline T LoadAcquireT( lf_os::Bytes<8> obj, volatile T * ptr ) { obj; return LoadAcquire64(ptr); }
	
	template <typename T>
	inline T LoadAcquire( volatile T * ptr )
	{
		return LoadAcquireT<T>(lf_os::Bytes<sizeof(T)>(),ptr);
	}
	
	//=====================================================

	// on x86 Relaxed == Acquire/Release
	//	note these must still be *atomic*
	
	template <typename T>
	inline void StoreRelaxed( T * pTo, T from )
	{
		// *pTo = from; // NO!
		StoreRelease(pTo,from);
	}
	
	template <typename T>
	inline T LoadRelaxed( T * ptr )
	{
		//return *ptr; // NO!
		return LoadAcquire(ptr);
	}
	
	template <typename T>
	inline void StoreRelaxed( volatile T * pTo, T from )
	{
		//*pTo = from;
		StoreRelease(pTo,from);
	}
	
	template <typename T>
	inline T LoadRelaxed( volatile T * ptr )
	{
		//return *ptr;
		return LoadAcquire(ptr);
	}
	
	//=====================================================
	
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
	inline bool AtomicCAS32(uint32 volatile * pInto,uint32 & oldVal, uint32 newVal)
	{
		LF_OS_ASSERT_NATURALLY_ALIGNED_POINTER(pInto);
		LF_OS_ASSERT_NATURALLY_ALIGNED_POINTER(&oldVal);
		LONG got = InterlockedCompareExchange((LONG *)pInto,(LONG)newVal,(LONG)oldVal);
		bool did = (got == (LONG)oldVal);
		oldVal = same_size_bit_cast_p<uint32>(got);
		return did;
	}
	
	inline bool AtomicCAS64(uint64 volatile * pInto,uint64 & oldVal, uint64 newVal)
	{
		LF_OS_ASSERT_NATURALLY_ALIGNED_POINTER(pInto);
		LF_OS_ASSERT_NATURALLY_ALIGNED_POINTER(&oldVal);
		__int64 got = _InterlockedCompareExchange64((__int64 *)pInto,(__int64)newVal,(__int64)oldVal);
		bool did = (got == (__int64)oldVal);
		oldVal = same_size_bit_cast_p<uint64>(got);
		return did;
	}
			
	template <typename T>
	inline bool AtomicCAS_T( lf_os::Bytes<4> obj, volatile T * pTo, T & oldVal, T newVal ) 
	{
		obj;
		return AtomicCAS32((volatile uint32 *)pTo,lf_os::same_size_bit_cast_p<uint32>(oldVal),lf_os::same_size_bit_cast_p<uint32>(newVal));
	}
	
	template <typename T>
	inline bool AtomicCAS_T( lf_os::Bytes<8> obj, volatile T * pTo, T & oldVal, T newVal ) 
	{
		obj;
		return AtomicCAS64((volatile uint64 *)pTo,lf_os::same_size_bit_cast_p<uint64>(oldVal),lf_os::same_size_bit_cast_p<uint64>(newVal));
	}
		
	#ifdef LF_OS_HAS_ATOMIC_128
		
	inline bool AtomicCAS128(Atomic128 volatile * pInto,Atomic128 & oldVal, const Atomic128 & newVal)
	{
		LF_OS_ASSERT_NATURALLY_ALIGNED_POINTER(pInto);
		LF_OS_ASSERT_NATURALLY_ALIGNED_POINTER(&oldVal);
		LF_OS_ASSERT_NATURALLY_ALIGNED_POINTER(&newVal);
		
		const __int64 * pNew64 = (const __int64 *)&newVal;
			
		unsigned char did = _InterlockedCompareExchange128((__int64 *)pInto,pNew64[1],pNew64[0],(__int64 *)&oldVal);
		return !! did;
	}
	
	template <typename T>
	inline bool AtomicCAS_T( lf_os::Bytes<16> obj, volatile T * pTo, T & oldVal, T newVal ) 
	{
		obj;
		return AtomicCAS128((volatile Atomic128 *)pTo,lf_os::same_size_bit_cast_p<Atomic128>(oldVal),lf_os::same_size_bit_cast_p<Atomic128>(newVal));
	}
	
	#endif // LF_OS_HAS_ATOMIC_128
		
	template <typename T>
	inline bool AtomicCAS( volatile T * pTo, T & oldVal, T newVal ) 
	{
		return AtomicCAS_T( lf_os::Bytes<sizeof(T)>(), pTo, oldVal, newVal );
	}
	
	//=====================================================
	
	inline uint32 AtomicExchange32(uint32 volatile * pInto,uint32 newVal)
	{
		LF_OS_ASSERT_NATURALLY_ALIGNED_POINTER(pInto);
		return (uint32) InterlockedExchange((volatile LONG *)pInto,newVal);
	}

	inline uint32 AtomicExchangeAdd32(uint32 volatile * pInto,uint32 inc)
	{
		LF_OS_ASSERT_NATURALLY_ALIGNED_POINTER(pInto);
		return (uint32) InterlockedExchangeAdd((volatile LONG *)pInto,(LONG)inc);
	}
	
	inline uint64 AtomicExchange64(uint64 volatile * pInto,uint64 newVal)
	{
		LF_OS_ASSERT_NATURALLY_ALIGNED_POINTER(pInto);
		return (uint64) InterlockedExchange64((volatile LONGLONG *)pInto,newVal);
	}

	inline uint64 AtomicExchangeAdd64(uint64 volatile * pInto,uint64 inc)
	{
		LF_OS_ASSERT_NATURALLY_ALIGNED_POINTER(pInto);
		return (uint64) InterlockedExchangeAdd64((volatile LONGLONG *)pInto,(LONGLONG)inc);
	}
		
	#ifdef LF_OS_HAS_ATOMIC_128
		
		
	inline Atomic128 AtomicExchange128(Atomic128 volatile * pInto,Atomic128 newVal)
	{
		LF_OS_ASSERT_NATURALLY_ALIGNED_POINTER(pInto);
		LF_OS_ASSERT_NATURALLY_ALIGNED_POINTER(&newVal);
		LF_OS_COMPILER_BARRIER();
		Atomic128 oldVal = LoadAcquire(pInto);
		while ( ! AtomicCAS128(pInto,oldVal,newVal) )
		{
			lf_os::HyperYieldProcessor();
		}
		LF_OS_COMPILER_BARRIER();
		
		return oldVal;
	}	
	
	template <typename T>
	inline T AtomicExchange_T( lf_os::Bytes<16> obj, volatile T * pTo, T newVal ) 
	{
		obj;
		Atomic128 ret = AtomicExchange128((volatile Atomic128 *)pTo,lf_os::same_size_bit_cast_p<Atomic128>(newVal));
		return lf_os::same_size_bit_cast_p<T>(ret);
	}
	
	#endif // LF_OS_HAS_ATOMIC_128
		
	template <typename T>
	inline T AtomicExchange_T( lf_os::Bytes<4> obj, volatile T * pTo, T newVal ) 
	{
		obj;
		uint32 ret = AtomicExchange32((volatile uint32 *)pTo,lf_os::same_size_bit_cast_p<uint32>(newVal));
		return lf_os::same_size_bit_cast_p<T>(ret);
	}
	
	template <typename T>
	inline T AtomicExchange_T( lf_os::Bytes<8> obj, volatile T * pTo, T newVal ) 
	{
		obj;
		uint64 ret = AtomicExchange64((volatile uint64 *)pTo,lf_os::same_size_bit_cast_p<uint64>(newVal));
		return lf_os::same_size_bit_cast_p<T>(ret);
	}
		
	template <typename T>
	inline T AtomicExchange( volatile T * pTo, T newVal ) 
	{
		return AtomicExchange_T( lf_os::Bytes<sizeof(T)>(), pTo, newVal );
	}
	
	template <typename T>
	inline T AtomicExchangeAdd_T( lf_os::Bytes<4> obj, volatile T * pTo, T newVal ) 
	{
		obj;
		uint32 ret = AtomicExchangeAdd32((volatile uint32 *)pTo,lf_os::same_size_bit_cast_p<uint32>(newVal));
		return lf_os::same_size_bit_cast_p<T>(ret);
	}
	
	template <typename T>
	inline T AtomicExchangeAdd_T( lf_os::Bytes<8> obj, volatile T * pTo, T newVal ) 
	{
		obj;
		uint64 ret = AtomicExchangeAdd64((volatile uint64 *)pTo,lf_os::same_size_bit_cast_p<uint64>(newVal));
		return lf_os::same_size_bit_cast_p<T>(ret);
	}
		
	template <typename T>
	inline T AtomicExchangeAdd( volatile T * pTo, T newVal ) 
	{
		return AtomicExchangeAdd_T( lf_os::Bytes<sizeof(T)>(), pTo, newVal );
	}
		
	//-----------------------------------------------

};

//===============================================	

#define LF_OS_NONATOMIC_PROXY(type)	nonatomic<type>
#define LF_OS_NONATOMIC(type)	nonatomic<type>
	
#ifndef $
// you can use a ($) like Relacy :
//	here it is a nop, though
#define $ access_info()
#endif

//===============================================

#ifndef CBLIB_LF_NS
#define CBLIB_LF_NS	cb_lf
#endif

namespace CBLIB_LF_NS
{

	enum memory_order  
	{
	  memory_order_relaxed, memory_order_consume, memory_order_acquire, memory_order_release,
	  memory_order_acq_rel, memory_order_seq_cst,
	  mo_relaxed =memory_order_relaxed ,
	  mo_consume =memory_order_consume ,
	  mo_acquire =memory_order_acquire ,
	  mo_release =memory_order_release ,
	  mo_acq_rel =memory_order_acq_rel ,
	  mo_seq_cst =memory_order_seq_cst 
	};
	
	struct access_info { };
	
	class backoff
	{
	public:
	
		enum { backoff_spincount = 100 };
		enum { backoff_yields = 10 };
					
		backoff() : m_count(0) { }
	
		void reset()
		{
			m_count = 0;
		}
	
		bool will_yield_sleep() const
		{
			return ( m_count >= backoff_spincount );
		}
		
		void yield()
		{
			// if Spins is 1 I want to spin once
			if ( m_count < backoff_spincount )
			{
				// spinning CPU
				lf_os::HyperYieldProcessor();
			}
			else
			{
				int sleepTime = m_count - backoff_spincount;
				// sleepTime starts at 0 and counts up linearly
			
				if ( sleepTime < backoff_yields )
				{
					lf_os::ThreadYieldToAny();
				}
				else
				{
					sleepTime = sleepTime - backoff_yields + 1;
					
					/*		
					if ( sleepTime > SpinBackOff_MaxSleep )
					{
						FAIL("Likely infinite loop in SpinBackOff\n");	
						m_count = 0; // for debugging, restart the count
					}
					*/
					
					lf_os::ThreadSleep(sleepTime);
				}
			}
			
			m_count++;
		}
		
		void yield(const access_info & rsp) 
		{
			rsp;
			yield();
		}
		
	private:
		int m_count;
	};
	
	//class linear_backoff : public backoff { };
	//class exp_backoff : public backoff { };
	
	template <typename T>
	class atomic
	{
	private:
		T			m_value;
		
	public:
	
		typedef atomic<T> this_type;
		
		atomic()  { }
		~atomic() { }
	
		atomic(const T & initVal) { store(initVal,mo_relaxed); }
		
		T fetch_add(const T & inc, memory_order mo);
	
		T fetch_or(const T & bitmask, memory_order mo); 
	
		bool compare_exchange_strong(T &oldV, const T & newV, memory_order mo_cas, memory_order mo_load = mo_acquire);
		bool compare_exchange_weak(T &oldV, const T & newV, memory_order mo_cas, memory_order mo_load = mo_acquire)
		{
			return compare_exchange_strong(oldV,newV,mo_cas,mo_load);
		}
	
		T load(memory_order mo) const;
	
		T exchange(const T & newV,memory_order mo);
	
		void store(const T & val,memory_order mo);
		
		// for relacey ($) syntax :
		this_type & operator () (const access_info & rsp) { rsp; return *this; }
		const this_type & operator () (const access_info & rsp) const { rsp; return *this; }
	
		//-----------------
		// cheater :
		T * GetPtr() { return &(m_value); }
	
		LF_OS_FORBID_CLASS_STANDARDS(atomic);
	};

	template <typename T>
	T atomic<T>::fetch_add(const T &  inc, memory_order mo)
	{
		LF_OS_COMPILER_BARRIER();
		// always seq_cst :
		mo;
		T ret = lf_os::AtomicExchangeAdd(&m_value,inc);
		LF_OS_COMPILER_BARRIER();
		return ret;
	}
	
	template <typename T>
	T atomic<T>::fetch_or(const T & mask, memory_order mo)
	{
		LF_OS_COMPILER_BARRIER();
		// always seq_cst :
		T old = lf_os::LoadAcquire(&m_value);
		backoff bo;
		while ( ! compare_exchange_weak(old,old|mask,mo) )
		{
			bo.yield();
		}
		LF_OS_COMPILER_BARRIER();
		return old;
	}
	
	template <typename T>
	bool atomic<T>::compare_exchange_strong(T &oldV, const T &  newV, memory_order mo_cas, memory_order mo_load)
	{
		mo_cas; mo_load;
		LF_OS_COMPILER_BARRIER();
		bool did = lf_os::AtomicCAS(&m_value,oldV,newV);
		LF_OS_COMPILER_BARRIER();
		return did;		
	}

	template <typename T>
	T atomic<T>::exchange(const T & newV,memory_order mo)
	{
		mo;
		LF_OS_COMPILER_BARRIER();
		// always seq_cst :
		T old = lf_os::AtomicExchange(&m_value,newV);
		LF_OS_COMPILER_BARRIER();
		return old;	
	}

	template <typename T>
	T atomic<T>::load(memory_order mo) const
	{
		LF_OS_COMPILER_BARRIER();
		T ret;
		if ( mo < mo_acquire ) ret = lf_os::LoadRelaxed(&m_value);
		else if ( mo == mo_acquire ) ret = lf_os::LoadAcquire(&m_value);
		//else ret = const_cast<OodleAtomic<T> &>(m_value).ExchangeAdd(0);
		else
		{
			const_cast<atomic<T> &>(*this).compare_exchange_strong(ret,ret,mo,mo);
		}
		LF_OS_COMPILER_BARRIER();
		return ret;
	}
	
	template <typename T>
	void atomic<T>::store(const T & val,memory_order mo)
	{
		LF_OS_COMPILER_BARRIER();
		if ( mo < mo_release ) lf_os::StoreRelaxed(&m_value,val);
		else if ( mo == mo_release ) lf_os::StoreRelease(&m_value,val);
		else exchange(val,mo);
		LF_OS_COMPILER_BARRIER();
	}
			
	template <typename T>
	class nonatomic
	{
	private:
		T	m_val;
	public:
		nonatomic() { }
		nonatomic(T initVal) : m_val(initVal) { }
		
		operator T & () { return m_val; }
		operator T () const { return m_val; }
		T operator -> () { return m_val; }
		void operator = (T t) { m_val = t; }
		bool operator == (T t) const { return m_val == t; }
		
		// for relacey ($) syntax :
		T & operator () (const access_info & rsp) { rsp; return m_val; }
		const T & operator () (const access_info & rsp) const { rsp; return m_val; }
	};

	//=================================

	class mutex
	{
	public:
		CRITICAL_SECTION	m_cs;
		
		 mutex() { InitializeCriticalSectionAndSpinCount(&m_cs,100); }
		~mutex() { DeleteCriticalSection(&m_cs); }
		
		void lock() { EnterCriticalSection(&m_cs); }
		void unlock() { LeaveCriticalSection(&m_cs); } 

		// for relacey ($) syntax :
		
		void lock(const access_info & rsp) { rsp; lock(); } 
		void unlock(const access_info & rsp) { rsp; unlock(); } 
		
		LF_OS_FORBID_CLASS_STANDARDS(mutex);
	};	
		
	class event
	{
	public:
		HANDLE m_event;
		
		 event() { m_event = CreateEvent(NULL, 0, 0, NULL); }
		~event() { CloseHandle(m_event); }
		
		void signal() { SetEvent(m_event); }
		void wait_clear()  { WaitForSingleObject(m_event,INFINITE); }
		bool wait_clear_timeout(int millis)  { return WaitForSingleObject(m_event,millis) == WAIT_OBJECT_0; }
		void clear_manual() { ResetEvent(m_event); }

		LF_OS_FORBID_CLASS_STANDARDS(event);
	};	
	
	class semaphore
	{
	public:
		HANDLE m_sem;
		
		 semaphore(int initCount = 0 ) { m_sem = CreateSemaphore( NULL, initCount, (1<<28), NULL ); }
		 ~semaphore() { CloseHandle(m_sem); }
		
		void post() { ReleaseSemaphore(m_sem,1,NULL); }
		void post(int n) { ReleaseSemaphore(m_sem,n,NULL); }
		void wait() { WaitForSingleObject(m_sem,INFINITE); }
		bool wait_timeout(int millis) { return WaitForSingleObject(m_sem,millis) == WAIT_OBJECT_0; }

		LF_OS_FORBID_CLASS_STANDARDS(semaphore);
	};
	
	// check for a windows.h with CONDITION_VARIABLE in it :
	#ifdef RTL_CONDITION_VARIABLE_INIT
	class condition_var
	{
	public:
		CONDITION_VARIABLE  m_cv;
		mutex				m_cs;
	
		condition_var()
		{ 
			InitializeConditionVariable( &m_cv );
		}
		~condition_var() { }
		
		void lock()	  { m_cs.lock(); }
		void unlock() { m_cs.unlock(); }
	
		void unlock_wait_lock() { SleepConditionVariableCS(&m_cv,&m_cs.m_cs, 0); }
		
		void signal_unlock() { m_cs.unlock(); WakeConditionVariable(&m_cv); }
		void broadcast_unlock() { m_cs.unlock(); WakeAllConditionVariable(&m_cv); }

		LF_OS_FORBID_CLASS_STANDARDS(condition_var);
	};
	#endif
	
	template <typename T>
	class lock_guard
	{
	public:
		lock_guard(T * t) : m_t(t) { if ( t) t->lock(); }
		~lock_guard() { unlock(); }
	
		void set_nolock(T * t) { m_t = t; }
	
		void lock(T * t) { unlock(); m_t = t; if ( t) t->lock(); }
		void unlock() { if ( m_t ) m_t->unlock(); m_t = NULL; }
	
	private:
		T * m_t;
		
		LF_OS_FORBID_CLASS_STANDARDS(lock_guard);
	};
	
};


#pragma once

/*

cblibRelacy :

adapter to use relacy/C++0x style syntax and call through to cblib/threading

also provides a test mechanism similar to Relacy

*/

#include <cblib/Threading.h>
#include <cblib/Rand.h>

// options :
extern bool cblibRelacy_rand_seed_clock;
extern bool cblibRelacy_suspend_resume;
extern bool cblibRelacy_do_scheduler;

#ifndef DO_STD_ATOMIC_SPOOF
#if _MSC_VER < 1700
#define DO_STD_ATOMIC_SPOOF 1
#else
#define DO_STD_ATOMIC_SPOOF 0
#endif
#endif

namespace rl
{
	struct relacy_schedule_point;
};

#define $  rl::relacy_schedule_point(__FILE__,__LINE__)
	
	#define mo_relaxed memory_order_relaxed 
	#define mo_consume memory_order_consume 
	#define mo_acquire memory_order_acquire 
	#define mo_release memory_order_release 
	#define mo_acq_rel memory_order_acq_rel 
	#define mo_seq_cst memory_order_seq_cst 

#if DO_STD_ATOMIC_SPOOF	
namespace std
{
	enum memory_order  
	{
	  memory_order_relaxed, memory_order_consume, memory_order_acquire, memory_order_release,
	  memory_order_acq_rel, memory_order_seq_cst
	};
	
	template <typename T>
	class atomic
	{
	private:
		cb::Atomic<T>	m_value;
	public:
	
		typedef atomic<T> this_type;
		
		atomic()  { }
		~atomic() { }
	
		atomic(T initVal) { store(initVal,mo_relaxed); }
		
		T fetch_add(T inc, memory_order mo = mo_seq_cst);
	
		T fetch_or(T bitmask, memory_order mo = mo_seq_cst); 
	
		bool compare_exchange_strong(T &oldV, T newV, memory_order mo = mo_seq_cst);
		bool compare_exchange_weak(T &oldV, T newV, memory_order mo = mo_seq_cst)
		{
			return compare_exchange_strong(oldV,newV,mo);
		}
	
		T load(memory_order mo = mo_seq_cst) const;
	
		T exchange(T newV,memory_order mo = mo_seq_cst);
	
		void store(T val,memory_order mo = mo_seq_cst);
		
		// for relacey ($) syntax :
		this_type & operator () (const rl::relacy_schedule_point & rsp) { return *this; }
	
		//-----------------
		// cheater :
		T * GetPtr() { return &(m_value.m_value); }
	
	private:
		FORBID_ASSIGNMENT(atomic);
	};

	template <typename T>
	T atomic<T>::fetch_add(T inc, memory_order mo)
	{
		CompilerReadWriteBarrier();
		// always seq_cst :
		T ret = m_value.ExchangeAdd(inc);
		CompilerReadWriteBarrier();
		return ret;
	}
	
	template <typename T>
	T atomic<T>::fetch_or(T mask, memory_order mo)
	{
		CompilerReadWriteBarrier();
		// always seq_cst :
		T old = m_value.LoadRelaxed();
		cb::SpinBackOff sbo;
		while ( ! compare_exchange_weak(old,old|mask,mo) )
		{
			sbo.BackOffYield();
		}
		CompilerReadWriteBarrier();
		return old;
	}
	
	template <typename T>
	bool atomic<T>::compare_exchange_strong(T &oldV, T newV, memory_order mo)
	{
		CompilerReadWriteBarrier();
		// always seq_cst :
		T got = m_value.CMPX(oldV,newV);
		CompilerReadWriteBarrier();
		bool ret = (got == oldV);
		oldV = got;
		return ret;		
	}

	template <typename T>
	T atomic<T>::exchange(T newV,memory_order mo)
	{
		CompilerReadWriteBarrier();
		// always seq_cst :
		T old = m_value.Exchange(newV);
		CompilerReadWriteBarrier();
		return old;	
	}

	template <typename T>
	T atomic<T>::load(memory_order mo) const
	{
		CompilerReadWriteBarrier();
		T ret;
		if ( mo < mo_acquire ) ret = m_value.LoadRelaxed();
		else if ( mo == mo_acquire ) ret = m_value.LoadAcquire();
		else ret = const_cast<cb::Atomic<T> &>(m_value).ExchangeAdd(0);
		CompilerReadWriteBarrier();
		return ret;
	}
	
	template <typename T>
	void atomic<T>::store(T val,memory_order mo)
	{
		CompilerReadWriteBarrier();
		if ( mo < mo_release ) m_value.StoreRelaxed(val);
		else if ( mo == mo_release ) m_value.StoreRelease(val);
		else m_value.Exchange(val);
		CompilerReadWriteBarrier();
	}

	class mutex : public cb::CriticalSection
	{
	public:
		mutex() {}
		~mutex() { }
		
		void lock() { Lock(); } 
		void unlock() { Unlock(); } 

		// for relacey ($) syntax :
		
		void lock(const rl::relacy_schedule_point & rsp) { Lock(); } 
		void unlock(const rl::relacy_schedule_point & rsp) { Unlock(); } 
	};	
};
#endif // DO_STD_ATOMIC_SPOOF

namespace rl
{
	//using std::memory_order;

	void DoSchedulePoint(const relacy_schedule_point & rsp);
	
	struct relacy_schedule_point
	{
		/*
		char const * m_f;
		int m_l;
		relacy_schedule_point(const char * f,const int l) : m_f(f) , m_l(l)
		*/
		
		__forceinline relacy_schedule_point(const char * f,const int l)
		{
			f; l;
			if ( cblibRelacy_do_scheduler )
			{
				DoSchedulePoint(*this);
			}
		}
	};

	class backoff : private cb::SpinBackOff
	{
	public:
		void yield(const rl::relacy_schedule_point & rsp) 
		{
			BackOffYield();
		}
	};
	
	class linear_backoff : public backoff { };
	class exp_backoff : public backoff { };
	
	struct test_suite_base
	{
		int m_thread_count;
		test_suite_base(int count) : m_thread_count(count) { }
		virtual ~test_suite_base() { }
		virtual void before() { }
		virtual void after() { }

		virtual void thread(unsigned int tidx) { }
	};
	
	
	template <typename t_test_T,int t_thread_count>
	struct test_suite : public test_suite_base
	{
		test_suite() : test_suite_base(t_thread_count) { }
	
		enum { c_thread_count = t_thread_count };
	};
	
	template <typename T>
	class var
	{
		T	m_val;
	public:
		var() { }
		var(T initVal) : m_val(initVal) { }
		
		operator T & () { return m_val; }
		operator T () const { return m_val; }
		void operator = (T t) { m_val = t; }
		bool operator == (T t) const { return m_val == t; }
		
		// for relacey ($) syntax :
		T & operator () (const rl::relacy_schedule_point & rsp) { return m_val; }
	};

	#undef VAR_T	
	#define VAR_T(t) rl::var<t>
	#define RL_VAR   VAR_T
	#define VAR(x)	x($)
	
	enum scheduler_type_e
	{
		sched_random,
		sched_bound,
		sched_full,
		sched_count,

		random_scheduler_type = sched_random,
		fair_context_bound_scheduler_type = sched_bound,
		fair_full_search_scheduler_type = sched_full,
		scheduler_type_count
	};

	struct test_params
	{
		// input params
		int			                 iteration_count;
		//void *					output_stream;
		//void *					progress_stream;
		unsigned                    progress_output_period;
		bool                        collect_history;
		bool                        output_history;
		scheduler_type_e            search_type;
		unsigned                    context_bound;
		unsigned                    execution_depth_limit;
		//string                      initial_state;
		cb::vector<char>			initial_state;

		// output params
		//test_result_e               test_result;
		//iteration_t                 stop_iteration;
		//string                      test_name;
		//string                      final_state;

		test_params()
		{
			iteration_count         = 1000;
			//output_stream           = &std::cout;
			//progress_stream         = &std::cout;
			progress_output_period  = 3;
			collect_history         = false;
			output_history          = false;
			search_type             = random_scheduler_type;
			context_bound           = 1;
			execution_depth_limit   = 2000;

			//test_result             = test_result_success;
			//stop_iteration          = 0;
		}
	};
	
	void RunTest(test_suite_base * base,test_params & p);
	
	template <typename t_test>
	void simulate(test_params & p)
	{
		char	pad1[LF_CACHE_LINE_SIZE];
		t_test	test;
		char	pad2[LF_CACHE_LINE_SIZE];
		
		test_suite_base * base = &test;
		pad1; pad2;
		
		RunTest(base,p);
	}
	
	inline unsigned rand(unsigned limit)
	{
		return cb::irandmod(limit);
	}
};

#if 0 // ! defined (NDEBUG)
//#  define DBG_PRINTF(mp_exp) std::printf mp_exp
#define DBG_PRINTF_PRE  buf,
#define DBG_PRINTF(mp_exp) do { char buf [1024]; \
            sprintf mp_exp ; \
            OutputDebugStringA(buf); } while(0)
#else
#define DBG_PRINTF_PRE 
#  define DBG_PRINTF(mp_exp) ((void)0)
#endif

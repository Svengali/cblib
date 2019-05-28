
//*

#include "OodleRelacy.h"

#define THREAD_REPEAT	1

/*/

// #define DO_RAND_SEED_CLOCK // else deterministic

// #define RL_ASSERT(ex)

#include "OodleThreading.h"
#include "OodleThreadDriver.h"

#define THREAD_REPEAT	1000

/**/

#define NUM_THREADS		2


//=======================================================

#include "PetersonLock.h"

//=======================================================

#define	NUM_COUNTERS	(3)

struct Peterson_Test : rl::test_suite<Peterson_Test, NUM_THREADS>
{
	PetersonLock	peter;
	
	NOT_THREAD_SAFE(int)	counters[NUM_COUNTERS];
	
    void before()
    {
		for(int i=0;i<NUM_COUNTERS;i++)
			StoreRelaxed(&counters[i],0);
	}
	
	void after()
	{
		for(int i=0;i<NUM_COUNTERS;i++)
			RL_ASSERT( LoadRelaxed(&counters[i]) == 10*THREAD_REPEAT );
	}
	
	
    void thread(unsigned index)
    {
		if ( index == 0 )
		{
			for(int which=0;which<NUM_COUNTERS;which++)
			{
				peter.Lock(index,which);
				
				int c = LoadRelaxed(&counters[which]);
				
				if ( rl::rand(4) == 0 ) RL_ThreadYield();
				
				StoreRelaxed(&counters[which], c + 7 );
				
				peter.Unlock(index);
			}
		}
		else
		{
			for(int which=0;which<NUM_COUNTERS;which++)
			{
				peter.Lock(index,which);
			
				int c = LoadRelaxed(&counters[which]);
				
				if ( rl::rand(4) == 0 ) RL_ThreadYield();
				
				StoreRelaxed(&counters[which], c + 3 );
				
				peter.Unlock(index);
			}
		}
    }
	
};


#ifdef RL_RELACY_HPP

int main()
{		
    rl::test_params params;

    //std::ostringstream stream; 
    //params.output_stream = &stream;
    
    /*
    
	params.search_type = rl::random_scheduler_type;
	params.iteration_count = 100000000; 
    
    /*/
    
    params.search_type = rl::fair_context_bound_scheduler_type;
    params.context_bound = 3;
    
    /**/
        
    rl::simulate<Peterson_Test>(params);
    
    return 0;
}

#else

//SingletonThreadSafe<LFSList_Test>	singleton = { 0 };

void thread_func(void * _test,unsigned index)
{
	Peterson_Test * test = (Peterson_Test *) _test;
	
	test->thread(index);
}
	
int main()
{	
	Peterson_Test test;
		
	int thread_repeat = THREAD_REPEAT;
	
	OodleThreadDriver(test,thread_func,thread_repeat);
	
	return 0;
}

#endif


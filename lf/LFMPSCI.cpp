
// select Relacy or Windows here :

/*

//#define RL_GC

#include "OodleRelacy.h"

/*/

// #define DO_RAND_SEED_LOCK // else deterministic

//#include "OodleThreading.h"
//#include "OodleThreadDriver.h"

#include "cblib/Threading.h"
USE_CB

/**/

//=================================================================================

#include "NodeAllocator.h"

//*

#ifdef RL_GC

#define SELECTED_ALLOCATOR	Leaky_Allocator

#else

//#define SELECTED_ALLOCATOR	Freelist_Allocator
#define SELECTED_ALLOCATOR	New_Allocator
//#define SELECTED_ALLOCATOR	Vector_Allocator

#endif

/**/

//#define SELECTED_ALLOCATOR	Leaky_Allocator

#define NUM_THREADS	4
//#define NUM_THREADS	2 // use just 2 for context_bound

//=================================================================================

#include "LFMPSCI.h"
	
//=================================================================================

static void linear_test()
{
    MPSC_FIFOI MPSCI;
    MPSC_FIFOI_Node_Allocator allocator;
	MPSC_FIFOI_Open(&MPSCI);
	RL_ASSERT( MPSC_FIFOI_IsEmpty(&MPSCI) );
	MPSC_FIFOI_PushData(&MPSCI,&allocator,(void *)1);
	MPSC_FIFOI_PushData(&MPSCI,&allocator,(void *)2);
	RL_ASSERT( ! MPSC_FIFOI_IsEmpty(&MPSCI) );
	MPSC_FIFOI_PushData(&MPSCI,&allocator,(void *)3);
	while( void * got = MPSC_FIFOI_PopData(&MPSCI,&allocator) )
	{
		printf("%d\n",(int)(intptr_t)got);
	}
	RL_ASSERT( MPSC_FIFOI_IsEmpty(&MPSCI) );
	MPSC_FIFOI_PushData(&MPSCI,&allocator,(void *)4);
	MPSC_FIFOI_PushData(&MPSCI,&allocator,(void *)5);
	while( void * got = MPSC_FIFOI_PopData(&MPSCI,&allocator) )
	{
		printf("%d\n",(int)(intptr_t)got);
	}
	RL_ASSERT( MPSC_FIFOI_IsEmpty(&MPSCI) );
    MPSC_FIFOI_Flush(&MPSCI,&allocator);
	MPSC_FIFOI_Close(&MPSCI);  
	allocator.Finalize();
}


void thread_func(void * _test,unsigned index);
    
#include <numeric>
    
struct MPSCI_Test 
#ifdef RL_RELACY_HPP
: rl::test_suite<MPSCI_Test, NUM_THREADS>
#endif
{
    MPSC_FIFOI s_;
    MPSC_FIFOI_Node_Allocator allocator;

    int produced_count[NUM_THREADS];
    int consumed_count[NUM_THREADS];
    int consumed_count_tot;
	
    void before()
    {   
		// bad : breaks determinism !
		//static bool s_once = true;
		//if ( s_once ) { s_once = false; linear_test(); }
		
		MPSC_FIFOI_Open(&s_);
    
		ZERO_VAL(produced_count);
		ZERO_VAL(consumed_count);
        consumed_count_tot = 0;
    }

    void after()
    {
		// consume the remainder :
		consumer();
        
        MPSC_FIFOI_Flush(&s_,&allocator);
        		
		MPSC_FIFOI_Close(&s_);  
		allocator.Finalize(); 
		
        int produced_count_tot = std::accumulate(produced_count,produced_count+NUM_THREADS,(int)0);
        
		//printf("produced_count_tot : %d\n",produced_count_tot);
		//printf("consumed_count_tot : %d\n",consumed_count_tot);

        // produced equals consumed :
        RL_ASSERT( produced_count_tot == consumed_count_tot );
        
    }

	void consumer()
	{
		#ifndef RL_RELACY_HPP
		for(int spins=0;spins<5000;spins++)
		#endif
		{
			while ( void * d = MPSC_FIFOI_PopData(&s_,&allocator) )
			{
				int data = (int) (intptr_t) d;
				int index = (data>>24);
				int count = data - (index<<24);
				
				consumed_count[index]++;
				RL_ASSERT( consumed_count[index] == count );
				
				consumed_count_tot ++;
			}
		}
	}
	
	void producer(unsigned index)
	{
		int count = rl::rand(8);
		
		for(int i=0;i<count;i++)
		{
			produce(index);
		}
	}
		
	void produce(unsigned index)
	{
		produced_count[index]++;
		int data = (index << 24) + produced_count[index];
	
		MPSC_FIFOI_PushData(&s_,&allocator,(void *)(intptr_t)data);
	}
	
    // very simple test for context_bound :
	// for Relacy :    
    void thread(unsigned index)
    {
		produce(index);
        
        if ( index == 0 )
        {
			consumer();
		}
	}
};

#if 1

// very simple test for context_bound :

void thread_func(void * _test,unsigned index)
{
	MPSCI_Test * test = (MPSCI_Test *) _test;
	
    if ( index == 0 )
    {
		test->consumer();
	}
	else
	{
		test->producer(index);
	}
	
}

#endif

#ifdef RL_RELACY_HPP

int main()
{
    rl::test_params params;

    //std::ostringstream stream; 
    //params.output_stream = &stream;
    
	//params.search_type = rl::random_scheduler_type;
	//params.iteration_count = 100000000; 
	//params.iteration_count = 10000; 
    
    params.search_type = rl::fair_context_bound_scheduler_type;
    params.context_bound = 2;
    
    rl::simulate<MPSCI_Test>(params);
    
    /*
        if ( produced_count_ < MAX_COUNT )
	        histo_produced_count_[ produced_count_ ] ++;
		if ( consumed_count_0 < MAX_COUNT )
	        histo_consumed_count_0[ consumed_count_0 ] ++;
		if ( consumed_count_1 < MAX_COUNT )
	        histo_consumed_count_1[ consumed_count_1 ] ++;
	      */
	      
    return 0;
}

#else

int main()
{	
	MPSCI_Test test;
	
	int thread_repeat = 100000;
	
	OodleThreadDriver(test,thread_func,thread_repeat);

	return 0;
}

#endif

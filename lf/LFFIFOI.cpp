
#include "cblib/Threading.h"
USE_CB

// select Relacy or Windows here :

/*

//#define RL_GC

#include "OodleRelacy.h"

/*/

//#define OODLE_RAND_SEED_CLOCK	true

//#include "OodleThreading.h"
//#include "OodleThreadDriver.h"


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
//#define SELECTED_ALLOCATOR	Leaky_Allocator

#endif

/**/

//#define SELECTED_ALLOCATOR	Leaky_Allocator

#define NUM_THREADS	4
//#define NUM_THREADS	2 // use just 2 for context_bound
//#define NUM_THREADS	1 // use just 2 for context_bound

//=================================================================================

#include "LFFIFOI.h"
	
//=================================================================================

#define MAX_COUNT	(10*NUM_THREADS)
int histo_produced_count_[ MAX_COUNT ] = { 0 };
int histo_consumed_count_0[ MAX_COUNT ] = { 0 };
int histo_consumed_count_1[ MAX_COUNT ] = { 0 };

static void linear_test()
{
    MPMC_FIFOI FIFOI;
    MPMC_FIFOI_Node_Allocator allocator;
	MPMC_FIFOI_Open(&FIFOI);
	RL_ASSERT( MPMC_FIFOI_IsEmpty(&FIFOI) );
	MPMC_FIFOI_PushData(&FIFOI,&allocator,(void *)1);
	MPMC_FIFOI_PushData(&FIFOI,&allocator,(void *)2);
	RL_ASSERT( ! MPMC_FIFOI_IsEmpty(&FIFOI) );
	MPMC_FIFOI_PushData(&FIFOI,&allocator,(void *)3);
	while( void * got = MPMC_FIFOI_PopData(&FIFOI,&allocator) )
	{
		printf("%d\n",(int)(intptr_t)got);
	}
	RL_ASSERT( MPMC_FIFOI_IsEmpty(&FIFOI) );
	MPMC_FIFOI_PushData(&FIFOI,&allocator,(void *)4);
	MPMC_FIFOI_PushData(&FIFOI,&allocator,(void *)5);
	while( void * got = MPMC_FIFOI_PopData(&FIFOI,&allocator) )
	{
		printf("%d\n",(int)(intptr_t)got);
	}
	RL_ASSERT( MPMC_FIFOI_IsEmpty(&FIFOI) );
    MPMC_FIFOI_Flush(&FIFOI,&allocator);
	MPMC_FIFOI_Close(&FIFOI);  
	allocator.Finalize();
}


#include <numeric>

#define SCOREBOARD_SIZE	(1<<20)
//#define SCOREBOARD_SIZE	(1<<10)
//#define SCOREBOARD_SIZE	(4)
//#define SCOREBOARD_SIZE	(1)

char * s_scoreboard = NULL;
	
struct MPMC_FIFOI_Test 
#ifdef RL_RELACY_HPP
: rl::test_suite<MPMC_FIFOI_Test, NUM_THREADS>
#endif
{
    MPMC_FIFOI list;
    MPMC_FIFOI_Node_Allocator allocator;

	// do a counter per thread so its thread safe
	//	not needed for Relacy but needed for real windows threads
    int produced_count[NUM_THREADS];
    int consumed_count[NUM_THREADS];

    void before()
    {    
		// bad : breaks determinism !
		//static bool s_once = true;
		//if ( s_once ) { s_once = false; linear_test(); }
	
		ZERO_VAL(produced_count);
		ZERO_VAL(consumed_count);
        
        MPMC_FIFOI_Open(&list);
        
        if ( s_scoreboard == NULL )
		{
			s_scoreboard = (char *) cb::malloc(SCOREBOARD_SIZE);
			memset(s_scoreboard,0xFF,SCOREBOARD_SIZE);
		}
    }

#ifdef RL_RELACY_HPP
    void after()
    {		
		// pop missed ones :
		int consumed_count_after = 0;
        while ( MPMC_FIFOI_PopData(&list,&allocator) )
        {
			consumed_count_after += 1;
        }
        MPMC_FIFOI_Close(&list);
        allocator.Finalize();
        
        int produced_count_tot = std::accumulate(produced_count,produced_count+NUM_THREADS,(int)0);
        int consumed_count_tot = std::accumulate(consumed_count,consumed_count+NUM_THREADS,(int)0) + consumed_count_after;
        
		//printf("produced_count_tot : %d\n",produced_count_tot);
		//printf("consumed_count_tot : %d\n",consumed_count_tot);

        // produced equals consumed :
        RL_ASSERT( produced_count_tot == consumed_count_tot );
    }
#else

	void after()
	{
		// pop missed ones :
		while ( ScoreboardConsume(NUM_THREADS) )
		{
		}
		
        MPMC_FIFOI_Close(&list);
        allocator.Finalize();
        
        // show histogram of what thread got in the scoreboard :
        //	lets us know if they're running live
        int histo[NUM_THREADS+1] = { 0 };
        for(int i=0;i<SCOREBOARD_SIZE;i++)
        {
			char index = s_scoreboard[i];
			RR_ASSERT_ALWAYS( index != (char)0xFF );
			RR_ASSERT_ALWAYS( index >= 0 && index <= NUM_THREADS );
			histo[ index ] ++;
        }
        for(int i=0;i<=NUM_THREADS;i++)
        {
			printf("%d : %d\n",i,histo[i]);
        }
		
	}
#endif // RL_RELACY_HPP
	
	// thread function for Relacy :
    void thread(unsigned index)
    {
        if ( MPMC_FIFOI_PopData(&list,&allocator) )
        {
			consumed_count[index] += 1;
        }
		
        int num = 1 + rl::rand(2);
        for(int i=0;i<num;i++)
        {
			MPMC_FIFOI_PushData(&list,&allocator,(void *)(intptr_t)(i+1));;
            produced_count[index] += 1;
        }

        while ( ! MPMC_FIFOI_IsEmpty(&list) )
        {
			if ( MPMC_FIFOI_PopData(&list,&allocator) )
			{
				consumed_count[index] += 1;
			}
        }
    }

	bool ScoreboardConsume(unsigned index)
	{
		void * d = MPMC_FIFOI_PopData(&list,&allocator);
		if ( ! d ) return false;
		
		int i = (int)(intptr_t) d;
		i--;
		// I just popped a new value :
		//	ensure nobody else has got this value yet
		// put my index in it
		RR_ASSERT_ALWAYS( s_scoreboard[i] == (char)0xFF );
		RR_ASSERT_ALWAYS( i >= 0 && i < SCOREBOARD_SIZE );
		s_scoreboard[i] = index;
		return true;
	}

};


#ifdef RL_RELACY_HPP

int main()
{		
    rl::test_params params;

    //std::ostringstream stream; 
    //params.output_stream = &stream;
    
    params.execution_depth_limit = 10000;
    
    /*
    
	params.search_type = rl::random_scheduler_type;
	params.iteration_count = 100000000; 
    
    /*/
    
    params.search_type = rl::fair_context_bound_scheduler_type;
    params.context_bound = 2;
    
    /**/
    
    rl::simulate<MPMC_FIFOI_Test>(params);
    
    return 0;
}

#else

// scoreboard thread func :

void thread_func(void * _test,unsigned index)
{
	MPMC_FIFOI_Test * test = (MPMC_FIFOI_Test *) _test;
	
	//test->thread(index);	
	
	int low = (index * SCOREBOARD_SIZE) / NUM_THREADS;
	int next = ((index+1) * SCOREBOARD_SIZE) / NUM_THREADS;
	RL_ASSERT( next <= SCOREBOARD_SIZE );

	for(int i=low;i<next;i++)
	{
		MPMC_FIFOI_PushData(&test->list,&test->allocator,(void *)(intptr_t)(1+i));
		
		if ( rl::rand(16) == 0 )
		{
			// every 16, pull an average of 16 :
			int count = rl::rand(32);
			for(int n=0;n<count;n++)
			{
				test->ScoreboardConsume(index);
			}
		}
	}	
	
	while ( test->ScoreboardConsume(index) )
	{
	}
}

int main()
{	
	MPMC_FIFOI_Test test;
	
	int thread_repeat = 1; // MUST BE 1 for scoreboard !!
	
	OodleThreadDriver(test,thread_func,thread_repeat);

	printf("press a key\n");
	getch();

	return 0;
}

#endif
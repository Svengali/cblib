
#ifndef CDEP

// select Relacy or Windows here :

//*

//#define RL_GC

#include "OodleRelacy.h"

/*/

// #define DO_RAND_SEED_LOCK // else deterministic

#include "OodleThreading.h"
#include "OodleThreadDriver.h"

/**/

//=================================================================================

#include "NodeAllocator.h"

//*

#ifdef RL_GC

#define SELECTED_ALLOCATOR	Leaky_Allocator

#else

#define SELECTED_ALLOCATOR	Freelist_Allocator
//#define SELECTED_ALLOCATOR	Vector_Allocator
//#define SELECTED_ALLOCATOR	Leaky_Allocator

#endif

/**/

//#define SELECTED_ALLOCATOR	Leaky_Allocator

//#define NUM_THREADS	4
//#define NUM_THREADS	3
#define NUM_THREADS	2 // use just 2 for context_bound

//=================================================================================

#include "LFSList.h"
	
//=================================================================================

static void linear_test()
{
    LFSList FIFOI;
    LFSNode_And_Int_Allocator allocator;
	LFSList_Open(&FIFOI);
	RL_ASSERT( LFSList_IsEmpty(&FIFOI) );
	LFSList_PushData(&FIFOI,&allocator,(void *)3);
	LFSList_PushData(&FIFOI,&allocator,(void *)2);
	RL_ASSERT( ! LFSList_IsEmpty(&FIFOI) );
	LFSList_PushData(&FIFOI,&allocator,(void *)1);
	while( void * got = LFSList_PopData(&FIFOI,&allocator) )
	{
		printf("%d\n",(int)(intptr_t)got);
	}
	RL_ASSERT( LFSList_IsEmpty(&FIFOI) );
	LFSList_PushData(&FIFOI,&allocator,(void *)5);
	LFSList_PushData(&FIFOI,&allocator,(void *)4);
	while( void * got = LFSList_PopData(&FIFOI,&allocator) )
	{
		printf("%d\n",(int)(intptr_t)got);
	}
	RL_ASSERT( LFSList_IsEmpty(&FIFOI) );
    //LFSList_Flush(&FIFOI,&allocator);
	LFSList_Close(&FIFOI);  
	allocator.Finalize();
}

struct LFSList_Linear_Test : rl::test_suite<LFSList_Linear_Test, 1>
{
    void before()
    {
		linear_test();
    }
    
    void after()
    {
    }
    
    void thread(unsigned index)
    {
    }

};

//=======================================================

#include <numeric>

//#define SCOREBOARD_SIZE	(1<<20)
#define SCOREBOARD_SIZE	(1<<20)

char * s_scoreboard = NULL;
	

struct LFSList_Test : rl::test_suite<LFSList_Test, NUM_THREADS>
{
    LFSList list;
    LFSNode_And_Int_Allocator allocator;

    int produced_count[NUM_THREADS];
    int consumed_count[NUM_THREADS];

    void before()
    {	
		RR_ZERO(produced_count);
		RR_ZERO(consumed_count);
        
        LFSList_Open(&list);
        
        if ( s_scoreboard == NULL )
		{
			s_scoreboard = (char *) malloc(SCOREBOARD_SIZE);
			memset(s_scoreboard,0xFF,SCOREBOARD_SIZE);
		}
    }

#ifdef RL_RELACY_HPP
    void after()
    {		
		int consumed_count_after = 0;
        while ( LFSList_PopData(&list,&allocator) )
        {
			consumed_count_after += 1;
        }
        LFSList_Close(&list);
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
		while ( ScoreboardConsume(NUM_THREADS) )
		{
		}
		
        LFSList_Close(&list);
        allocator.Finalize();
        
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
	
	#if 0 
	
	// more complex thread :
	
    void thread(unsigned index)
    {
        if ( LFSList_PopData(&list,&allocator) )
        {
			consumed_count[index] += 1;
        }
		
        int num = 1 + rl::rand(2);
        for(int i=0;i<num;i++)
        {
			LFSList_PushData(&list,&allocator,(void *)(intptr_t)(i+1));;
            produced_count[index] += 1;
        }

        while ( ! LFSList_IsEmpty(&list) )
        {
			if ( LFSList_PopData(&list,&allocator) )
			{
				consumed_count[index] += 1;
			}
        }
    }
    
    #else
    
    // very simple thread for context bound :
    
    void thread(unsigned index)
    {
		LFSList_PushData(&list,&allocator,(void *)(intptr_t)(77));;
        produced_count[index] += 1;
        
        while ( LFSList_PopData(&list,&allocator) )
        {
			consumed_count[index] += 1;
        }		
    }
    
    #endif

	bool ScoreboardConsume(unsigned index)
	{
		void * d = LFSList_PopData(&list,&allocator);
		if ( ! d ) return false;
		
		int i = (int)(intptr_t) d;
		i--;
		RR_ASSERT_ALWAYS( s_scoreboard[i] == (char)0xFF );
		RR_ASSERT_ALWAYS( i >= 0 && i < SCOREBOARD_SIZE );
		s_scoreboard[i] = index;
		return true;
	}

};


#if 1

// thread_func for scoreboard :

void thread_func(void * _test,unsigned index)
{
	LFSList_Test * test = (LFSList_Test *) _test;
	
	//test->thread(index);	
	
	int low = (index * SCOREBOARD_SIZE) / NUM_THREADS;
	int next = ((index+1) * SCOREBOARD_SIZE) / NUM_THREADS;
	RL_ASSERT( next <= SCOREBOARD_SIZE );

	// test : 
	//DO_ONCE_THREAD_SAFE( printf("once\n") );

	for(int i=low;i<next;i++)
	{
		LFSList_PushData(&test->list,&test->allocator,(void *)(intptr_t)(1+i));
		
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

#endif


#ifdef RL_RELACY_HPP

int main()
{		
    rl::test_params params;

    //std::ostringstream stream; 
    //params.output_stream = &stream;
    
	params.search_type = rl::random_scheduler_type;
	params.iteration_count = 1; 
    rl::simulate<LFSList_Linear_Test>(params);
    
    /*
    
	params.search_type = rl::random_scheduler_type;
	params.iteration_count = 100000000; 
    
    /*/
    
    params.search_type = rl::fair_context_bound_scheduler_type;
    params.context_bound = 2;
    
    /**/
        
    rl::simulate<LFSList_Test>(params);
    
    return 0;
}

#else

//SingletonThreadSafe<LFSList_Test>	singleton = { 0 };

int main()
{	
	LFSList_Test test;
	
	//LFSList_Test * l = singleton.Get();
	
	/*
	float f = 77.f;
	U32 t = rr::same_size_bit_cast<U32>(f);
	*/
	
	int thread_repeat = 1; // must be 1 for scoreboard test
	
	OodleThreadDriver(test,thread_func,thread_repeat);

	return 0;
}

#endif

#endif


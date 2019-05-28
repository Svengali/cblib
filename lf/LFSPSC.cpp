#define THREAD_RELACY
// select Relacy or Windows here :

/*

#define RL_GC

#include "OodleRelacy.h"

/*/

// #define DO_RAND_SEED_LOCK // else deterministic

// #define RL_ASSERT(ex)

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
//#define SELECTED_ALLOCATOR	Vector_Allocator
//#define SELECTED_ALLOCATOR	Leaky_Allocator
#define SELECTED_ALLOCATOR	New_Allocator

#endif

/**/

//#define SELECTED_ALLOCATOR	Leaky_Allocator

//#define NUM_THREADS	4
#define NUM_THREADS	2 // use just 2 for context_bound

//=================================================================================

#include "LFSPSC.h"
	
//=================================================================================

//=================================================================================

static void linear_test()
{
    SPSC_FIFO FIFOI;
    SPSC_FIFO_Node_Allocator allocator;
	SPSC_FIFO_Open(&FIFOI,&allocator);
	RL_ASSERT( SPSC_FIFO_IsEmpty(&FIFOI) );
	SPSC_FIFO_PushData(&FIFOI,&allocator,(void *)1);
	SPSC_FIFO_PushData(&FIFOI,&allocator,(void *)2);
	RL_ASSERT( ! SPSC_FIFO_IsEmpty(&FIFOI) );
	SPSC_FIFO_PushData(&FIFOI,&allocator,(void *)3);
	while( void * got = SPSC_FIFO_PopData(&FIFOI,&allocator) )
	{
		printf("%d\n",(int)(intptr_t)got);
	}
	RL_ASSERT( SPSC_FIFO_IsEmpty(&FIFOI) );
	SPSC_FIFO_PushData(&FIFOI,&allocator,(void *)4);
	SPSC_FIFO_PushData(&FIFOI,&allocator,(void *)5);
	while( void * got = SPSC_FIFO_PopData(&FIFOI,&allocator) )
	{
		printf("%d\n",(int)(intptr_t)got);
	}
	RL_ASSERT( SPSC_FIFO_IsEmpty(&FIFOI) );
    SPSC_FIFO_Flush(&FIFOI,&allocator);
	SPSC_FIFO_Close(&FIFOI,&allocator);  
	allocator.Finalize();
}


struct SPSC_Test 
#ifdef RL_RELACY_HPP
: rl::test_suite<SPSC_Test, NUM_THREADS>
#endif
{
    SPSC_FIFO s_;
    SPSC_FIFO_Node_Allocator allocator;

    int produced_count_; // owned by producer
    int consumed_count_; // owned by consumer

    void before()
    {
		// bad : breaks determinism !
		//static bool s_once = true;
		//if ( s_once ) { s_once = false; linear_test(); }
    
		SPSC_FIFO_Open(&s_,&allocator);
    
        produced_count_ = 0;
        consumed_count_ = 0;
    }

    void after()
    {
		int thread_consumed = consumed_count_;
		consume();
        
        SPSC_FIFO_Flush(&s_,&allocator);
        
        //printf(" %d = %d + %d\n",produced_count_,consumed_count_0,consumed_count_1);
        
        // produced equals consumed :
        RL_ASSERT( produced_count_ == consumed_count_ );
        
        //printf("consumed : %d , %d\n",thread_consumed,consumed_count_-thread_consumed);
        
		SPSC_FIFO_Close(&s_,&allocator);  
		allocator.Finalize(); 
    }

	#if 0
	
	void consume()
	{
		while ( void * data = SPSC_FIFO_PopData(&s_,&allocator) )
		{
			int i = (int) (intptr_t) data;
			++consumed_count_;
			RL_ASSERT( i == consumed_count_ );
		}
	}
		
	/*

	checked out : OodleThreading is doing the right thing ! booya
	
	SPSC_FIFO_PushData(&s_,&allocator,(void *)(intptr_t)produced_count_);

00401DDF  mov         ecx,ebp 
00401DE1  call        Vector_Allocator<SPSC_FIFO_Node>::Alloc (401300h) 

// this is the FIFO Push :

00401DE6  mov         dword ptr [eax+4],edi		// edi is the data element
00401DE9  mov         dword ptr [eax],0			// eax is node , [eax] is node->next
00401DEF  mov         ecx,dword ptr [esi+40h]   // [esi+40h] is the fifo head
00401DF2  mov         dword ptr [ecx],eax		// [ecx] is head->next
00401DF4  mov         dword ptr [esi+40h],eax   // head = node
	
	*/
	
    void thread(unsigned index)
    {
		// bottom thread produces
        if ( index == 0 )
        {
			// single mean thread works on the bottom :
        
			int count = 1 + rl::rand(4);
			
			for(int i=0;i<count;i++)
			{
				++produced_count_;
				SPSC_FIFO_PushData(&s_,&allocator,(void *)(intptr_t)produced_count_);
			}
            
        }
        else if ( index == 1 )
        {
			// other threads spin and try to pop top :
			consume();
		}
    }
    
    #else
    
    // very simple test for context_bound :
    
	void consume()
	{
		while ( SPSC_FIFO_PopData(&s_,&allocator) != NULL )
		{
			consumed_count_ += 1;
		}
	}		
		
    void thread(unsigned index)
    {
		if ( index == 0 )
		{
			SPSC_FIFO_PushData(&s_,&allocator,(void *)(intptr_t)(1));
		    produced_count_ += 1;
		    
			SPSC_FIFO_PushData(&s_,&allocator,(void *)(intptr_t)(2));
		    produced_count_ += 1;
		    
			SPSC_FIFO_PushData(&s_,&allocator,(void *)(intptr_t)(3));
		    produced_count_ += 1;
		}
		else if ( index == 1 )
		{		
			consume();
			consume();
			consume();
			consume();
		}
	}
    
    #endif
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
    
    rl::simulate<SPSC_Test>(params);
    
    return 0;
}

#else

void thread_func(void * _test,unsigned index)
{
	SPSC_Test * test = (SPSC_Test *) _test;
	test->thread(index);
}

int main()
{	
	SPSC_Test test;
	
	int thread_repeat = 10000000;
	
	OodleThreadDriver(test,thread_func,thread_repeat);

	printf("press a key\n");
	getch();

	return 0;
}

#endif

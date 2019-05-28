#include "cblibRelacy.h"
#include "cblib/Log.h"
#include "cblib/Rand.h"
#include "cblib/Win32Util.h"
#include <time.h>
#include <stdlib.h>

USE_CB

//===============================================

// options :

bool cblibRelacy_rand_seed_clock = false;
bool cblibRelacy_suspend_resume = false;
bool cblibRelacy_do_scheduler = false;

//===============================================

struct ThreadDriverControl
{
	rl::test_suite_base * test_base;
	HANDLE				handle;
	int					index;
	int					repeats;
	HANDLE				thread_start_event;
	HANDLE				thread_done_event;

	char				pad[LF_CACHE_LINE_SIZE];

	ThreadDriverControl() { }
};

DWORD WINAPI RunTest_ThreadRoutine(LPVOID lpThreadParameter)
{
	ThreadDriverControl * pControl = (ThreadDriverControl *) lpThreadParameter; 
	unsigned index = pControl->index;
	
	// if you don't srand on each thread you will get the exact same rand sequence in everything
	//	because rand is in the TLS
    int seed = index * 2147001325 + 715136305;

    if ( cblibRelacy_rand_seed_clock )
    {
		seed += clock();
    }
	
	srand(seed);
	rand();
	rand();
		
	WaitForSingleObject(pControl->thread_start_event,INFINITE);
	
	for(int i=0;i<pControl->repeats;i++)
	{		
		pControl->test_base->thread(index);
		
		SignalObjectAndWait(pControl->thread_done_event,pControl->thread_start_event,INFINITE,FALSE);
	}
	
	return 0;
}

void rl::RunTest(test_suite_base * base,test_params & p)
{
	
	int thread_repeat = p.iteration_count;
	//s_rand_seed_clock = rand_seed_clock;
	int thread_count = base->m_thread_count;
	
	// per-thread control :
	cb::vector<ThreadDriverControl> controls;
	controls.resize(thread_count);
	
	// order will be a permutation :
	cb::vector<int> order;
	order.resize(thread_count);
	
	// copy these out so I can WaitForMultiple :
	cb::vector<HANDLE> thread_done_events;
	thread_done_events.resize(thread_count);
	
	for(int i=0;i<thread_count;i++)
	{
		controls[i].test_base = base;
		controls[i].index = i;
		controls[i].repeats = thread_repeat;
	
		controls[i].thread_start_event = CreateEvent(NULL,0,0,NULL);
		controls[i].thread_done_event = CreateEvent(NULL,0,0,NULL);
		
		controls[i].handle = 0;
		
		MemoryBarrier();
		
		// let the thread start immediately :
		// (make sure control is all set up first)
		controls[i].handle = CreateThread(NULL,0,RunTest_ThreadRoutine,(void *)&(controls[i]),0,NULL);
	
		thread_done_events[i] = controls[i].thread_done_event;
	
		order[i] = i;
	}
	
	/*
	for(int i=0;i<thread_count;i++)
	{
		ResumeThread(controls[i].handle);
	}
	*/
			
	int last_percent_done = 0;
	int last_percent_clock = 0;// clock();
	
	for(int rep=0;rep<thread_repeat;rep++)
	{
		// permute thread order : 
		// this makes it so thread[0] is not always the first to get running
		
		lprintf_v2( "%d/%d :\n",rep,thread_repeat);
		
		for(int i=0;i<thread_count;i++)
		{
			//int r = i + rrRandMod(thread_count - i);
			int r = i + irandmod(thread_count - i);
			std::swap( order[i], order[r] );
		}

		//-----------------------------------------------

		// do before :

		lprintf_v2( " before\n" );

		base->before();
		
		MemoryBarrier(); // flush results
		lprintf_v2( " thread\n" );

		// do all threads :
		
		for(int i=0;i<thread_count;i++)
		{
			int t = order[i];

			// I have a quite intentional race in starting the threads one by one
			//	gives me some instruction scrambling
			SetEvent(controls[t].thread_start_event);
		}
		
		if ( cblibRelacy_suspend_resume )
		{
			// juggle the threads to try to mess them up :
			for(int i=0;i<thread_count;i++)
			{
				SuspendThread(controls[i].handle);
				ResumeThread(controls[i].handle);
			}
		}
			
		// make sure they are all done :
		/*
		for(int i=0;i<thread_count;i++)
		{
			int t = order[i];
		
			WaitForSingleObject(controls[t].thread_done_event,INFINITE);
		}
		*/
		
		WaitForMultipleObjects(thread_count,thread_done_events.data(),TRUE,INFINITE);
		
		
		lprintf_v2( " after\n" );
		
		// do after :
		
		MemoryBarrier();
		base->after();
		MemoryBarrier();
		
		//-----------------------------------------------
		
		int percent_done = (rep * 100)/thread_repeat;
		//if ( percent_done != last_percent_done &&
		if ( ( clock() - last_percent_clock) > (CLOCKS_PER_SEC/2) )
		{
			last_percent_clock = clock();
			last_percent_done = percent_done;
			lprintf(" %d%% (%d/%d)\n",percent_done,rep,thread_repeat);
		}
	}
	
	lprintf(" 100%% (%d/%d)\n",thread_repeat,thread_repeat);
	
	// give the threads a last kick to get them out of their iteration loop :		
	for(int i=0;i<thread_count;i++)
	{
		SetEvent(controls[i].thread_start_event);
	}
	
	// wait for thread exit :
	for(int i=0;i<thread_count;i++)
	{
		WaitForSingleObject( controls[i].handle, INFINITE );
	}
	
	for(int i=0;i<thread_count;i++)
	{
		CloseHandle( controls[i].handle );
		CloseHandle( controls[i].thread_start_event );
		CloseHandle( controls[i].thread_done_event );
	}
	
}

// DoSchedulePoint is called from a $
//	randomly fiddle with the timing of our thread
void rl::DoSchedulePoint(const relacy_schedule_point & rsp)
{
	// TODO :
	// could check FILE/LINE in rsp to do selective fiddles
	//  or use a mechanism like RR_FUZZ to make sure we do
	//	at least one schedule fiddling at each unique location
	
	int r = rl::rand(200);
	if ( r < 5 )
	{
		Sleep( 1 );
	}
	else if ( r < 10 )
	{
		Sleep( 0 );
	}
	else if ( r < 15 )
	{
		SwitchToThread();
	}
	else if ( r < 50 )
	{
		for(int i=0;i<r;i++)
		{
			HyperYieldProcessor();
		}
	}
	else
	{
		for(int i=0;i<r;i++)
		{
			volatile int g = i;
			g;
		}
	}
}

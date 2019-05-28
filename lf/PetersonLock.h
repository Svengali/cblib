#pragma once

#define THREAD_RELACY
#include "cblib/Threading.h"

START_CB

/****

PetersonLock is normally a bad way to do a simple mutex

but it is kind of handy in that you can lock a certain *item* , passed in as "what"

e.g. you could lock just one member of an array for example

***/

#define PETERSON_LOCK_NOT_WANTED	(-1)
#define PETERSON_LOCK_WANTED		(1)

LF_ALIGN_TO_CACHE_LINE struct PetersonLock
{
    ATOMIC_VAR(int) flag[2];
    ATOMIC_VAR(int) turn;
    
    #ifndef RL_RELACY_HPP
    char pad[LF_CACHE_LINE_SIZE - 3*sizeof(ATOMIC_VAR(int))];
	#endif
	
    PetersonLock()
    {
        StoreRelaxed( &flag[0] , PETERSON_LOCK_NOT_WANTED );
        StoreRelaxed( &flag[1] , PETERSON_LOCK_NOT_WANTED );
        StoreRelaxed( &turn, 0 );
    }

    void Lock(int t01,int what = PETERSON_LOCK_WANTED)
    {
		RL_ASSERT( t01 == 0 || t01 == 1 );
    
		// t01 is me, t10 is him
		int t10 = t01^1;
    
        StoreRelaxed( & flag[t01], what );
        
        //turn($).exchange(t10, rl::memory_order_acq_rel);
		AtomicExchange(&turn, t10);

		SpinBackOff bo;

        while ( LoadAcquire( & flag[t10] ) == what
            && LoadAcquire( & turn ) == t10 )
        {
			// spin
			bo.BackOffYield();
        }
        
        // Acquires mean stuff we read in the mutex stays in the mutex
	}
	
	void Unlock(int t01)
	{
		RL_ASSERT( t01 == 0 || t01 == 1 );
		
		// Release makes stuff we wrote in the Mutex go out first
		
        StoreRelease( & flag[t01], PETERSON_LOCK_NOT_WANTED );
    }
    
};

END_CB

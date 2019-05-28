#include "ThreadLog.h"
#include "vector.h"
#include "Win32Util.h"

START_CB

//=========================================================================================

/**

// thread log :

Uses a vector, which we reserve on the main thread, which could grow from the worker thread
Once the vector grows doesn't allocate, but does grab mutexes

An alternative implementation could be done using nodes that are push/poped with the non-locking linked list method
But that requires allocating the nodes somehow
And can get out of order, which is bad

**/

// CB : this ThreadLogMessage is kind of stupid; I should just have a single vector<char> and push
//  strings on to that, and then the flush can just grab strings by searching for the nulls
struct ThreadLogMessage
{
    char buf[128];
};
CriticalSection * s_threadLogMutex = NULL;
vector<ThreadLogMessage> s_threadLogQueue; // @TODO: replace this with a linked list so we don't depend on templates/
bool s_threadLogInit = false;

// s_unflushedCount is an optimization to make Flush a fast NOP when the queue is empty
//volatile uint32 s_unflushedCount = 0;

void ThreadLogOpen()
{
    if ( s_threadLogMutex )
        return;
        
    s_threadLogMutex = new CriticalSection();
    s_threadLogQueue.reserve(128); // make it unlikely that we alloc
    s_threadLogInit = true;
    //s_unflushedCount = 0;
}

void ThreadLogClose()
{
    if ( s_threadLogMutex )
    {
        s_threadLogMutex->Lock();
        s_threadLogInit = false;
        //s_unflushedCount = 0;
        s_threadLogQueue.release();
        s_threadLogMutex->Unlock();
        
        delete s_threadLogMutex;
        s_threadLogMutex = NULL;
    }
}

// called from thread to add logs :
void ThreadLog(const char * fmt,...)
{
    if ( ! s_threadLogInit )
        return;
    
    CB_SCOPE_CRITICAL_SECTION(*s_threadLogMutex);

    //LockedAddXchg(&s_unflushedCount, 1);

    s_threadLogQueue.push_back();

    char * ptr = s_threadLogQueue.back().buf;
    int size = MEMBER_SIZE(ThreadLogMessage, buf);

    va_list arg;
    va_start(arg,fmt);
    _vsnprintf(ptr,size-1,fmt,arg);
    ptr[size-1] = 0;
}

// called from main thread to flush out logs :
//  they go out via lprintf with whatever current settings are
void ThreadLogFlush()
{
    if ( ! s_threadLogInit )
        return;
    
    // I want this to be a light NOP in the case of an empty queue
    
    // hmm just to test for zero
    //   I don't have to do anything interlocked, eh?
    //if ( s_unflushedCount == 0 )
    //  return; 

	/*
    if ( LockedCmpXchg(&s_unflushedCount,0,0) == 0)
        return;
    */
    
    CB_SCOPE_CRITICAL_SECTION(*s_threadLogMutex);
    
    for(int i=0;i<s_threadLogQueue.size32();i++)
    {
        lprintf( s_threadLogQueue[i].buf );
    }   
    s_threadLogQueue.clear();
    //s_unflushedCount = 0;
}

//=======================================================================

END_CB

#include "Base.h"
#include "Callback.h"
#include "vector.h"
#include "Timer.h"
#include <stdio.h>

START_CB

//========================================================================================
// CallbackQueue for executing callbacks at a given time
//
// CallbackQueue could be log(N) to insert and O(1) to tick if I kept the list sorted by time
//	but don't bother for now

namespace CallbackQueue
{
	vector<CallbackPtr>	m_queue;
};


void CallbackQueue::Add(CallbackPtr cb)
{
	m_queue.push_back(cb);
}
	
void CallbackQueue::Tick()
{
	if ( m_queue.empty() )
		return;

	double now = Timer::GetSeconds();
	
	// go backwards, deleting
	for(int i=m_queue.size()-1;i>=0;i--)
	{
		if ( now >= m_queue[i]->GetTime() )
		{
			m_queue[i]->Do();
			m_queue[i] = NULL;

			m_queue.erase_u(i);
		}
	}
}

//=======================================================================================

struct TestC
{
	TestC(int i)
	{
		m_i = i;
	}
	
	void DoStuff1()
	{
		printf("%d\n",m_i);
	}

	void DoStuff2(int a)
	{
		printf("%d %d\n",m_i,a);
	}

	void DoStuff3(int a,int b)
	{
		printf("%d %d %d\n",m_i,a,b);
	}

	int	m_i;
};

SPtrFwd(TestCR);
class TestCR : public TestC, public RefCounted
{
public:
	TestCR(int i) : TestC(i)
	{
	}
	
};

void global_func()
{
	puts("hello");
}

void callback_test()
{
	vector<CallbackPtr> cbs;

	TestC t1(4);
	TestC t2(2);

	TestCRPtr t3( new TestCR(3) );

	CallbackPtr cb;
	cb = make_cbm(&t1,&TestC::DoStuff1,0.0);
	cbs.push_back(cb);
	
	cb = make_cbm(t3,&TestC::DoStuff1,0.0);
	cbs.push_back(cb);

	cb = make_cb(global_func,0.0);
	cbs.push_back(cb);
	
	cbs.push_back( make_cbm(&t2,&TestC::DoStuff2,4,0.0) );

	cbs.push_back( make_cbm(&t2,&TestC::DoStuff3,9,9,0.0) );

	vector<CallbackPtr>::iterator it;
	for(it = cbs.begin();it != cbs.end();++it)
	{
		(*it)->Do();
	}
	
	for(it = cbs.begin();it != cbs.end();++it)
	{
		(*it) = NULL;
	}
}


//========================================================================================

END_CB

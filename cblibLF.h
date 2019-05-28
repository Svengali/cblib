#pragma once

#include <cblib/LF/cblibCpp0x.h>

using namespace CBLIB_LF_NS;

//=================================================================		

// singly linked node with atomic next pointer
//	used by a few primitives
struct lf_slist_node
{
	atomic<lf_slist_node *>	m_next;
};

// slist node + aba counter
struct LF_OS_ALIGN_2POINTERS lf_slist_node_and_count
{
	lf_slist_node *	p;
	intptr_t		c;
};

// mpmc_lifo
//	like windows "SList" aka a stack
//	lockfree for multiple producers & multiple consumers
struct LF_OS_ALIGN_2POINTERS lf_mpmc_lifo
{
	atomic<lf_slist_node_and_count>	m_head;
	
	lf_mpmc_lifo()
	{
		lf_slist_node_and_count zero = { 0, 0 };
		m_head($).store(zero,mo_relaxed);
	}
	~lf_mpmc_lifo()
	{
	}
	
	void push(lf_slist_node * node)
	{
		backoff bo;
				
		lf_slist_node_and_count localHead = m_head($).load(mo_acquire);
			
		for(;;)
		{
			// this assert usually means you are pushing a node twice :
			LF_OS_ASSERT( node != localHead.p );
		
			// node is owned by me, set up ->next
			node->m_next($).store( localHead.p, mo_relaxed );
					
			lf_slist_node_and_count newHead;
			newHead.p = node;
			newHead.c = localHead.c; // no count change
		
			// try to CAS in the new head :
			if ( m_head($).compare_exchange_strong(localHead,newHead,mo_acq_rel,mo_acquire) )
			{
				return;
			}
			// else retry
			// localHead was reloaded bycompare_exchange_strong
			
			bo.yield($);
		}
	}
	
	bool empty() const
	{
		lf_slist_node_and_count head = m_head($).load(mo_acquire);
		return ( head.p == NULL );
	}
	
	lf_slist_node * pop()
	{
		backoff bo;
		
		lf_slist_node_and_count localHead = m_head($).load(mo_acquire);
		
		for(;;)
		{
			lf_slist_node * node = localHead.p;
			
			if ( node == NULL )
			{
				// list is empty
				return NULL;
			}
			
			// try to advance the head and inc aba counter :	
			lf_slist_node_and_count newHead;
			newHead.p = node->m_next($).load(mo_relaxed);
			newHead.c = localHead.c + 1;
				
			// CAS new head in :
			if ( m_head($).compare_exchange_strong(localHead,newHead,mo_acq_rel,mo_acquire) )
			{
				LF_OS_ASSERT( newHead.p != node );
			
				return node;
			}
			
			// localHead was reloaded
			bo.yield($);
		}
	}
	
	// replace whole list with "newList" and return the old list
	// this could just be a very fast exchange() except for aba risk
	lf_slist_node * swap(lf_slist_node * newList)
	{
		backoff bo;
		
		lf_slist_node_and_count newHead;
		newHead.p = newList;
		
		lf_slist_node_and_count localHead = m_head($).load(mo_acquire);
			
		for(;;)
		{
			newHead.c = localHead.c + 1;
				
			if ( m_head($).compare_exchange_strong(localHead,newHead,mo_acq_rel,mo_acquire) )
			{
				LF_OS_ASSERT( newList != localHead.p || newList == 0 );
			
				return localHead.p;
			}
			
			// localHead was reloaded
			bo.yield($);
		}
	}
	
	// grab the whole list :
	lf_slist_node * pop_all()
	{		
		return swap(NULL);
	}
	
};

//=================================================================

// singly linked node with nonatomic next pointer	
//	must be bitwise identical to lf_slist_node
struct lf_slist_node_nonatomic
{
	lf_slist_node_nonatomic *	m_next;
};

LF_OS_COMPILER_ASSERT( sizeof(lf_slist_node_nonatomic) == sizeof(lf_slist_node) );

LF_OS_COMPILER_ASSERT( offsetof(lf_slist_node_nonatomic,m_next) == offsetof(lf_slist_node,m_next) );

// mpsc_fifo 
//	built on mpmc_lifo with a single-thread consumer reversal list
//	lockfree for multiple producers & a single consumer
struct LF_OS_ALIGN_2POINTERS lf_mpsc_fifo
{
public:

	lf_mpsc_fifo() : m_fifo(NULL) { }
	~lf_mpsc_fifo() { }
	
	void push(lf_slist_node * node)
	{
		// just push lifo, multi-producer safe
		m_lifo.push(node);
	}
	
	// only one thread (single consumer) can call pop :
	lf_slist_node * pop()
	{
		if ( m_fifo == NULL )
		{
			// grab lifo to fifo :
			fetch();
		}
		
		// try to pop something off the fifo :
		lf_slist_node_nonatomic * head = m_fifo;
		
		if ( head == NULL )
			return NULL;
		
		m_fifo = head->m_next;
		
		return (lf_slist_node *) head;		
	}
	
private:
	lf_mpmc_lifo	m_lifo;
	char			m_pad[LF_OS_CACHE_LINE_SIZE];
	lf_slist_node_nonatomic * m_fifo; // owned by the single consumer thread

	void fetch(void)
	{
		LF_OS_ASSERT( m_fifo == NULL );
		
		// grab everything on the lock-free lifo
		lf_slist_node * lifo = m_lifo.pop_all();
				
		if ( lifo == NULL )
			return;
		
		// I own lifo list now, so no need to use atomic ops on it :
		lf_slist_node_nonatomic * node = (lf_slist_node_nonatomic *) lifo;
		
		// reverse the list into m_fifo :
		while( node )
		{
			lf_slist_node_nonatomic * next = node->m_next;
			
			node->m_next = m_fifo;
			m_fifo = node;
			
			node = next;
		}
	}

};

// mpmc fifo with mutex on the consumer
//	built on lf_mpsc_fifo
struct LF_OS_ALIGN_2POINTERS lf_mpmc_mutex_fifo
{
public:
	 lf_mpmc_mutex_fifo() {}
	~lf_mpmc_mutex_fifo() {}
	
	void push(lf_slist_node * node)
	{
		m_mpsc.push(node);
	}
	
	lf_slist_node * pop()
	{
		lock_guard<mutex> g(&m_pop_lock);
		return m_mpsc.pop();
	}	

private:
	lf_mpsc_fifo	m_mpsc;
	char			m_pad[LF_OS_CACHE_LINE_SIZE];
	mutex			m_pop_lock;
};

//=================================================================			
// spsc fifo
//	lock free for single producer, single consumer
//	requires an allocator
//	and a dummy node so the fifo is never empty
template <typename t_data>
struct lf_spsc_fifo_t
{
public:

    lf_spsc_fifo_t()
    {
		// initialize with one dummy node :
		node * dummy = new node;
		m_head = dummy;
		m_tail = dummy;
    }

    ~lf_spsc_fifo_t()
    {
		// should be one node left :
		LF_OS_ASSERT( m_head == m_tail );
		delete m_head;
    }

    void push(const t_data & data)
    {
		node * n = new node(data);
		// n->next == NULL from constructor
        m_head->next.store(n, memory_order_release); 
        m_head = n;
    }

	// returns true if a node was popped
	//	fills *pdata only if the return value is true
    bool pop(t_data * pdata)
    {
		// we're going to take the data from m_tail->next
		//	and free m_tail
        node* t = m_tail;
        node* n = t->next.load(memory_order_acquire);
        if ( n == NULL )
            return false;
        *pdata = n->data; // could be a swap
        m_tail = n;
        delete t;
        return true;
    }

private:

	struct node
	{
		atomic<node *>		next;
		nonatomic<t_data>	data;
		
		node() : next(NULL) { }
		node(const t_data & d) : next(NULL), data(d) { }
	};

	// head and tail are owned by separate threads,
	//	make sure there's no false sharing :
	nonatomic<node *>	m_head;
	char				m_pad[LF_OS_CACHE_LINE_SIZE];
	nonatomic<node *>	m_tail;
};


// mpmc fifo
//	built by putting a mutex on each end of spsc fifo
// the two-end mutexes mean that contention is low in most use cases
template <typename t_data>
struct lf_mpmc_twomutex_fifo_t
{
public:

	lf_mpmc_twomutex_fifo_t() { }
	~lf_mpmc_twomutex_fifo_t() { }


    void push(const t_data & data)
    {
		lock_guard<mutex> g(&m_push_lock);
		
		m_spsc_fifo.push(data);
    }

    bool pop(t_data * pdata)
    {
		lock_guard<mutex> g(&m_pop_lock);
		
		return m_spsc_fifo.pop(pdata);
    }

private:
	
	mutex	m_push_lock;
	char	m_pad1[LF_OS_CACHE_LINE_SIZE];
	mutex	m_pop_lock;
	char	m_pad2[LF_OS_CACHE_LINE_SIZE];
	lf_spsc_fifo_t<t_data>	m_spsc_fifo;
};


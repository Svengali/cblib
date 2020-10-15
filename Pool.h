#pragma once

#include "Base.h"
#include "entry_array.h"

START_CB

template <typename t_type,int num_per_chunk>
class Pool
{
public:
	typedef t_type entry_type;
	t_type dummy_item;
	
	Pool() : m_chunkList(NULL), m_freeList(NULL), m_next(num_per_chunk)
	{
	}
	
	~Pool()
	{
		FreeAllChunks();
	}
	
	void Reset() 
	{
		FreeAllChunks(); 
	}

	t_type * Alloc()
	{
		if ( m_freeList )
		{
			FreeList * f = m_freeList;
			m_freeList = f->next;
			t_type * entry = (t_type *)f;
			entry_array::construct(entry);
			return entry;
		}
		else
		{
			if ( m_next == num_per_chunk )
				AddChunk();

			t_type * ret = & m_chunkList->items[m_next];
			m_next++;	
			return ret;
		}
	}
	
	void Free(t_type * entry)
	{
		entry_array::destruct(entry);
		// reinterpret only after destructing :
		FreeList * f = (FreeList *) entry;
		f->next = m_freeList;
		m_freeList = f;
	}

	void FreeVoid(void *p) { Free( (t_type *)p ); }

private:
	struct FreeList
	{
		FreeList * next;
	};
	struct Chunk
	{
		Chunk * next;
		t_type items[num_per_chunk];
	};

	Chunk  *	m_chunkList;
	FreeList *	m_freeList;
	int			m_next;

	void AddChunk()
	{
		Chunk * cur = new Chunk;
		m_next = 0;
		
		cur->next = m_chunkList;
		m_chunkList = cur;
	}
		
	void FreeAllChunks()
	{
		Chunk * c = m_chunkList;
		while ( c )
		{
			Chunk * next = c->next;
			delete c;
			c = next;
		}
		m_freeList = NULL;
		m_chunkList = NULL;
		m_next = num_per_chunk;
	}
};


END_CB


// don't go through debug memory system, it uses us !!
#define FROM_MEMORY_CPP

#include "cblib/Base.h"
#include "cblib/SmallAllocator_Greedy.h"
#include "cblib/Log.h"

#include <stdlib.h> // for malloc

#define WIN32_LEAN_AND_MEAN
#include <windows.h> // for VirtualAlloc

START_CB

/*******************************************

GreedyAllocator

very fast greedy allocator; never releases memory
back to the low-level system allocator.  Passes
large allocations through; optimizes small allocations.
Released small chunks are available for future allocations
of that same size, but not for any allocation of any other.

-> this is faster than the STLPort pooled allocator,
and it's faster than Alexei Alexandrescu's allocator.
The over-head can be quite high, however.

-> The STLPort "node_alloc" is just like this; it never
restores memory to the pool.

@@ : could do a non-linear Index/Size mapping, something like this :
	0 : 4	(steps of 4)
	1 : 8
	2 : 12
	3 : 16	(steps of 8)
	4 : 24
	5 : 32
	...
	X : 128
	X+1 : 160
	X+2 : 192


*********************************************/

// LowLevel Alloc/Free :
//	!! replace these with system calls

static inline void * LowLevelAlloc(const int size)
{
	return VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
}
static inline void LowLevelFree(void * p,const int)
{
	VirtualFree(p,0,MEM_RELEASE);
}
/*
static inline void * LowLevelAlloc(const int size)
{
	return malloc(size);
}
static inline void LowLevelFree(void * p,const int size)
{
	free(p);
}
*/

#ifdef _DEBUG
#define DO_STATS
#define DO_INVALIDATE_DATA
#endif

namespace SmallAllocator_Greedy
{

//{=============================================================================
	enum
	{
		c_SmallChunk_MaxBytes = 256,
		c_Align_Shift = 3,
		c_Align_Round = (1<<c_Align_Shift)-1,
		c_SmallChunk_Count = c_SmallChunk_MaxBytes>>c_Align_Shift,
		c_SmallChunk_AllocSize = 8192,
		c_SmallChunk_MemHunkSize = c_SmallChunk_AllocSize - 4 // 4096 - sizeof(ChunkBlock)
	};

//}{=============================================================================

	class SmallChunkAllocator
	{
	private:
		
		struct FreeChunk
		{
			FreeChunk * m_pNext;
		};

		#pragma warning(disable : 4200) // zero-byte array
		struct ChunkBlock
		{
			// @@ would get better alignment if pNext came *after* m_memory
			ChunkBlock * m_pNext;
			char m_memory[0];
		};
		#pragma warning(default : 4200) // zero-byte array

	public:

		//-------------------------------------------------------------------------

		void Construct(const int size)
		{
			ASSERT( (size&3) == 0 );
			ASSERT( size >= 4 );
			m_pFree  = NULL;
			m_pHunks = NULL;
			m_nHunkSize = size;
			m_nChunks = (c_SmallChunk_MemHunkSize/m_nHunkSize);
		
			#ifdef DO_STATS
			stats_AllocatedBytes = stats_UsedChunks = 0;
			#endif
		}

		void Destruct()
		{
			int nChunkBytes = m_nChunks * m_nHunkSize;

			ChunkBlock * pCurHunk = m_pHunks;
			while(pCurHunk)
			{
				ChunkBlock * pNextHunk = pCurHunk->m_pNext;
				LowLevelFree(pCurHunk,sizeof(ChunkBlock) + nChunkBytes);
				pCurHunk = pNextHunk;
			}
			m_pFree  = NULL;
			m_pHunks = NULL;
			// you can now use me again
			
			#ifdef DO_STATS
			stats_AllocatedBytes = stats_UsedChunks = 0;
			#endif
		}

		//-------------------------------------------------------------------------

		__forceinline void * Allocate()
		{
			if ( ! m_pFree )
			{
				Extend();
				ASSERT(m_pFree);
			}
	
			#ifdef DO_STATS
			stats_UsedChunks++;
			#endif
			
			void * ptr = m_pFree;
			m_pFree = m_pFree->m_pNext;
			
			#ifdef DO_INVALIDATE_DATA
			memset(ptr,0xCD,m_nHunkSize);
			#endif

			return ptr;
		}

		//-------------------------------------------------------------------------
		__forceinline void Free(void * p)
		{
			ASSERT( p );
			// just push it on the free list :
			
			#ifdef DO_INVALIDATE_DATA
			memset(p,0xDD,m_nHunkSize);
			#endif

			((FreeChunk *) p)->m_pNext = m_pFree;
			m_pFree = (FreeChunk *) p;
			
			#ifdef DO_STATS
			stats_UsedChunks--;
			#endif

		}

		//-------------------------------------------------------------------------
		#ifdef DO_STATS
		int	stats_AllocatedBytes;
		int stats_UsedChunks;
		#endif
		//-------------------------------------------------------------------------

	private:

		void Extend()
		{
			DURING_ASSERT( const int nChunkBytes = m_nChunks * m_nHunkSize );
			ASSERT( nChunkBytes <= c_SmallChunk_MemHunkSize );
			ASSERT( sizeof(ChunkBlock) + nChunkBytes <= c_SmallChunk_AllocSize );
			
			#ifdef DO_STATS
			stats_AllocatedBytes += c_SmallChunk_AllocSize;
			#endif

			// add a hunk :
			ChunkBlock * pNewHunk = (ChunkBlock *) LowLevelAlloc( c_SmallChunk_AllocSize );
			ASSERT(pNewHunk);
			pNewHunk->m_pNext = m_pHunks;
			m_pHunks = pNewHunk;

			// add all chunks in the hunk to the free list :
			// make sure they're added in order, not reverse order			
			char * ptr = m_pHunks->m_memory;
			FreeChunk ** pFreePtr = &m_pFree;
			for(int i=0;i<m_nChunks;i++)
			{
				FreeChunk * pFree = (FreeChunk *)ptr;
				*pFreePtr = pFree;
				pFreePtr = &(pFree->m_pNext);
				ptr += m_nHunkSize;
			}
			*pFreePtr = NULL;
		}

		//-------------------------------------------------------------------------
		// data :

		FreeChunk * m_pFree;
		ChunkBlock * m_pHunks;
		int			m_nChunks;
		int			m_nHunkSize;
	
	}; // end SmallChunkAllocator

//}{=============================================================================

	static SmallChunkAllocator	* s_pAllocators = NULL;

	__forceinline int IndexToSize(const int index)
	{
		return (index+1)<<c_Align_Shift;
	}
	__forceinline int SizeToIndex(const int size)
	{
		return ((size+c_Align_Round)>>c_Align_Shift) - 1;
	}

	SmallChunkAllocator & SmallAllocator(const int which)
	{
		ASSERT( which >= 0 && which < c_SmallChunk_Count );
		if ( ! s_pAllocators )
		{
			ASSERT( (c_Align_Round>>c_Align_Shift) == 0 );
			ASSERT( ((c_Align_Round+1)>>c_Align_Shift) == 1 );
			ASSERT( IndexToSize(0) > 0 );

			// allocate and construct
			//	do NOT use new[] because it might use us !!
			s_pAllocators = (SmallChunkAllocator *) LowLevelAlloc(sizeof(SmallChunkAllocator) * c_SmallChunk_Count);
			for(int i=0;i<c_SmallChunk_Count;i++)
			{
				s_pAllocators[i].Construct(IndexToSize(i));
			}
		}
		return s_pAllocators[which];
	}

	void Destroy()
	{
		if ( ! s_pAllocators )
			return;
		
		for(int i=0;i<c_SmallChunk_Count;i++)
		{
			SmallAllocator(i).Destruct();
		}

		LowLevelFree(s_pAllocators,(sizeof(SmallChunkAllocator) * c_SmallChunk_Count));
		s_pAllocators = NULL;
	}


//}{=============================================================================

#ifdef DO_STATS //{

	void Log()
	{
		if ( ! s_pAllocators )
			return;

		for(int i=0;i<c_SmallChunk_Count;i++)
		{
			if ( SmallAllocator(i).stats_AllocatedBytes > 0 )
			{
				int usedBytes = SmallAllocator(i).stats_UsedChunks * (i+1)*4;
				float overhead = (SmallAllocator(i).stats_AllocatedBytes - usedBytes) * 100.f / (usedBytes ? usedBytes : 1 );

				lprintf("chunk size : %d , num out : %d , allocated bytes : %d , overhead = %1.1f %%\n",
						(i+1)*4, SmallAllocator(i).stats_UsedChunks ,
						SmallAllocator(i).stats_AllocatedBytes,
						overhead);
			}
		}
	}

#else //}{

	void Log()
	{
		// nada
	}

#endif //} DO_STATS

//}{=============================================================================

	void * Allocate(const int size)
	{
		if ( size > c_SmallChunk_MaxBytes )
		{
			return malloc(size);
		}
		else
		{
			// allocate small chunk

			// align to four bytes :
			const int which = SizeToIndex(size);
			ASSERT(which >= 0 && which < c_SmallChunk_Count );
			return SmallAllocator(which).Allocate();
		}
	}

	void	Free(void * p,const int size)
	{
		if ( size > c_SmallChunk_MaxBytes )
		{
			free(p);
		}
		else
		{
			if ( ! p )
				return;

			// free small chunk

			// align to four bytes :
			const int which = SizeToIndex(size);
			ASSERT(which >= 0 && which < c_SmallChunk_Count );
			SmallAllocator(which).Free(p);
		}
	}

//}=============================================================================
}; // namespace

END_CB

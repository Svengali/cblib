#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE

// don't go through debug memory system, it uses us !!
#define FROM_MEMORY_CPP

#include "cblib/Base.h"
#include "cblib/SmallAllocator_Greedy.h"
#include "cblib/Log.h"
#include "cblib/MemTrack.h"
#include "cblib/Threading.h"
#include "cblib/LF/LFSList.h"

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
The memory waste over-head can be quite high, however.

-> The STLPort "node_alloc" is just like this; it never
restores memory to the pool.

Greedy SmallAlloc :
	
	freelist on each chunk can be independently lockfree pushed & popped
	
	do variable-alignment sizes
		then just use a [256] array to point to the next larger size
		{4,8,12,16,20,24,28,32,} then +8, then +16
	
	can do a Free() without size by using the 64k granularity of VirtualAlloc
		take the pointer and >> 16 -> a 16 bit index
		only works on 32 bit pointers

---------------------------

Threading model :

Thereis a SmallChunkAllocator for each size

Each SmallChunkAllocator can be talked to independently

If they have to Extend they will go through to the OS allocator which is thread safe

Each SmallChunkAllocator is basically just a singly-linked-list free list
	To allocate you just Pop off it
	Use lock-free stack pop
	
If its null it means we need to extend
	handle race case that two people can get null and both try to extend at the same time
	use my critsec

*********************************************/

// LowLevel Alloc/Free :
//	!! replace these with system calls

static inline void * _HeapAlloc(const size_t size)
{
	return GlobalAlloc(0,size);
}
static inline void _HeapFree(void * p)
{
	GlobalFree(p);
}


#ifdef CB_64

/**

in 64 bit, we reserve a big chunk of virtual address space for all small allocs
we can see if an alloc was "small" or not by checking if its pointer is in that range
if it is, then I can find the base of its chunk by just rounding to the next lower 64k
from the chunk I get the allocator

**/

//static intptr_t s_chunkSize = (1i64<<32);
static intptr_t s_chunkSize = (1i64<<30);
static intptr_t s_chunkBase = 0;
static intptr_t s_nextChunkAddr = 0;

static void _ChunkInit()
{
	s_chunkBase = (intptr_t) VirtualAlloc(NULL, s_chunkSize , MEM_RESERVE, PAGE_READWRITE );
	ASSERT_RELEASE( s_chunkBase != 0 );
	s_nextChunkAddr = s_chunkBase;
}

static void _ChunkDestroy()
{
	VirtualFree((void *)s_chunkBase,s_chunkSize,MEM_RELEASE);
	s_chunkBase = s_nextChunkAddr = 0;
}
		
static bool _ChunkIsInRange(void * ptr)
{
	intptr_t i = (intptr_t) ptr;
	if ( i < s_chunkBase || i > (s_chunkBase + s_chunkSize) )
		return false;
	return true;
}

static inline void * _ChunkAlloc(const size_t size)
{
	void * ptr = VirtualAlloc((void *)s_nextChunkAddr, size, MEM_COMMIT, PAGE_READWRITE);
	ASSERT_RELEASE( ptr != NULL );
	
	MEMORY_BASIC_INFORMATION mbi = { 0 };
	VirtualQuery(ptr,&mbi,sizeof(mbi));
	s_nextChunkAddr = (intptr_t) mbi.BaseAddress + mbi.RegionSize;
	ASSERT( s_nextChunkAddr >= ( (intptr_t)ptr + (intptr_t)size ) );
	ASSERT( _ChunkIsInRange(ptr ) );
	return ptr;
}
static inline void _ChunkFree(void * ptr)
{
	ASSERT( _ChunkIsInRange(ptr ) );
	VirtualFree(ptr,0,MEM_DECOMMIT);
}

#else
	
static void _ChunkInit()
{
}

static void _ChunkDestroy()
{
}

// c_SmallChunk_AllocSize needs to be 64k if you usse VirtualAlloc for chunks
static inline void * _ChunkAlloc(const size_t size)
{
	return VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
}
static inline void _ChunkFree(void * p)
{
	VirtualFree(p,0,MEM_RELEASE);
}

#endif //
	
#ifdef _DEBUG
#define DO_STATS
#define DO_INVALIDATE_DATA
#endif

/*
static bool InterlockedIsEmptySList(PSLIST_HEADER header)
{
	PSLIST_ENTRY entry = InterlockedPopEntrySList(header);
	if ( entry == NULL )
		return true;
	InterlockedPushEntrySList(header,entry);
	return false;
}
*/

namespace SmallAllocator_Greedy
{

// SList requires MEMORY_ALLOCATION_ALIGNMENT == 8

	// 4 is the minimum size because each block is unioned with a pointer
	static const int c_sizes[] = 
	{
//		4,8,12,16,
//		20,24,28,32,

	#ifndef CB_64
		8,16,24,32,40,48,56,
	#else
		16,32,48,
	#endif
		64,80,96,112,128,
		160,192,224,256
		
	#ifdef CB_64
		,288,320,352,384,416,448,480,512
	#endif
	};
	static const int c_numSizes = ARRAY_SIZE(c_sizes);

//{=============================================================================
	enum
	{
	#ifdef CB_64
		c_SmallChunk_MaxBytes = 512,
	#else
		c_SmallChunk_MaxBytes = 256,
	#endif	
		c_SmallChunk_Count = c_numSizes,
		
		//c_SmallChunk_AllocSize = 8192,
		c_SmallChunk_AllocShift = 16,
		c_SmallChunk_AllocSize = (1<<c_SmallChunk_AllocShift),
		c_SmallChunk_MemHunkSize = c_SmallChunk_AllocSize - 2*sizeof(void *), // 4096 - sizeof(ChunkBlock)

	};

	static bool IsVAAligned(void * ptr)
	{
		intptr_t newHunkAddr = (intptr_t) ptr;
		return ( ( newHunkAddr & (c_SmallChunk_AllocSize -1) ) == 0 );
	}
	
	#ifndef CB_64
	
	/*
	
	In 32 bit, I use a pointer for every VirtualAlloc page to tell if it's in the Small Block or not.
	
	This could just be a single bit, not a pointer (32X less memory overhead)
	
	and if the bit says "yes you are a small block" then use the 64-bit method to find the Allocator
	
	*/
	
	//static const uint32 c_MaxVirtualAddress = (1UL<<31);
	//static const uint32 c_NumVAChunkPtrs = (c_MaxVirtualAddress / c_SmallChunk_AllocSize);
	static const uint32 c_NumVAChunkPtrs = (1UL<<16); // supports 4GB of addresses 
	
	struct SmallChunkAllocator;
	
	// s_vaChunkBits tells us if a VA page is in the small pool or not
	//	I need one bit for each 64k page
	//   (4GB/64k) = 64k pages exist
	//   64k * 1 bit = 8k bytes
	static uint32 s_vaChunkBits[c_NumVAChunkPtrs/32] = { 0 };

	static int GetVAIndex(void * ptr)
	{
		intptr_t newHunkAddr = (intptr_t) ptr;
		intptr_t newHunkI = newHunkAddr >> c_SmallChunk_AllocShift;
		ASSERT( newHunkI >= 0 && newHunkI < c_NumVAChunkPtrs );
		return (int) newHunkI;
	}
	static uint32 GetVAIndexBit(int index)
	{
		int i = index/32;
		int b = index - i*32;
		uint32 val = LoadRelaxed(&(s_vaChunkBits[i]));
		return val & (1UL<<b);
	}
	static void SetVAIndexBit(int index)
	{
		int i = index/32;
		int b = index - i*32;
		uint32 mask = (1UL<<b);
		
		for(;;)
		{
			uint32 oldV = LoadRelaxed(&(s_vaChunkBits[i]));
			uint32 newV = oldV | mask;
			if ( AtomicCAS(&(s_vaChunkBits[i]),oldV,newV) )
				break;
		}
	}
	
	static void ClearVAIndexBit(int index)
	{
		int i = index/32;
		int b = index - i*32;
		uint32 mask = (1UL<<b);
		
		for(;;)
		{
			uint32 oldV = LoadAcquire(&(s_vaChunkBits[i]));
			uint32 newV = oldV & ~mask;
			if ( AtomicCAS(&(s_vaChunkBits[i]),oldV,newV) )
				break;
		}
	}
	
	#endif
		
	
	
//}{=============================================================================

	// SmallChunkAllocator's constructor and destructor never get called
	//	it's made with alloc/free not new/delete
	struct SmallChunkAllocator
	{
	public:
		
		struct FreeChunk
		{
			FreeChunk * m_pNext;
		};

		struct ChunkBlock
		{
			// mem must be first for alignment :
			char m_memory[c_SmallChunk_MemHunkSize];
			SmallChunkAllocator * m_pOwner;
			ChunkBlock * m_pNext;
		};
		COMPILER_ASSERT( sizeof(ChunkBlock) == c_SmallChunk_AllocSize );

		//-------------------------------------------------------------------------

		void Init(const int size)
		{
			ASSERT( (size&3) == 0 );
			ASSERT( size >= 4 );
			
			// for SList :
			ASSERT( (size % MEMORY_ALLOCATION_ALIGNMENT) == 0 );
			
			m_pHunks = NULL;
			m_nHunkSize = size;
			m_nChunks = (c_SmallChunk_MemHunkSize/m_nHunkSize);
		
			//InitializeSListHead(&m_freeList);
			LFSList_Open(&m_freeList);
		
			InitializeCriticalSectionAndSpinCount(&m_critsec,SpinBackOff::SpinBackOff_Spins);
		
			#ifdef DO_STATS
			stats_AllocatedBytes = stats_UsedChunks = 0;
			#endif
		}

		int GetAllocSize() const { return m_nHunkSize; }

		void Destruct()
		{
			// I'm not sure if trying to be careful here at all does any good :	
			EnterCriticalSection(&m_critsec);
			
			ChunkBlock * pCurHunk = m_pHunks;
			m_pHunks = NULL;
			LFSList_Close(&m_freeList);
			//InterlockedFlushSList(&m_freeList);
			
			LeaveCriticalSection(&m_critsec);
						
			//int nChunkBytes = m_nChunks * m_nHunkSize;

			while(pCurHunk)
			{
				ChunkBlock * pNextHunk = pCurHunk->m_pNext;
				
  				#ifndef CB_64				
				// this is kind of pointless cuz we are shutting down
				int index = GetVAIndex(pCurHunk);
				ClearVAIndexBit(index);
				#endif
							
				_ChunkFree(pCurHunk);
				
				pCurHunk = pNextHunk;
			}
			// you can now use me again
			
			DeleteCriticalSection(&m_critsec);
			
			#ifdef DO_STATS
			stats_AllocatedBytes = stats_UsedChunks = 0;
			#endif
		}

		//-------------------------------------------------------------------------

		__forceinline void * Allocate()
		{
			/*
			CB_SCOPE_CRITICAL_SECTION(m_critsec);
			if ( ! m_pFree )
			{
				Extend();
				ASSERT(m_pFree);
			}
	
			void * ptr = m_pFree;
			m_pFree = m_pFree->m_pNext;
			*/
			
			// lockfree pop :
			//	 if I get a null, then take a critsec and do Extend
			void * ptr;
			for(;;)
			{
				//ptr = InterlockedPopEntrySList(&m_freeList);
				ptr = (void *) LFSList_Pop(&m_freeList);
				if ( ptr == NULL )
				{
					Extend();
				}
				else
				{
					break;
				}
			}		
			
			#ifdef DO_STATS
			stats_UsedChunks++;
			#endif
			
			#ifdef DO_INVALIDATE_DATA
			memset(ptr,0xCD,m_nHunkSize);
			#endif

			return ptr;
		}

		//-----------------------------------------------------
		__forceinline void Free(void * p)
		{
			ASSERT( p );
			// just push it on the free list :
			
			#ifdef DO_INVALIDATE_DATA
			memset(p,0xDD,m_nHunkSize);
			#endif

			// lockfree push :
			//InterlockedPushEntrySList(&m_freeList,(PSLIST_ENTRY)p);
			LFSList_Push(&m_freeList,(LFSNode *)p);

			//CB_SCOPE_CRITICAL_SECTION(m_critsec);
			//((FreeChunk *) p)->m_pNext = m_pFree;
			//m_pFree = (FreeChunk *) p;
			
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
			EnterCriticalSection(&m_critsec);
			
			// see if there was a double entry and somebody else already extended us :
			//if ( ! InterlockedIsEmptySList(&m_freeList) )
			if ( ! LFSList_IsEmpty(&m_freeList) )
			{
				LeaveCriticalSection(&m_critsec);
				return;
			}
			
			// okay, note that even though we have a critsec, other people can be
			//	doing Push/Pop on our freelist !
			
			DURING_ASSERT( const int nChunkBytes = m_nChunks * m_nHunkSize );
			ASSERT( nChunkBytes <= c_SmallChunk_MemHunkSize );
			
			#ifdef DO_STATS
			stats_AllocatedBytes += c_SmallChunk_AllocSize;
			#endif

			// add a hunk :
			ChunkBlock * pNewHunk = (ChunkBlock *) _ChunkAlloc( c_SmallChunk_AllocSize );
			ASSERT(pNewHunk);
			pNewHunk->m_pOwner = this;
			pNewHunk->m_pNext = m_pHunks;
			m_pHunks = pNewHunk;

			#ifndef CB_64
			// set the vaChunkPtrs to point at me :
			ASSERT( IsVAAligned(pNewHunk) );
			int newHunkI = GetVAIndex(pNewHunk);
			SetVAIndexBit(newHunkI);
			#endif

			// add all chunks in the hunk to the free list :
			// make sure they're added in order, not reverse order	
			//	cuz we want to pop off in linear memory order - before for cache		
			
			// start at the end and push :
			//	this seems unnecessarily heavy, but it's actually best because
			//	if other threads are stalled on this I want them to get to freeList ASAP
			char * ptr = m_pHunks->m_memory;
			
			// !! we can release the critsec NOW (we can push on our pointers without the lock)
			//	(actually need to push at least a few first)
			//LeaveCriticalSection(&m_critsec);
			
			ptr += (m_nChunks-1) * m_nHunkSize;
			for(int i=0;i<m_nChunks;i++)
			{
				//InterlockedPushEntrySList (&m_freeList,(PSLIST_ENTRY)ptr);
				LFSList_Push(&m_freeList,(LFSNode *)ptr);
				ptr -= m_nHunkSize;
			}
			
			LeaveCriticalSection(&m_critsec);
		}

		//-------------------------------------------------------------------------
		// data :

		//DECL_ALIGN(16) SLIST_HEADER		m_freeList; // align 16 btw
		LF_ALIGN_TO_CACHE_LINE LFSList		m_freeList; // align 16 btw
		
		LF_ALIGN_TO_CACHE_LINE CRITICAL_SECTION	m_critsec;

		// only used by Extend/Destruct ; protected by critsec :
		DECL_ALIGN(16) ChunkBlock * m_pHunks;
		int			m_nChunks;
		int			m_nHunkSize;
		//int			pad;
		
	}; // end SmallChunkAllocator

#ifdef _WIN64

	COMPILER_ASSERT( sizeof(void *) == 8 );
	COMPILER_ASSERT( sizeof(SLIST_ENTRY) == 16 );
	COMPILER_ASSERT( sizeof(LFSNode) == 8 );
	COMPILER_ASSERT( sizeof(ULONGLONG) == 8 );
	COMPILER_ASSERT( sizeof(LFSList) == 16 || sizeof(LFSList) == 8 );
	COMPILER_ASSERT( sizeof(RTL_CRITICAL_SECTION) == 40 );
	COMPILER_ASSERT( sizeof(SmallChunkAllocator) >= 64 );
	// Grr size is changed by DO_STATS
	
#else
	COMPILER_ASSERT( sizeof(void *) == 4 );
	//COMPILER_ASSERT( sizeof(SLIST_ENTRY) == 4 );
	COMPILER_ASSERT( sizeof(LFSNode) == 4 );
	COMPILER_ASSERT( sizeof(ULONGLONG) == 8 );
	COMPILER_ASSERT( sizeof(LFSList) == 8 );
	COMPILER_ASSERT( sizeof(RTL_CRITICAL_SECTION) == 24 );
	COMPILER_ASSERT( sizeof(SmallChunkAllocator) >= 64 );
	// Grr size is changed by DO_STATS
#endif

//}{=============================================================================

	static SmallChunkAllocator * GetSmallChunkAllocator(void * ptr)
	{
		intptr_t newHunkAddr = (intptr_t) ptr;
		//intptr_t newHunkI = newHunkAddr >> c_SmallChunk_AllocShift;
		//intptr_t base = newHunkI << c_SmallChunk_AllocShift;
		intptr_t base = newHunkAddr & ( ~ ((1i64 << c_SmallChunk_AllocShift)-1) );
		SmallChunkAllocator::ChunkBlock * chunk = (SmallChunkAllocator::ChunkBlock *) base;
		return chunk->m_pOwner;
	}
	
//}{=============================================================================
	
	// s_pAllocators must be a "singleton" made on demand
	//	cuz of allocs done at cinit
	// @@ could use the TLS method of singleton instead ?
	DECL_ALIGN(16) static SmallChunkAllocator	* volatile s_pAllocators = NULL;
	//DECL_ALIGN(16) static Atomic<SmallChunkAllocator *> s_pAllocators = { NULL };
	static int s_sizeToIndexTable[c_SmallChunk_MaxBytes+1] = { 0 };
	
	__forceinline int IndexToSize(const int index)
	{
		if ( index < 0 ) return 0;
		return c_sizes[index];
	}
	// SizeToIndex cannot be used until after init !
	__forceinline int SizeToIndex(const int size)
	{
		ASSERT( size <= c_SmallChunk_MaxBytes );
		ASSERT( s_sizeToIndexTable[c_SmallChunk_MaxBytes] != 0 );
		
		return s_sizeToIndexTable[size];
	}

	// Init on first use so we work in cinit
	void SmallAllocators_Init()
	{
		// @@ should this check be a LoadAcquire ?
		if ( s_pAllocators )
			return;

		// thread protect me :
		Singleton_Lock();
		
		if ( s_pAllocators ) // double check
		{
			Singleton_Unlock();
			return;
		}
		
		_ChunkInit();
		
		ASSERT( c_sizes[ c_numSizes - 1] == c_SmallChunk_MaxBytes );
	
		// allocate and construct
		//	do NOT use new[] because it might use us !!
		SmallChunkAllocator * pAllocs = (SmallChunkAllocator *) _HeapAlloc(sizeof(SmallChunkAllocator) * c_SmallChunk_Count);
		for(int i=0;i<c_SmallChunk_Count;i++)
		{
			int size = IndexToSize(i);
			pAllocs[i].Init(size);
			
			while( size > IndexToSize(i-1) )
			{
				s_sizeToIndexTable[size] = i;
				size--;
			}
		}
		
		StoreRelease(&s_pAllocators,pAllocs);
		
		Singleton_Unlock();
	}

	void Destroy()
	{			
		// Destroy is not really thread safe !
		//	don't be misled by the fakey bullshit in here,
		//	if you call this when some threads are still allocating you will die
		
		//SmallChunkAllocator * pAllocs = (SmallChunkAllocator *) AtomicExchangePointer((void **)&s_pAllocators,NULL);
		
		SmallChunkAllocator * pAllocs = (SmallChunkAllocator *) AtomicExchange((void **)&s_pAllocators,(void *)NULL);
		
		if ( ! pAllocs )
			return;
		
		for(int i=0;i<c_SmallChunk_Count;i++)
		{
			pAllocs[i].Destruct();
		}

		_HeapFree(pAllocs);
	
		_ChunkDestroy();
	}


//}{=============================================================================

#ifdef DO_STATS //{

	void Log()
	{
		if ( ! s_pAllocators )
			return;
		// @@ not thread safe !

		for(int i=0;i<c_SmallChunk_Count;i++)
		{
			const SmallChunkAllocator & chunk = s_pAllocators[i];
			if ( chunk.stats_AllocatedBytes > 0 )
			{
				int usedBytes = chunk.stats_UsedChunks * (i+1)*4;
				float overhead = (chunk.stats_AllocatedBytes - usedBytes) * 100.f / (usedBytes ? usedBytes : 1 );

				lprintf("chunk size : %d , num out : %d , allocated bytes : %d , overhead = %1.1f %%\n",
						(i+1)*4, chunk.stats_UsedChunks ,
						chunk.stats_AllocatedBytes,
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

	void * Allocate(const size_t size)
	{
		// c_SmallChunk_MaxBytes is included in stuff we handle
		if ( size > c_SmallChunk_MaxBytes )
		{
			void * ptr = _HeapAlloc(size);
			
			AutoMemTrack_Add(ptr,size,eMemTrack_Malloc);
			
			return ptr;
		}
		else
		{
			// allocate small chunk

			SmallAllocators_Init();
			
			// align to four bytes :
			const int which = SizeToIndex((int)size);
			
			ASSERT(which >= 0 && which < c_SmallChunk_Count );
			
			void * ptr = s_pAllocators[which].Allocate();

			AutoMemTrack_Add(ptr,size,eMemTrack_Small);
		
			return ptr;
		}
	}

	void	Free(void * p,const size_t size)
	{
		if ( ! p )
			return;
				
		AutoMemTrack_Remove(p);
		
		if ( s_pAllocators == NULL )
		{
			_HeapFree(p);
			return;
		}
		
		if ( size > c_SmallChunk_MaxBytes )
		{
			_HeapFree(p);
		}
		else
		{
			// free small chunk

			//SmallAllocators_Init(); // @@ should be done !
			ASSERT( s_pAllocators != NULL );
			
			// align to four bytes :
			const int which = SizeToIndex((int)size);
			
			ASSERT(which >= 0 && which < c_SmallChunk_Count );
			
			s_pAllocators[which].Free(p);
		}
	}
	
	void	FreeNoSize(void * p)
	{
		if ( ! p )
			return;
				
		AutoMemTrack_Remove(p);
		
		if ( s_pAllocators == NULL )
		{
			_HeapFree(p);
			return;
		}
		
		#ifdef CB_64
		
		if ( _ChunkIsInRange(p) )
		
		#else
		
		int index = GetVAIndex(p);
		if ( GetVAIndexBit(index) )
		
		#endif
		
		{		
			SmallChunkAllocator * alloc = GetSmallChunkAllocator(p);
		
			alloc->Free(p);
		}
		else
		{
			_HeapFree(p);
		}
	}
	
	
	int		GetMemSize(void * p)
	{
		if ( ! p )
			return 0;

		if ( s_pAllocators == NULL )
		{
			// large ; on heap
			//return c_SmallChunk_AllocSize;
			size_t size = GlobalSize(p);
			ASSERT( size > c_SmallChunk_MaxBytes );
			return check_value_cast<int>( size );
		}
				
		#ifdef CB_64
		
		if ( _ChunkIsInRange(p) )
		
		#else
		
		int index = GetVAIndex(p);
		if ( GetVAIndexBit(index) )
		
		#endif
		
		{		
			SmallChunkAllocator * alloc = GetSmallChunkAllocator(p);
		
			return alloc->GetAllocSize();
		}
		else
		{
			// large ; on heap
			//return c_SmallChunk_AllocSize;
			size_t size = GlobalSize(p);
			ASSERT( size > c_SmallChunk_MaxBytes );
			return check_value_cast<int>( size );
		}
	}

//}=============================================================================
}; // namespace

END_CB

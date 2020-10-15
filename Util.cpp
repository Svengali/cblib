#include "Util.h"
#include "Log.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <typeinfo>

START_CB

#ifdef CB_64
COMPILER_ASSERT( sizeof(void *) == 8 );
COMPILER_ASSERT( sizeof(void *) == sizeof(intptr_t) );
#else
COMPILER_ASSERT( sizeof(void *) == 4 );
COMPILER_ASSERT( sizeof(void *) == sizeof(intptr_t) );
#endif

//-----------------------------------------------------------


//-----------------------------------------------------------

const uint8 c_bitTable[256] =
{
	0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
};

int GetNumBitsSet(uint64 word)
{
	int ret = 0;
	while( word )
	{
		uint8 bottom = (uint8)(word & 0xFF);
		word >>= 8;
		ret += c_bitTable[bottom];
	}
	return ret;
}

const char * type_name(const type_info & ti)
{
	return ti.name();
}


// memswap like memmove
// no idea if this works and I don't care
void memswap (void * b1, void * b2, size_t s)
{
	static const int c_memswap_chunksize = 64;
	typedef Bytes<c_memswap_chunksize>	t_memswap_chunk;
	
	char * p1 = (char *)b1;
	char * p2 = (char *)b2;
	
	size_t n = s / c_memswap_chunksize;
	while(n--)
	{
		/*
		ByteSwap(*((t_memswap_chunk *)p1),*((t_memswap_chunk *)p2));
		p1 += c_memswap_chunksize;
		p2 += c_memswap_chunksize;
		*/
		
		uint64 * a1 = (uint64 *)p1;
		uint64 * a2 = (uint64 *)p2;
		ByteSwap(a1[0],a2[0]);
		ByteSwap(a1[1],a2[1]);
		ByteSwap(a1[2],a2[2]);
		ByteSwap(a1[3],a2[3]);
		ByteSwap(a1[4],a2[4]);
		ByteSwap(a1[5],a2[5]);
		ByteSwap(a1[6],a2[6]);
		ByteSwap(a1[7],a2[7]);
		p1 += c_memswap_chunksize;
		p2 += c_memswap_chunksize;
	}
	
	size_t r = s % c_memswap_chunksize;
	while(r--)
	{
		char save1 = *p1;
		*p1++ = *p2;
		*p2++ = save1;
	}
}

END_CB


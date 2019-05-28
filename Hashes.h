#pragma once

#include "Base.h"
#include "Util.h"
#include "StrUtil.h"

// get the extern :
extern "C" unsigned long __cdecl _lrotl(unsigned long, int);
        
#pragma intrinsic(_lrotl)


START_CB

// The recommended File Hash :
uint64 StrongHash64(const uint8 * bytes, int size);

inline uint32 BitShuffle(uint32 state)
{
    // Bob Jenkins's Mix
    state += (state << 12);
    state ^= (state >> 22);
    state += (state << 4);
    state ^= (state >> 9);
    state += (state << 10);
    state ^= (state >> 2);
    state += (state << 7);
    state ^= (state >> 12);
    return state;
}

//#define ROTATE(x,k)	(((x)<<(k)) | ((x)>>(32-(k))))
#define ROTATE(x,k)	_lrotl(x,k)

// reversible mixer from BurtleBurtle :
//  a,b,c should typically be U32's
#define HASH_MIX3(a,b,c) do { \
  a -= c;  a ^= ROTATE(c, 4);  c += b; \
  b -= a;  b ^= ROTATE(a, 6);  a += c; \
  c -= b;  c ^= ROTATE(b, 8);  b += a; \
  a -= c;  a ^= ROTATE(c,16);  c += b; \
  b -= a;  b ^= ROTATE(a,19);  a += c; \
  c -= b;  c ^= ROTATE(b, 4);  b += a; \
} while(0)

// after FinalMix3 "c" has the best value
#define HASH_FINALMIX3(a,b,c) do { \
  c ^= b; c -= ROTATE(b,14); \
  a ^= c; a -= ROTATE(c,11); \
  b ^= a; b -= ROTATE(a,25); \
  c ^= b; c -= ROTATE(b,16); \
  a ^= c; a -= ROTATE(c,4);  \
  b ^= a; b -= ROTATE(a,14); \
  c ^= b; c -= ROTATE(b,24); \
} while(0)


// DWORD_ZERO_BYTE :
//	does this 4-char dword have any zero byte ?
#define DWORD_ZERO_BYTE(V)		(((V) - 0x01010101UL) & ~(V) & 0x80808080UL)

//---------------------------------------------------
//
// FNV and Murmur are good 32 bit hashes
//	note : to use in smaller tables make sure you bit-fold down !

inline uint32 FNVHash (const char * data, int len) 
{
	uint32 hash = (uint32)(2166136261);
	for (int i=0; i < len; i++) 
	{
		hash = ((uint32)(16777619) * hash) ^ data[i];
	}
	return hash;
}

inline uint32 FNVHashStr(const char * str)
{
	uint32 hash = (uint32)(2166136261);
	while (*str)
	{
		hash = ((uint32)(16777619) * hash) ^ (*str++);
	}
	return hash;
}

inline uint32 FNVHashStrInsensitive(const char * str)
{
	uint32 hash = (uint32)(2166136261);
	while (*str)
	{
		char c = *str++;
		hash = ((uint32)(16777619) * hash) ^ tokenchar(c);
	}
	return hash;
}

uint32 MurmurHash2( const void * key, int len, uint32 seed = 0x12345678 );

//---------------------------------------------------
// CB :
//	simple hashes for 32->32 and 64->32
//	@@ TODO : these are just Murmurs ; there's got to be a simpler/better thing to do here ...
//	see http://www.concentric.net/~Ttwang/tech/inthash.htm

/*

use this ?

public int hash32shiftmult(int key)
{
  int c2=0x27d4eb2d; // a prime or an odd constant
  key = (key ^ 61) ^ (key >>> 16);
  key = key + (key << 3);
  key = key ^ (key >>> 4);
  key = key * c2;
  key = key ^ (key >>> 15);
  return key;
}

*/

inline uint32 Hash32(uint32 v)
{
	const uint32 m = 0x5bd1e995;
	
	// Mix 4 bytes at a time into the hash
	v *= m; 
	v ^= v >> 24; 
	v *= m; 
	
	uint32 h = 0x7B218BC0; // 0x12345678 * 0x5bd1e995;
	//h *= m; 
	h ^= v;
		
	// Do a few final mixes of the hash to ensure the last few
	// bytes are well-incorporated.

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
}

/*

use this ?

public int hash6432shift(long key)
{
  key = (~key) + (key << 18); // key = (key << 18) - key - 1;
  key = key ^ (key >>> 31);
  key = key * 21; // key = (key + (key << 2)) + (key << 4);
  key = key ^ (key >>> 11);
  key = key + (key << 6);
  key = key ^ (key >>> 22);
  return (int) key;
}

*/
	
inline uint32 HashTwo32(uint32 v0,uint32 v1)
{
	const uint32 m = 0x5bd1e995;
	
	//uint32 h = 0x12345678;

	// Mix 4 bytes at a time into the hash
	v0 *= m; 
	v0 ^= v0 >> 24; 
	v0 *= m; 
	
	uint32 h = 0x7B218BC0; // 0x12345678 * 0x5bd1e995;
	//h *= m; 
	h ^= v0;
	
	v1 *= m; 
	v1 ^= v1 >> 24; 
	v1 *= m; 
	
	h *= m; 
	h ^= v1;
	
	// Do a few final mixes of the hash to ensure the last few
	// bytes are well-incorporated.

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
}

inline uint32 Hash64(uint64 v)
{
	uint32 v0 = (uint32)(v>>32);
	uint32 v1 = (uint32)(v);

	return HashTwo32(v0,v1);	
}

//---------------------------------------------------

END_CB

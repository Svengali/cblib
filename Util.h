#pragma once

#include "cblib/Base.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

START_CB

//-------------------------
// my mallocs :

void * mymalloc(size_t s);

#define CBNEW			new
#define CBALLOC(size)	cb::mymalloc(size)
#define CBFREE(ptr)		free(ptr)

//-----------------------------------------------------------
// char * string utils

bool iswhitespace(char c);

const char * skipwhitespace(const char * str);

// returns true if the beginning of str matches pre
bool strpresame(const char * str,const char * pre);
bool stripresame(const char * str,const char * pre);

// returns length that prefix is the same , return is in [0,MIN(strlen(str1),strlen(str2)]
int strpresamelen(const char * str1,const char * str2);

// strrep changed fm to to
void strrep(char * str,char fm,char to);

// count # of occurence of 'c' in str
int strcount(const char * str,char c);

inline const char * strrchrorend(const char * str,char c)
{
	const char * ptr = strrchr(str,c);
	if ( ptr )
		return ptr;
	else
		return ptr + strlen(ptr);
}
inline char * strrchrorend(char * str,char c)
{
	char * ptr = strrchr(str,c);
	if ( ptr )
		return ptr;
	else
		return ptr + strlen(ptr);
}

#define errputs(str) fprintf(stderr,"%s\n",str)

// my strncpy makes sure "to" is null terminated
// NOTE : if you "USE_CB" and just call strncpy() it will call *this* not the library one
void strncpy(char * to,const char *fm,int count);

// tokenchar promotes a char like we do for the CRC - makes it upper case & back slashes
char tokenchar(char c);

// strtcmp is a strcmp like a "Token" - that is, case & slashes don't affect it
int strtcmp(const char * p1,const char * p2);

inline bool strtsame(const char * s1,const char * s2)
{
	return strtcmp(s1,s2) == 0;
}

const char * extensionpart(const char * path);
const char * stristr(const char * search_in,const char * search_for);
const char * strrstr(const char * search_in,const char * search_for);
const char * strristr(const char * search_in,const char * search_for);
const char * strichr(const char * search_in,const char search_for);
const char * filepart(const char * path);
bool isallwhitespace( const char * pstr );
void killtailingwhite( char * ptr );

//strstrend : handy call for when you want to find a substr and get the pointer to the END of it :
inline const char * strstrend(const char * search_in,const char * search_for)
{
	const char * ptr = strstr(search_in,search_for);
	if ( ! ptr ) return NULL;
	return ptr + strlen(search_for);
}

inline const char * stristrend(const char * search_in,const char * search_for)
{
	const char * ptr = stristr(search_in,search_for);
	if ( ! ptr ) return NULL;
	return ptr + strlen(search_for);
}

//strstrend : handy call for when you want to find a substr and get the pointer to the END of it :
inline char * strstrend(char * search_in,const char * search_for)
{
	return (char *)strstrend( (const char *)search_in, search_for );
}

inline char * stristrend(char * search_in,const char * search_for)
{
	return (char *)stristrend( (const char *)search_in, search_for );
}

inline char * skipwhitespace(char * str)
{
	return (char *)skipwhitespace( (const char *)str );
}
inline char * stristr(char * search_in,const char * search_for)
{
	return (char *)stristr( (const char *)search_in, search_for );
}
inline char * strrstr(char * search_in,const char * search_for)
{
	return (char *)strrstr( (const char *)search_in, search_for );
}
inline char * strristr(char * search_in,const char * search_for)
{
	return (char *)strristr( (const char *)search_in, search_for );
}
	
inline const char * strend(const char * ptr)
{
	return ptr + strlen(ptr);
}
inline char * strend(char * ptr)
{
	return ptr + strlen(ptr);
}
	
inline bool strisame(const char * s1,const char * s2)
{
	return _stricmp(s1,s2) == 0;
}

inline bool strsame(const char * s1,const char * s2)
{
	return strcmp(s1,s2) == 0;
}

char * SkipCppComments(const char * ptr );
char * FindMatchingBrace(const char * start);
char * MultiLineStrChr(const char * start,int c);
char * TokToComma(char * ptr);

//-----------------------------------------------------------

// little helper : are all values in [begin,end) the same?
template <typename t_iterator>
bool same(t_iterator begin,t_iterator end)
{
	t_iterator ptr( begin+1 );
	while(ptr < end)
	{
		if ( *ptr != *begin )
			return false;
		++ptr;
	}
	return true;
}

// use std::accumulate() for iterators
template <typename t_type>
t_type sum(const t_type * begin,const t_type * end)
{
	t_type ret(0);
	for(const t_type * ptr = begin;ptr != end;++ptr)
	{
		ret += *ptr;
	}
	return ret;
}

template <class t_iterator>
double averagefloat(const t_iterator & begin,const t_iterator & end)
{
	double ret(0);
	int count = 0;
	for(t_iterator ptr = begin;ptr != end;++ptr)
	{
		ret += *ptr;
		count ++;
	}
	if ( count > 0 )
	{
		ret /= count;
	}
	return ret;
}

template <typename t_type>
void set_all(t_type * begin,t_type * end,const t_type & val)
{
	for(t_type * ptr = begin;ptr != end;++ptr)
	{
		*ptr = val;
	}
}

template <typename T>
T array_min(const T * b,const T * e)
{
	T ret = *b;
	const T * p = b+1;
	while ( p < e )
	{
		ret = MIN(ret,*p);
		++p;
	}
	return ret;
}

template <typename T>
T array_max(const T * b,const T * e)
{
	T ret = *b;
	const T * p = b+1;
	while ( p < e )
	{
		ret = MAX(ret,*p);
		++p;
	}
	return ret;
}

template <typename t_vector>
void erase_u(t_vector & vec,const int index)
{
	vec[index] = vec.back();
	vec.pop_back();
}

//-----------------------------------------------------------
// misc shit

int GetNumBitsSet(uint64 word);

int argint(int argc,char * argv[],int & i);
double argfloat(int argc,char * argv[],int & i);

//-------------------------------------------------------------------
// templated misc functions

template<typename Type> inline void Swap(Type &a,Type &b)
{
	Type c = a; a = b; b = c;
}

template<typename Type> inline const Type Min(const Type &a,const Type &b)
{
	return MIN(a,b);
}

template<typename Type> inline const Type Max(const Type &a,const Type &b)
{
	return MAX(a,b);
}

template<class Type,class Type1,class Type2> inline Type Clamp(const Type &x,const Type1 &lo,const Type2 &hi)
{
	return ( ( x < lo ) ? lo : ( x > hi ) ? hi : x );
}

inline bool IsPow2(const int x)
{
	return ! (x & ~(-x));
}

inline int NextPow2(const int x)
{
	// @@ could be faster, but who cares?
	int y = 1;
	while ( y < x )
	{
		y += y;
	}
	return y;
}

// Aligns v up, alignment must be a power of two.
inline int AlignUp(const int v, const int alignment)
{
	ASSERT(IsPow2(alignment));
	ASSERT( v >= 0 );
	return (v+(alignment-1)) & ~(alignment-1);
}

// Aligns v down, alignment must be a power of two.
inline int AlignDown(const int v, const int alignment)
{
	ASSERT(IsPow2(alignment));
	ASSERT( v >= 0 );
	return v & ~(alignment-1);
}

//-----------------------------------------------------------
// float math helpers

//! float == test with slop
inline bool fequal(const float f1,const float f2,const float tolerance = EPSILON)
{
	const float diff = fabsf(f1 - f2);
	return diff <= tolerance;
}

inline bool fisint(const double f,const float tolerance = EPSILON)
{
	double rem = f - ((int)f);
	return ( rem < tolerance ) || ( rem > (1.0 - tolerance) );
}

//! return a Clamp float in the range lo to hi
inline float fclamp(const float x,const float lo,const float hi)
{
	return ( ( x < lo ) ? lo : ( x > hi ) ? hi : x );
}

//! return a Clamp float in the range 0 to 1
inline float fclampunit(const float x)
{
	return ( ( x < 0.f ) ? 0.f : ( x > 1.f ) ? 1.f : x );
}

//! return a lerped float at time "t" in the interval from lo to hi
//	t need not be in [0,1] , we can extrapolate too
inline float flerp(const float lo,const float hi,const float t)
{
	return lo + t * (hi - lo);
}

inline float faverage(const float x,const float y)
{
	return (x + y)*0.5f;
}

//! fsign; returns 1.f or -1.f for positive or negative float
//! could do a much faster version using float_AS_INT if this is needed
inline float fsign(const float f)
{
	return (f >= 0.f) ? 1.f : -1.f;
}

// clamp f to positive
inline float fpos(const float f)
{
	if ( f < 0.f ) return 0.f;
	return f;
}

// clamp f to negative
inline float fneg(const float f)
{
	if ( f > 0.f ) return 0.f;
	return f;
}

inline float fsquare(const float f)
{
	return f*f;
}

inline float fcube(const float f)
{
	return f*f*f;
}

//! float == 0.f test with slop
inline bool fiszero(const float f1,const float tolerance = EPSILON)
{
	return fabsf(f1) <= tolerance;
}

//! return (f1 in [0,1]) with slop
inline bool fiszerotoone(const float f1,const float tolerance = EPSILON)
{
	return (f1 >= -tolerance && f1 <= (1.f + tolerance) );
}

inline bool fisinrange(const float f,const float lo,const float hi)
{
	return f >= lo && f <= hi;
}

inline bool fisinrangesloppy(const float f,const float lo,const float hi,const float tolerance = EPSILON)
{
	return f >= lo - tolerance && f <= hi + tolerance;
}

//! float == 1.f test with slop
inline bool fisone(const float f1,const float tolerance = EPSILON)
{
	return fabsf(f1-1.f) <= tolerance;
}

//! return a float such that flerp(lo,hi, fmakelerper(lo,hi,v) ) == v
inline float fmakelerpernoclamp(const float lo,const float hi,const float v)
{
	ASSERT( hi > lo );
	return (v - lo) / (hi - lo);
}

inline float fmakelerperclamped(const float lo,const float hi,const float v)
{
	ASSERT( hi > lo );
	return fclampunit( (v - lo) / (hi - lo) );
}

inline int ftoi(const float f)
{
	return (int)f;
}

inline int froundint(const float f)
{
	return ftoi(f + 0.5f);
}

inline float fsamplelerp(const float * array,const int size,const float t)
{
	if ( t <= 0.f )
		return array[0];
	if ( t >= (size-1) )
		return array[size-1];
	const int i = ftoi(t);
	ASSERT( i >= 0 && i < (size-1) );
	const float lerper = t - i;
	return flerp( array[i], array[i+1], lerper );	
}

//-------------------------------------------------------------------

template <typename t_type>
class ScopedSet
{
public:
	ScopedSet(t_type * ptr,const t_type & val) :
		m_ptr(ptr),m_previous(*ptr)
	{
		*m_ptr = val;
	}
	~ScopedSet()
	{
		*m_ptr = m_previous;
	}
private:
	t_type * m_ptr;
	const t_type m_previous;
	void operator = (const ScopedSet<t_type> & other);
};

template <int t_bool>
struct BoolToType
{
	enum { value = t_bool };
};

typedef BoolToType<true>	BoolAsType_True;
typedef BoolToType<false>	BoolAsType_False;

//-------------------------------------------------------------------

const char * type_name(const type_info & ti);

END_CB

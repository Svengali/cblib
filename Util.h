#pragma once

#include "cblib/Base.h"
#include "cblib/Mem.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

START_CB

//-----------------------------------------------------------

#define CB_UNROLL2(op)	do { op; op; } while(0)
#define CB_UNROLL4(op)	do { CB_UNROLL2(op); CB_UNROLL2(op); } while(0)
#define CB_UNROLL6(op)	do { CB_UNROLL4(op); CB_UNROLL2(op); } while(0)
#define CB_UNROLL8(op)	do { CB_UNROLL4(op); CB_UNROLL4(op); } while(0)
#define CB_UNROLL10(op)	do { CB_UNROLL8(op); CB_UNROLL2(op); } while(0)
#define CB_UNROLL12(op)	do { CB_UNROLL8(op); CB_UNROLL4(op); } while(0)
#define CB_UNROLL16(op)	do { CB_UNROLL8(op); CB_UNROLL8(op); } while(0)


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
// WARNING : sum() is very innacurate, use kahan_accumulate instead ! (or sumfloat)
template <typename t_type>
t_type sum(const t_type * begin,const t_type * end,t_type ret)
{
	for(const t_type * ptr = begin;ptr != end;++ptr)
	{
		ret += *ptr;
	}
	return ret;
}

template <typename t_type>
t_type sum_abs(const t_type * begin,const t_type * end,t_type ret)
{
	for(const t_type * ptr = begin;ptr != end;++ptr)
	{
		ret += ABS(*ptr);
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

template <typename t_iter>
//static inline typename std::iterator_traits<t_iter>::value_type
static inline typename t_iter::value_type
iterator_min(t_iter b,t_iter e)
{
	typename t_iter::value_type ret = *b;
	t_iter p = b;
	++p;
	while ( p != e )
	{
		ret = MIN(ret,*p);
		++p;
	}
	return ret;
}

template <typename t_iter>
//static inline typename std::iterator_traits<t_iter>::value_type
static inline typename t_iter::value_type
iterator_max(t_iter b,t_iter e)
{
	typename t_iter::value_type ret = *b;
	t_iter p = b;
	++p;
	while ( p != e )
	{
		ret = MAX(ret,*p);
		++p;
	}
	return ret;
}

template <typename T>
T array_max_abs(const T * b,const T * e)
{
	T ret = *b;
	const T * p = b+1;
	while ( p < e )
	{
		ret = MAX(ret,ABS(*p));
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
// accurate accumulator for floats :
// NOTEZ : if Iter is of type float, you should typically force accum to double by passing in (double)0.0

template <typename t_iter,typename t_accum>
static inline t_accum kahan_accumulate(t_iter begin, t_iter end, t_accum accum) 
{
	t_accum err = 0;
	for(;begin != end;++begin)
	{
		t_accum compensated = *begin + err;
		t_accum tmp = accum + compensated;
		err = accum - tmp;
		err += compensated;
		accum = tmp;
	}
	return accum;
}

#ifdef CB_STL_INCLUDED
template <typename t_iter>
static inline typename std::iterator_traits<t_iter>::value_type
kahan_accumulate(t_iter begin, t_iter end)
{
	return kahan_accumulate(begin, end,
                         typename std::iterator_traits<t_iter>::value_type());
}
#endif

template <class t_iterator>
double sumfloat(const t_iterator & begin,const t_iterator & end)
{
	return kahan_accumulate(begin,end,(double)0.0);
}

template <class t_iterator>
double averagefloat(const t_iterator & begin,const t_iterator & end)
{
	int count = end - begin;
	double ret = sumfloat(begin,end);
	if ( count > 0 )
	{
		ret /= count;
	}
	return ret;
}

//-----------------------------------------------------------
// misc shit

int GetNumBitsSet(uint64 word);

// memswap like memmove
void memswap (void *, void *, size_t );

//---------------------------------------------------------------------------

// same_size_bit_cast casts the bits in memory
//	eg. it's not a value cast
template <typename t_to, typename t_fm>
t_to & same_size_value_cast( t_fm & from )
{
	COMPILER_ASSERT( sizeof(t_to) == sizeof(t_fm) );
	// just value cast :
	return (t_to) from;
}

// same_size_bit_cast casts the bits in memory
//	eg. it's not a value cast
template <typename t_to, typename t_fm>
t_to & same_size_bit_cast_p( t_fm & from )
{
	COMPILER_ASSERT( sizeof(t_to) == sizeof(t_fm) );
	// cast through char * to make aliasing work ?
	char * ptr = (char *) &from;
	return *( (t_to *) ptr );
}

// same_size_bit_cast casts the bits in memory
//	eg. it's not a value cast
// cast with union is better for gcc / Xenon :
template <typename t_to, typename t_fm>
t_to & same_size_bit_cast_u( t_fm & from )
{
	COMPILER_ASSERT( sizeof(t_to) == sizeof(t_fm) );
	union _bit_cast_union
	{
		t_fm fm;
		t_to to;		
	};
	//_bit_cast_union converter = { from };
	// return converter.to;
	_bit_cast_union * converter = (_bit_cast_union *) &from;
	return converter->to;
}

// check_value_cast just does a static_cast and makes sure you didn't wreck the value
template <typename t_to, typename t_fm>
t_to check( const t_fm & from )
{
	t_to to = static_cast<t_to>(from);
	ASSERT( static_cast<t_fm>(to) == from );
	return to;
}

#define check_value_cast check

// check_int_cast : common simple case of check_value_cast
template <typename t_fm>
inline int check_int( const t_fm & from )
{
	int to = static_cast<int>(from);
	ASSERT( static_cast<t_fm>(to) == from );
	return to;
}

#define check_int_cast check_int

inline int ptr_diff_32( ptrdiff_t diff )
{
	return check_int_cast(diff);
}

//-------------------------------------------------------------------
// templated misc functions

template <int n_count>
class Bytes
{
public:
	char bytes[n_count];
};

COMPILER_ASSERT( sizeof( Bytes<13> ) == 13 );

template <int n_count>
bool operator == ( const Bytes<n_count> & lhs, const Bytes<n_count> & rhs )
{
	for(int i=0;i<n_count;i++)
		if ( lhs.bytes[i] != rhs.bytes[i] )
			return false;
	return true;
}

template <typename t_type>
void ByteCopy(t_type * pTo,const t_type & from)
{
	typedef Bytes< sizeof(t_type) > t_bytes;
	COMPILER_ASSERT( sizeof(t_bytes) == sizeof(t_type) );
	
	*(reinterpret_cast< t_bytes * >(pTo)) = reinterpret_cast< const t_bytes & >(from);
}

template <typename t_type>
void ByteSwap(t_type & a,t_type & b)
{
	typedef Bytes< sizeof(t_type) > t_bytes;
	t_bytes c; // don't use t_type cuz we don't want a constructor or destructor
	ByteCopy((t_type *)&c,a);
	ByteCopy(&a,b);
	ByteCopy(&b,reinterpret_cast< const t_type & >(c));
}

template <typename t_type>
bool ByteEqual(const t_type & lhs,const t_type & rhs)
{
	typedef Bytes< sizeof(t_type) > t_bytes;
	COMPILER_ASSERT( sizeof(t_bytes) == sizeof(t_type) );
	
	return reinterpret_cast< const t_bytes & >(lhs) == reinterpret_cast< const t_bytes & >(rhs);
}

//-------------------------------------------------------------------
// Swap :
//
//	call cb::Swap(a,b)
//	default action is *byte* swap - that's usually what you want
//	if byte swap is not okay, override swap_functor<type>
//	you can do this easily with
//		CB_DEFINE_MEMBER_SWAP or CB_DEFINE_ASSIGNMENT_SWAP 

// AssignmentSwap utility if ByteSwap is no good
template<typename Type> inline void AssignmentSwap(Type &a,Type &b)
{
	Type c = a;  // Type c(a);
	a = b;
	b = c;
}

template<typename Type>
struct swap_functor
{
	void operator () (Type &a,Type &b)
	{
		//AssignmentSwap(a,b);
		// default is BYTES !
		ByteSwap(a,b);
	}
};

template<typename Type> inline void Swap(Type &a,Type &b)
{
	swap_functor<Type>()(a,b);
}

#define CB_DEFINE_MEMBER_SWAP(Type,Swap)	START_CB template<> struct swap_functor<Type> { void operator () (Type &a,Type &b) { a.Swap(b); } }; END_CB
#define CB_DEFINE_ASSIGNMENT_SWAP(Type,Swap)	START_CB template<> struct swap_functor<Type> { void operator () (Type &a,Type &b) { AssignmentSwap(a,b); } }; END_CB

//-------------------------------------------------------------------

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

inline uint8 ClampTo8(int val)
{
	// @@ used a lot, could be faster
	return (uint8) Clamp(val,0,255);
}

inline bool IsPow2(const int x)
{
	return ! (x & ~(-x));
}

// NextPow2 is >= x , if you call it on a Pow2 you get yourself !
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

// PrevPow2 is <= x, if you call it on a Pow2 you get yourself !
inline int PrevPow2(const int x)
{
	// @@ could be faster, but who cares?
	if ( IsPow2(x) ) return x;
	else return NextPow2(x)>>1;
}

// Aligns v up, alignment must be a power of two.
inline uint32 AlignUp(const uint32 v, const int alignment)
{
	ASSERT(IsPow2(alignment));
	ASSERT( v >= 0 );
	return (v+(alignment-1)) & ~(alignment-1);
}

// Aligns v down, alignment must be a power of two.
inline uint32 AlignDown(const uint32 v, const int alignment)
{
	ASSERT(IsPow2(alignment));
	ASSERT( v >= 0 );
	return v & ~(alignment-1);
}

// Aligns v up, alignment must be a power of two.
inline uint64 AlignUp(const uint64 v, const int alignment)
{
	ASSERT(IsPow2(alignment));
	ASSERT( v >= 0 );
	return (v+(alignment-1)) & ~(alignment-1);
}

// Aligns v down, alignment must be a power of two.
inline uint64 AlignDown(const uint64 v, const int alignment)
{
	ASSERT(IsPow2(alignment));
	ASSERT( v >= 0 );
	return v & ~(alignment-1);
}

// Aligns v up, alignment must be a power of two.
inline void * AlignUp(const void * v, const int alignment)
{
	ASSERT(IsPow2(alignment));
	#ifdef CB_64
	return (void *) cb::AlignUp( (uint64)v , alignment );
	#else
	return (void *) cb::AlignUp( (uint32)v , alignment );
	#endif
}

// Aligns v down, alignment must be a power of two.
inline void * AlignDown(const void * v, const int alignment)
{
	ASSERT(IsPow2(alignment));
	#ifdef CB_64
	return (void *) cb::AlignDown( (uint64)v , alignment );
	#else
	return (void *) cb::AlignDown( (uint32)v , alignment );
	#endif
}

//-------------------------------------------------------------------

template <typename t_type>
class ScopedSet
{
public:
	ScopedSet(t_type * ptr,const t_type val) :
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

template <bool t_bool>
struct BoolToType
{
	enum { value = t_bool };
};

typedef BoolToType<true>	BoolAsType_True;
typedef BoolToType<false>	BoolAsType_False;

//-------------------------------------------------------------------

// wrapper so you can get type name without including type_info.h
const char * type_name(const type_info & ti);

END_CB

//-------------------------------------------------------------------

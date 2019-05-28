/*!
  \file
  \brief Vec2i : 2-Space Vector of INTS
*/

#pragma once

#include "cblib/Base.h"
#include "cblib/FloatUtil.h" // cuz I use doubles

START_CB

//{===========================================================================================

class Vec2i
{
public:
	//! Vec2i is just a bag of data;
	int x,y;

	//-------------------------------------------------------------------

	__forceinline Vec2i()
	{
		// do-nada constructor;
		// can't invalidate ints, so just put something ugly in
		DURING_ASSERT( x = 0xABADF00D );
		DURING_ASSERT( y = 0xABADF00D );
	}
	
	// important : do NOT declare copy constructors which are the default !
	//	VC needs you to not declare them for it to do full return-value optimization!
	// default operator = is fine

	explicit __forceinline Vec2i(const int ix,const int iy) : 
		x(ix), y(iy)
	{
	}

	//-----------------------------------------
	// common static vectors
	//	use like Vec2i::zero
	
	static Vec2i zero;
	static Vec2i unitX;
	static Vec2i unitY;
	static Vec2i unitXneg;
	static Vec2i unitYneg;

	//-------------------------------------------------------------------
	// math operators on vectors

	void operator *= (const int scale);
	void operator += (const Vec2i & v);
	void operator -= (const Vec2i & v);
	bool operator == (const Vec2i & v) const;
	bool operator != (const Vec2i & v) const;

	// array style access :
	int	operator [](const int c) const;
	int & operator [](const int c);
	
	//-------------------------------------------------------------------
	
	bool IsValid() const;

	int64 LengthSqr() const;
	//double Length() const;

	void Set(const int ix,const int iy);

	//-------------------------------------------------------------------
	
	void	Log() const; //!< writes xy to Log; does NOT add a \n !

	static const Vec2i MakePerpCW(const Vec2i & v)
	{
		ASSERT( v.IsValid() );
		return Vec2i( v.y, -v.x);
	}

	static const Vec2i MakePerpCCW(const Vec2i & v)
	{
		ASSERT( v.IsValid() );
		return Vec2i(-v.y, v.x);
	}

	//-------------------------------------------------------------------
}; // end of class Vec2i

//}{===========================================================================================
// INLINE FUNCTIONS

//-------------------------------------------------------------------
// math operators on vectors

inline void Vec2i::operator *= (const int scale)
{
	ASSERT(IsValid());
	x *= scale;
	y *= scale;
	ASSERT(IsValid());
}

inline void Vec2i::operator += (const Vec2i & v)
{
	ASSERT(IsValid() && v.IsValid());
	x += v.x;
	y += v.y;
	ASSERT(IsValid());
}

inline void Vec2i::operator -= (const Vec2i & v)
{
	ASSERT(IsValid() && v.IsValid());
	x -= v.x;
	y -= v.y;
	ASSERT(IsValid());
}

inline bool Vec2i::operator == (const Vec2i & v) const
{
	ASSERT(IsValid() && v.IsValid());
	return (x == v.x &&
			y == v.y );
}

inline bool Vec2i::operator != (const Vec2i & v) const
{
	return ! (*this == v);
}

// array style access :
inline int Vec2i::operator [](const int c) const
{
	ASSERT( c >= 0 && c < 2 );
	ASSERT(IsValid());
	return (&x)[c];
}

inline int & Vec2i::operator [](const int c)
{
	ASSERT( c >= 0 && c < 2 );
	// may not be valid yet
	//ASSERT(IsValid());
	return (&x)[c];
}

//-------------------------------------------------------------------

inline bool Vec2i::IsValid() const
{
	return true;
}

inline int64 Vec2i::LengthSqr() const
{
	ASSERT(IsValid());
	int64 ret = int64(x) * int64(x);
	ret += int64(y) * int64(y);
	return ret;
}

/*
inline double Vec2i::Length() const
{
	return sqrt( double( LengthSqr() ) );
}
*/

inline void Vec2i::Set(const int ix,const int iy)
{
	x = ix;
	y = iy;
	ASSERT(IsValid());
}

//}{===========================================================================================
// MATH OPERATORS

__forceinline int64 operator * (const Vec2i & u,const Vec2i & v) // dot
{
	ASSERT(u.IsValid() && v.IsValid());
	return ( int64(u.x) * int64(v.x) + int64(u.y) * int64(v.y) );
}

__forceinline const Vec2i operator * (const int f,const Vec2i & v) // scale
{
	ASSERT(v.IsValid());
	return Vec2i( f * v.x , f * v.y );
}

__forceinline const Vec2i operator * (const Vec2i & v,const int f) // scale
{
	ASSERT(v.IsValid());
	return Vec2i( f * v.x , f * v.y );
}

__forceinline const Vec2i operator + (const Vec2i & u,const Vec2i & v) // add
{
	ASSERT(u.IsValid() && v.IsValid());
	return Vec2i( u.x + v.x , u.y + v.y );
}

__forceinline const Vec2i operator - (const Vec2i & u,const Vec2i & v) // subtract
{
	ASSERT(u.IsValid() && v.IsValid());
	return Vec2i( u.x - v.x , u.y - v.y );
}

inline int64 DistanceSqr(const Vec2i & a,const Vec2i &b)
{
	ASSERT( a.IsValid() && b.IsValid() );
	int64 u = a.x - b.x;
	int64 v = a.y - b.y;
	return u*u + v*v;
}

// Area is *signed* based on handedness
// (this is actually triangle area TIMES TWO)
//  if "abc" is *COUNTER-CLOCKWISE* then Area is positive
int64 Area(const Vec2i & a,const Vec2i &b,const Vec2i &c);

// some simple predicates build on Area :

inline bool Colinear(const Vec2i & a,const Vec2i &b,const Vec2i &c)
{
	return Area(a,b,c) == 0;
}

inline bool ColinearEdges(const Vec2i & a,const Vec2i &b)
{
	return ( int64(a.x) * int64(b.y) == int64(a.y) * int64(b.x) );
}

// indicates if "c" is left of the segment "ab"
// same as asking of "abc" is counter-clockwise
inline bool Left(const Vec2i & a,const Vec2i &b,const Vec2i &c)
{
	return Area(a,b,c) > 0;
}

// Right indicates if "c" is left of the segment "ab"
// same as asking of "abc" is counter-clockwise
inline bool Right(const Vec2i & a,const Vec2i &b,const Vec2i &c)
{
	return Area(a,b,c) < 0;
}

// indicates if "c" is left or ON of the segment "ab"
inline bool LeftOrOn(const Vec2i & a,const Vec2i &b,const Vec2i &c)
{
	return Area(a,b,c) >= 0;
}

//}===========================================================================================

END_CB

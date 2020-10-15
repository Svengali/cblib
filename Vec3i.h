/*!
  \file
  \brief Vec3i : 3-Space Vector of INTS

	3-Space Vector
	public data
*/

#pragma once

#include "Base.h"
#include "Util.h"
#include "FloatUtil.h"

START_CB

//{===========================================================================================

//! 3-Space Vector
class Vec3i
{
public:
	//! Vec3i is just a bag of data;
	int x,y,z;

	//-------------------------------------------------------------------

	__forceinline Vec3i()
	{
		//! do-nada constructor; 
		// can't invalidate ints, so just put something ugly in
		DURING_ASSERT( x = 0xABADF00D );
		DURING_ASSERT( y = 0xABADF00D );
		DURING_ASSERT( z = 0xABADF00D );
	}
	
	// important : do NOT declare copy constructors which are the default !
	//	VC needs you to not declare them for it to do full return-value optimization!
	// default operator = is fine

	explicit __forceinline Vec3i(const int ix,const int iy,const int iz) : 
		x(ix), y(iy), z(iz)
	{
	}

	//-----------------------------------------
	//! common static vectors
	//!	use like Vec3i::zero
	
	static const Vec3i zero;
	static const Vec3i unitX;
	static const Vec3i unitY;
	static const Vec3i unitZ;
	static const Vec3i unitXneg;
	static const Vec3i unitYneg;
	static const Vec3i unitZneg;

	//-------------------------------------------------------------------
	// math operators on vectors

	void operator *= (const int scale);
	void operator += (const Vec3i & v);
	void operator -= (const Vec3i & v);
	bool operator == (const Vec3i & v) const; //!< exact equality test
	bool operator != (const Vec3i & v) const; //!< exact equality test

	//! array style access :
	int   operator [](const int c) const;
	int & operator [](const int c);
	
	//-------------------------------------------------------------------
	
	bool IsValid() const;
	bool IsSafeToSquare64() const;
	static bool ProductIsSafe64(const Vec3i & u,const Vec3i & v);
	static bool ProductIsSafe32(const Vec3i & u,const Vec3i & v);

	int64	LengthSqr() const;
	//double Length() const;
		
	//-------------------------------------------------------------------
	// Set mutators

	void Set(const int ix,const int iy,const int iz);

	// this = t * s
	void SetScaled(const Vec3i & t,const int s);
	// this = t x v
	void SetCross(const Vec3i & t,const Vec3i & v);
	//! this = min(this,v) ; component-wise
	void SetMin(const Vec3i &v);
	//! this = min(this,v) ; component-wise
	void SetMax(const Vec3i &v);
	//! this += t*v
	void AddScaled(const Vec3i &v,const int t);
	//! this = ca * a + cb * b
	void SetWeightedSum(const int ca,const Vec3i &a,const int cb,const Vec3i &b);
	//! this = a + cb * b
	void SetWeightedSum(const Vec3i &a,const int cb,const Vec3i &b);
	//! this.x *= a.x; etc.
	void ComponentwiseScale(const Vec3i &a);

	//-------------------------------------------------------------------
	// just IO as bytes

	void	Log() const; //!< writes xyz to Log; does NOT add a \n !

}; // end of class Vec3i

//}{===========================================================================================

inline bool Vec3i::IsValid() const
{
	//ASSERT( fisvalid(x) && fisvalid(y) && fisvalid(z) );
	return true;
}

//-------------------------------------------------------------------
// math operators on vectors

inline void Vec3i::operator *= (const int scale)
{
	ASSERT( IsValid() );
	x *= scale;
	y *= scale;
	z *= scale;
	ASSERT( IsValid() );
}

inline void Vec3i::operator += (const Vec3i & v)
{
	ASSERT( IsValid() && v.IsValid() );
	x += v.x;
	y += v.y;
	z += v.z;
	ASSERT( IsValid() );
}

inline void Vec3i::operator -= (const Vec3i & v)
{
	ASSERT( IsValid() && v.IsValid() );
	x -= v.x;
	y -= v.y;
	z -= v.z;
	ASSERT( IsValid() );
}

inline bool Vec3i::operator == (const Vec3i & v) const
{
	ASSERT( IsValid() && v.IsValid() );
	return (x == v.x &&
			y == v.y &&
			z == v.z);
}

inline bool Vec3i::operator != (const Vec3i & v) const
{
	ASSERT( IsValid() && v.IsValid() );
	return !operator==(v);
}

//! array style access :
inline int Vec3i::operator [](const int c) const
{
	ASSERT( c >= 0 && c < 3 );
	ASSERT( IsValid() );
	return (&x)[c];
}

inline int & Vec3i::operator [](const int c)
{
	ASSERT( c >= 0 && c < 3 );
	// may not be valid yet
	//ASSERT( IsValid() );
	return (&x)[c];
}

//-------------------------------------------------------------------

inline int64 Vec3i::LengthSqr() const
{
	ASSERT( IsSafeToSquare64() );
	return int64(x) * int64(x) + int64(y) * int64(y) + int64(z) * int64(z);
}

/*
inline double Vec3i::Length() const
{
	return sqrt( double( LengthSqr() ) );
}
*/

//-------------------------------------------------------------------
// Set mutators

inline void Vec3i::Set(const int ix,const int iy,const int iz)
{
	x = ix;
	y = iy;
	z = iz;
	ASSERT( IsValid() );
}

// this = t * s
inline void Vec3i::SetScaled(const Vec3i & t,const int s)
{
	ASSERT( t.IsValid() );
	x = t.x * s;
	y = t.y * s;
	z = t.z * s;
	ASSERT( IsValid() );
}

// this = t x v
inline void Vec3i::SetCross(const Vec3i & t,const Vec3i & v)
{
	ASSERT( &t != this && &v != this );
	ASSERT( t.IsValid() && v.IsValid() );
	x = t.y * v.z - t.z * v.y;
	y = t.z * v.x - t.x * v.z;
	z = t.x * v.y - t.y * v.x;
	ASSERT( IsValid() );
}

inline void Vec3i::SetMin(const Vec3i &v)
{
	ASSERT( v.IsValid() && IsValid() );
	x = MIN(x,v.x);
	y = MIN(y,v.y);
	z = MIN(z,v.z);
	ASSERT( IsValid() );
}
inline void Vec3i::SetMax(const Vec3i &v)
{
	ASSERT( v.IsValid() && IsValid() );
	x = MAX(x,v.x);
	y = MAX(y,v.y);
	z = MAX(z,v.z);
	ASSERT( IsValid() );
}

inline void Vec3i::AddScaled(const Vec3i &v,const int t)
{
	ASSERT( v.IsValid() && IsValid() );
	x += t * v.x;
	y += t * v.y;
	z += t * v.z;
	ASSERT( IsValid() );
}

inline void Vec3i::SetWeightedSum(const int ca,const Vec3i &a,const int cb,const Vec3i &b)
{
	ASSERT( a.IsValid() && b.IsValid() );
	x = ca * a.x + cb * b.x;
	y = ca * a.y + cb * b.y;
	z = ca * a.z + cb * b.z;
	ASSERT( IsValid() );
}

inline void Vec3i::SetWeightedSum(const Vec3i &a,const int cb,const Vec3i &b)
{
	ASSERT( a.IsValid() && b.IsValid() );
	x = a.x + cb * b.x;
	y = a.y + cb * b.y;
	z = a.z + cb * b.z;
	ASSERT( IsValid() );
}

inline void Vec3i::ComponentwiseScale(const Vec3i &a)
{
	ASSERT( a.IsValid() && IsValid() );
	x *= a.x;
	y *= a.y;
	z *= a.z;
	ASSERT( IsValid() );
}

//}{===========================================================================================
// Math operators
//
__forceinline int64 operator * (const Vec3i & u,const Vec3i & v) // dot
{
	ASSERT( Vec3i::ProductIsSafe64(u,v) );
	return ( int64(u.x) * int64(v.x) + int64(u.y) * int64(v.y) + int64(u.z) * int64(v.z) );
}

__forceinline const Vec3i operator * (const int f,const Vec3i & v) // scale
{
	ASSERT( v.IsValid() );
	return Vec3i( f * v.x , f * v.y , f * v.z );
}

__forceinline const Vec3i operator * (const Vec3i & v,const int f) // scale
{
	ASSERT( v.IsValid() );
	return Vec3i( f * v.x , f * v.y , f * v.z );
}

__forceinline const Vec3i operator + (const Vec3i & u,const Vec3i & v) // add
{
	ASSERT( u.IsValid() && v.IsValid() );
	return Vec3i( u.x + v.x , u.y + v.y , u.z + v.z );
}

__forceinline const Vec3i operator - (const Vec3i & u,const Vec3i & v) // subtract
{
	ASSERT( u.IsValid() && v.IsValid() );
	return Vec3i( u.x - v.x , u.y - v.y , u.z - v.z );
}

__forceinline const Vec3i operator ^ (const Vec3i & u,const Vec3i & v) // cross
{
	ASSERT( Vec3i::ProductIsSafe32(u,v) );
	return Vec3i( 
		u.y * v.z - u.z * v.y,
		u.z * v.x - u.x * v.z,
		u.x * v.y - u.y * v.x );
}

inline int64 DistanceSqr(const Vec3i & a,const Vec3i &b)
{
	ASSERT( Vec3i::ProductIsSafe64(a,b) );
	const int64 u = a.x - b.x;
	const int64 v = a.y - b.y;
	const int64 w = a.z - b.z;
	return u*u + v*v + w*w;
}

// AreaSqr return the square of (twice the area) of the triangle (a,b,c)
int64	AreaSqr( const Vec3i & a, const Vec3i & b, const Vec3i & c );
//  AreaSqr uses the *fourth* power of a coordinate, so it's not safe; use AreaSqrD
double	AreaSqrD( const Vec3i & a, const Vec3i & b, const Vec3i & c );

// if any of the three points are degenerate (like a == b) then Colinear returns true
bool	Colinear( const Vec3i & a, const Vec3i & b, const Vec3i & c );

// Volume is *signed* ; this is actually SIX times the real volume
// if "abc" is CCW , then right-hand rule determines the normal, and if "p" is on the front
//	side of triangle "abc", then Volume is positive
int64  Volume( const Vec3i & a, const Vec3i & b, const Vec3i & c, const Vec3i & p );
//double  VolumeD( const Vec3i & a, const Vec3i & b, const Vec3i & c, const Vec3i & p );

// VolumeSign returns 1,0,-1
//int  VolumeSign( const Vec3i & a, const Vec3i & b, const Vec3i & c, const Vec3i & p );

//}===========================================================================================

// very limited 64-bit int vec :

struct Vec3i64
{
	int64 x,y,z;
	
	//-------------------------------------------------------------------

	__forceinline Vec3i64()
	{
		//! do-nada constructor; 
		// can't invalidate ints, so just put something ugly in
		DURING_ASSERT( x = 0xABADF00D );
		DURING_ASSERT( y = 0xABADF00D );
		DURING_ASSERT( z = 0xABADF00D );
	}
	
	// important : do NOT declare copy constructors which are the default !
	//	VC needs you to not declare them for it to do full return-value optimization!
	// default operator = is fine

	explicit __forceinline Vec3i64(const int ix,const int iy,const int iz) : 
		x(ix), y(iy), z(iz)
	{
	}

	//-----------------------------------------
};

__forceinline void SetCross(Vec3i64 * pVec, const Vec3i & u, const Vec3i & v)
{
	pVec->x = int64(u.y) * v.z - int64(u.z) * v.y;
	pVec->y = int64(u.z) * v.x - int64(u.x) * v.z,
	pVec->z = int64(u.x) * v.y - int64(u.y) * v.x;
}

__forceinline void SetTriangleCross(Vec3i64 * pVec, const Vec3i & a, const Vec3i & b, const Vec3i & c)
{
	const Vec3i e1 = b - a;
	const Vec3i e2 = c - a;
	
	SetCross(pVec,e1,e2);
}

//}===========================================================================================

END_CB

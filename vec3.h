/*!
  \file
  \brief 3-Space Vector

	3-Space Vector
	public data
*/

#pragma once

#include "cblib/Base.h"
#include "cblib/Util.h"
#include "cblib/FloatUtil.h"

START_CB

//{===========================================================================================

//! 3-Space Vector
class Vec3
{
public:
	//! Vec3 is just a bag of data;
	float x,y,z;

	//-------------------------------------------------------------------

	__forceinline Vec3()
	{
		//! do-nada constructor; 
		finvalidatedbg(x);
		finvalidatedbg(y);
		finvalidatedbg(z);
	}
	
	// important : do NOT declare copy constructors which are the default !
	//	VC needs you to not declare them for it to do full return-value optimization!
	// default operator = is fine

	__forceinline Vec3(const float ix,const float iy,const float iz) : 
		x(ix), y(iy), z(iz)
	{
	}
	
	enum EConstructorNormalized { eNormalize };
	Vec3(const EConstructorNormalized e,const float ix,const float iy,const float iz);
	Vec3(const EConstructorNormalized e,const Vec3 & other);

	//-----------------------------------------
	//! common static vectors
	//!	use like Vec3::zero
	
	static const Vec3 zero;
	static const Vec3 unitX;
	static const Vec3 unitY;
	static const Vec3 unitZ;
	static const Vec3 unitXneg;
	static const Vec3 unitYneg;
	static const Vec3 unitZneg;

	//-------------------------------------------------------------------
	// math operators on vectors

	void operator *= (const float scale);
	void operator /= (const float scale);
	void operator += (const Vec3 & v);
	void operator -= (const Vec3 & v);
	bool operator == (const Vec3 & v) const; //!< exact equality test
	bool operator != (const Vec3 & v) const; //!< exact equality test

	//! array style access :
	float   operator [](const int c) const;
	float & operator [](const int c);
	
	//-------------------------------------------------------------------
	
	bool IsValid() const;
	bool IsNormalized(const float tolerance = EPSILON) const;

	float LengthSqr() const;
	float Length() const;
	
	float LengthSqrXY() const;
	float LengthXY() const;

	void NormalizeFast(); // about 110 clocks

	//! NormalizeSafe :
	//!	returns the length
	/*! if length is less than EPSILON, returns fallback and length of 0 */
	float NormalizeSafe(const Vec3 & fallback = unitZ) ;

	//! this = -this
	void Invert();
	
	//-------------------------------------------------------------------
	// Set mutators

	void Set(const float ix,const float iy,const float iz);

	// this = t * s
	void SetScaled(const Vec3 & t,const float s);
	// this = t x v
	void SetCross(const Vec3 & t,const Vec3 & v);
	//! this = (v1 + v2)/2
	void SetAverage(const Vec3 &v1,const Vec3 &v2);
	//! this = v1 * (1-t) + v2 * t;
	void SetLerp(const Vec3 &v1,const Vec3 &v2,const float t);
	//! this = min(this,v) ; component-wise
	void SetMin(const Vec3 &v);
	//! this = min(this,v) ; component-wise
	void SetMax(const Vec3 &v);
	//! this += t*v
	void AddScaled(const Vec3 &v,const float t);
	//! this = ca * a + cb * b
	void SetWeightedSum(const float ca,const Vec3 &a,const float cb,const Vec3 &b);
	//! this = a + cb * b
	void SetWeightedSum(const Vec3 &a,const float cb,const Vec3 &b);
	//! this.x *= a.x; etc.
	void ComponentwiseScale(const Vec3 &a);

	//-------------------------------------------------------------------
	
	//! fuzzy equality test
	static bool Equals(const Vec3 &a,const Vec3 &b,const float tolerance = EPSILON);

	//-------------------------------------------------------------------
	// just IO as bytes

	void	Log() const; //!< writes xyz to Log; does NOT add a \n !

}; // end of class Vec3

//}{===========================================================================================

inline Vec3::Vec3(const EConstructorNormalized e,const float ix,const float iy,const float iz)
{
	const float lengthSqr = ix*ix + iy*iy + iz*iz;
	ASSERT( lengthSqr > 0.f );
	const float scale = ReciprocalSqrt(lengthSqr);
	x = ix * scale;
	y = iy * scale;
	z = iz * scale;	
}

inline Vec3::Vec3(const EConstructorNormalized e,const Vec3 & other)
{
	const float lengthSqr = other.LengthSqr();
	ASSERT( lengthSqr > 0.f );
	const float scale = ReciprocalSqrt(lengthSqr);
	x = other.x * scale;
	y = other.y * scale;
	z = other.z * scale;
}
	
inline bool Vec3::IsValid() const
{
	ASSERT_LOW( fisvalid(x) && fisvalid(y) && fisvalid(z) );
	return true;
}

//-------------------------------------------------------------------
// math operators on vectors

inline void Vec3::operator *= (const float scale)
{
	x *= scale;
	y *= scale;
	z *= scale;
}

inline void Vec3::operator /= (const float scale)
{
	ASSERT( scale != 0.f );
	const float inv = 1.f / scale;
	x *= inv;
	y *= inv;
	z *= inv;
}

inline void Vec3::operator += (const Vec3 & v)
{
	x += v.x;
	y += v.y;
	z += v.z;
}

inline void Vec3::operator -= (const Vec3 & v)
{
	x -= v.x;
	y -= v.y;
	z -= v.z;
}

inline bool Vec3::operator == (const Vec3 & v) const
{
	return (x == v.x &&
			y == v.y &&
			z == v.z);
}

inline bool Vec3::operator != (const Vec3 & v) const
{
	return !operator==(v);
}

//! array style access :
inline float Vec3::operator [](const int c) const
{
	ASSERT( c >= 0 && c < 3 );
	return (&x)[c];
}

inline float & Vec3::operator [](const int c)
{
	ASSERT( c >= 0 && c < 3 );
	return (&x)[c];
}

//-------------------------------------------------------------------

inline float Vec3::LengthSqr() const
{
	return x * x + y * y + z * z;
}

inline float Vec3::Length() const
{
	return sqrtf( LengthSqr() );
}

inline float Vec3::LengthSqrXY() const
{
	return x * x + y * y;
}

inline float Vec3::LengthXY() const
{
	return sqrtf( LengthSqrXY() );
}


// uses fast SSE reciprocal sqrt
//	does NOT return the length because of that	
// about 110 clocks
//	this is just as fast as the SSE 4-vector normalize ! 
inline void Vec3::NormalizeFast()
{
	const float lengthSqr = LengthSqr();
	const float recipSqrt = ReciprocalSqrt(lengthSqr);
	(*this) *= recipSqrt;
	
	ASSERT(IsNormalized());
}

//! NormalizeSafe : about 140 clocks
//!	returns the length
//! if length is less than EPSILON, returns fallback and length of 0
inline float Vec3::NormalizeSafe(const Vec3 & fallback /*= unitZ*/) 
{
	const float length = Length();
	if ( length < EPSILON )
	{
		*this = fallback;
//		ASSERT(IsNormalized());
		return 0.f;
	}
	else
	{
		(*this) *= 1.f / length;
		ASSERT(IsNormalized());
		return length;
	}
}

inline bool Vec3::IsNormalized(const float tolerance /*= EPSILON*/) const
{
	// @@ part of the EPSILON disaster
	return fisone( LengthSqr(), /*fsquare*/ (tolerance) );
}

inline void Vec3::Invert()
{
	x = -x;
	y = -y;
	z = -z;
}

//-------------------------------------------------------------------
// Set mutators

inline void Vec3::Set(const float ix,const float iy,const float iz)
{
	x = ix;
	y = iy;
	z = iz;
}

// this = t * s
inline void Vec3::SetScaled(const Vec3 & t,const float s)
{
	x = t.x * s;
	y = t.y * s;
	z = t.z * s;
}

// this = t x v
inline void Vec3::SetCross(const Vec3 & t,const Vec3 & v)
{
	ASSERT( &t != this && &v != this );
	x = t.y * v.z - t.z * v.y;
	y = t.z * v.x - t.x * v.z;
	z = t.x * v.y - t.y * v.x;
}

inline void Vec3::SetAverage(const Vec3 &v1,const Vec3 &v2)
{
	x = 0.5f * (v2.x + v1.x);
	y = 0.5f * (v2.y + v1.y);
	z = 0.5f * (v2.z + v1.z);
}

inline void Vec3::SetLerp(const Vec3 &v1,const Vec3 &v2,const float t)
{
	// doing v1 * (1-t) + v2 * t has better float accuracy than v1 + t *(v2 - v1)
	const float s = 1.f - t;
	x = v1.x * s + t * v2.x;
	y = v1.y * s + t * v2.y;
	z = v1.z * s + t * v2.z;
}

inline void Vec3::SetMin(const Vec3 &v)
{
	x = MIN(x,v.x);
	y = MIN(y,v.y);
	z = MIN(z,v.z);
}
inline void Vec3::SetMax(const Vec3 &v)
{
	x = MAX(x,v.x);
	y = MAX(y,v.y);
	z = MAX(z,v.z);
}

inline void Vec3::AddScaled(const Vec3 &v,const float t)
{
	x += t * v.x;
	y += t * v.y;
	z += t * v.z;
}

inline void Vec3::SetWeightedSum(const float ca,const Vec3 &a,const float cb,const Vec3 &b)
{
	x = ca * a.x + cb * b.x;
	y = ca * a.y + cb * b.y;
	z = ca * a.z + cb * b.z;
}

inline void Vec3::SetWeightedSum(const Vec3 &a,const float cb,const Vec3 &b)
{
	x = a.x + cb * b.x;
	y = a.y + cb * b.y;
	z = a.z + cb * b.z;
}

inline void Vec3::ComponentwiseScale(const Vec3 &a)
{
	x *= a.x;
	y *= a.y;
	z *= a.z;
}

//-------------------------------------------------------------------

inline /*static*/ bool Vec3::Equals(const Vec3 &a,const Vec3 &b,const float tolerance /*= EPSILON*/)
{
	return fequal(a.x,b.x,tolerance) && fequal(a.y,b.y,tolerance) && fequal(a.z,b.z,tolerance);
}

//}{===========================================================================================
// Math operators
//
// one of the many dangerous things about binary operators is
//	order of evaluation.  Check this out :
//		float = - vec1 * vec2
// what does that do?  Actually, in C++, it invokes unary operator-
//	which makes a temporary vector and then does the dot (rather than
//	do the dot and then invoke operator - on a float).  ugly.  Of course
//	this is even much worse with things like :
//		vec = - Mat * Mat * vec
//	This line could be done like this :
//		vec = - ( Mat * (Mat * vec) )
//	with ideal efficiency
//	but standard C++ will evaluate like this :
//		vec = ((- Mat) * Mat) * vec
//	with way-below-best efficiency

//Dot product.
__forceinline float operator * (const Vec3 & u,const Vec3 & v) // dot
{
	return ( u.x * v.x + u.y * v.y + u.z * v.z );
}

__forceinline const Vec3 operator - (const Vec3 & v) // negative
{
	return Vec3( - v.x , - v.y , - v.z );
}

__forceinline const Vec3 operator * (const float f,const Vec3 & v) // scale
{
	return Vec3( f * v.x , f * v.y , f * v.z );
}

__forceinline const Vec3 operator * (const Vec3 & v,const float f) // scale
{
	return Vec3( f * v.x , f * v.y , f * v.z );
}

//Cross product.
__forceinline const Vec3 operator ^ (const Vec3 & u,const Vec3 & v)
{
	return Vec3( 
		u.y * v.z - u.z * v.y,
		u.z * v.x - u.x * v.z,
		u.x * v.y - u.y * v.x );
}

__forceinline const Vec3 operator / (const Vec3 & v,const float f) // scale
{
	ASSERT( f != 0.f );
	const float inv = 1.f / f;
	return Vec3( inv * v.x , inv * v.y , inv * v.z );
}

__forceinline const Vec3 operator + (const Vec3 & u,const Vec3 & v) // add
{
	return Vec3( u.x + v.x , u.y + v.y , u.z + v.z );
}

__forceinline const Vec3 operator - (const Vec3 & u,const Vec3 & v) // subtract
{
	return Vec3( u.x - v.x , u.y - v.y , u.z - v.z );
}

//}===========================================================================================

END_CB

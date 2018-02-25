/*!
  \file
  \brief Vec2 : 2-Space Vector
	Vec2 is just a bag of data;
*/

#pragma once

#include "cblib/Base.h"
#include "cblib/FloatUtil.h"

START_CB

//{===========================================================================================

class Vec2
{
public:
	//! Vec2 is just a bag of data;
	float x,y;

	//-------------------------------------------------------------------

	__forceinline Vec2()
	{
		// do-nada constructor; 
		finvalidatedbg(x);
		finvalidatedbg(y);
	}
	
	// important : do NOT declare copy constructors which are the default !
	//	VC needs you to not declare them for it to do full return-value optimization!
	// default operator = is fine

	explicit __forceinline Vec2(const float ix,const float iy) : 
		x(ix), y(iy)
	{
	}

	enum EConstructorNormalized { eNormalize };
	Vec2(const EConstructorNormalized e,const float ix,const float iy);
	Vec2(const EConstructorNormalized e,const Vec2 & other);

	//-----------------------------------------
	// common static vectors
	//	use like Vec2::zero
	
	static const Vec2 zero;
	static const Vec2 unitX;
	static const Vec2 unitY;
	static const Vec2 unitXneg;
	static const Vec2 unitYneg;

	//-------------------------------------------------------------------
	// math operators on vectors

	void operator *= (const float scale);
	void operator /= (const float scale);
	void operator += (const Vec2 & v);
	void operator -= (const Vec2 & v);
	bool operator == (const Vec2 & v) const; //!< exact equality
	bool operator != (const Vec2 & v) const; //!< exact equality

	// array style access :
	float	operator [](const int c) const;
	float & operator [](const int c);
	
	//-------------------------------------------------------------------
	
	bool IsValid() const;
	bool IsNormalized(const float tolerance = EPSILON) const;

	float LengthSqr() const;
	float Length() const;

	void Set(const float ix,const float iy);
	void SetLerp(const Vec2 &a,const Vec2 &b, const float t);
	void SetAverage(const Vec2 &v1,const Vec2 &v2);
	void SetMin(const Vec2 &v);
	void SetMax(const Vec2 &v);

	void NormalizeFast();
	
	//! NormalizeSafe :
	//!	returns the length
	float NormalizeSafe(const Vec2 & fallback = unitX); //!< if length is less than tiny, returns fallback and length of 0

	//-------------------------------------------------------------------
	
	//! fuzzy equality
	static bool Equals(const Vec2 &a,const Vec2 &b,const float tolerance = EPSILON);

	//-------------------------------------------------------------------
}; // end of class Vec2

//}{===========================================================================================
// INLINE FUNCTIONS

inline Vec2::Vec2(const EConstructorNormalized,const float ix,const float iy)
{
	const float lengthSqr = ix*ix + iy*iy;
	ASSERT( lengthSqr > 0.f );
	const float scale = 1.f / sqrtf(lengthSqr);
	x = ix * scale;
	y = iy * scale;
}

inline Vec2::Vec2(const EConstructorNormalized,const Vec2 & other)
{
	const float lengthSqr = other.LengthSqr();
	ASSERT( lengthSqr > 0.f );
	const float scale = 1.f / sqrtf(lengthSqr);
	x = other.x * scale;
	y = other.y * scale;
}

//-------------------------------------------------------------------
// math operators on vectors

inline void Vec2::operator *= (const float scale)
{
	x *= scale;
	y *= scale;
}

inline void Vec2::operator /= (const float scale)
{
	ASSERT( scale != 0.f );
	const float inv = 1.f / scale;
	x *= inv;
	y *= inv;
}

inline void Vec2::operator += (const Vec2 & v)
{
	x += v.x;
	y += v.y;
}

inline void Vec2::operator -= (const Vec2 & v)
{
	x -= v.x;
	y -= v.y;
}

inline bool Vec2::operator == (const Vec2 & v) const
{
	return (x == v.x &&
			y == v.y );
}

inline bool Vec2::operator != (const Vec2 & v) const
{
	return !operator==(v);
}

// array style access :
inline float Vec2::operator [](const int c) const
{
	ASSERT( c >= 0 && c < 2 );
	return (&x)[c];
}

inline float & Vec2::operator [](const int c)
{
	ASSERT( c >= 0 && c < 2 );
	// may not be valid yet
	return (&x)[c];
}

//-------------------------------------------------------------------

inline bool Vec2::IsValid() const
{
	ASSERT( fisvalid(x) && fisvalid(y) );
	return true;
}

inline float Vec2::LengthSqr() const
{
	return x * x + y * y;
}

inline float Vec2::Length() const
{
	return sqrtf( LengthSqr() );
}

inline void Vec2::Set(const float ix,const float iy)
{
	x = ix;
	y = iy;
}

inline void Vec2::SetLerp(const Vec2 &a,const Vec2 &b, const float t)
{
	x = flerp(a.x,b.x,t);
	y = flerp(a.y,b.y,t);
}

inline void Vec2::SetAverage(const Vec2 &v1,const Vec2 &v2)
{
	// gWarnING : may produce different results than SetLerp(0.5) due to floating round-off
	x = 0.5f * (v2.x + v1.x);
	y = 0.5f * (v2.y + v1.y);
}

inline void Vec2::SetMin(const Vec2 &v)
{
	x = Min(x,v.x);
	y = Min(y,v.y);
}
inline void Vec2::SetMax(const Vec2 &v)
{
	x = Max(x,v.x);
	y = Max(y,v.y);
}

inline void Vec2::NormalizeFast()
{
	const float lengthSqr = LengthSqr();
	ASSERT( lengthSqr > 0.f );
	(*this) *= 1.f / sqrtf(lengthSqr);
	ASSERT(IsNormalized());
}

// NormalizeSafe :
//	returns the length
inline float Vec2::NormalizeSafe(const Vec2 & fallback /*= unitX*/) //	if length is less than tiny, returns fallback and length of 0
{
	const float length = Length();
	if ( length < EPSILON )
	{
		*this = fallback;
		ASSERT(IsNormalized());
		return 0.f;
	}
	else
	{
		(*this) *= 1.f / length;
		ASSERT(IsNormalized());
		return length;
	}
}

inline bool Vec2::IsNormalized(const float tolerance /*= EPSILON*/) const
{
	// @@ part of the EPSILON disaster
	return fisone( LengthSqr(), /*fsquare*/ (tolerance) );
}

//-------------------------------------------------------------------

inline /*static*/ bool Vec2::Equals(const Vec2 &a,const Vec2 &b,const float tolerance /*= EPSILON*/)
{
	return fequal(a.x,b.x,tolerance) && fequal(a.y,b.y,tolerance);
}

//}{===========================================================================================
// MATH OPERATORS

//----------------------------------------------------------------------------------

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

__forceinline float operator * (const Vec2 & u,const Vec2 & v) // dot
{
	return ( u.x * v.x + u.y * v.y );
}

__forceinline const Vec2 operator - (const Vec2 & v) // negative
{
	return Vec2( - v.x , - v.y );
}

__forceinline const Vec2 operator * (const float f,const Vec2 & v) // scale
{
	return Vec2( f * v.x , f * v.y );
}

__forceinline const Vec2 operator * (const Vec2 & v,const float f) // scale
{
	return Vec2( f * v.x , f * v.y );
}

__forceinline const Vec2 operator / (const Vec2 & v,const float f) // scale
{
	ASSERT( f != 0.f );
	const float inv = 1.f / f;
	return Vec2( inv * v.x , inv * v.y );
}

__forceinline const Vec2 operator + (const Vec2 & u,const Vec2 & v) // add
{
	return Vec2( u.x + v.x , u.y + v.y );
}

__forceinline const Vec2 operator - (const Vec2 & u,const Vec2 & v) // subtract
{
	return Vec2( u.x - v.x , u.y - v.y );
}

//}===========================================================================================

END_CB

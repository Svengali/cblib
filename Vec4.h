#pragma once

#include "cblib/Base.h"
#include "cblib/FloatUtil.h"
#include "cblib/Vec3.h"

//{===========================================================================================

START_CB

//! 4-Space Vector
class Vec4
{
public:

	//! Vec4 is just a bag of data;
	float x,y,z,w;

	//-------------------------------------------------------------------

	__forceinline Vec4()
	{
		//! do-nada constructor; 
		finvalidatedbg(x);
		finvalidatedbg(y);
		finvalidatedbg(z);
		finvalidatedbg(w);
	}
	
	// important : do NOT declare copy constructors which are the default !
	//	VC needs you to not declare them for it to do full return-value optimization!
	// default operator = is fine

	explicit __forceinline Vec4(const float ix,const float iy,const float iz,const float iw) : 
		x(ix), y(iy), z(iz), w(iw)
	{
	}
	explicit __forceinline Vec4(const Vec3 &v,const float iw) : 
		x(v.x), y(v.y), z(v.z), w(iw)
	{
	}

	//-----------------------------------------
	//! common static Vectors
	//!	use like Vec4::zero
	
	static const Vec4 zero;
	static const Vec4 unitX;
	static const Vec4 unitY;
	static const Vec4 unitZ;
	static const Vec4 unitXneg;
	static const Vec4 unitYneg;
	static const Vec4 unitZneg;

	//-------------------------------------------------------------------
	// math operators on Vectors

	void operator *= (const float scale);
	void operator /= (const float scale);
	void operator += (const Vec4 & v);
	void operator -= (const Vec4 & v);
	bool operator == (const Vec4 & v) const; //!< exact equality test
	bool operator != (const Vec4 & v) const; //!< exact equality test

	//! array style access :
	float   operator [](const int c) const;
	float & operator [](const int c);
	
	//-------------------------------------------------------------------
	
	bool IsValid() const;
	bool IsNormalized(const float tolerance = EPSILON) const;

	float LengthSqr() const;
	float Length() const;
	
	void NormalizeFast(); // about 110 clocks

	//! NormalizeSafe :
	//!	returns the length
	/*! if length is less than EPSILON, returns fallback and length of 0 */
	float NormalizeSafe(const Vec4 & fallback = unitZ,const float tolerance = EPSILON) ;

	//! this = -this
	void Invert();
	
	//! get the Vec3 part of this Vec4
	const Vec3 & GetVec3() const		{ return *((const Vec3 *)this); }
	Vec3 &		MutableVec3()			{ return *((Vec3 *)this); }
	void		SetVec3(const Vec3 &v)	{ MutableVec3() = v; }

	// extend three-vec with a 1 in the homogenous "w" and then do the dot
	float DotExtendOne(const Vec3 & v) const;

	static float Distance(const Vec4 & a,const Vec4 &b);
	static float DistanceSqr(const Vec4 & a,const Vec4 &b);

	//-------------------------------------------------------------------
	// Set mutators

	void Set(const float ix,const float iy,const float iz,const float iw);
	void Set(const Vec3 & v,const float iw);

	//! this = (v1 + v2)/2
	void SetAverage(const Vec4 &v1,const Vec4 &v2);
	//! this = v1 * (1-t) + v2 * t;
	void SetLerp(const Vec4 &v1,const Vec4 &v2,const float t);
	//! this = min(this,v) ; component-wise
	void SetMin(const Vec4 &v);
	//! this = min(this,v) ; component-wise
	void SetMax(const Vec4 &v);
	//! this += t*v
	void AddScaled(const Vec4 &v,const float t);
	//! this = ca * a + cb * b
	void SetWeightedSum(const float ca,const Vec4 &a,const float cb,const Vec4 &b);
	//! this = a + cb * b
	void SetWeightedSum(const Vec4 &a,const float cb,const Vec4 &b);
	//! this.x *= a.x; etc.
	void ComponentwiseScale(const Vec4 &a);

	//-------------------------------------------------------------------
	
	//! fuzzy equality test
	static bool Equals(const Vec4 &a,const Vec4 &b,const float tolerance = EPSILON);

}; // end of class Vec4

//}{===========================================================================================

inline bool Vec4::IsValid() const
{
	ASSERT( fisvalid(x) && fisvalid(y) && fisvalid(z) && fisvalid(w) );
	return true;
}

//-------------------------------------------------------------------
// math operators on Vectors

inline void Vec4::operator *= (const float scale)
{
	x *= scale;
	y *= scale;
	z *= scale;
	w *= scale;
}

inline void Vec4::operator /= (const float scale)
{
	ASSERT( scale != 0.f );
	const float inv = 1.f / scale;
	x *= inv;
	y *= inv;
	z *= inv;
	w *= inv;
}

inline void Vec4::operator += (const Vec4 & v)
{
	x += v.x;
	y += v.y;
	z += v.z;
	w += v.w;
}

inline void Vec4::operator -= (const Vec4 & v)
{
	x -= v.x;
	y -= v.y;
	z -= v.z;
	w -= v.w;
}

inline bool Vec4::operator == (const Vec4 & v) const
{
	return (x == v.x &&
			y == v.y &&
			z == v.z &&
			w == v.w);
}

inline bool Vec4::operator != (const Vec4 & v) const
{
	return !operator==(v);
}

//! array style access :
inline float Vec4::operator [](const int c) const
{
	ASSERT( c >= 0 && c < 4 );
	return (&x)[c];
}

inline float & Vec4::operator [](const int c)
{
	ASSERT( c >= 0 && c < 4 );
	// may not be valid yet
	return (&x)[c];
}

//-------------------------------------------------------------------

// extend three-vec with a 1 in the homogenous "w" and then do the dot
inline float Vec4::DotExtendOne(const Vec3 & v) const
{
	return GetVec3() * v + w;
}

inline float Vec4::LengthSqr() const
{
	return x * x + y * y + z * z + w * w;
}

inline float Vec4::Length() const
{
	return sqrtf( LengthSqr() );
}

inline void Vec4::NormalizeFast() // about 110 clocks
{
	// @@ could use fast SSE reciprocal sqrt
	//	does NOT return the length because of that
	const float lengthSqr = LengthSqr();
	ASSERT( lengthSqr > 0.f );
	(*this) *= 1.f / sqrtf(lengthSqr);
	ASSERT(IsNormalized());
}

//! NormalizeSafe :
//!	returns the length
/*! if length is less than EPSILON, returns fallback and length of 0 */
inline float Vec4::NormalizeSafe(const Vec4 & fallback /*= unitZ*/,const float tolerance /*= EPSILON*/) 
{
	const float length = Length();
	if ( length < tolerance )
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

inline bool Vec4::IsNormalized(const float tolerance /*= EPSILON*/) const
{
	// @@ part of the EPSILON disaster
	return fisone( LengthSqr(), /*fsquare*/ (tolerance) );
}

inline void Vec4::Invert()
{
	x = -x;
	y = -y;
	z = -z;
	w = -w;
}

//-------------------------------------------------------------------
// Set mutators

inline void Vec4::Set(const float ix,const float iy,const float iz,const float iw)
{
	x = ix;
	y = iy;
	z = iz;
	w = iw;
}
inline void Vec4::Set(const Vec3 & v,const float iw)
{
	SetVec3(v);
	w = iw;
}

inline void Vec4::SetAverage(const Vec4 &v1,const Vec4 &v2)
{
	// WARNING : may produce different results than SetLerp(0.5) due to floating round-off
	x = 0.5f * (v2.x + v1.x);
	y = 0.5f * (v2.y + v1.y);
	z = 0.5f * (v2.z + v1.z);
	w = 0.5f * (v2.w + v1.w);
}

inline void Vec4::SetLerp(const Vec4 &v1,const Vec4 &v2,const float t)
{
	x = v1.x + t * (v2.x - v1.x);
	y = v1.y + t * (v2.y - v1.y);
	z = v1.z + t * (v2.z - v1.z);
	w = v1.w + t * (v2.w - v1.w);
}

inline void Vec4::SetMin(const Vec4 &v)
{
	x = Min(x,v.x);
	y = Min(y,v.y);
	z = Min(z,v.z);
	w = Min(w,v.w);
}
inline void Vec4::SetMax(const Vec4 &v)
{
	x = Max(x,v.x);
	y = Max(y,v.y);
	z = Max(z,v.z);
	w = Max(w,v.w);
}

inline void Vec4::AddScaled(const Vec4 &v,const float t)
{
	x += t * v.x;
	y += t * v.y;
	z += t * v.z;
	w += t * v.w;
}

inline void Vec4::SetWeightedSum(const float ca,const Vec4 &a,const float cb,const Vec4 &b)
{
	x = ca * a.x + cb * b.x;
	y = ca * a.y + cb * b.y;
	z = ca * a.z + cb * b.z;
	w = ca * a.w + cb * b.w;
}

inline void Vec4::SetWeightedSum(const Vec4 &a,const float cb,const Vec4 &b)
{
	x = a.x + cb * b.x;
	y = a.y + cb * b.y;
	z = a.z + cb * b.z;
	w = a.w + cb * b.w;
}

inline void Vec4::ComponentwiseScale(const Vec4 &a)
{
	x *= a.x;
	y *= a.y;
	z *= a.z;
	w *= a.w;
}

//-------------------------------------------------------------------

inline /*static*/ bool Vec4::Equals(const Vec4 &a,const Vec4 &b,const float tolerance /*= EPSILON*/)
{
	return fequal(a.x,b.x,tolerance) && fequal(a.y,b.y,tolerance) && fequal(a.z,b.z,tolerance) && fequal(a.w,b.w,tolerance);
}

//}{===========================================================================================
// Math operators
//
// one of the many dangerous things about binary operators is
//	order of evaluation.  Check this out :
//		float = - vec1 * Vec2
// what does that do?  Actually, in C++, it invokes unary operator-
//	which makes a temporary Vector and then does the dot (rather than
//	do the dot and then invoke operator - on a float).  ugly.  Of course
//	this is even much worse with things like :
//		vec = - Mat * Mat * vec
//	This line could be done like this :
//		vec = - ( Mat * (Mat * vec) )
//	with ideal efficiency
//	but standard C++ will evaluate like this :
//		vec = ((- Mat) * Mat) * vec
//	with way-below-best efficiency

__forceinline float operator * (const Vec4 & u,const Vec4 & v) // dot
{
	return ( u.x * v.x + u.y * v.y + u.z * v.z + u.w * v.w );
}

__forceinline const Vec4 operator * (const float f,const Vec4 & v) // scale
{
	return Vec4( f * v.x , f * v.y , f * v.z , f * v.w );
}

__forceinline const Vec4 operator * (const Vec4 & v,const float f) // scale
{
	return Vec4( f * v.x , f * v.y , f * v.z , f * v.w );
}

__forceinline const Vec4 operator / (const Vec4 & v,const float f) // scale
{
	ASSERT( f != 0.f );
	const float inv = 1.f / f;
	return Vec4( inv * v.x , inv * v.y , inv * v.z , inv * v.w );
}

__forceinline const Vec4 operator + (const Vec4 & u,const Vec4 & v) // add
{
	return Vec4( u.x + v.x , u.y + v.y , u.z + v.z , u.w + v.w );
}

__forceinline const Vec4 operator - (const Vec4 & u,const Vec4 & v) // subtract
{
	return Vec4( u.x - v.x , u.y - v.y , u.z - v.z , u.w - v.w );
}

inline float Vec4::DistanceSqr(const Vec4 & a,const Vec4 &b)
{
	return fsquare(a.x - b.x) + fsquare(a.y - b.y) + fsquare(a.z - b.z) + fsquare(a.w - b.w);
}

inline float Vec4::Distance(const Vec4 & a,const Vec4 &b)
{
	return sqrtf( Vec4::DistanceSqr(a,b) );
}

//}===========================================================================================

END_CB

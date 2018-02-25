/*!
  \file
  \brief Quat : a Quaternion = a rotation representation
*/

#pragma once

#include "cblib/Vec3.h"

START_CB

class Mat3;

//{=======================================================================

//-------------------------------------------------
//! Quaternion
/*!	
	quat is somewhat more private than a Vector
	(the members are private, for example)
	since almost all quaternion manipulation is
	 expected to go through these functions
	(eg. you should not be mucking with the parameters
	 individually)

	 Quat is a "leaf" class : you may make it on the
		stack, take sizeof() and compare and write its bytes
		it has no virtuals !

	 Q and -Q represent the same rotation
	 a Rotation is 3 continuous degrees of freedom (two angles, one magnitude), mudolo one bit
	 we see this in the quat because we have four floats,
		one of which is fully determined by normalization,
	  and the net sign of the quaternion gives a double-cover of
		the rotation group

	Q is :
		{ axis * sin(angle/2) , cos(angle/2) }

	negating m_v OR m_w flips the rotation to -angle

	{x,y,z,w}
	{0,0,0,1} and {0,0,0,-1} are both the identity

	{1,0,0,0} is a 180 degree rotation around X

-------------------------------------------------*/

class Quat
{
public:

	//-------------------------------------------------
	//! Constructors

	inline  Quat()
	{
		finvalidatedbg(m_w);
		// m_v invalidated by its constructor
	}

	//! the identity quat is "no rotation", just like a matrix
	enum EConstructorIdentity { eIdentity };
	explicit inline  Quat(EConstructorIdentity) : m_v(0,0,0) , m_w(1)
	{
		// identity
	}

	//! make a quat from its parts
	explicit inline  Quat(const Vec3 & v, const float w) : m_v(v) , m_w(w)
	{
	}

	//! make a quat from its parts
	explicit inline  Quat(const float x,const float y,const float z, const float w) : m_v(x,y,z) , m_w(w)
	{
	}

	enum EConstructorAxisAngle { eAxisAngle };
	explicit inline  Quat(EConstructorAxisAngle,const Vec3 & axis, const float angle)
	{
		SetFromAxisAngle(axis,angle);
	}

	// important : do NOT declare copy constructors which are the default !
	//	VC needs you to not declare them for it to do full return-value optimization!
	// default operator = is fine

	static const Quat identity;

	//-------------------------------------------------

	bool IsValid() const;

	//! quats should almost always be normalized
	//!	IsNormalized not included in IsValid because they can
	//!	go temporarily un-norm'ed during manipulations
	bool IsNormalized(const float tolerance = EPSILON) const;

	float Normalize();

	float Length() const;
	float LengthSqr() const;

	//! SetNegativePartner makes the negative quat Q
	//!	which represents the same rotation,
	//!  but will act differently under linear
	//!	manipulations (such as Lerp or Add)
	void SetNegativePartner();

	//! Invert makes a quat Q^(-1) such that
	//!	Q^(-1) Q = identity	
	void Invert();

	//! InvertNormalized is a faster version of Invert
	//!	 which only works on Normalized quats (ASSERTs)
	void InvertNormalized();

	void SetIdentity();

	void SetComponents(const Vec3 &v,const float w);
	void SetComponents(const float i,const float j,const float k,const float w);

	//! Component index from 0 to 3
	float GetComponent(const int i) const;
	void SetComponent(const int i,const float f);

	const Vec3 & GetVector() const;
	float GetScalar() const;

	//! GetAngle is Expensive !!
	//!  only works on a normalized quat
	//!	in [0,2pi]
	float GetAngle() const;

	//! GetAngleMod2Pi in [0,pi] , returns how
	//!	 many radians *taking the shorter path*
	float GetAngleMod2Pi() const;

	//! scalar dot product as if the quat was a four-vector
	//!	this is the cos of the angle between the quats (?)
	float Component4Dot(const Quat & q) const;

	//! DifferenceSqr for comparison & soft equality only !
	//!	NOTEZ : negative partners will NOT be called equal, even though they are the same rotation
	float DifferenceSqr(const Quat & q) const;

	//! exact equality test
	//!	NOTEZ : negative partners will NOT be called equal, even though they are the same rotation
	bool operator == (const Quat & other) const;
	//! soft equality test
	//!	NOTEZ : negative partners will NOT be called equal, even though they are the same rotation
	static bool Equals(const Quat &a,const Quat &b,const float tolerance = EPSILON);

	//! soft equality AND negative partners are counterd!
	static bool EqualRotations(const Quat &a,const Quat &b,const float tolerance = EPSILON);

	//-------------------------------------------------
	// Basic math operations :
	
	//! this += q
	void Add(const Quat & q);

	//! this -= q
	void Subtract(const Quat & q);
	
	//! this = (q1 + q2)/2
	// WARNING : may produce different results than SetLerp(0.5) due to floating round-off
	void SetAverage(const Quat &q1,const Quat &q2);

	//! this = q1 * (1-t) * q2 * t
	//! SetLerp is NOT SLerp ; it's a straight linear interpolation!
	void SetLerp(const Quat &q1,const Quat &q2,const float t);
	
	//! this += t * q
	void AddScaled(const Quat &q,const float t);

	//! this = a - b;
	void SetDifference(const Quat &a,const Quat &b);

	//! this = a + b;
	void SetSum(const Quat &a,const Quat &b);

	//! this = ca * a + cb * b;
	void SetWeightedSum(const float ca,const Quat &a,const float cb,const Quat &b);

	//! this = s * q;
	void SetScaled(const float s, const Quat& q);
	//-------------------------------------------------
	// Multiplies :
	
	//! this = a * b;
	//!	 neither a nor b can be "this" (ASSERTs)
	void SetMultiple(const Quat &a,const Quat &b);

	//! this = q * this
	//!	 "q" cannot be "this" (ASSERTs)
	void LeftMultiply(const Quat &q);

	//! this = this * q
	//!	 "q" cannot be "this" (ASSERTs)
	void RightMultiply(const Quat &q);

	//-------------------------------------------------

	//! accessors for rotations in the form of a rotation axis and an angle in radians
	void SetFromAxisAngle(const Vec3 & axis, const float angle);
	void GetAxisAngle(Vec3 * pAxis, float * pAngle) const;
	//! GetAxisAngleMod2Pi gives you an angle in [0,pi] - it
	//!	 always gives you the shortest-path rotation
	void GetAxisAngleMod2Pi(Vec3 * pAxis, float * pAngle) const;

	void SetXRotation(const float angle);
	void SetYRotation(const float angle);
	void SetZRotation(const float angle);

	/*! a "ScaledVector" is a rotation axis, whose length is
		the angle
	  the nice thing about this is that it's a minimal
		representation of a rotation, and it has no axis-flip
		degeneracy (flipped axis and flipped angle gives the
		same ScaledVector), so "Q" and "-Q" give the same ScaledVector
	
	  "ScaledVector" seems like a pretty good way to interpolate
		 rotations, and it's even more compact than a quat !
	*/
	void SetFromScaledVector(const Vec3 & v); // this is aka "Exp"
	//! GetScaledVector needs a normalized quat
	void GetScaledVector(Vec3 * pV) const; // this is aka "Log"

	//! GetMatrix requires a normalize quat !!
	//! about 90 clocks
	void GetMatrix(Mat3 * pInto) const; 
	
	//! GetMatrixUnNormalized is faster than
	//!		doing a normalize then a GetMatrix
	//! but slower than GetMatrix on an already-normalized quat
	void GetMatrixUnNormalized(Mat3 * pInto) const;

	//! SetFromMatrix : only on orthonormal matrices !!
	//	(ASSERTs)
	void SetFromMatrix(const Mat3 & m);

	//-------------------------------------------------
	//! Rotate; apply the quat to a vector;
	//!	 rotates the vector to make a new one
	//! about 90 clocks
	const Vec3 Rotate(const Vec3 & u) const; 

	//! RotateUnnormalized
	//! works for unnormalized quats (does a divide)
	//! to = Q^(-1) * fm * Q
	const Vec3 RotateUnnormalized(const Vec3 &fm) const;

	//-------------------------------------------------
private:

	/*! data:
		 Quat = m_v * {ijk} + m_w
		 ij = -k
		 ii = -1
		 etc.

		 current implementation of Get/Set Component requires the memory
		 layout to be "vec then scalar"
	*/

	Vec3		m_v;
	float		m_w;
};

//}{=======================================================================
//
// Functions
//

inline bool Quat::IsValid() const
{
	ASSERT( m_v.IsValid() && fisvalid(m_w) );
	return true;
}

//! exact equality test
inline bool Quat::operator == (const Quat & other) const
{
	ASSERT( IsValid() && other.IsValid() );
	return m_v == other.m_v && m_w == other.m_w;
}

//! soft equality test
inline /*static*/ bool Quat::Equals(const Quat &a,const Quat &b,const float tolerance /*= EPSILON*/)
{
	ASSERT( a.IsValid() && b.IsValid() );
	return Vec3::Equals(a.m_v,b.m_v,tolerance) && fequal(a.m_w,b.m_w,tolerance);
}

//! soft equality AND negative partners are counterd!
inline /*static*/ bool Quat::EqualRotations(const Quat &a,const Quat &b,const float tolerance /*= EPSILON*/)
{
	ASSERT( a.IsValid() && b.IsValid() );
	
	if ( Vec3::Equals(a.m_v,b.m_v,tolerance) && fequal(a.m_w,b.m_w,tolerance) )
		return true;
	if ( Vec3::Equals(a.m_v,-1.f * b.m_v,tolerance) && fequal(a.m_w,- b.m_w,tolerance) )
		return true;
	return false;
}

//! quats should almost always be normalized
//!	IsNormalized not included in IsValid because they can
//!	go temporary un-norm'ed during manipulations
inline bool Quat::IsNormalized(const float tolerance /*= EPSILON*/) const
{
	ASSERT(IsValid());
	// @@ epsilon disaster ?
	return fisone( LengthSqr(), /*fsquare*/ tolerance );
}

inline float Quat::Normalize()
{
	const float length = Length();
	ASSERT( length > EPSILON );
	const float scale = 1.f / length;
	m_v *= scale;
	m_w *= scale;
	return length;
}

inline float Quat::Length() const
{
	return sqrtf(LengthSqr());
}

inline float Quat::LengthSqr() const
{
	return m_v.LengthSqr() + m_w*m_w;
}

// SetNegativePartner makes the negative quat Q
//	which represents the same rotation,
//  but will act differently under linear
//	manipulations (such as Lerp)
inline void Quat::SetNegativePartner() // memberwise negative
{
	m_v.Invert();
	m_w = - m_w;
}

// Invert makes a quat Q^(-1) such that
//	Q^(-1) Q = identity	
inline void Quat::Invert()
{
	const float invLenSqr = 1.f / LengthSqr();
	m_v *= invLenSqr;
	m_w *= - invLenSqr; // not negated
}

inline void Quat::InvertNormalized()
{
	// Invert() for normalized quats is easier :
	ASSERT( IsNormalized() );
	//m_v.Invert();
	// m_w unchanged
	// this is the same since -Q ~ Q
	// and faster
	m_w = - m_w;
}

inline void Quat::SetIdentity()
{
	m_v = Vec3::zero;
	m_w = 1.f;
}

inline void Quat::SetComponents(const Vec3 &v,const float w)
{
	m_v = v;
	m_w = w;
}

inline void Quat::SetComponents(const float i,const float j,const float k,const float w)
{
	m_v.x = i;
	m_v.y = j;
	m_v.z = k;
	m_w = w;
}

inline const Vec3 & Quat::GetVector() const
{
	return m_v;
}

inline float Quat::GetScalar() const
{
	return m_w;
}

inline float Quat::GetAngle() const // Expensive !!
{
	ASSERT( IsNormalized() );
	return acosf_safe( m_w ) * 2.f;
}

inline float Quat::Component4Dot(const Quat & q) const
{
	return m_v * q.m_v + m_w * q.m_w;
}

//-------------------------------------------------
// Basic math operations :

inline void Quat::Add(const Quat & q)
{
	ASSERT( q.IsValid() && IsValid() );
	m_v += q.m_v;
	m_w += q.m_w;
	ASSERT( IsValid() );
}

inline void Quat::Subtract(const Quat & q)
{
	ASSERT( q.IsValid() && IsValid() );
	m_v -= q.m_v;
	m_w -= q.m_w;
	ASSERT( IsValid() );
}

inline void Quat::AddScaled(const Quat &v,const float t)
{
	ASSERT( v.IsValid() && IsValid() );
	m_v.AddScaled(v.m_v,t);
	m_w += t * v.m_w;
	ASSERT( IsValid() );
}

inline void Quat::SetAverage(const Quat &v1,const Quat &v2)
{
	ASSERT( v1.IsValid() && v2.IsValid() );
	// WARNING : may produce different results than SetLerp(0.5) due to floating round-off
	m_v.SetAverage(v1.m_v,v2.m_v);
	m_w = 0.5f * (v2.m_w + v1.m_w);
	ASSERT( IsValid() );
}

inline void Quat::SetLerp(const Quat &v1,const Quat &v2,const float t)
{
	ASSERT( v1.IsValid() && v2.IsValid() );
	m_v.SetLerp(v1.m_v,v2.m_v,t);
	m_w = v1.m_w + t * (v2.m_w - v1.m_w);
	ASSERT( IsValid() );
}

// this = a - b;
inline void Quat::SetDifference(const Quat &a,const Quat &b)
{
	ASSERT( a.IsValid() && b.IsValid() );
	m_v = a.m_v - b.m_v;
	m_w = a.m_w - b.m_w;
	ASSERT( IsValid() );
}

// this = a + b;
inline void Quat::SetSum(const Quat &a,const Quat &b)
{
	ASSERT( a.IsValid() && b.IsValid() );
	m_v = a.m_v + b.m_v;
	m_w = a.m_w + b.m_w;
	ASSERT( IsValid() );
}

// this = ca * a + cb * b;
inline void Quat::SetWeightedSum(const float ca,const Quat &a,const float cb,const Quat &b)
{
	ASSERT( a.IsValid() && b.IsValid() );
	m_v.SetWeightedSum(ca,a.m_v,cb,b.m_v);
	m_w = ca * a.m_w + cb * b.m_w;
	ASSERT( IsValid() );
}

// this = s * q;
inline void Quat::SetScaled(const float s, const Quat& q)
{
	ASSERT( q.IsValid() );
	m_v = s * q.m_v;
	m_w = s * q.m_w;
	ASSERT( IsValid() );
}


inline void Quat::SetMultiple(const Quat &a,const Quat &b)
{
	ASSERT( a.IsValid() && b.IsValid() );
	ASSERT( this != &a && this != &b );

	// you get an 'i' part from
	//	a.w * b.i and b.w * a.i and a.j * b.k and a.k * b.j

	#define QI m_v.x
	#define QJ m_v.y
	#define QK m_v.z
	#define QW m_w

	QI = a.QI * b.QW + a.QW * b.QI + a.QJ * b.QK - a.QK * b.QJ;
	QJ = a.QJ * b.QW + a.QW * b.QJ + a.QK * b.QI - a.QI * b.QK;
	QK = a.QK * b.QW + a.QW * b.QK + a.QI * b.QJ - a.QJ * b.QI;
	QW = a.QW * b.QW - a.QI * b.QI - a.QJ * b.QJ - a.QK * b.QK;

	#undef QI
	#undef QJ
	#undef QK
	#undef QW

	/*
	//same as :

	m_v.SetCross(a.m_v,b.m_v);
	m_v += a.m_w * b.m_v;
	m_v += b.m_w * a.m_v;

	m_w = a.m_w * b.m_w - a.m_v * b.m_v;

	// which is neat for mathematical analysis, but not efficient
	*/

	ASSERT( IsValid() );
}

inline void Quat::LeftMultiply(const Quat &q)
{
	const Quat temp(*this);
	SetMultiple(q,temp);
}

inline void Quat::RightMultiply(const Quat &q)
{
	const Quat temp(*this);
	SetMultiple(temp,q);
}

inline float Quat::GetComponent(const int i) const
{
	ASSERT(IsValid());
	return ((float *)this)[i];
}

inline void Quat::SetComponent(const int i,const float f)
{
	ASSERT(fisvalid(f));
	((float *)this)[i] = f;
}

END_CB

//}=======================================================================

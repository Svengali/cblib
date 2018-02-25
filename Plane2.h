#pragma once

#include "Plane.h" // for Plane::ESide

#include "Vec2.h"
#include "Vec3.h"

START_CB

class Mat2;

//{=========================================================================================================

/*!
 Plane2

	Nearly identical to "Plane" ; maintain fixes in both !!

*/

class Plane2
{
public:
	// use Plane::ESide

	//-------------------------------------------------------------------------------
	// constructors :

	Plane2()
	{
		// do-nada constructor;
		finvalidatedbg(m_offset);
	}

	Plane2(const Vec2 & normal,const Vec2 & point)
	{
		SetFromNormalAndPoint(normal,point);
	}

	Plane2(const Vec2 & normal,const float offset) :
		m_normal(normal), m_offset(offset)
	{
		ASSERT( IsValid() );
	}

	// default operator = and copy constructor are fine

	//-------------------------------------------------------------------------------
	// Basic tests :

	bool IsValid() const;

	//! exact equality test
	bool operator == (const Plane2 & other) const;
	//! soft equality test
	static bool Equals(const Plane2 &a,const Plane2 &b,const float tolerance = EPSILON);

	//-------------------------------------------------------------------------------
	// basic Set's

	//! define a Plane2 with a normal and a point on the Plane2
	void SetFromNormalAndPoint(const Vec2 & normal,const Vec2 & point);

	//float SetFromThreePoints(const Vec2 & point1,const Vec2 & point2,const Vec2 & point3);

	//-------------------------------------------------------------------------------
	// Point-Plane2 tests

	//! Signed distance to point; positive is in front of the Plane2
	float DistanceToPoint(const Vec2 & point) const;

	//! Point Side; returns "On" as Front; only eFront and eBack returned
	Plane::ESide PointSideGE(const Vec2 & point) const;

	//! Point Side with Trinary return : Front/Back/Intersecting
	Plane::ESide PointSideOrOn(const Vec2 & point,const float tolerance = EPSILON) const;

	//-------------------------------------------------------------------------------
	// modifications :

	//! Rotate and Transform rotate the whole Plane2, not just the normal
	void Rotate(const Mat2 & mat);
	void Translate(const Vec2 & v);

	//! moves the Plane2 by Delta along its normal
	//! if you do MoveForwards( DistanceToPoint( vert ) );
	//! then you will be on vert
	void MoveForwards(const float delta);

	//! reverse the facing of the Plane2
	void FlipNormal(void);

	//-------------------------------------------------------------------------------
	// ray/plane tests :
	// These differ from the LSS segment test in that the result will be 'on' the plane.

	/// Find the distance along a ray where a line intersects this plane (dir must be normalized).
    bool    	GetRayIntersectionTime( const Vec2 &base,
    						    const Vec2 &dir,
    						    float *pResult ) const;

	/// Find where a ray intersects this plane (dir must be normalized).
    bool    	GetRayIntersectionPoint( const Vec2 &base,
    							    const Vec2 &dir,
    							    Vec2 *pResult ) const;

	//-------------------------------------------------------------------------------
	// Get members :

	const Vec2 & GetNormal() const		{ ASSERT(IsValid()); return m_normal; }
	const float GetOffset() const		{ ASSERT(IsValid()); return m_offset; }

	const Vec3 & GetVec3() const		{ ASSERT(IsValid()); return *((const Vec3 *)this); }

	//! makes an arbitrary single point which is on the Plane2
	//!	within Distance <= EPSILON
	const Vec2 MakePointOnPlane2() const;

	//! make the closest point on the Plane2 to the argument
	const Vec2 GetClosestPointOnPlane2(const Vec2 & arg) const;

	//-------------------------------------------------------------------------------

private:

	Vec2		m_normal;
	float		m_offset;

}; // end Plane2 class

//}{=========================================================================================================
//
//	Functions
//

//! exact equality test
inline bool Plane2::operator == (const Plane2 & other) const
{
	return ( m_normal == other.m_normal && m_offset == other.m_offset );
}

//! soft equality test
inline /*static*/ bool Plane2::Equals(const Plane2 &a,const Plane2 &b,const float tolerance /*= EPSILON*/)
{
	return Vec2::Equals(a.m_normal,b.m_normal) && fequal(a.m_offset,b.m_offset,tolerance);
}

inline void Plane2::SetFromNormalAndPoint(const Vec2 & normal,const Vec2 & point)
{
	ASSERT( normal.IsNormalized() );
	m_normal = normal;
	m_offset = - (m_normal * point);

	ASSERT( IsValid() );
}

//-------------------------------------------------------------------------------
// Point-Plane2 tests

inline float Plane2::DistanceToPoint(const Vec2 & point) const
{
	ASSERT(IsValid() && point.IsValid());
	return (m_normal * point) + m_offset;
}

inline Plane::ESide Plane2::PointSideGE(const Vec2 & point) const
{
	ASSERT(IsValid() && point.IsValid());
	const float d = DistanceToPoint(point);
	return ( d >= 0.f ) ? Plane::eFront : Plane::eBack;
}

inline Plane::ESide Plane2::PointSideOrOn(const Vec2 & point,const float tolerance /*= EPSILON*/) const
{
	ASSERT(IsValid() && point.IsValid());
	const float d = DistanceToPoint(point);
	if ( d > tolerance )
		return Plane::eFront;
	if ( d <-tolerance )
		return Plane::eBack;
	return Plane::eIntersecting;
}

//-------------------------------------------------------------------------------
// modifications :

inline void Plane2::Translate(const Vec2 & v)
{
	ASSERT(IsValid() && v.IsValid());
	// slide the offset
	m_offset -= v * m_normal;
	ASSERT(IsValid());
}

//! moves the Plane2 by Delta along its normal
//! if you do MoveForwards( DistanceToPoint( vert ) );
//! then you will be on vert
inline void Plane2::MoveForwards(const float delta)
{
	ASSERT(IsValid());
	m_offset -= delta;
	ASSERT(IsValid());
}

inline void Plane2::FlipNormal(void)
{
	ASSERT(IsValid());
	m_normal *= -1;
	m_offset *= -1;
	ASSERT(IsValid());
}

inline const Vec2 Plane2::MakePointOnPlane2() const
{
	ASSERT( PointSideOrOn( m_normal * (- m_offset) ) == Plane::eIntersecting );
	return m_normal * (- m_offset);
}

inline const Vec2 Plane2::GetClosestPointOnPlane2(const Vec2 & arg) const
{
	// find the distance to the Plane2
	const float d = DistanceToPoint(arg);
	// move arg onto the Plane2 in the normal direction
	return arg - d * m_normal;
}

//}=========================================================================================================

END_CB
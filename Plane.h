#pragma once

#include "cblib/Base.h"
#include "cblib/Vec3.h"

START_CB

namespace Cull
{
	enum EResult
	{
		eIn,
		eOut,
		eCrossing
	};
};

class Mat3;
class Frame3;
class Frame3Scaled;
class Segment;
struct SegmentResults;

//{=========================================================================================================

/*!
 Plane
	equivalent to a 4-vector

	Plane is higher level than Frame3, but lower level than all
	other primitives.  So, Sphere, Box, etc. all know about Plane,
	and Plane doesn't know about them.

	Nearly identical to "Plane2" ; maintain fixes in both !!

 @@ : can't really usefully forward declare Plane
	because of the enum ESide contained within
*/

class Plane
{
public:

	/*!	Plane side enum

		Plane front & back must be 0 and 1 cuz I use ^= 1 to switch between them
		(and they could be used to index BSP children arrays)

		eIntersecting & eOn are only used by routines that make sense to return either
		 eIntersecting or eOn
		most routines (like PointSide and BoxSide) only return eIntersecting
		Segment and Polygon routines can return eIntersecting or eOn
		 in which case eOn means coplanar-with.
	*/
	enum ESide
	{
		eFront=0,
		eBack =1,
		eIntersecting=2,
		eOn=3
	};

	//-------------------------------------------------------------------------------
	// constructors :

	Plane()
	{
		// do-nada constructor;
		finvalidatedbg(m_offset);
	}

	Plane(const Vec3 & normal,const Vec3 & point)
	{
		SetFromNormalAndPoint(normal,point);
	}

	Plane(const Vec3 & normal,const float offset) :
		m_normal(normal), m_offset(offset)
	{
		ASSERT( IsValid() );
	}

	// default operator = and copy constructor are fine

	//-------------------------------------------------------------------------------
	// Basic tests :

	bool IsValid() const;

	//! exact equality test
	bool operator == (const Plane & other) const;
	//! soft equality test
	static bool Equals(const Plane &a,const Plane &b,const float tolerance = EPSILON);

	//-------------------------------------------------------------------------------
	// basic Set's

	//! define a Plane with a normal and a point on the plane
	void SetFromNormalAndPoint(const Vec3 & normal,const Vec3 & point);

	/*! define a Plane with three points on the plane
	 returns the area normally, zero on failure !!
	 facing of plane defined by counter-clockwise (??) orientation of the triangle
		defined by these three points
	 can fail if the points are on top of eachother
	*/
	float SetFromThreePoints(const Vec3 & point1,const Vec3 & point2,const Vec3 & point3);

	//-------------------------------------------------------------------------------
	// Point-Plane tests

	//! Signed distance to point; positive is in front of the plane
	float DistanceToPoint(const Vec3 & point) const;

	//! Point Side; returns "On" as Front; only eFront and eBack returned
	ESide PointSideGE(const Vec3 & point) const;

	//! Point Side with Trinary return : Front/Back/Intersecting
	ESide PointSideOrOn(const Vec3 & point,const float tolerance = EPSILON) const;

	//-------------------------------------------------------------------------------
	// modifications :

	//! Rotate and Transform rotate the whole plane, not just the normal
	void Transform(const Frame3 & xf);
	void TransformByInverse(const Frame3Scaled & xf);
	void Rotate(const Mat3 & mat);
	void Translate(const Vec3 & v);

	//! moves the plane by Delta along its normal
	//! if you do MoveForwards( DistanceToPoint( vert ) );
	//! then you will be on vert
	void MoveForwards(const float delta);

	//! reverse the facing of the plane
	void FlipNormal(void);

	//-------------------------------------------------------------------------------
	// seg (LSS) tests :

	//! SegmentSide can return Front,Back,Intersecting, or On
	ESide SegmentSide(const Vec3 & fm,const Vec3 & to, const float radius = 0.f) const;
	ESide SegmentSide(const Segment & seg) const;

	bool IntersectVolume(const Segment & seg) const;
	bool IntersectSurface(const Segment & seg,SegmentResults * pResults) const;

	//-------------------------------------------------------------------------------
	// ray/plane tests :
	// These differ from the LSS segment test in that the result will be 'on' the plane.

	/// Find the distance along a ray where a line intersects this plane (dir must be normalized).
    bool    	GetRayIntersectionTime( const Vec3 &base,
    						    const Vec3 &dir,
    						    float *pResult ) const;

	/// Find where a ray intersects this plane (dir must be normalized).
    bool    	GetRayIntersectionPoint( const Vec3 &base,
    							    const Vec3 &dir,
    							    Vec3 *pResult ) const;

	//-------------------------------------------------------------------------------
	// Get members :

	const Vec3 & GetNormal(void) const		{ ASSERT(IsValid()); return m_normal; }
	const float GetOffset() const			{ ASSERT(IsValid()); return(m_offset); }

	//! makes an arbitrary single point which is on the plane
	//!	within Distance <= EPSILON
	const Vec3 MakePointOnPlane() const;

	//! make the closest point on the plane to the argument
	const Vec3 GetClosestPointOnPlane(const Vec3 & arg) const;

	//! ProjectOneAxisToPlane returns pos[axis] projected onto the plane
	//!	 plane normal must NOT be zero in that axis !!
	float ProjectOneAxisToPlane(const Vec3 & pos,const int axis) const;

	//-------------------------------------------------------------------------------

private:

	Vec3		m_normal;
	float		m_offset;

}; // end Plane class

//}{=========================================================================================================
//
//	Functions
//

//! exact equality test
inline bool Plane::operator == (const Plane & other) const
{
	return ( m_normal == other.m_normal && m_offset == other.m_offset );
}

//! soft equality test
inline /*static*/ bool Plane::Equals(const Plane &a,const Plane &b,const float tolerance /*= EPSILON*/)
{
	return Vec3::Equals(a.m_normal,b.m_normal) && fequal(a.m_offset,b.m_offset,tolerance);
}

inline void Plane::SetFromNormalAndPoint(const Vec3 & normal,const Vec3 & point)
{
	ASSERT( normal.IsNormalized() );
	m_normal = normal;
	m_offset = - (m_normal * point);

	ASSERT( IsValid() );
}

//-------------------------------------------------------------------------------
// Point-Plane tests

inline float Plane::DistanceToPoint(const Vec3 & point) const
{
	ASSERT(IsValid() && point.IsValid());
	return (m_normal * point) + m_offset;
}

inline Plane::ESide Plane::PointSideGE(const Vec3 & point) const
{
	ASSERT(IsValid() && point.IsValid());
	const float d = DistanceToPoint(point);
	return ( d >= 0.f ) ? eFront : eBack;
}

inline Plane::ESide Plane::PointSideOrOn(const Vec3 & point,const float tolerance /*= EPSILON*/) const
{
	ASSERT(IsValid() && point.IsValid());
	const float d = DistanceToPoint(point);
	if ( d > tolerance )
		return eFront;
	if ( d <-tolerance )
		return eBack;
	return eIntersecting;
}

//-------------------------------------------------------------------------------
// modifications :

inline void Plane::Translate(const Vec3 & v)
{
	ASSERT(IsValid() && v.IsValid());
	// slide the offset
	m_offset -= v * m_normal;
	ASSERT(IsValid());
}

//! moves the plane by Delta along its normal
//! if you do MoveForwards( DistanceToPoint( vert ) );
//! then you will be on vert
inline void Plane::MoveForwards(const float delta)
{
	ASSERT(IsValid());
	m_offset -= delta;
	ASSERT(IsValid());
}

inline void Plane::FlipNormal(void)
{
	ASSERT(IsValid());
	m_normal.Invert();
	m_offset *= -1;
	ASSERT(IsValid());
}

inline const Vec3 Plane::MakePointOnPlane() const
{
	ASSERT( PointSideOrOn( m_normal * (- m_offset) ) == eIntersecting );
	return m_normal * (- m_offset);
}

inline const Vec3 Plane::GetClosestPointOnPlane(const Vec3 & arg) const
{
	// find the distance to the plane
	const float d = DistanceToPoint(arg);
	// move arg onto the plane in the normal direction
	return arg - d * m_normal;
}

END_CB

//}=========================================================================================================

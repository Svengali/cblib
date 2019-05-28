/*!
  \file
  \brief 
	Sphere
	16 bytes
	a center (Vec3) and a radius

	Sphere is a "Volume Primitive" just like AxialBox, OrientedBox, etc.

	See VolumeUtil.h
*/

#pragma once

#include "cblib/Util.h"
#include "cblib/FloatUtil.h"
#include "cblib/Plane.h"
#include "cblib/Vec3.h"
#include "cblib/Vec3U.h"

START_CB

class Frame3;
class Frame3Scaled;
class Segment;
struct SegmentResults;


//{===============================================================================

class Sphere
{
public:
	//--------------------------------------------------------------
	// Constructors

	//! default constructor sets no data members
	Sphere()
	{
		finvalidatedbg(m_radius);
	}

	Sphere(const Vec3 & center,const float radius) :
		m_center(center), m_radius(radius)
	{
	}

	bool IsValid() const
	{
		ASSERT( fisvalid(m_radius) && m_center.IsValid() );
		ASSERT( m_radius >= 0.f );
		return true;
	}

	// default copy constructor and assignment are good

	static const Sphere unit; // at zero, radius of 1
	static const Sphere zero; // at zero, radius of zero

	static bool Equals(const Sphere &a,const Sphere &b,const float tolerance = EPSILON);

	//--------------------------------------------------------------
	// Simple queries , Radius/Center accessors

	float GetRadius() const				{ /*ASSERT(IsValid());*/ return m_radius; }
	void SetRadius(const float r)		{ ASSERT(r >= 0.f); m_radius = r; }
	float & MutableRadius()				{ return m_radius; }

	const Vec3 & GetCenter() const		{ /*ASSERT(IsValid());*/ return m_center; }
	void SetCenter(const Vec3 &c)		{ ASSERT(c.IsValid()); m_center = c; }
	Vec3 & MutableCenter()				{ return m_center; }
	
	void Expand(const float r)			{ ASSERT(IsValid()); m_radius += r; ASSERT(IsValid()); }

	void Set(const Vec3 &c,const float r);
	void SetToPoint(const Vec3 &c);

	float GetVolume() const;
	float GetSurfaceArea() const;

	//--------------------------------------------------------------

	//! TransformCenter : does nada to Radius
	void TransformCenter(const Frame3 & xf);
	void Translate(const Vec3 &v);
	void Scale(const float s);
	
	//! scales the centre postion, not the radius
	void ScaleCenter(const float s); 
	
	// this == s is fine
	void SetTransformed(const Sphere &s,const Frame3Scaled & xfs);

	//--------------------------------------------------------------
	// Distance/Intersects/Contains

	//! does a sqrt !
	//!	returns 0.f for a point inside the sphere
	float DistanceTo(const Vec3 & point) const;

	//! DifferenceDistSqr : compare two spheres,
	//!	return a difference in units of distance^2
	float DifferenceDistSqr(const Sphere &sphere ) const;

	//! returns true if the spheres touch or are inside eachother
	bool Intersects(const Sphere & sphere, const float eps = EPSILON) const;

	//! returns true if the point is touching or in the sphere
	bool Contains(const Vec3 & point, const float eps = EPSILON) const;

	//! returns true if this wholly contains the argument sphere
	bool Contains(const Sphere & sphere, const float eps = EPSILON) const;

	//--------------------------------------------------------------
	// Building/Combining of Spheres

	//! only changes radius
	//! does a sqrt if the sphere changes
	bool ExtendToPoint(const Vec3 &v);

	/*! make this sphere the one that encloses spheres 1 and 2
		 s1 or s2 == this is Ok
		 returns a result indicating what work was done;
		 either one of the arguments was selected, or a new sphere
		  was made surrounding both
	*/
	enum ESetEnclosingResult
	{
		eSetToSphere1,
		eSetToSphere2,
		eSetToSphereSurrounding,
	};
	ESetEnclosingResult SetEnclosing(const Sphere &s1,const Sphere &s2);

	//! returns whether any work was actually done
	bool ExpandToEnclose(const Sphere &s);

	void	SetLerp(const Sphere& s0, const Sphere& s1, const float f);

	//--------------------------------------------------------------
	//! Sphere-LSS intersection tests

	bool IntersectVolume(const Segment & seg) const;
	bool IntersectSurface(const Segment& seg, SegmentResults* pResults) const;

	//--------------------------------------------------------------
	//! Plane-Sphere tests

	float PlaneDist(const Plane & plane) const;
	Plane::ESide PlaneSide(const Plane & plane) const;

	//--------------------------------------------------------------

private:

	//! data :
	//! 16 bytes ; perfect for packing

	Vec3	m_center;
	float	m_radius;
};

//}{===============================================================================
//
//	FUNCTIONS
//

//--------------------------------------------------------------
// Simple Mutators

inline void Sphere::Set(const Vec3 &c,const float r)
{
	SetCenter(c);
	SetRadius(r);
	ASSERT(IsValid()); 
}

inline void Sphere::SetToPoint(const Vec3 &c)
{
	SetCenter(c);
	SetRadius(0.f);
	ASSERT(IsValid()); 
}

inline float Sphere::GetVolume() const
{
	ASSERT(IsValid());
	static const float c_volumeCoefficient = ( 4.f * PIf / 3.f );
	return c_volumeCoefficient * fcube(m_radius);
}

inline float Sphere::GetSurfaceArea() const
{
	ASSERT(IsValid());
	static const float c_areaCoefficient = ( 4.f * PIf );
	return c_areaCoefficient * fsquare(m_radius);	
}
	
inline void Sphere::Translate(const Vec3 &v)
{
	ASSERT( IsValid() );
	m_center += v;
	ASSERT( IsValid() );
}

inline void Sphere::Scale(const float s)
{
	ASSERT( IsValid() );
	m_radius *= s;
	ASSERT( IsValid() );
}

inline void Sphere::ScaleCenter(const float s)
{
	ASSERT( IsValid() );
	m_center *= s;
	ASSERT( IsValid() );
}

inline void	Sphere::SetLerp(const Sphere& s0, const Sphere& s1, const float f)
{
	ASSERT(s0.IsValid());
	ASSERT(s1.IsValid());

	m_center.SetLerp(s0.m_center, s1.m_center, f);
	m_radius = s0.m_radius + (s1.m_radius - s0.m_radius) * f;
	
	ASSERT(IsValid());
}


//--------------------------------------------------------------
// Distance/Intersects/Contains

inline float Sphere::DistanceTo(const Vec3 & point) const // does a sqrt !
{
	ASSERT( IsValid() && point.IsValid() );
	const float dist = Distance(m_center,point) - m_radius;
	return MAX(dist,0.f);
}

// DifferenceDistSqr : compare two spheres,
//	return a difference in units of distance^2
inline float Sphere::DifferenceDistSqr(const Sphere &sphere ) const
{
	ASSERT( IsValid() && sphere.IsValid() );
	float dSqr = fsquare( m_radius - sphere.m_radius );
	dSqr += DistanceSqr(m_center , sphere.m_center );
	return dSqr;
}

// returns true if the spheres touch or are inside eachother
inline bool Sphere::Intersects(const Sphere & sphere, const float eps /* = EPSILON */) const
{
	ASSERT( IsValid() && sphere.IsValid() );
	const float distSqr = DistanceSqr(m_center,sphere.m_center);
	return ( distSqr <= fsquare(m_radius + sphere.m_radius + eps) );
}

// returns true if the point is touching or in the sphere
inline bool Sphere::Contains(const Vec3 & point, const float eps /* = EPSILON */) const
{
	ASSERT( IsValid() && point.IsValid() );
	const float distSqr = DistanceSqr(point,m_center);
	return ( distSqr <= fsquare(m_radius  + eps) );
}

// returns true if this wholly contains the argument sphere
inline bool Sphere::Contains(const Sphere & sphere, const float eps /* = EPSILON */) const
{
	ASSERT( IsValid() && sphere.IsValid() );
	ASSERT(eps >= 0.0f);
	
	// the goal is this, but we do it without sqrt :
	// return ( (dist + sphere.m_radius) <= m_radius );
	
	// can't contain a sphere that's bigger than me !
	const float radiusDiff = m_radius - sphere.m_radius + eps;

	if ( radiusDiff < 0.f )
		return false;

	const float distSqr = DistanceSqr(m_center,sphere.m_center);
	
	return ( distSqr <= fsquare(radiusDiff) );
}

//--------------------------------------------------------------
// Building/Combining of Spheres

inline bool Sphere::ExtendToPoint(const Vec3 &v) // does a sqrt if the sphere changes
{
	ASSERT( IsValid() && v.IsValid() );
	const float distSqr = DistanceSqr(m_center,v);
	// @@ no epsilon here at the moment
	if ( distSqr <= fsquare(m_radius) )
		return false; // point already in the sphere !

	DURING_ASSERT(const float oldRadius = m_radius);

	m_radius = sqrtf(distSqr);
	
	ASSERT( m_radius >= oldRadius );

	ASSERT( IsValid() );
	return true; // returns whether sphere was changes
}

//--------------------------------------------------------------
//! Plane-Sphere tests

inline float Sphere::PlaneDist(const Plane & plane) const
{
	ASSERT( IsValid() && plane.IsValid() );
	const float fDist = plane.DistanceToPoint(m_center);
	if ( fDist > m_radius )
		return fDist - m_radius;
	else if ( fDist < - m_radius )
		return fDist + m_radius;
	else
		return 0.f;
}

inline Plane::ESide Sphere::PlaneSide(const Plane & plane) const
{
	ASSERT( IsValid() && plane.IsValid() );
	const float fDist = plane.DistanceToPoint(m_center);
	if ( fDist > m_radius )
		return Plane::eFront;
	else if ( fDist < - m_radius )
		return Plane::eBack;
	else
		return Plane::eIntersecting;
}

//}===============================================================================

END_CB

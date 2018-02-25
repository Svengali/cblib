#pragma once

#include "cblib/Vec3.h"

START_CB

class Frame3Scaled;
class Frame3;

//{=========================================================================================

/*! SegmentResults

	simple collision result reporting data type
	This is an "SDT"

	use "time" to make hit point from the Segment
*/

struct SegmentResults
{
	float	time;		//!< 0 -> 1 , NOT clamped until you Validate
	Vec3	normal;		//!< NOT normalized until you Validate
	Vec3	collidedPoint;	//!< The first contact point on the collided surface (not necessarily on the segment)
	//!< the "collidedPoint" should be a valid point on the surface of the touched object
	//!< the "time" value gives you a hit point on the segment
	//!< use "GetHitPoint" on segment to turn "time" into a point
	//!< the "time" contact is when the sphere is stopped, the hit point result is NOT on the hit surface
	//!< if the LSS starts out in collision, SegmentResults should return a time of zero, and a hitPoint
	//!<	which is the closest point to the start point, inside the initial sphere

	/*
	Seg.GetHitPoint() gives you the point on the segment axis where
	the sphere was stopped.  So, you can move your sphere up to GetHitPoint ;
	it's NOT on the surface that was hit, in fact it should be "radius" distance
	away from it.
	*/

	SegmentResults()
	{
		time = FLT_MAX;
		normal = Vec3::unitZ;
		// collidedPoint invalidated by its constructor
		//collidedPoint = Vec3::zero;
	}

	void Reset()
	{
		time = FLT_MAX;
		normal = Vec3::unitZ;
	}

	//! at the end of gathering, call Validate()
	//!  it's an optimization to not do this each time SegmentResults is filled out
	void Validate()
	{
		time = fclampunit(time);
		normal.NormalizeSafe();
	}
};

//}{=========================================================================================

/*! Segment : An LSS (Line Swept Sphere)

	simple class for two end-points with a radius
	a segment is just like a ray with length

	everyone should implement
	interaction with Segment via :

	bool IntersectVolume(const Segment & seg) const;
	bool IntersectSurface(const Segment & seg,SegmentResults * pResults) const;
*/

class Segment
{
public:

	//-------------------------------------------------------

	// no default constructor :
	
	//! if fm-to are too close together, segment will return IsZero() and cannot be used !!!
	Segment(const Vec3 &fm,const Vec3 & to,
						const float radius = 0.0f)
	{
		ASSERT( radius >= 0.0f );
		m_radius = radius;
		SetFromEnds(fm,to);
	}

	//! version with additional supplied normal, for more accuracy
	Segment(const Vec3 &fm,const Vec3 & to,const Vec3 & normal,
						const float radius = 0.0f);

	Segment(const Segment & other,const float fraction);
						
	bool IsZero() const; //!< returns true for a segment too short to use; will fail IsValid() if IsZero() !!

	// default operator = and operator == and copy constructor are fine

	bool IsValid() const;
	static bool Equals(const Segment &a,const Segment &b,const float tolerance = EPSILON);

	//-------------------------------------------------------
	// Accessors :

	const Vec3 & GetFm() const			{ ASSERT(m_fm.IsValid()); return m_fm; }
	const Vec3 & GetTo() const			{ ASSERT(m_to.IsValid()); return m_to; }
	const Vec3 & GetNormal() const		{ ASSERT(m_normal.IsNormalized()); return m_normal; }
	const Vec3 & GetNormalNoVerify() const { return m_normal; }	// needed by CollisionQuery
	const Vec3 & GetInvNormal() const	{ ASSERT(m_invNormal.IsValid()); return m_invNormal; }
	float GetLength() const				{ return m_length; }
	float GetInvLength() const			{ ASSERT( fisvalid(m_invLength) ); return m_invLength; }
	float GetRadius() const				{ ASSERT( m_radius >= 0.f ); return m_radius; }

	const Vec3 GetHitPoint(const float tHit) const;
	const Vec3 GetCenter() const;
	float GetRadiusInDirection(const Vec3 & dir) const;
	float GetRadiusOnAxis(const int i) const; //!< i in 0-2

	//-------------------------------------------------------
	//! ClosestDistSqrToPoint does NOT use m_radius !

	float ClosestDistSqrToPoint(const Vec3 & toPoint) const;

	//-------------------------------------------------------
	// Mutators:
	void	TransformByTranspose(const Frame3Scaled& xform);
	void	TransformByTranspose(const Frame3& xform);

	//-------------------------------------------------------

	/** Collision tests.  Treats this segment as a cylinder with
		hemispherical endcaps (i.e. does NOT treat *this* segment as
		an LSS).  The query segment *is* treated as an LSS. */
	bool IntersectVolume(const Segment & seg) const;
	bool IntersectSurface(const Segment & seg,SegmentResults * pResults) const;

private:

	void SetFromEnds(const Vec3 &fm,const Vec3 & to, const Vec3 * pNormal = NULL);
	// will throw on a tiny segment !! 

	void MakeInvNormal();

	//-------------------------------------------------------
	// data :

	Vec3	m_fm,m_to;
	Vec3	m_normal;
	Vec3	m_invNormal; // for gAxialBox
	float	m_length;
	float	m_invLength;
	float	m_radius;
};

//}{=========================================================================================
// Functions

inline float Segment::GetRadiusInDirection(const Vec3 & dir) const
{
	ASSERT( IsValid() );
	return fabsf(m_normal * dir) * m_length * 0.5f + m_radius;
}

inline float Segment::GetRadiusOnAxis(const int i) const
{
	ASSERT( IsValid() );
	return fabsf(m_normal[i]) * m_length * 0.5f + m_radius;
}

//}=========================================================================================

END_CB

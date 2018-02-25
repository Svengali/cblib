#include "cblib/Base.h"
#include "Plane.h"
#include "Mat3.h"
#include "Frame3.h"
#include "Frame3Scaled.h"
#include "Segment.h"
#include "Vec3U.h"

START_CB

//=============================================================================================

bool Plane::IsValid() const
{
	ASSERT( fisvalid(m_offset) );
	ASSERT( m_normal.IsValid() );
	ASSERT( m_normal.IsNormalized() );
	return true;
}

/*! define a Plane with three points on the plane
 returns the area normally, zero on failure !!
 facing of plane defined by counter-clockwise (??) orientation of the triangle
	defined by these three points
 can fail if the points are on top of eachother
*/
float Plane::SetFromThreePoints(const Vec3 & point1,const Vec3 & point2,const Vec3 & point3)
{
	SetTriangleCross(&m_normal,point1,point2,point3);
	const float areaSqr = m_normal.LengthSqr();

	if ( areaSqr < EPSILON )
	{
		m_normal = Vec3::zero;
		m_offset = 0;

		//ASSERT(!IsValid());

		return 0.f;
	}
	else
	{
		const float area = sqrtf(areaSqr);
		m_normal *= (1.f/area);
		m_offset = - (m_normal * point1);

		ASSERT(IsValid());

		return area;
	}
}

void Plane::Rotate(const Mat3 & mat)
{
	ASSERT(IsValid());
	ASSERT( mat.IsOrthonormal() );

	// rotate the normal
	m_normal = mat.Rotate(m_normal);

	ASSERT(IsValid());
}

void Plane::Transform(const Frame3 & xf)
{
	ASSERT(IsValid());
	ASSERT( xf.IsOrthonormal() );

	// rotate the normal
	m_normal = xf.Rotate(m_normal);

	// slide the offset
	m_offset -= xf.GetTranslation() * m_normal;

	ASSERT(IsValid());
}

void Plane::TransformByInverse(const Frame3Scaled & xfs)
{
	ASSERT(IsValid());
	ASSERT( xfs.GetMatrix().IsOrthonormal() );
	ASSERT( !fiszero(xfs.GetScale()) );

	m_offset = ((xfs.GetTranslation() * m_normal) + m_offset) / xfs.GetScale();
	m_normal = xfs.GetMatrix().RotateByTranspose(m_normal);

	ASSERT(IsValid());
}

float Plane::ProjectOneAxisToPlane(const Vec3 & pos,const int axis) const
{
	static const int c_plus1mod3[3] = { 1,2,0 };
	static const int c_plus2mod3[3] = { 2,0,1 };

	const int o1 = c_plus1mod3[axis];
	const int o2 = c_plus2mod3[axis];

	const Vec3 & normal = GetNormal();

	ASSERT( ! fiszero(normal[axis]) );

	//result[o1] = p[o1];
	//result[o2] = p[o2];
	float result = (-GetOffset() - (pos[o1] * normal[o1] + pos[o2] * normal[o2])) / normal[axis];

	#ifdef DO_ASSERTS
	{
		Vec3 v;
		v[o1] = pos[o1];
		v[o2] = pos[o2];
		v[axis] = result;

		ASSERT( PointSideOrOn(v) == Plane::eIntersecting );
	}
	#endif

	return result;
}

//=============================================================================================

Plane::ESide Plane::SegmentSide(const Vec3 & fm,const Vec3 & to, const float radius /*= 0.f*/) const
{
	ASSERT(IsValid() && fm.IsValid() && to.IsValid());

	const float dFm = DistanceToPoint(fm);
	const float dTo = DistanceToPoint(to);

	if ( dFm < -radius && dTo < -radius )
	{
		return eBack;
	}
	else if ( dFm > radius && dTo > radius )
	{
		return eFront;
	}
	else if ( fabsf(dFm) <= EPSILON && fabsf(dTo) <= EPSILON )
	{
		return eOn;
	}
	else
	{
		return eIntersecting;
	}
}

Plane::ESide Plane::SegmentSide(const Segment & seg) const
{
	ASSERT(IsValid() && seg.IsValid() );
	return SegmentSide(seg.GetFm(),seg.GetTo(),seg.GetRadius());
}

bool Plane::IntersectVolume(const Segment & seg) const
{
	ASSERT(IsValid() && seg.IsValid() );
	const ESide side = SegmentSide(seg);
	return (side == eIntersecting || side == eOn );
}

//! IntersectSurface
//!	intersect an LSS with a plane, as a surface
bool Plane::IntersectSurface(const Segment & seg,SegmentResults * pResults) const
{
	// @@ UNTESTED

	ASSERT(IsValid() && seg.IsValid() );
	ASSERT(pResults);

	const float dFm = DistanceToPoint(seg.GetFm());
	const float dTo = DistanceToPoint(seg.GetTo());
	const float radius = seg.GetRadius();

	// positive distance is the front side

	if ( dTo >= dFm )
	{
		// moving away or parallel to the plane ->
		//	no collision
		return false;
	}

	ASSERT( dTo < dFm );

	if ( dFm <= 0.f )
	{
		// then dTo also <= 0.f since dTo < dFm
		// all on back
		return false;
	}
	else if ( dTo > radius )
	{
		// then dFm also > radius since dFm > dTo
		// all on front
		return false;
	}
	else
	{
		ASSERT( dFm > dTo );
		ASSERT( dFm >= 0.f );
		ASSERT( dTo <= radius );

		if ( dFm <= radius )
		{
			// starting in collision
			pResults->time = 0.f;
			pResults->normal = m_normal;
			// Return the point on the plane, closest to the segment From point.
			pResults->collidedPoint = seg.GetFm() - m_normal * dFm;
			return true;
		}

		ASSERT( dFm > radius );

		// just intersect the segment with a plane that's closer by radius

		ASSERT( (dFm - dTo) != 0.f );

		const float invSum = 1.f / (dFm - dTo); // positive
		ASSERT(invSum > 0.f);

		pResults->time = (dFm - radius) * invSum;
		pResults->time = fclampunit(pResults->time);
		pResults->normal = m_normal;

		// Compute the contact point... sweep the sphere up to the
		// contact time.  The contact occurs on the sphere, in the
		// direction towards the plane (i.e. opposite direction of the
		// plane normal).

		pResults->collidedPoint = seg.GetHitPoint(pResults->time) - m_normal * radius;

		return true;
	}
}

//=============================================================================================

// @@@ This is a distance, not a time

bool Plane::GetRayIntersectionTime(const Vec3 &point, const Vec3 &dir, float *pResult) const
{
	ASSERT( dir.IsNormalized() );
	float denom = m_normal * dir;	// dot product

	if(denom == 0.f)
		return false;

	*pResult = -DistanceToPoint(point) / denom;
	return true;
}

bool Plane::GetRayIntersectionPoint(const Vec3 &point, const Vec3 &dir, Vec3 *pResult) const
{
	float t;
	if(GetRayIntersectionTime(point, dir, &t))
	{
		if(t >= 0.f)
		{
			*pResult = point + dir * t;
			return true;
		}
	}
	return false;
}

//=============================================================================================

END_CB


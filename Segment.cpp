#include "Base.h"
#include "Segment.h"
#include "Sphere.h"
#include "vec3u.h"
#include "Frame3.h"
#include "Frame3Scaled.h"
//#include "DebugUtil/ExceptionUtil.h"

START_CB

/*****************************

The fundamental collision operation is like this :

while descending the BV tree
	use "Volume" tests
	consider and descend closer bounds first
when you hit a leaf
	use the "Surface" test
	Change all future segments to end at the found hit point

So, you find a close result as quickly as possible, and
then you only consider things that can give you a closer
result.

*********************************/

//-------------------------------------------------------

//! version with additional supplied normal, for more accuracy
Segment::Segment(const Vec3 &fm,const Vec3 & to,const Vec3 & normal,
					const float radius)
{
	ASSERT( radius >= 0.0f );
	m_radius = radius;
	SetFromEnds(fm,to,&normal);
}

Segment::Segment(const Segment & other,const float fraction) :
	m_fm(other.m_fm),
	m_normal(other.m_normal),
	m_invNormal(other.m_invNormal),
	m_radius(other.m_radius)
{
	m_length = other.m_length * fraction;
	
	if ( IsZero() )
	{
		// make it *actually* zero :
		m_to = m_fm;
		m_length = 0.f;
		m_invLength = FLT_MAX;
		m_normal = Vec3::unitZ;
		m_invNormal = Vec3(-FLT_MAX,-FLT_MAX,1.f);
		return;
	}
	else
	{
		m_to = other.GetHitPoint(fraction);
		m_invLength = 1.f / m_length;
	}
}
	
void Segment::SetFromEnds(const Vec3 &fm,const Vec3 & to, const Vec3 * pNormal /*= NULL*/)
{
	m_fm = fm;
	m_to = to;
	m_normal = to - fm;
	m_length = m_normal.Length();
	
	// cbloom 3-11-02 : tolerate invalid Segments in the constructor;
	//	client must check IsZero() and bail immediately
	if ( IsZero() )
	{
		// make it *actually* zero :
		m_to = fm;
		m_length = 0.f;
		m_invLength = FLT_MAX;
		m_normal = Vec3::unitZ;
		m_invNormal = Vec3(-FLT_MAX,-FLT_MAX,1.f);
		return;
	}

	m_invLength = 1.f / m_length;
	m_normal *= m_invLength;

	if ( pNormal != NULL )
	{
		// supplied normal should be "more accurate"
		// This error can be distressingly big !!
		ASSERT( Vec3::Equals(m_normal,*pNormal,0.1f) || m_length < 1e-3f );
		m_normal = *pNormal;
	}

	MakeInvNormal();
	ASSERT(IsValid());
}
	
void Segment::MakeInvNormal()
{
	// make m_invNormal
	// being careful not to divide by ~zero
	// used by gAxialBox & KDTree
	for(int i=0;i<3;i++)
	{
		static const float	SAFETY_FACTOR = 1000000.f;
		static const float	DANGER_LIMIT_MAX =
			FLT_MAX
			/ (4096.0f /* LEGAL_WORLD_SIZE_IN_METERS */
			   * 5.0f /* MAXIMUM_GEOMETRY_SCALE */
			   * SAFETY_FACTOR);
		static const float	DANGER_LIMIT_MIN = 1.0f / DANGER_LIMIT_MAX;
		// DANGER_LIMIT_MAX is still huge, ~1.66e+28.
		// DANGER_LIMIT_MIN (~6.02e-29) is still infinitesimal
		// compared to EPSILON, by a factor of like 8.3e+24

		const float val = m_normal[i];
		if (val >= -DANGER_LIMIT_MIN && val <= DANGER_LIMIT_MIN)
		{
			// val is tiny tiny, we don't want to divide by it.

			m_invNormal[i] = - FLT_MAX;
			// it's important that this be negative for gAxialBox

			// this axis is nearly degenerate, so just force it to be
			// totally degenerate so that KDTree and AxialBox can
			// handle it right
			ASSERT( fiszero(m_normal[i]) );
			m_normal[i] = 0.f;
		}
		else
		{
			m_invNormal[i] = 1.f/val;
		}
	}
}

//-------------------------------------------------------

const Vec3 Segment::GetHitPoint(const float tHit) const
{
	ASSERT( IsValid() );
	return MakeLerp( GetFm(), GetTo(), tHit );
}

const Vec3 Segment::GetCenter() const
{
	ASSERT( IsValid() );
	return MakeAverage(m_fm,m_to);
}

bool Segment::IsValid() const
{
	ASSERT( m_fm.IsValid() );
	ASSERT( m_to.IsValid() );
	ASSERT( m_normal.IsNormalized() );
	ASSERT( fisvalid(m_radius) );
	ASSERT( m_radius >= 0.f );
	// allow use of Zero Segments :
	//ASSERT( ! IsZero() );

	return true;
}

bool Segment::IsZero() const
{
	return ( m_length <= EPSILON );
}

//-------------------------------------------------------

/*static*/ bool Segment::Equals(const Segment &a,const Segment &b,const float tolerance /*= EPSILON*/)
{
	ASSERT( a.IsValid() && b.IsValid() );

	if ( ! Vec3::Equals(a.m_fm,b.m_fm,tolerance) )
		return false;
	if ( ! Vec3::Equals(a.m_to,b.m_to,tolerance) )
		return false;
	if ( ! fequal(a.m_radius,b.m_radius,tolerance) )
		return false;

	return true;
}

//-------------------------------------------------------

// @@ inline ?
float Segment::ClosestDistSqrToPoint(const Vec3 & toPoint) const
{
	// does NOT use m_radius !
	ASSERT( IsValid() );
	ASSERT( toPoint.IsValid() );

	if ( IsZero() )
	{
		return DistanceSqr(GetFm(),toPoint);
	}
	else
	{
		const Vec3 fmToPoint(toPoint - GetFm());

		const float hypotenuseSqr = fmToPoint.LengthSqr();

		const float alongSeg = fmToPoint * GetNormal();

		const float perpDistSqr = hypotenuseSqr - fsquare(alongSeg);

		if ( alongSeg < 0.f )
		{
			return perpDistSqr + fsquare(alongSeg);
		}
		else if ( alongSeg > GetLength() )
		{
			return perpDistSqr + fsquare(alongSeg - GetLength());		
		}
		else
		{
			return perpDistSqr;
		}
	}
}


/** Transform this segment by the inverse of the given Frame3Scaled.
    Essentially this transforms this segment into the object-space
    defined by the Frame3Scaled.  Assumes (and asserts) the matrix
    part of xform is orthonormal. */
void	Segment::TransformByTranspose(const Frame3Scaled& xform)
{
	ASSERT(xform.GetMatrix().IsOrthonormal());

	const float scale = xform.GetScale();
	
	if ( fisone(scale) )
	{
		m_normal = xform.GetMatrix().RotateByTranspose( m_normal );

		m_fm = xform.GetFrame().TransformTranspose(m_fm);
		m_to = xform.GetFrame().TransformTranspose(m_to);
	}
	else
	{
		const float invScale = 1.f / scale;

		// opposite of what you'd think cuz it's TRANSPOSE
		m_radius *= invScale;
		m_length *= invScale;
		m_invLength *= scale;
	
		m_normal = xform.GetMatrix().RotateByTranspose( m_normal );

		m_fm = xform.TransformTranspose(m_fm);
		m_to = xform.TransformTranspose(m_to);
	}
	
	MakeInvNormal();
	
	ASSERT(IsValid());
}


/** Transform this segment by the inverse of the given Frame3.
    Essentially this transforms this segment into the object-space
    defined by the Frame3.  Assumes (and asserts) the matrix part of
    xform is orthonormal. */
void	Segment::TransformByTranspose(const Frame3& xform)
{
	ASSERT(xform.GetMatrix().IsOrthonormal());

	m_normal = xform.RotateByTranspose( m_normal );

	m_fm = xform.TransformTranspose(m_fm);
	m_to = xform.TransformTranspose(m_to);

	MakeInvNormal();
	
	ASSERT(IsValid());
}


//-------------------------------------------------------


/** Return true if the given segment intersects this segment. */
bool Segment::IntersectVolume(const Segment &) const
{
	ASSERT(0);	// \todo implement!

	// find t such that seg.GetHitPoint(t) is closest to this line.  Quadratic eq.
	// t = clamp(t, 0, 1);
	// return ClosestDistSqrToPoint(seg.GetHitPoint(t)) < (this radius + seg radius) ^ 2;

	return false;
}


/** Find the contact, if any, between the given segment and this
	segment.  Returns true and fills in *pResult if there's contact;
	returns false and leaves *pResult alone if no contact.

	This segment is treated as a static capped cylinder, NOT an LSS.
	The query segment IS treated as an LSS (i.e. it's swept along the
	from-to segment over time, and the results reflect the time/place
	of contact). */
bool	Segment::IntersectSurface(const Segment& seg, SegmentResults* pResults) const
{
	ASSERT(IsValid());
	ASSERT(seg.IsValid());
	ASSERT(pResults);

	// Test vs. endcaps first.
	Sphere	fromCap(GetFm(), GetRadius()), toCap(GetTo(), GetRadius());
	bool	foundHit = false;
	SegmentResults	tempResults;

	// Check from-cap.
	if (fromCap.IntersectSurface(seg, &tempResults))
	{
		// Hit the from-cap.  Best (first) hit so far.
		foundHit = true;
		*pResults = tempResults;
	}

	// Check the to-cap.
	if (toCap.IntersectSurface(seg, &tempResults))
	{
		// Hit the to-cap.  See if this hit is better than existing
		// hit (if any).
		if (foundHit == false						// First hit; therefore the best so far.
			|| tempResults.time < pResults->time)	// New hit is better than existing hit.
		{
			foundHit = true;
			*pResults = tempResults;
		}
	}

	ASSERT( ! foundHit || (seg.GetNormal() * pResults->normal) < 0.f );

	// Compute limits for the t-value of the query segment, to limit
	// the query to between the endpoints of *this segment.

	float	dot = this->GetNormal() * seg.GetNormal();
	float	tLimit0, tLimit1;

	if (fabs(dot) > EPSILON)
	{
		tLimit0 = ((this->GetFm() - seg.GetFm()) * this->GetNormal()) / dot / seg.GetLength();	// @@ check this.
		tLimit1 = ((this->GetTo() - seg.GetFm()) * this->GetNormal()) / dot / seg.GetLength();	// @@ check this.

		if (fabs(dot) > 1.0f - EPSILON)
		{
			// Parallel segments -- this is an easy special case.  If
			// the segment didn't hit an endcap, then there's only one
			// other possibility for contact, which is that the query
			// is embedded in the cylinder.
			Vec3	p = seg.GetFm() - this->GetFm();
			float	dist = p * this->GetNormal();
			if (dist < 0 || dist > this->GetLength())
			{
				// segment starts outside our cylindrical area -- no
				// possibility of contact other than the endcaps,
				// which we already checked.
			}
			else
			{
				// segment could start out embedded in our cylinder.
				// Check the distance to our axis.
				Vec3	perp = p;
				perp -= this->GetNormal() * (perp * this->GetNormal());	// project into the plane perpendicular to this segment.

				float	testRadius = this->GetRadius() + seg.GetRadius();
				if (p.LengthSqr() < testRadius * testRadius)
				{
					// Query start is embedded in our segment.
					foundHit = true;

					Vec3 normal = perp;
					if ( normal.NormalizeSafe() == 0.f )
					{
//						FAIL("bad");
						// Happens when query start is embedded, and
						// extremely close to an endpoint.  We don't
						// have useful normal information, so handle this
						// as not-a-hit.
						//
						// @@ tulrich 6-27-2002 NEEDS REVIEW
						return foundHit;
					}

					// @@ cbloom change 5-28-02 :
					//if (normal * seg.GetNormal() >= EPSILON)
					if (normal * seg.GetNormal() >= 0.f)
					{
						// Not actually a valid hit; this happens when the query
						// is close to or barely in contact, but pointing away.
						return foundHit;
					}

					pResults->time = 0;
					pResults->normal = normal;
					pResults->collidedPoint = this->GetFm() + (p - perp) + pResults->normal * GetRadius(); // Point on our cylinder nearest to the approximate hit point.
					
					ASSERT( (seg.GetNormal() * pResults->normal) < 0.f );
				}

			}
			
			return foundHit;
		}
	}
	else
	{
		// Perpendicular segments.

		// No inherent length limit.
		tLimit0 = 0;
		tLimit1 = 1;

		// Check to see if the query is within bounds, though...

		// query is in a perpendicular plane -- compute the distance
		// from our from to that plane, along our normal.
		float	dist = (seg.GetFm() - this->GetFm()) * this->GetNormal();
		if (dist < 0 || dist > this->GetLength())
		{
			// Query segment can't touch our cylindrical part.  Heh heh...
			return foundHit;
		}
	}

	if (tLimit0 > tLimit1)
	{
		// swap.
		float	temp = tLimit0;
		tLimit0 = tLimit1;
		tLimit1 = temp;
	}

	// Only look for hits within the bounds of the cylinder body.
	float	tmin = MAX(0.f, tLimit0);
	float	tmax = MIN(1.f, tLimit1);

	if (tmin > tmax ||
		(foundHit && pResults->time < tmin))
	{
		// no chance to intersect.
		return foundHit;
	}

	// Transform to a more convenient coordinate space.  this segment
	// will run from the origin, along the x axis.

	Vec3	rx = this->GetNormal();
	Vec3	ry, rz;
	GetTwoPerpNormals(rx, &ry, &rz);

	Vec3	v(seg.GetFm() - this->GetFm());
	Vec3	segFrom(v * rx, v * ry, v * rz);

	v = seg.GetNormal();
	Vec3	segNorm(v * rx, v * ry, v * rz);

	// find t such that seg.GetHitPoint(t) is within the contact distance to this line.  Quadratic eq.
	//
	// p(t) = seg_prime.fm + seg_prime.norm * length * t;
	//
	// dist^2 = p(t).y * p(t).y + p(t).z * p(t).z;
	// 0 = (seg_prime.norm.y * t + seg_prime.fm.y)^2 + (seg_prime.norm.z * t + seg_prime.fm.z)^2 - (radius1 + radius2)^2

	// Re quadratic equation: in grade school I learned it as
	//
	//	x = (-b +/- sqrt(b^2 - 4ac)) / 2a
	//
	// Numerical Recipes says this "is asking for trouble".  Their
	// recommended version is:
	//
	// q = -0.5 * (b + sign(b) * sqrt(b*b - 4*a*c))
	//
	// t0 = q/a, t1 = c/q

	float	a = segNorm.y * segNorm.y + segNorm.z * segNorm.z;
	float	b = 2 * (segNorm.y * segFrom.y + segNorm.z * segFrom.z);
	float	c = segFrom.y * segFrom.y + segFrom.z * segFrom.z
		- (seg.GetRadius() + this->GetRadius()) * (seg.GetRadius() + this->GetRadius());
	
	// Compute the part inside the sqrt().
	float	radical = b * b - 4 * a * c;

	if (radical <= 0)
	{
		// No contact against the cylinder body.
		return foundHit;
	}

	//@@@@ cbloom weird case when radical is exactly zero ;
	//	I wind up hitting asserts about normals not going at each other

	float	root = sqrtf(radical);

	ASSERT(a > 0);

	float	sign_b = 1;
	if (b < 0)
	{
		sign_b = -1;
	}

	float	q = -0.5f * (b + sign_b * root);
	float	t0 = q / a;
	float	t1 = c / q;

	// Convert linear distance back to a proportion of the segment length.
	t0 *= seg.GetInvLength();
	t1 *= seg.GetInvLength();
	
	// Make sure t0 <= t1
	if (t0 > t1)
	{
		// swap.
		float	temp = t0;
		t0 = t1;
		t1 = temp;
	}

	if (t0 < tmin)
	{
		if (t1 < tmin)
		{
			// No hit vs. cylinder body.
			return foundHit;
		}
		
		ASSERT(fisvalid(GetInvLength()));

		// Segment starts out inside the cylinder.

		Vec3	approxHit = seg.GetHitPoint(tmin);

		// Make a normal perpendicular to this segment's axis.
		Vec3	normal = approxHit - GetFm();
		normal -= this->GetNormal() * (normal * this->GetNormal());	// subtract out the component parallel to this->GetNormal().
		if ( normal.NormalizeSafe() == 0.f )
		{
//			FAIL("bad");
			// This happens if the hit is very close to the start of
			// this segment.  Can probably just treat this as
			// not-a-hit.
			//
			// @@ tulrich 6-27-2002 NEEDS REVIEW
			return foundHit;
		}

		// @@ cbloom change 5-28-02 :
		//if (normal * seg.GetNormal() >= EPSILON)
		if (normal * seg.GetNormal() >= 0.f)
		{
			// Not actually a valid hit; this happens when the query
			// is close to or barely in contact, but pointing away.
			return foundHit;
		}

		// Fill in results structure.
		pResults->time = tmin;
		pResults->normal = normal;
		pResults->collidedPoint = GetHitPoint(GetNormal() * (approxHit - GetFm()) * GetInvLength())
			+ pResults->normal * GetRadius();	// Point on our cylinder nearest to the approximate hit point.

		ASSERT( (seg.GetNormal() * pResults->normal) < 0.f );
		return true;
	}
	else if (t0 > tmax)
	{
		// Segment doesn't touch the cylinder.
		return foundHit;
	}
	else
	{
		ASSERT(fisvalid(GetInvLength()));

		Vec3	hitPoint = seg.GetHitPoint(t0);

		// Make a normal perpendicular to this segment's axis.
		Vec3	normal = hitPoint - GetFm();
		normal -= this->GetNormal() * (normal * this->GetNormal());	// subtract out the component parallel to this->GetNormal().
		if ( normal.NormalizeSafe() == 0.f )
		{
			// Happens if the hitPoint is very close to the cylinder
			// axis.  Can happen if our radius is small and the query
			// hits close to the axis.
			//
			// Unfortunately we don't have good normal info, so we
			// must cook something up.  Take the neg of the query
			// normal, and make is perpendicular to our axis.
			normal = -seg.GetNormal();
			normal -= this->GetNormal() * (normal * this->GetNormal());
			//VERIFY(normal.NormalizeSafe() != 0.0f);
			normal.NormalizeFast();
		}

		// @@ cbloom change 6-17-02 :
		if (normal * seg.GetNormal() >= 0.f)
		{
			// Not actually a valid hit; this happens when the query
			// is close to or barely in contact, but pointing away.
			return foundHit;
		}

		// Segment enters cylinder at t0.
		pResults->time = t0;
		pResults->normal = normal;
		pResults->collidedPoint = hitPoint - pResults->normal * seg.GetRadius();

		ASSERT( (seg.GetNormal() * pResults->normal) < 0.f );
		return true;
	}
}

END_CB

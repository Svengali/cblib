#include "Base.h"
#include "Sphere.h"
#include "Frame3.h"
#include "Frame3Scaled.h"
#include "Segment.h"

START_CB

//-----------------------------------------------------------

/*static*/ const Sphere Sphere::unit(Vec3(0.f,0.f,0.f),1.f);
	// do NOT use Vec3::zero here ; cannot use other statics
	//	in initialization of statics
/*static*/ const Sphere Sphere::zero(Vec3(0.f,0.f,0.f),0.f);

//-----------------------------------------------------------

/*static*/ bool Sphere::Equals(const Sphere &a,const Sphere &b,const float tolerance /*= EPSILON*/)
{
	return fequal(a.GetRadius(),b.GetRadius(),tolerance) && Vec3::Equals(a.GetCenter(),b.GetCenter(),tolerance);
}

void Sphere::TransformCenter(const Frame3 & xf)
{
	ASSERT(IsValid() && xf.IsValid());
	m_center = xf.Transform(m_center);
	ASSERT(IsValid());
}

void Sphere::SetTransformed(const Sphere &s,const Frame3Scaled & xf)
{
	ASSERT(s.IsValid() && xf.IsValid());
	m_center = xf.Transform(s.m_center);
	m_radius = xf.GetScale() * s.m_radius;
	ASSERT(IsValid());
}

bool Sphere::ExpandToEnclose(const Sphere &s)
{
	// return whether any work was actually done

	const int chosen = SetEnclosing(*this, s);

	if ( chosen == eSetToSphere1 )
		return false;
	else
		return true;
}

/*! make this sphere the one that encloses spheres 1 and 2
	 s1 or s2 == this is Ok
		 returns a result indicating what work was done;
		 either one of the arguments was selected, or a new sphere
		  was made surrounding both
		eSetToSphere1,
		eSetToSphere2,
		eSetToSphereSurrounding,
*/
Sphere::ESetEnclosingResult Sphere::SetEnclosing(const Sphere &s1,const Sphere &s2)
{
	// s1 or s2 == this is Ok
	if ( s1.Contains(s2) )
	{
		*this = s1;
		return eSetToSphere1;
	}
	else if ( s2.Contains(s1) )
	{
		*this = s2;
		return eSetToSphere2;
	}
	else
	{
		// make the minimal-radius sphere containing s1 and s2

		const float dist = Distance(s1.m_center,s2.m_center);
		
		const float newradius = (dist + s1.m_radius + s2.m_radius) * 0.5f;
		
		ASSERT( newradius >= s1.m_radius );
		ASSERT( newradius >= s2.m_radius );

		const float r = newradius - s1.m_radius;

		ASSERT( r >= 0.f );

		const float ratio = r / dist;

		ASSERT( ratio >= 0.f && ratio <= 1.f );

		//@@dmoore: this can fail on large spheres, expand by unitless epsilon...
		m_radius = newradius * (1 + EPSILON);
		m_center.SetLerp( s1.m_center, s2.m_center , ratio );

		ASSERT(IsValid());

		#ifdef _DEBUG
		{
		Sphere test(*this);
		// Contains has epsilon built in
		//test.Expand( EPSILON );
		ASSERT( test.Contains(s1) );
		ASSERT( test.Contains(s2) );
		}
		#endif

		return eSetToSphereSurrounding;
	}
}

//-----------------------------------------------------------
//! intersecting an LSS with a sphere is just like
//!	intersecting a segment with a bigger sphere

bool Sphere::IntersectVolume(const Segment & seg) const
{
	ASSERT( IsValid() );
	ASSERT( seg.IsValid() );

	// @@ inline ?
	//	this is fast for "Zero" Segments (Spheres) as well
	const float closestDistSqr = seg.ClosestDistSqrToPoint(m_center);

	return ( closestDistSqr <= fsquare(m_radius + seg.GetRadius() + EPSILON) );
}


/** Find the contact, if any, between a segment and this sphere.
	Returns true and fills in *pResult if there's contact; returns
	false and leaves *pResult alone if no contact. */
bool	Sphere::IntersectSurface(const Segment& seg, SegmentResults* pResults) const
{

	ASSERT(IsValid());
	ASSERT(seg.IsValid());
	ASSERT(pResults);

	const float L = seg.GetLength();
	// @@@@ CB - for LSS-Triangle, m_radius is always 0.f
	const float R = seg.GetRadius() + m_radius;
	// @@@@ CB - for LSS-Triangle, this "P" is usually already computed
	const Vec3 P( m_center - seg.GetFm() );
	const Vec3 & N = seg.GetNormal();

	const float dot = P*N;
	
	if ( dot < 0.f )
	{
		// segment is going away from the sphere !
		//	even if we start inside, it's not a collision cuz we're allowed to go away
		return false;
	}
	// this check is not needed, but provided a good quick reject :
	if ( dot > L+R )
	{
		// segment is too short to reach!
		return false;
	}
	
	const float PLenSqr = P.LengthSqr();
	const float RSqr = R*R;
	
	if ( PLenSqr <= RSqr )
	{
		// starts in contact or inside!
		pResults->normal = -P;
		if ( pResults->normal.NormalizeSafe() == 0.f )
		{
			// start point is nearly on the sphere center!
			pResults->normal = - N;
		}
		pResults->collidedPoint = GetCenter() + pResults->normal * GetRadius();
		pResults->time = 0.f;
		return true;
	}
	
	const float dotSqr = dot*dot;
	const float dSqr = PLenSqr - dotSqr;
	const float sSqr = RSqr - dSqr;
	
	//if ( dSqr >= R*R )
	if ( sSqr <= 0.f )
	{
		// @@ (if sSqr == 0.f that's an exactly grazing hit)
		// no hit, ray's closest approach to the sphere is farther than R
		return false;
	}

	// if dotSqr was less than sSqr , we would've started inside the sphere
	ASSERT( dotSqr >= sSqr );
	
	//const float s = sqrtf(sSqr);
	//const float dAlong = dot - s;
	// if ( dAlong > L ) // no hit
	// if ( dot - L > s ) // no hit
	
	// gSegment enters gSphere at t0.
	
	if ( dot > L && fsquare(dot-L) > sSqr )
	{
		// gSegment doesn't reach the gSphere.
		return false;
	}
	else
	{
		// gSegment enters gSphere at dAlong.
		const float s = sqrtf(sSqr);
		const float dAlong = dot - s;
		ASSERT( dAlong <= L );
		pResults->time = dAlong / L;
		pResults->normal = seg.GetHitPoint(pResults->time) - GetCenter();
		pResults->normal.NormalizeSafe();
		pResults->collidedPoint = GetCenter() + pResults->normal * GetRadius();

		return true;
	}
}

//-----------------------------------------------------------

END_CB

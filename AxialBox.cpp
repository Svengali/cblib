#include "cblib/Base.h"
#include "AxialBox.h"
#include "Plane.h"
#include "Segment.h"
#include "Sphere.h"
#include "Frame3.h"
#include "Frame3Scaled.h"
#include "VolumeUtil.h"

START_CB

//-------------------------------------------------------------------------------------------

/*static*/ const AxialBox AxialBox::unitBox(-0.5f,0.5f);

//-------------------------------------------------------------------------------------------

bool AxialBox::IsValid() const
{
	// ASSERT on float validity :
	ASSERT( m_max.IsValid() );
	ASSERT( m_min.IsValid() );
	// consistency condition : max bigger than min !!
	ASSERT( m_max.x >= m_min.x );
	ASSERT( m_max.y >= m_min.y );
	ASSERT( m_max.z >= m_min.z );
	return true;
}

void AxialBox::Transform(const Frame3 & xf)
{
	Rotate( xf.GetMatrix() );
	Translate( xf.GetTranslation() );
}

void AxialBox::Transform(const Frame3Scaled & xf)
{
	Scale( xf.GetScale() );
	Rotate( xf.GetMatrix() );
	Translate( xf.GetTranslation() );
}	

void AxialBox::Rotate(const Mat3 & xf)
{
	ASSERT( IsValid() );

	// the min Frame3ed :
	const Vec3 newMin = xf.Rotate(m_min);

	// the three edges Frame3ed :
	Vec3 vx = xf.GetColumnX();
	Vec3 vy = xf.GetColumnY();
	Vec3 vz = xf.GetColumnZ();

	vx *= (m_max.x - m_min.x);
	vy *= (m_max.y - m_min.y);
	vz *= (m_max.z - m_min.z);

	// same as :
	m_min = m_max = newMin;
	if ( vx.x < 0 ) m_min.x += vx.x; else m_max.x += vx.x;
	if ( vx.y < 0 ) m_min.y += vx.y; else m_max.y += vx.y;
	if ( vx.z < 0 ) m_min.z += vx.z; else m_max.z += vx.z;
	if ( vy.x < 0 ) m_min.x += vy.x; else m_max.x += vy.x;
	if ( vy.y < 0 ) m_min.y += vy.y; else m_max.y += vy.y;
	if ( vy.z < 0 ) m_min.z += vy.z; else m_max.z += vy.z;
	if ( vz.x < 0 ) m_min.x += vz.x; else m_max.x += vz.x;
	if ( vz.y < 0 ) m_min.y += vz.y; else m_max.y += vz.y;
	if ( vz.z < 0 ) m_min.z += vz.z; else m_max.z += vz.z;
	
	ASSERT( IsValid() );
}

//-------------------------------------------------------------------------------------------


/** Sets the box to enclose the specified LSS segment. */
void	AxialBox::SetEnclosing(const Segment& seg)
{
	// it may NOT be valid !!
	//ASSERT( seg.IsValid() );

	// Enclose the line segment.
	m_min = MakeMin(seg.GetFm(), seg.GetTo());
	m_max = MakeMax(seg.GetFm(), seg.GetTo());

	// Expand by the radius.
	Vec3	radvector(seg.GetRadius(), seg.GetRadius(), seg.GetRadius());
	m_min -= radvector;
	m_max += radvector;

	ASSERT( IsValid() );
}


/*!
	GetCorner takes an id from 0 to 7 ; 0 == lo,lo,lo, 7 = hi,hi,hi ,
	and all the other corners are indicated by bit flags {1,2,4}
*/
const Vec3 AxialBox::GetCorner(const int i) const
{
	ASSERT( IsValid() );
	ASSERT( (i&7) == i );

	return Vec3(
			( i & 1 ) ? m_max.x : m_min.x ,
			( i & 2 ) ? m_max.y : m_min.y ,
			( i & 4 ) ? m_max.z : m_min.z 
			);
}

//-------------------------------------------------------------------------------------------

/*! v should be in the box
	makes inBox in 0->1, a range in "box space"
*/
const Vec3 AxialBox::ScaleVectorIntoBox(const Vec3 & v) const
{
	ASSERT( IsValid() );
	// you should've called PadNullDimensions !!
	ASSERT( GetSizeX() > 0.f && GetSizeY() > 0.f && GetSizeZ() > 0.f );
	//ASSERT( Contains(v) );
	return Vec3(
					(v.x - m_min.x) / GetSizeX(),
					(v.y - m_min.y) / GetSizeY(),
					(v.z - m_min.z) / GetSizeZ()
				);
}

/*! inBox should be in 0->1
	makes v in the box's min,max range
*/
const Vec3 AxialBox::ScaleVectorFromBox(const Vec3 & inBox) const
{
	ASSERT( IsValid() );
	return Vec3(
		inBox.x * GetSizeX() + m_min.x,
		inBox.y * GetSizeY() + m_min.y,
		inBox.z * GetSizeZ() + m_min.z
		);
}

//-------------------------------------------------------------------------------------------
/*! Plane functions
	nearly identical to the OrientedBox versions,
	uses GetRadiusInDirection
*/
float AxialBox::PlaneDist(const Plane & plane) const
{
	ASSERT( IsValid() && plane.IsValid() );
	const Vec3 center( GetCenter() );
	const float distToCenter = plane.DistanceToPoint(center);

	const Vec3 & normal = plane.GetNormal();
	const float radius = GetRadiusInDirection( normal );

	if ( distToCenter > radius )
		return distToCenter - radius;
	else if ( distToCenter < - radius )
		return distToCenter + radius;
	else
		return 0.f;
}

Plane::ESide AxialBox::PlaneSide(const Plane & plane) const
{
	ASSERT( IsValid() && plane.IsValid() );
	const Vec3 center( GetCenter() );
	const float distToCenter = plane.DistanceToPoint(center);

	const Vec3 & normal = plane.GetNormal();
	// expand radius by epsilon to err on the side of returning more Intersecting results
	const float radius = GetRadiusInDirection( normal ) + EPSILON;

	if ( distToCenter > radius )
		return Plane::eFront;
	else if ( distToCenter < - radius )
		return Plane::eBack;
	else
		return Plane::eIntersecting;
}

//-------------------------------------------------------------------------------------------

/*!
PadNullDimensions makes sure that no axis of the box is degenerate; it
tries to make sure that it is at least minSize in thickness
AP: Currently this tries to make it at least 2x minSize in thickness
*/
void AxialBox::PadNullDimensions(const float minSize /*= EPSILON*/)
{
	ASSERT( IsValid() && minSize >= 0.f );
	for(int i=0;i<3;i++)
	{
		if ( GetSize(i) < minSize )
		{
			m_min[i] -= minSize;
			m_max[i] += minSize;

			ASSERT( m_min[i] != m_max[i] );
		}
	}
}

void AxialBox::GrowToCube()
{
	ASSERT( IsValid() );

	Vec3 diagonal = m_max - m_min;

	float maxDim = MAX3( diagonal.x, diagonal.y, diagonal.z );

	for(int i=0;i<3;i++)
	{
		float d = diagonal[i];
		float delta = (maxDim - d)*0.5f;
		m_max[i] += delta;
		m_min[i] -= delta;
	}

	//ASSERT( fequal(GetVolume(),fcube(maxDim)) );
}

//-------------------------------------------------------------------------------------------
// IntersectVolume
//	just does an LSS-Box touch test
//

bool AxialBox::IntersectVolume_Woo(const Segment & seg) const
{
	// tulrich: I think there's a bug in here -- long segments get
	// rejected by the radius check (marked below with "XXX") even
	// though they do intersect the box.
	ASSERT(0);

	ASSERT( IsValid() && seg.IsValid() );

	// find the closest point on the box to the segment
	//
	// I don't really like the special role of the "fm" point and the "front" face here
	//	I'd like to have just a simple symmetric distance computation
	// -> you can do that with a separating-axis test; see the OBB code; could easily
	//	use that code here with optimizations for knowing that the axes are aligned
	// -> the LSS version is not really any more expensive than the ray version;
	//	it's just got more thorough handling of the epsilon cases, which are rare
	//
	// Original code by Andrew Woo, from "Graphics Gems", Academic Press, 1990

	const Vec3 & fm = seg.GetFm();

	bool isInside = true;	
	Vec3 MaxT(-1,-1,-1); // anything < 0
	int i;

	// Find candidate planes.
	// unrolled
	{
		if ( fm.x < m_min.x )
		{
			isInside = false;
			MaxT.x = (m_min.x - fm.x) * seg.GetInvNormal().x;
		}
		else if ( fm.x > m_max.x )
		{
			isInside = false;
			MaxT.x = (m_max.x - fm.x) * seg.GetInvNormal().x;
		}
	
		if ( fm.y < m_min.y )
		{
			isInside = false;
			MaxT.y = (m_min.y - fm.y) * seg.GetInvNormal().y;
		}
		else if ( fm.y > m_max.y )
		{
			isInside = false;
			MaxT.y = (m_max.y - fm.y) * seg.GetInvNormal().y;
		}
	
		if ( fm.z < m_min.z )
		{
			isInside = false;
			MaxT.z = (m_min.z - fm.z) * seg.GetInvNormal().z;
		}
		else if ( fm.z > m_max.z )
		{
			isInside = false;
			MaxT.z = (m_max.z - fm.z) * seg.GetInvNormal().z;
		}
	}

	// Ray origin inside bounding box
	if ( isInside )
	{
		return true;
	}

	// we've now identified three front-facing faces of the box
	//  the largest distance to those three plances is the one
	//	we actually hit
	// Get largest of the maxT's for final choice of intersection
	int WhichPlane = 0;
	if ( MaxT.y > MaxT[WhichPlane] ) WhichPlane = 1;
	if ( MaxT.z > MaxT[WhichPlane] ) WhichPlane = 2;
	
	// this largest distance is the length of the ray to hit the box :
	float hitDist = MaxT[WhichPlane];
	const float radius = seg.GetRadius();

	// XXX tulrich: I think this check is wrong -- incorrectly rejects long segments that barely reach the box
	if ( hitDist < - radius || hitDist > seg.GetLength() + radius )
	{
		return false;
	}

	Vec3 vHit;
	// Check final candidate actually inside box
	//  fire the ray at the box and see if it actually hits it
	//	if you have a mostly-Z ray, you only need to test XY extents, etc.
	bool outSide = false;
	for(i=0;i<3;i++)
	{
		if ( i != WhichPlane )
		{
			float val = fm[i] + hitDist * seg.GetNormal()[i];

			if ( val < m_min[i] )
			{
				outSide = true;
				vHit[i] = m_min[i];
			}
			else if ( val > m_max[i] )
			{
				outSide = true;
				vHit[i] = m_max[i];
			}
			else
			{
				vHit[i] = val;
			}
		}
	}

	if ( ! outSide )
	{
		// hit the face !
		return true;
	}

	if ( fm[WhichPlane] < m_min[WhichPlane] )
	{
		vHit[WhichPlane] = m_min[WhichPlane];
	}
	else
	{
		vHit[WhichPlane] = m_max[WhichPlane];
	}

	// missed the face :
	//  vHit is the closest point on the box to the segment :
	// @@@@ : I'm not sure about this !!!

	const float closestDistSqr = seg.ClosestDistSqrToPoint(vHit);

	return ( closestDistSqr <= fsquare(radius) );
}


//-------------------------------------------------------------------------------------------

// _SeparatingAxis
bool AxialBox::IntersectVolume(const Segment & seg) const
{
	ASSERT( IsValid() && seg.IsValid() );
	// use separating axis between an LSS and an AABB
	//
	// see notes in OBB
	//

	if ( seg.IsZero() )
	{
		return cb::Intersects(*this,Sphere(seg.GetFm(),seg.GetRadius()));
	}

	const Vec3 & segNormal = seg.GetNormal();
	const Vec3 centerBoxMinusSeg( seg.GetCenter() - GetCenter() );

	//@@ ? necessary ? even if not, it's a good quick reject
	// first test the segment normal axis :
	{
		const float separationOnAxis = centerBoxMinusSeg * segNormal;

		const float radiusOnAxis = seg.GetLength() * 0.5f + seg.GetRadius() + GetRadiusInDirection(segNormal);
		
		// @@ CB - might be faster to use Pierre's int-style fabs & compare
		if ( fabsf(separationOnAxis) > radiusOnAxis )
		{
			// centers are further than sum of radii : that's a no-overlap
			return false;
		}
	}

	// these first three tests are just segment-plane tests
	// (these are the three axes of the box)

	int i;
	for(i=0;i<3;i++)
	{
		//const Vec3 & boxAxis = m_axes.GetRow(i);
		// boxAxis = unit[i]

		// project box and segment onto boxAxis
		const float separationOnAxis = centerBoxMinusSeg[i];

		// Seg : return fabsf(m_normal * dir) * m_length * 0.5f + m_radius;

		const float radiusOnAxis = seg.GetRadiusOnAxis(i) + (m_max[i] - m_min[i])*0.5f;

		if ( fabsf(separationOnAxis) > radiusOnAxis )
		{
			// centers are further than sum of radii : that's a no-overlap
			return false;
		}
	}

	// these are the three crosses of the box faces with segment normal :

	static const int plus1mod3[3] = { 1, 2, 0 };
	static const int plus2mod3[3] = { 2, 0, 1 };
	for(i=0;i<3;i++)
	{
		//const Vec3 & boxAxis = m_axes.GetRow(i);
		// boxAxis = unit[i]

		// now check cross of axis with seg normal :
		//const Vec3 crossAxis( boxAxis ^ segNormal );
		const int i1 = plus1mod3[i];
		const int i2 = plus2mod3[i];
		Vec3 crossAxis;
		crossAxis[i ] = 0.f;
		crossAxis[i1] = - segNormal[i2];
		crossAxis[i2] =   segNormal[i1];

		/*
		const float crossAxisLen = crossAxis.Length();
		// sqrt !! bad !!
		// all the "distances" in here are actually scaled by crossAxisLen

		// project box and segment onto crossAxis
		const float separationOnAxis = crossAxis * centerBoxMinusSeg;

		const float radiusOnAxis = seg.GetRadius() * crossAxisLen + GetRadiusInDirection(crossAxis);

		if ( fabsf(separationOnAxis) > radiusOnAxis )
		{
			// centers are further than sum of radii : that's a no-overlap
			return false;
		}
		*/
		
		//	8-02-04 CB :
		// rearrange and square both sides to eliminte a sqrt :
		// @@ could optimize a tiny bit more by using the knowledge that crossAxis always has one zero component,
		//		so we can do two multiplies in the dots instead of three
		//	could also unroll the loop to use hard-coded indexes instead of variables
				
		// project box and segment onto crossAxis
		const float separationOnAxis = crossAxis * centerBoxMinusSeg;
		
		const float lhs = fabsf(separationOnAxis) - GetRadiusInDirection(crossAxis);
		
		// since we're comparing squares, that does an implicit fabs, so we have to also check the sign
		if ( lhs > 0.f )
		{
			//const float crossAxisLenSqr = crossAxis.LengthSqr();
			const float crossAxisLenSqr = fsquare(segNormal[i1]) + fsquare(segNormal[i2]);
			const float rhs_sqr = fsquare( seg.GetRadius() ) * crossAxisLenSqr;
			
			// since we're comparing squares, that does an implicit fabs, so we have to also check the sign
			if ( fsquare(lhs) > rhs_sqr )
			{
				// centers are further than sum of radii : that's a no-overlap
				return false;
			}
		}
	}

	// no direction rules me out :
	return true;
}

//-------------------------------------------------------------------------------------------


//! IntersectSurface
//!	intersect an LSS with the surface of an axial box
bool AxialBox::IntersectSurface(const Segment& seg, SegmentResults * pResults) const
{
	// @@ UNTESTED

	ASSERT(seg.IsValid());

	// Translate the from & to points so we can do our tests as if the
	// box is centered at the origin.
	Vec3	from(seg.GetFm() - GetCenter());
	Vec3	to(seg.GetTo() - GetCenter());

	ASSERT(IsValid());
	ASSERT(pResults);

	// Clip segment against the box, expanded by LSS radius.
	//
	// I.e. we're intersecting a thin ray against an expanded box,
	// instead of a thick ray against a box.

	int	clipAxis = -1;	// remember which axis we clipped against last.
	float	clipDir = 0;	// set to -1 or 1 according to which side of the box the segment was last clipped against.

	// [tmin,tmax] is the interval during which the ray is inside the expanded box.
	// t is parameterized so that p = t*ray_dir + ray_start, i.e. the ray extent is [0,length]
	float	tmin = 0;
	float	tmax = seg.GetLength();

	for (int axis = 0; axis < 3; axis++)
	{
		float axisLimit = GetExtent()[axis] + seg.GetRadius();

		if (seg.GetNormal()[axis] > 0)
		{
			// segment approaches box from the negative side.

			float	tenter = (-axisLimit - from[axis]) * seg.GetInvNormal()[axis];
			float	texit = (axisLimit - from[axis]) * seg.GetInvNormal()[axis];

			if (tenter > tmin)
			{
				tmin = tenter;

				clipAxis = axis;
				clipDir = -1;
			}

			if (texit < tmax)
			{
				tmax = texit;
			}
		}
		else if (seg.GetNormal()[axis] < 0)
		{
			// LSS approaches box from the positive side.

			float	tenter = (axisLimit - from[axis]) * seg.GetInvNormal()[axis];
			float	texit = (-axisLimit - from[axis]) * seg.GetInvNormal()[axis];

			if (tenter > tmin)
			{
				tmin = tenter;

				clipAxis = axis;
				clipDir = 1;
			}

			if (texit < tmax)
			{
				tmax = texit;
			}
		}
		else
		{
			// Segment is practically perpendicular to this axis of the box.

			if (from[axis] < -axisLimit || from[axis] > axisLimit)
			{
				// no possible hit.
				return false;
			}
		}
	}

	if (tmin > tmax)
	{
		// no possible hit.
		return false;
	}

	if (clipAxis == -1)
	{
		// Didn't clip against any axes, and didn't reject completely.
		// That means the from point is inside the expanded box.

		// @@ tulrich: this is probably incomplete...  should return a
		// hit if query is moving towards the interior of the box?

		// @@ IMPORTANT TODO: Actually, also need to test if the from
		// point is inside the *unexpanded* box.  If so, then we can
		// arguably bail here.  If not, then we need to do edge checks
		// -- the from point could be inside the expanded box, but
		// still outside the box+sphere, in the space around edges &
		// corners.

		// Since we're doing one-sided collision, don't return a hit
		// -- try to allow query to exit the box.
		return false;
	}

	// Advance the from[] value up to tmin
	from += seg.GetNormal() * tmin;

	// The segment from point is now clipped to the expanded box.
	// Check it to see if it's on a face or near an edge.  If it's
	// near an edge, then we want to identify that edge and query the
	// segment against it.

	Vec3	edgeSpec(Vec3::zero);
	int	edgeSpecCount = 0;
	if (seg.GetRadius() > 0)
	{
		for (int axis = 0; axis < 3; axis++)
		{
			if (from[axis] >= -GetExtent()[axis])
			{
				if (from[axis] <= GetExtent()[axis])
				{
					// OK on this axis.
					// edgeSpec[axis] = 0;
				}
				else
				{
					// check edge towards axis +
					edgeSpec[axis] = 1;
					edgeSpecCount++;
				}
			}
			else
			{
				// check edge towards axis -
				edgeSpec[axis] = -1;
				edgeSpecCount++;
			}
		}
	}

	if (edgeSpecCount <= 1)
	{
		// Face hit.

		from += GetCenter();

		pResults->time = tmin * seg.GetInvLength();
		pResults->time = fclampunit(pResults->time);
		pResults->normal = Vec3::zero;
		ASSERT(clipDir != 0);
		pResults->normal[clipAxis] = clipDir;
		pResults->collidedPoint = from;
		pResults->collidedPoint[clipAxis] -= clipDir * seg.GetRadius();	// shift the point back onto the surface of the original (unexpanded) box

		return true;
	}
	else if (edgeSpecCount == 2)
	{
		// Segment enters through a corner of the expanded box, so the
		// first contact, if any, will be near an edge.  We only need
		// to check against the single nearest edge, because a capped
		// cylinder test also includes the box vertices.

		// Construct a segment to use for the edge query.
		Vec3	edgeFrom, edgeTo;
		for (int i = 0; i < 3; i++)
		{
			if (edgeSpec[i] == 0)
			{
				edgeFrom[i] = GetExtent()[i];
				edgeTo[i] = -GetExtent()[i];
			}
			else
			{
				edgeFrom[i] = edgeTo[i] = GetExtent()[i] * edgeSpec[i];
			}
		}
		edgeFrom += GetCenter();
		edgeTo += GetCenter();

		Segment	edge(edgeFrom, edgeTo, EPSILON);
		// @@ Actually I really want a ZERO-radius segment.  Probably
		// the thing to do is make a LineSegmentIntersectSurface() function,
		// instead of using Segment::IntersectSurface().

		if (edge.IsZero())
		{
			// Segment too short!  Fall back on sphere test.
			Sphere	sphere(edgeFrom, EPSILON);
			return sphere.IntersectSurface(seg, pResults);
		}

		return edge.IntersectSurface(seg, pResults);
	}
	else
	{
		// Segment enters through a corner.
		Vec3	corner = GetCorner(
			((edgeSpec[0] == 1) ? 1 : 0)
			| ((edgeSpec[1] == 1) ? 2 : 0)
			| ((edgeSpec[2] == 1) ? 4 : 0));
		Sphere	sphere(corner, EPSILON);
		return sphere.IntersectSurface(seg, pResults);
	}
}

END_CB

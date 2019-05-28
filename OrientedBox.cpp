#include "cblib/Base.h"
#include "OrientedBox.h"
#include "Segment.h"
#include "Frame3.h"
#include "Plane.h"
#include "Sphere.h"
#include "Mat3Util.h"

#include <stdlib.h>
//#include "gOBBCompact.h"

#include "cblib/VolumeUtil.h"

START_CB

//==================================================================

/*static*/ const OrientedBox OrientedBox::unitBox(eUnitBox);

//==================================================================

// this version takes the "Box To World" Matrix - eg. not transposed
//	(it does a transpose, so it's not as fast)
OrientedBox::OrientedBox(EBoxToWorldMatrix e,const Mat3 & mat,const Vec3 & center,const Vec3 & radii) :
	m_center(center), m_radii(radii)
{
	ASSERT( mat.IsOrthonormal() );
	GetTranspose(mat,&m_axes);	
}
	
bool OrientedBox::IsValid() const
{
	ASSERT( m_axes.IsValid() && m_radii.IsValid() && m_center.IsValid() );
	ASSERT( m_radii.x >= 0.f && m_radii.y >= 0.f && m_radii.z >= 0.f );
	ASSERT( m_axes.IsOrthonormal() );
	return true;
}

void OrientedBox::SetBoxToWorldMatrix(const Mat3 & mat)
{
	ASSERT( mat.IsOrthonormal() );
	GetTranspose(mat,&m_axes);
	// whole OBB may not be valid yet
}

void OrientedBox::Rotate(const Mat3 & m)
{
	ASSERT(IsValid());
	ASSERT(m.IsOrthonormal());
	Mat3 mt;
	GetTranspose(m,&mt);
	m_axes.LeftMultiply(mt);
}

/*!
	GetCorner takes an id from 0 to 7 ; 0 == lo,lo,lo, 7 = hi,hi,hi ,
	and all the other corners are indicated by bit flags {1,2,4}
*/
const Vec3 OrientedBox::GetCorner(const int i) const
{
	ASSERT( IsValid() );
	ASSERT( (i&7) == i );

	const Vec3 unitCorner(
			( i & 1 ) ? m_radii.x : - m_radii.x ,
			( i & 2 ) ? m_radii.y : - m_radii.y ,
			( i & 4 ) ? m_radii.z : - m_radii.z 
			);

	return TransformBoxToWorld(unitCorner);
}

//==================================================================

const Vec3 OrientedBox::GetClampInside(const Vec3 & v) const
{
	ASSERT( IsValid() );

	//! @@ \todo could surely be more efficient, avoiding the two transforms

	const Vec3 vInBox = TransformWorldToBox(v);

	const Vec3 vClampedInBox(
			fclamp(vInBox.x,-m_radii.x,m_radii.x),
			fclamp(vInBox.y,-m_radii.y,m_radii.y),
			fclamp(vInBox.z,-m_radii.z,m_radii.z)
		);

	return TransformBoxToWorld(vClampedInBox);
}

float OrientedBox::DistanceSqr(const Vec3 & v) const
{
	ASSERT( IsValid() );
	/*
	const Vec3 v_box = Frame3WorldToBox(v);
	
	constVec3 v_clamped_in_box(
			fclamp(v_box.x,-m_radii.x,m_radii.x),
			fclamp(v_box.y,-m_radii.y,m_radii.y),
			fclamp(v_box.z,-m_radii.z,m_radii.z)
		);

	return v_clamped_in_box.DistanceToSqr(v_box);
	*/

	// accumulate dSqr in the 3 axes
	float dSqr = 0.f;

	const Vec3 vMinusCenter = v - m_center;
	for(int i=0;i<3;i++)
	{
		const float alongAxis = vMinusCenter * m_axes.GetRow(i);
		const float r = m_radii[i];
		if ( alongAxis > r )
			dSqr += fsquare(alongAxis - r);
		else if ( alongAxis < -r )
			dSqr += fsquare(alongAxis + r);
	}

	return dSqr;
}

bool OrientedBox::Contains(const Vec3 & v) const
{
	ASSERT( IsValid() );

	/*
	// clear but inefficient :
	Vec3 v_box = Frame3WorldToBox(v);
	v_box.Abs();
	return ( v_box.x <= m_radii.x && v_box.y <= m_radii.y && v_box.z <= m_radii.z );
	*/
	
	const Vec3 vMinusCenter = v - m_center;
	for(int i=0;i<3;i++)
	{
		const float alongAxis = vMinusCenter * m_axes.GetRow(i);

		//! @@ \todo be a bit greedy about saying that people right on my surface are inside me ?
		//!	by expanding r by EPSILON ?
		const float r = m_radii[i];

		if ( fabsf(alongAxis) > r )
			return false;
	}

	return true;
}

bool OrientedBox::Contains(const Vec3 & v,const float tolerance) const
{
	ASSERT( IsValid() );
	
	const Vec3 vMinusCenter = v - m_center;
	for(int i=0;i<3;i++)
	{
		const float alongAxis = vMinusCenter * m_axes.GetRow(i);

		// be a bit greedy about saying that people right on my surface are inside me ?
		// 	by expanding r by EPSILON ?
		const float r = m_radii[i] + tolerance;

		if ( fabsf(alongAxis) > r )
			return false;
	}

	return true;
}

void OrientedBox::ExtendToPoint(const Vec3 & v)
{
	ASSERT( IsValid() );
	
	// extend each of the three radii to contain "v"

	const Vec3 vMinusCenter = v - m_center;
	for(int i=0;i<3;i++)
	{
		const float alongAxis = vMinusCenter * m_axes.GetRow(i);
		const float r = m_radii[i];
		const float absAlongAxis = fabsf(alongAxis);
		m_radii[i] = MAX(r,absAlongAxis);
	}
}

// which = 1,2,3, and negative
//	makes a plane for a face, pointing out
void OrientedBox::GetPlane(Plane * pPlane,const EBoxFace eWhich) const
{
	ASSERT( pPlane );
	ASSERT( IsValid() );

	const int which = (int) eWhich;

	ASSERT( abs(which) >= 1 && abs(which) <= 3 );

	const int coord = abs(which) - 1;
	const bool isNegative = (which < 0);
	const float sign = isNegative ? -1.f : 1.f;
	
	pPlane->SetFromNormalAndPoint(m_axes.GetRow(coord) * sign,m_center);
	pPlane->MoveForwards(m_radii[coord]);
}

//==================================================================
// Plane functions
//	nearly identical to the AxialBox versions,
//	uses GetRadiusInDirection

float OrientedBox::PlaneDist(const Plane & plane) const
{
	ASSERT(IsValid());
	const Vec3 & center = GetCenter();
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

Plane::ESide OrientedBox::PlaneSide(const Plane & plane) const
{
	ASSERT(IsValid());
	const Vec3 & center = GetCenter();
	const float distToCenter = plane.DistanceToPoint(center);

	const Vec3 & normal = plane.GetNormal();
	const float radius = GetRadiusInDirection( normal );

	if ( distToCenter > radius )
		return Plane::eFront;
	else if ( distToCenter < - radius )
		return Plane::eBack;
	else
		return Plane::eIntersecting;
}

//==================================================================

/*
bool OrientedBox::IntersectVolume(const Segment & seg_world) const
{
	ASSERT(IsValid());
	// 593 clocks
	// not the most efficient; just adapt this to AABB

	const AxialBox ab( m_center - m_radii , m_center + m_radii );

	// rotate the segment to box-space
	//	m_axes is world-to-box
	const Vec3 fm_box = m_axes.Rotate(seg_world.GetFm());
	const Vec3 to_box = m_axes.Rotate(seg_world.GetTo());

	const Segment seg_box(fm_box,to_box,seg_world.GetRadius());

	return ab.IntersectVolume(seg_box);
}
*/

bool OrientedBox::IntersectVolume(const Segment & seg) const
{
	ASSERT(IsValid() && seg.IsValid());
	// use separating axis between an LSS and an OBB
	// we must check 6 axes : the three faces of the box, and
	//	the three faces crossed with the segment normal :
	//
	// 600 clocks when you hit, 140 when you miss
	//
	// assembly shows it's all inlined nicely

	if ( seg.IsZero() )
	{
		return Intersects(*this,Sphere(seg.GetFm(),seg.GetRadius()));
	}

	const Vec3 & segNormal = seg.GetNormal();
	const Vec3 centerBoxMinusSeg = seg.GetCenter() - m_center;

	//@@ ? necessary ?
	// first test the segment normal axis :
	{
		const float separationOnAxis = centerBoxMinusSeg * segNormal;

		const float radiusOnAxis = seg.GetLength() * 0.5f + seg.GetRadius() + GetRadiusInDirection(segNormal);
		
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
		const Vec3 & boxAxis = m_axes.GetRow(i);

		// project box and segment onto boxAxis
		const float separationOnAxis = boxAxis * centerBoxMinusSeg;

		const float radiusOnAxis = seg.GetRadiusInDirection(boxAxis) + m_radii[i];

		if ( separationOnAxis > radiusOnAxis ||
			 separationOnAxis <-radiusOnAxis )
		{
			// centers are further than sum of radii : that's a no-overlap
			return false;
		}
	}

	for(i=0;i<3;i++)
	{
		const Vec3 & boxAxis = m_axes.GetRow(i);

		// now check cross of axis with seg normal :
		const Vec3 crossAxis = MakeCross( boxAxis , segNormal );
		const float crossAxisLen = crossAxis.Length();
		// sqrt !! bad !!
		// all the "distances" in here are actually scaled by crossAxisLen

		// project box and segment onto crossAxis
		const float separationOnAxis = crossAxis * centerBoxMinusSeg;

		const float radiusOnAxis = seg.GetRadius() * crossAxisLen + GetRadiusInDirection(crossAxis);

		if ( separationOnAxis > radiusOnAxis ||
			 separationOnAxis <-radiusOnAxis )
		{
			// centers are further than sum of radii : that's a no-overlap
			return false;
		}
	}

	// no direction rules me out :
	return true;
}

//==================================================================

//namespace OrientedBoxUtil
//{

static __forceinline bool TestBoxFaceSeparatingAxis(const OrientedBox & ob1,const OrientedBox & ob2,
												const int iAxis1,const Vec3 & center1to2)
{
	// test face of ob1 against ob2 :
	const Vec3 & axis = ob1.GetMatrix().GetRow(iAxis1);
	ASSERT( axis.IsNormalized() );

	const float separationOnAxis = axis * center1to2;
	
	const float radiusOnAxis = ob1.GetRadii()[iAxis1] + ob2.GetRadiusInDirection(axis);

	// centers are further than sum of radii : that's a no-overlap
	return ( fabsf(separationOnAxis) > radiusOnAxis );
}

bool Intersects(const OrientedBox & ob1,const OrientedBox & ob2)
{
	ASSERT( ob1.IsValid() && ob2.IsValid() );
	// separating axis test :
	//	6 face normals
	//	9 edge x edge axes

	const Vec3 center1to2 = ob2.GetCenter() - ob1.GetCenter();

	int i;
	for(i=0;i<3;i++)
	{
		if ( TestBoxFaceSeparatingAxis(ob1,ob2,i,center1to2) )
			return false;
		
		if ( TestBoxFaceSeparatingAxis(ob2,ob1,i,center1to2) )
			return false;
	}

	// now test all the cross directions :
	for(i=0;i<3;i++)
	{
		for(int j=0;j<3;j++)
		{
			const Vec3 crossAxis = MakeCross( ob1.GetMatrix().GetRow(i) , ob2.GetMatrix().GetRow(j) );
			
			const float separationOnAxis = crossAxis * center1to2;
			
			const float radiusOnAxis = ob1.GetRadiusInDirection(crossAxis) + ob2.GetRadiusInDirection(crossAxis);

			// if the axes are degenerate (eg. i == j on an axis-aligned box)
			//	then crossAxis will be zero, and separationOnAxis and radiusOnAxis
			//	will both be zero
			// so : do not return false if both are zero; hence do not test >= !!

			if ( fabsf(separationOnAxis) > radiusOnAxis )
			{
				// centers are further than sum of radii : that's a no-overlap
				return false;
			}
		}
	}

	return true;
}

bool IntersectsRough_FaceNormals(const OrientedBox & ob1,const OrientedBox & ob2)
{
	ASSERT( ob1.IsValid() && ob2.IsValid() );

	// separating axis test :
	//	6 face normals
	//	9 edge x edge axes
	// IntersectsRough_FaceNormals is an approximate version that just does the 6 face normals
	//	and skips the other 9 tests
	//	it will fail to return non-overlap occasionally

	const Vec3 center1to2 = ob2.GetCenter() - ob1.GetCenter();

	int i;
	for(i=0;i<3;i++)
	{
		if ( TestBoxFaceSeparatingAxis(ob1,ob2,i,center1to2) )
			return false;
		
		if ( TestBoxFaceSeparatingAxis(ob2,ob1,i,center1to2) )
			return false;
	}

	return true;
}

bool IntersectsRough_SphereLike(const OrientedBox & ob1,const OrientedBox & ob2)
{
	ASSERT( ob1.IsValid() && ob2.IsValid() );
	// separating axis test :
	//	just along the axis between the centers

	const Vec3 center1to2 = ob2.GetCenter() - ob1.GetCenter();

	const float separationOnAxis = center1to2.LengthSqr();
	
	const float radiusOnAxis = ob1.GetRadiusInDirection(center1to2) + ob2.GetRadiusInDirection(center1to2);
	ASSERT( radiusOnAxis >= 0.f );

	// center1to2 is NOT normalized
	// so radiusOnAxis is scaled up by Length(center1to2)
	// but separationOnAxis is actually the square of Length(center1to2)
	// so the overall factor of Length(center1to2) divides out

	return ( separationOnAxis <= radiusOnAxis );
}

/*******************

8-11-01 :
measurement with some random boxes :

accurate    : 41.8 % hits : 848.2 clocks
FaceNormals : 43.3 % hits : 348.3 clocks : 2.5 % failure
SphereLike  : 54.3 % hits : 184.2 clocks : 21.5 % failure

Obviously the exact numbers depend a lot on the specific
boxes you throw at it, but this is a pretty good rough
report.

The Failure percentage indicates the fraction of possible
misses which were incorrectly marked as hits.

Conclusions :

FaceNormals is *very* accurate and quite a bit faster than
the full test.

********************/

//};

//==================================================================

END_CB

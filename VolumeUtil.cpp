#include "Base.h"
#include "VolumeUtil.h"
#include "Mat3.h"
#include "Frame3Scaled.h"
#include "Sphere.h"
#include "AxialBox.h"
#include "Cylinder.h"
#include "OrientedBox.h"
#include "Segment.h"
#include "Mat3Util.h"

START_CB

//namespace VolumeUtil
//{

//---------------------------------------------------------------------------------

//! the "Sphere" is treated as a cube; the "Cube" contains the actual "sphere"
//!	 you may think of this as an approximation of the AxialBox-Sphere test
bool BoxIntersectsCube(const AxialBox & ab, const Sphere & cube)
{
	return	( ab.GetMin().x <= cube.GetCenter().x + cube.GetRadius() && ab.GetMax().x >= cube.GetCenter().x - cube.GetRadius() ) &&
			( ab.GetMin().y <= cube.GetCenter().y + cube.GetRadius() && ab.GetMax().y >= cube.GetCenter().y - cube.GetRadius() ) &&
			( ab.GetMin().z <= cube.GetCenter().z + cube.GetRadius() && ab.GetMax().z >= cube.GetCenter().z - cube.GetRadius() );
}

//---------------------------------------------------------------------------------

void MakeBoxAroundSphere( AxialBox * pb, const Sphere & s)
{
	ASSERT(pb);
	const float radius = s.GetRadius();
	const Vec3 vRadius( radius, radius, radius ); 
	pb->SetMin( s.GetCenter() - vRadius );
	pb->SetMax( s.GetCenter() + vRadius );
	ASSERT( pb->IsValid() );
}

void MakeSphereAroundBox( Sphere * ps, const AxialBox & b)
{
	ASSERT(ps);
	ASSERT( b.IsValid() );
	ps->SetCenter( b.GetCenter() );
	ps->SetRadius( sqrtf(b.GetRadiusSqr()) );
	ASSERT( ps->IsValid() );
}

void MakeSphereAroundBox( Sphere * ps, const OrientedBox & b)
{
	ASSERT(ps);
	ASSERT( b.IsValid() );
	ps->SetCenter( b.GetCenter() );
	ps->SetRadius( sqrtf(b.GetRadiusSqr()) );
	ASSERT( ps->IsValid() );
}

void MakeSphereAroundSegment( Sphere* ps, const Segment& s)
{
	ASSERT(ps);
	ASSERT( s.IsValid() );
	ps->SetCenter((s.GetFm() + s.GetTo()) * 0.5f);
	ps->SetRadius(s.GetLength() * 0.5f + s.GetRadius());
	ASSERT(ps->IsValid());
}


void MakeBoxAroundOrientedBox( AxialBox * pab, const OrientedBox & ob)
{
	ASSERT(pab);

	Mat3 mt;
	GetTranspose( ob.GetMatrix() , &mt );
	// mt is the box-to-world Transform

	const Vec3 & r = ob.GetRadii();
	const Vec3 & c = ob.GetCenter();

	// same ?	
	//const Vec3 rw = Vec3U::MakeAbs( mt * r );
	//pab->Set( c - rw, c + rw );

	pab->SetMin( - r );
	pab->SetMax( r );
	pab->Rotate( mt );
	pab->Translate( c );
	
	ASSERT( pab->IsValid() );
}

void MakeOrientedBoxFromAxialBox( OrientedBox * pob, const AxialBox & ab)
{
	ASSERT(pob);
	pob->SetMatrixToIdentity();
	pob->SetCenter( ab.GetCenter() );
	pob->SetRadii( ab.GetDiagonal() * 0.5f );
	ASSERT( pob->IsValid() );
}

void MakeOrientedBoxFromAxialBox( OrientedBox * pob, const AxialBox & ab, const Mat3 & mat)
{
	ASSERT(pob);
	pob->SetCenter( ab.GetCenter() );
	pob->SetRadii( ab.GetDiagonal() * 0.5f );
	pob->SetBoxToWorldMatrix(mat);
	ASSERT( pob->IsValid() );
}


/** Given an AxialBox in local coordinates, which is placed in the
    world with the given xfs, this function sets *pob to the
    corresponding oriented box, in world coords. */
void MakeOrientedBoxFromLocalAxialBox( OrientedBox * pob, const AxialBox & ab, const Frame3Scaled& xfs)
{
	ASSERT(pob);
	pob->SetCenter( xfs.Transform(ab.GetCenter()) );
	pob->SetRadii( ab.GetDiagonal() * 0.5f * xfs.GetScale() );
	pob->SetBoxToWorldMatrix(xfs.GetMatrix());	// @@ check!
	ASSERT( pob->IsValid() );
}



//---------------------------------------------------------------------------------

bool Intersects(const Sphere &s, const OrientedBox & b)
{
	const float distSqr = b.DistanceSqr(s.GetCenter());
	return ( distSqr <= fsquare(s.GetRadius()) );
}

bool Intersects(const Sphere &s, const AxialBox & b)
{
	const float distSqr = b.DistanceSqr(s.GetCenter());
	return ( distSqr <= fsquare(s.GetRadius()) );
}

bool Intersects(const AxialBox & ab,const OrientedBox &ob)
{
	//! \todo @@ very inefficient temp implementation; improve if used !
	OrientedBox obForAB;
	MakeOrientedBoxFromAxialBox(&obForAB,ab);
	return Intersects(ob,obForAB);
}

/*
bool Intersects(const Cylinder &s, const Sphere & b)
{
	float dSqr = s.GetDistanceSqr(b.GetCenter());
	return dSqr <= fsquare( b.GetRadius() );
}
*/
	
//---------------------------------------------------------------------------------

//! Box contains Sphere
bool BoxContainsSphere(const AxialBox & b, const Sphere &s)
{
	// @@ UNTESTED !

	// Sphere center must be inside radius of the box :
	
	const float radius = s.GetRadius();
	

	// IsValid will check to see if the box was too small, so that
	//	the new max is not greater than the min
	
	//isvalid asserts, that'll cause BCS tests to assert if the box is smaller than the Sphere. 
	//if ( ! smallerb.IsValid() ) 
	//	return false;

	//I think want we want is this logic from the IsValid:
	//ASSERT( m_max.x >= m_min.x );
	//ASSERT( m_max.y >= m_min.y );
	//ASSERT( m_max.z >= m_min.z );

	const Vec3 smallerBMin(b.GetMin() + Vec3(radius,radius,radius));
	const Vec3 smallerBMax(b.GetMax() - Vec3(radius,radius,radius));
	if( smallerBMin[0] > smallerBMax[0] || 
		smallerBMin[1] > smallerBMax[1] || 
		smallerBMin[2] > smallerBMax[2])
	{
		return false;
}

	const AxialBox smallerb( smallerBMin,smallerBMax );
	ASSERT( smallerb.GetVolume() < b.GetVolume() );

	return smallerb.Contains(s.GetCenter());
}

//! Sphere contains Box
bool SphereContainsBox(const Sphere &s, const AxialBox & b)
{
	// @@ UNTESTED !

	// find the farthest corner of the box from the sphere center
	Vec3 minMinusCenter = MakeAbs( b.GetMin() - s.GetCenter() );
	Vec3 maxMinusCenter = MakeAbs( b.GetMax() - s.GetCenter() );

	int corner = 0;

	if ( maxMinusCenter.x > minMinusCenter.x ) corner += 1;
	if ( maxMinusCenter.y > minMinusCenter.y ) corner += 2;
	if ( maxMinusCenter.z > minMinusCenter.z ) corner += 4;

	Vec3 vCorner( b.GetCorner(corner) );

	return s.Contains(vCorner);
}

//---------------------------------------------------------------------------------

bool Intersects(const Cylinder &s, const Sphere & b)
{
	float dSqr = s.GetDistanceSqr(b.GetCenter());
	return dSqr <= fsquare( b.GetRadius() );
}

void MakeBoxAroundCylinder( AxialBox * pb, const Cylinder & c)
{
	ASSERT(pb);
	pb->SetToPoint( c.GetBase() );
	pb->MutableMax().z += c.GetHeight();
	const float radius = c.GetRadius();
	pb->MutableMin().x -= radius;
	pb->MutableMin().y -= radius;
	pb->MutableMax().x += radius;
	pb->MutableMax().y += radius;
	ASSERT( pb->IsValid() );
}

	

END_CB

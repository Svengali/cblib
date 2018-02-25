#include "Base.h"
#include "cblib/VolumeUtil.h"
#include "cblib/Mat3.h"
#include "cblib/Frame3Scaled.h"
#include "cblib/Sphere.h"
#include "cblib/AxialBox.h"
//#include "cblib/Cylinder.h"
#include "cblib/OrientedBox.h"
#include "cblib/Segment.h"
#include "cblib/Mat3Util.h"

START_CB

//namespace VolumeUtil
//{

//---------------------------------------------------------------------------------

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

/*
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
*/

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
	// sphere center must be inside radius of the box :
	
	AxialBox sphereBox( s.GetCenter(), s.GetRadius() );
	ASSERT( sphereBox.IsValid() );

	return b.Contains(sphereBox);
}

bool BoxContainsSphereXY(const AxialBox & b, const Sphere &s)
{
	// sphere center must be inside radius of the box :
	AxialBox sphereBox( s.GetCenter(), s.GetRadius() );
	ASSERT( sphereBox.IsValid() );

	return b.ContainsXY(sphereBox);
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

//}; // gVolume

END_CB

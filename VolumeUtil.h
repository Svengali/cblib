#pragma once

#include "cblib/Base.h"

/*! ********************

namespace VolumeUtil

Volume Primitives :
	AxialBox
	Sphere
	OrientedBox

Lower Level :
	Vec3
	Segment (LSS)
	Plane

Higher Level :
	Frustum
	Cone

The volumes should never see each other in their headers
This namespace encapsulates cross-volume work.

-------------------------------------------------------------

All the Volume Primitives should implement very similar APIs, so that
in theory they could be dropped into template generic algorithms.

The Volume "Policy" looks something like this :

class AnyVolume
{

	bool IsValid() const;
	
	void Translate(const Vec3 &v);
	void Scale(const Vec3 & s);
	void Rotate(const Mat3 & xf);

	float DistanceSqr(const Vec3 & v) const;
	bool Contains(const Vec3 & v) const;
	const Vec3 GetCenter() const;
	float GetRadiusSqr() const;
	float GetRadiusInDirection(const Vec3 & dir) const;
	float GetVolume() const;
	float DifferenceDistSqr(const gAnyVolume &ab ) const;

	bool IntersectVolume(const Segment & seg) const;
	bool IntersectSurface(const Segment & seg,SegmentResults * pResults) const;

	float PlaneDist(const Plane & Plane) const;
	Plane::ePlaneSide PlaneSide(const Plane & Plane) const;

};

***********************/

START_CB

class Mat3;
class Frame3Scaled;
class Sphere;
class AxialBox;
class OrientedBox;
class Segment;
class Cylinder;

//namespace VolumeUtil
//{
	//----------------------------------------------------------------------

	//! boolean intersection tests :
	bool Intersects(const Sphere &s, const AxialBox & b);
	bool Intersects(const Sphere &s, const OrientedBox & b);
	bool Intersects(const AxialBox & ab, const OrientedBox &ob);
	bool Intersects(const Cylinder &s, const Sphere & b);
	
	inline bool Intersects(const AxialBox & b, const Sphere &s) { return Intersects(s,b); }
	inline bool Intersects(const OrientedBox & b, const Sphere &s) { return Intersects(s,b); }
	inline bool Intersects(const OrientedBox &ob, const AxialBox & ab) { return Intersects(ab,ob); }
	inline bool Intersects(const Sphere & b,const Cylinder &s) { return Intersects(s,b); }

	bool BoxIntersectsCube(const AxialBox & ab, const Sphere & cube);

	//! One primitive Contains another tests :
	bool BoxContainsSphere(const AxialBox & b, const Sphere &s);
	bool BoxContainsSphereXY(const AxialBox & b, const Sphere &s);
	bool SphereContainsBox(const Sphere & s,const AxialBox &b);

	//----------------------------------------------------------------------
	//! "Make" functions to make the primitive that encloses another :

	void MakeSphereAroundBox( Sphere * ps, const AxialBox & b);
	void MakeSphereAroundBox( Sphere * ps, const OrientedBox & b);
	void MakeSphereAroundSegment( Sphere* ps, const Segment& s);

	void MakeBoxAroundSphere( AxialBox * pab, const Sphere & b);
	void MakeBoxAroundOrientedBox( AxialBox * pab, const OrientedBox & ob);
	void MakeBoxAroundCylinder( AxialBox * pab, const Cylinder & b);
	
	void MakeOrientedBoxFromAxialBox( OrientedBox * pob, const AxialBox & ab);
	void MakeOrientedBoxFromAxialBox( OrientedBox * pob, const AxialBox & ab, const Mat3 & mat);
	// NOTEZ : this MakeOrientedBoxFromAxialBox treats ab differently from the others !
	//	it transforms it by the xfs
	void MakeOrientedBoxFromLocalAxialBox( OrientedBox * pob, const AxialBox & ab, const Frame3Scaled& xfs);

	//----------------------------------------------------------------------

//};

END_CB


// end VolumeUtil

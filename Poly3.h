#pragma once

#include "cblib/Vec3.h"
#include "cblib/AxialBox.h"
#include "cblib/Plane.h"
#include "Core/vector.h"

struct Poly2;
class Frustum;
class Segment;

#define POLY_MAX_VERTS	(64)

/*

Poly for math work.

NOT FOR STORAGE

Poly2/Poly3 are nearly identical; maintain changes across them

--------------------------------------

Poly3 is *convex* , *planar*, and *counterclockwise*

*/

//==========================================================================================================

// Poly should be *counterclockwise*
struct Poly3
{
	vector<Vec3>	m_verts;
	Plane			m_plane;

	bool IsValid() const;

	void Clear();
	void Reverse();

	int GetNumVerts() const { return m_verts.size(); }
	const Vec3 & GetNormal() const { return m_plane.GetNormal(); }
	
	float ComputeArea() const;
	const Vec3 ComputeCenter() const;
	void ComputeBBox(AxialBox * pBox) const;

	void ComputeSphereFast(Sphere * pSphere) const;
	
	#ifndef _XBOX
	void ComputeSphereGood(Sphere * pSphere) const;
	#endif
	
	//! Poly::PlaneSide return Front,Back,On,Intersecting
	Plane::ESide PlaneSide(const Plane & plane,const float epsilon = EPSILON) const;
};

//==========================================================================================================

namespace Poly3Util
{

	void MakeQuadOnPlane(Poly3 * pInto, const Plane & onPlane, const AxialBox & refBox);

	bool ClipToBox(Poly3 * pInto, const Poly3 & from, const AxialBox & refBox);
	bool ClipToFrustum(Poly3*         pInto,
					   const Poly3&   front,
					   const Frustum& frustum);

	//! PutPointsOnPlane pushes the points in Poly back onto its plane
	void PutPointsOnPlane(Poly3 * pInto);

	//! FitPlane fills pPoly->m_plane
	//	"false" indicates a bad error
	bool FitPlane(Poly3 * pPoly);

	/*
	  CleanPoly removes colinear segments, and tries
	  to makes poly convex if it's gotten little wiggles
	*/
	bool CleanPoly(const Poly3 & from,Poly3 * pTo);

	/*
	  CleanPoly removes colinear segments, and tries
	  to makes poly convex if it's gotten little wiggles
	  
	  CleanPolySerious uses rigorous integer 2d convex hull
	*/
	bool CleanPolySerious(const Poly3 & from,Poly3 * pTo);

	void Project2d(Poly2 * pInto, const Poly3 & from,const Vec3 & basePos);
	void Lift2d(Poly3 * pInto, const Poly2 & from, const Plane & onPlane,const Vec3 & basePos);

	/*
	  ClipPoly to a plane; makes front-side and back-side results.
	  Result poly points can be NULL if you don't care about either one of them.
	  Plane has a thickness of epsilon.
	*/
	Plane::ESide ClipPoly(const Poly3 & from,Poly3 * pToF,Poly3 * pToB,const Plane & plane,const float epsilon = EPSILON);

	inline bool ClipPolyB(const Poly3 & from,Poly3 * pTo,const Plane & plane,const float epsilon = EPSILON)
	{
		ClipPoly(from,NULL,pTo,plane,epsilon);
		return pTo->GetNumVerts() >= 3;
	}

	inline bool ClipPolyF(const Poly3 & from,Poly3 * pTo,const Plane & plane,const float epsilon = EPSILON)
	{
		ClipPoly(from,pTo,NULL,plane,epsilon);
		return pTo->GetNumVerts() >= 3;
	}

	bool LineSegmentIntersects(const Poly3 & poly,const Vec3& start,const Vec3& end);

	// Projects the point onto the polygon's plane, and tests if the polygon it
	bool IsProjectedPointInPoly(const Poly3& poly, const Vec3& point);

	bool PolyIntersectsSphere(const Poly3& poly, const Sphere& sphere);
	
	bool SegmentIntersects(const Poly3& poly, const Segment & seg);
	//bool SegmentIntersects(const Poly3& poly, const CollisionQuery & q);
}; // Poly3Util

//==========================================================================================================

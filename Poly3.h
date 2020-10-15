#pragma once

#include "Vec3.h"
#include "AxialBox.h"
#include "Plane.h"
#include "vector.h"

START_CB

//#define POLY_MAX_VERTS	(64)

/*

gPoly for math work.

NOT FOR STORAGE

Poly2/Poly3 are nearly identical; maintain changes across them
--------------------------------------

Poly3 is *convex* , *planar*, and *counterclockwise*


*/

struct Poly2;
class Frustum;
class Segment;
class Sphere;

//==========================================================================================================

// gPoly should be *counterclockwise*
struct Poly3
{
	//vector_s<Vec3,POLY_MAX_VERTS>	m_verts;
	vector<Vec3>	m_verts;
	
	// just hold the plane on the side if you want it
	//Plane							m_plane;

	bool IsValid() const { return true; }

	void Clear();
	void Reverse();

	int GetNumVerts() const { return m_verts.size32(); }
	
	float ComputeArea() const;
	const Vec3 ComputeCenter() const;
	void ComputeBBox(AxialBox * pBox) const;

	void ComputeSphereFast(Sphere * pSphere) const;
	void ComputeSphereGood(Sphere * pSphere) const;

//	const Vec3 & GetNormal() const { return m_plane.GetNormal(); }
	
	//! gPoly::PlaneSide return Front,Back,On,Intersecting
	Plane::ESide PlaneSide(const Plane & plane,const float epsilon = EPSILON) const;
};

//==========================================================================================================

//namespace Poly3Util {

void MakeQuadOnPlane(Poly3 * pInto, const Plane & onPlane, const AxialBox & refBox);

bool ClipToBox(Poly3 * pInto, const Poly3 & from, const AxialBox & refBox);

bool ClipToFrustum(Poly3*         pInto,
					   const Poly3&   front,
					   const Frustum& frustum);
					   
bool LineSegmentIntersects(const Poly3 & poly,const Plane & polyPlane,const Vec3& start,const Vec3& end);

// Projects the point onto the polygon's plane, and tests if the polygon it
bool IsProjectedPointInPoly(const Poly3& poly,	const Plane & polyPlane, const Vec3& point);

bool PolyIntersectsSphere(const Poly3& poly,const Plane & polyPlane, const Sphere& sphere);

bool SegmentIntersects(const Poly3& poly,const Plane & polyPlane, const Segment & seg);
//bool SegmentIntersects(const Poly3& poly, const CollisionQuery & q);

//! PutPointsOnPlane pushes the points in Poly back onto its plane
void PutPointsOnPlane(Poly3 * pInto, const Plane & onPlane);

//! FitPlane fills pPlane
//	"false" indicates a bad error
bool FitPlane(const Poly3 & poly,Plane * pPlane);

/*
	CleanPoly removes colinear segments, and tries
	to makes poly convex if it's gotten little wiggles
*/
bool CleanPoly(const Poly3 & from,Poly3 * pTo);

/*
	CleanPoly removes colinear segments, and tries
	to makes poly convex if it's gotten little wiggles

	CleanPolySerious uses rigorous integer 3d convex hull
*/
bool CleanPolySerious(const Poly3 & from,const Plane & plane,Poly3 * pTo);

void Project2d(Poly2 * pInto, const Poly3 & from,const Plane & onPlane,const Vec3 & basePos);
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

//==========================================================================================================
// Edge

// ClipEdge isn't really a Poly3 thing, but it's basically ClipPoly so put it here
struct Edge
{
	Vec3	verts[2];
};

Plane::ESide ClipEdge(const Edge &edge,
										Edge * pFront,
										Edge * pBack,
										const Plane & plane,
										const float epsilon = EPSILON);

inline bool ClipEdgeF(const Edge &edge,
										Edge * pFront,
										const Plane & plane,
										const float epsilon = EPSILON)
{
	Plane::ESide side = ClipEdge(edge,pFront,NULL,plane,epsilon);
	return ( side != Plane::eBack );
}

bool ClipToBox(Edge * pInto, const Edge & from, const AxialBox & refBox);

//}; // Poly3Util

//==========================================================================================================

END_CB

#pragma once

#include "cblib/Vec2.h"
#include "cblib/Rect.h"
#include "cblib/Plane2.h"
#include "cblib/vector_s.h"

START_CB

#define POLY_MAX_VERTS	(64)

/*

Poly for math work.

NOT FOR STORAGE

Poly2/Poly3 are nearly identical; maintain changes across them

*/

//==========================================================================================================

// Poly should be *counterclockwise*
struct Poly2
{
	vector_s<Vec2,POLY_MAX_VERTS>	m_verts;

	void Clear();
	void Reverse();

	int GetNumVerts() const { return m_verts.size(); }
	
	float GetArea() const;
	const Vec2 GetCenter() const;
	void GetBBox(RectF * pBox) const;

	//! Poly::PlaneSide return Front,Back,On,Intersecting
	Plane::ESide PlaneSide(const Plane2 & plane) const;
};

//==========================================================================================================

namespace Poly2Util
{

/*
	CleanPoly removes colinear segments, and tries
	to makes poly convex if it's gotten little wiggles
*/
bool CleanPoly(const Poly2 & from,Poly2 * pTo);

/*
	CleanPoly removes colinear segments, and tries
	to makes poly convex if it's gotten little wiggles

	CleanPolySerious uses rigorous integer 2d convex hull
*/
bool CleanPolySerious(const Poly2 & from,Poly2 * pTo);

/*
	ClipPoly to a plane; makes front-side and back-side results.
	Result poly points can be NULL if you don't care about either one of them.
	Plane has a thickness of epsilon.
*/
Plane::ESide ClipPoly(const Poly2 & from,Poly2 * pToF,Poly2 * pToB,const Plane2 & plane,const float epsilon = EPSILON);

inline bool ClipPolyB(const Poly2 & from,Poly2 * pTo,const Plane2 & plane,const float epsilon = EPSILON)
{
	ClipPoly(from,NULL,pTo,plane,epsilon);
	return pTo->GetNumVerts() >= 3;
}

inline bool ClipPolyF(const Poly2 & from,Poly2 * pTo,const Plane2 & plane,const float epsilon = EPSILON)
{
	ClipPoly(from,pTo,NULL,plane,epsilon);
	return pTo->GetNumVerts() >= 3;
}

}; // Poly2Util

//==========================================================================================================

END_CB

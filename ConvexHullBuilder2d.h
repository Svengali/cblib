#pragma once
#ifndef GALAXY_CONVEXHULLBUILDER2D_H
#define GALAXY_CONVEXHULLBUILDER2D_H

#include "Base.h"
#include "vector.h"
#include "Vec2i.h"

START_CB

namespace ConvexHullBuilder2d
{
	//-----------------------------------------
	
	void Make2d(	const Vec2i * pVerts,
					const int numVerts,
					vector<Vec2i> & vHullPolygon);
	// optimal convex hull builder; O(NlogN) due to a sort step
	// pHull will be filled with the verts that make up the hull,
	//	in counter-clockwise order (eg. a polygon)
	// pHull must have enough space for numVerts !!

	bool IsConvexHull2d(
					const Vec2i * pVerts,
					const int numVerts,
					const Vec2i * pHull,
					const int numHullVerts);
	// pHull should be *counterclockwise* with no degenerates

	//-----------------------------------------
};

END_CB

#endif // GALAXY_CONVEXHULLBUILDER_H
#pragma once

#include "vector.h"
#include "Vec2i.h"

START_CB

/*

ConvexDecomposition2d for *Counter-Clockwise* wound polygons in 2d.

*/

extern bool CheckSelfIntersection2d(const vector<Vec2i> & contour);
extern bool CheckSelfIntersection2d(const vector<Vec2> & contour);

// ConvexDecomposition2d is currently O(N^3)
extern void ConvexDecomposition2d(const vector<Vec2i> & contour,vector< vector<int> > * pPolygons);

END_CB

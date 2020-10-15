#pragma once

#include "vector.h"
#include "Vec2.h"
#include "Vec2i.h"

START_CB

/*

Triangulate2d for *Counter-Clockwise* wound polygons in 2d.
That's a "contour" of points.
The int version is more robust

The PointCloud versions work on just a bag of points.  It makes edges around the convex hull of the points then
triangulates the interior so there's a vert on every provided point.

*/

extern bool Triangulate2d_Approx(const vector<Vec2> & contour,vector<int> * pTriangles);
extern bool Triangulate2d_ViaInts(const vector<Vec2> & contour,vector<int> * pTriangles);

extern bool Triangulate2d(const vector<Vec2i> & contour,vector<int> * pTriangles);

extern void DelaunayImprove2d(const vector<Vec2i> & contour,const vector<int> & triangles,vector<int> * pTriangles);

extern bool DelaunayTriangulate2d(const vector<Vec2i> & contour,vector<int> * pTriangles);
extern bool DelaunayTriangulate2d_ViaInts(const vector<Vec2> & contour,vector<int> * pTriangles);

extern bool DelaunayTriangulate2d_PointCloudHull(const vector<Vec2i> & points,vector<int> * pTriangles);
extern bool DelaunayTriangulate2d_PointCloudHull(const vector<Vec2> & points,vector<int> * pTriangles);

END_CB

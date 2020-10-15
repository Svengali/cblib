#pragma once

#include "Segment.h"
#include "Vec3.h"

START_CB

// masks for the edgeVertCheckFlags :
#define LSSTRI_MASK_EDGE0	1
#define LSSTRI_MASK_EDGE1	2
#define LSSTRI_MASK_EDGE2	4
#define LSSTRI_MASK_VERT0	8
#define LSSTRI_MASK_VERT1	16
#define LSSTRI_MASK_VERT2	32
#define LSSTRI_MASK_ALL		63

bool SegmentTriangleIntersect(const Segment& seg, SegmentResults* pResults,
								const Vec3 &v0,
								const Vec3 &v1,
								const Vec3 &v2);
								
bool SegmentTriangleIntersect(
					const Segment& seg, SegmentResults* result,
					const int edgeVertCheckFlags,
					const Vec3& v0, const Vec3& v1, const Vec3& v2,
					const float tMax);
					
void SegmentTriangle_Test();

END_CB

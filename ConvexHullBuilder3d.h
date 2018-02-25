#pragma once
#ifndef GALAXY_CONVEXHULLBUILDER3D_H
#define GALAXY_CONVEXHULLBUILDER3D_H

#include "cblib/Base.h"
#include "cblib/vector.h"
#include "cblib/Vec3i.h"

START_CB

struct Facei;
struct Edgei;

namespace ConvexHullBuilder3d
{
	//-----------------------------------------
	
	void Make3d_Incremental(	
					const Vec3i * pVerts,
					const int numVerts,
					vector<Facei> & vHullFaces,
					const int allowedError = 0);
	// Convex Hull in 3d by randomized incremental vert addition
	//	this is O(N*H) ;
	// returns an empty list of faces if the input verts are degenerate
	
	void GetHullVerts(const vector<Facei> & vHullFaces,
						vector<Vec3i> & vHullVerts);
		// GetHullVerts does RemoveDegenerateVerts

	void RemoveDegenerateVerts(vector<Vec3i> & verts);
		// RemoveDegenerateVerts removes identical verts;
		// !! it reorders 'verts' as well !!

	bool IsConvexHull3d(
					const Vec3i * pVerts,
					const int numVerts,
					const vector<Facei> & vHullFaces,
					const int allowedError = 0);

	//-----------------------------------------
};

END_CB

#endif // GALAXY_CONVEXHULLBUILDER_H

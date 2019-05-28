#pragma once
#ifndef GALAXY_VOLUMEBUILDER_H
#define GALAXY_VOLUMEBUILDER_H

#include "cblib/Base.h"

START_CB

class Mat3;
class Vec3;
class Vec2;
class Vec3i;
class Vec2i;
class OrientedBox;
class AxialBox;
class Sphere;
class RectF;

//namespace VolumeBuilder
//{
	//-----------------------------------------
	// helpers :

	void ComputeCovariance(Mat3 * pMat,
				const Vec3 * pVerts,
				const int numVerts);

	void ComputeCovariantBasis(Mat3 * pMat,
				const Vec3 * pVerts,
				const int numVerts);
		// the CovariantBasis is the matrix made
		//	from the normalized eigenvectors of the
		//	Covariance matrix

	//-----------------------------------------
	// O(N)
	void MakeFastSphere(Sphere * pS,
				const Vec3 * pVerts,
				const int numVerts);

	void MakeMiniSphere(Sphere * pS,
				const Vec3 * pVerts,
				const int numVerts);

	//-----------------------------------------
	// O(N)
	void BoundingRectangle(RectF * pR,
				const Vec2 * pVerts,
				const int numVerts);

	//-----------------------------------------
	// O(N)
	void MakeAxialBox(AxialBox * pBox,
				const Vec3 * pVerts,
				const int numVerts);

	//-----------------------------------------
	// O(N)
	//	builds an OBB equal to the AxialBox around the verts
	void AxialOBB(OrientedBox * pBox,
				const Vec3 * pVerts,
				const int numVerts);

	void BuildBoxGivenAxes(
			OrientedBox * pBox,
			const Vec3 * pVerts,
			const int numVerts,
			const Mat3 & axes);
			
	enum EOBBCriterion
	{
		eOBB_Volume,
		eOBB_SurfaceArea,
		eOBB_ShortestMajorAxis
	};

	// lower rating is better
	float RateOBB(const OrientedBox & obb,EOBBCriterion eCrit);
					
	// OptimalOBBByVolume :
	//	the normals should be the normals of
	//	the convex hull of the verts
	// O(N^2logN)
	void OptimalOBB(OrientedBox * pBox,
				const Vec3 * pVerts,
				const int numVerts,
				const Vec3 * pNormals,
				const int numNormals,
				EOBBCriterion eCrit,
				AxialBox * pAB = NULL);
	
	// OptimalOBB also does OptimalOBBFixedDirections
	//  OptimalOBBFixedDirections is fast; O(NlogN)
	void OptimalOBBFixedDirections(OrientedBox * pBox,
				const Vec3 * pVerts,
				const int numVerts,
				EOBBCriterion eCrit,
				bool optimize,
				AxialBox * pAB);
	
	// OptimalOBBBarequetHarPeled optimizes always
	void OptimalOBBBarequetHarPeled(OrientedBox * pBox,
				const Vec3 * pVerts,
				const int numVerts,
				EOBBCriterion eCrit,
				const int kLimit);
				
	void IterativeRefineOBB(OrientedBox * pBox,
					const AxialBox & ab,
					const Vec3 * pVerts,
					const int numVerts,
					EOBBCriterion eCrit,
					const bool tryAxes);
				
	// OBBByCovariance
	//	the "traditional" method using the covariance matrix
	//	really just not very good
	void OBBByCovariance(OrientedBox * pBox,
				const Vec3 * pVerts,
				const int numVerts);

	void OBBByCovarianceOptimized(OrientedBox * pBox,
				const Vec3 * pVerts,
				const int numVerts,
				EOBBCriterion eCrit);

	void OBBGoodHeuristic(OrientedBox * pBox,
				const Vec3 * pVerts,
				const int numVerts,
				EOBBCriterion eCrit);
				
	void OBBGivenCOV(OrientedBox * pBox,
				const Vec3 * pVerts,
				const int numVerts,
				const Mat3 & givenAxes,
				EOBBCriterion eCrit,
				bool optimize);
			
	//-----------------------------------------
	// 2d and 3d versions of "FindOptimalRectangle"
	//	find the minimum-area oriented rectangle 
	//	around a set of verts, in O(NlogN) time
	//	using 2d convex hull and rotating calipers

	enum EOptimalCriterion
	{
		eOptimal_Area,
		eOptimal_Perimeter,
		eOptimal_MinimumLargeDimension,
		eOptimal_MinimumSmallDimension
	};

	void FindOptimalRectangle(
			const Vec2 * pVerts,
			const int numVerts,
			const RectF & axialRect,
			Vec2 * pBasis1,
			Vec2 * pBasis2,
			RectF * pRectInBasis = NULL,
			EOptimalCriterion eOptimal = eOptimal_Area);

	void FindOptimalRectangle(
			const Vec3 * pVerts,
			const int numVerts,
			const AxialBox & ab,
			const Vec3 & normal1,
			Vec3 * pNormal2,
			Vec3 * pNormal3,
			EOptimalCriterion eOptimal = eOptimal_Area);

	//-----------------------------------------
	// the SphereExact routines make the
	//	sphere that lies on those points
	//	*not* the smallest sphere that encloses
	//	those points.

	void SphereExact1(Sphere * pS,
			const Vec3 & v1);
	void SphereExact2(Sphere * pS,
			const Vec3 & v1,
			const Vec3 & v2);
	void SphereExact3(Sphere * pS,
			const Vec3 & v1,
			const Vec3 & v2,
			const Vec3 & v3);
	void SphereExact4(Sphere * pS,
			const Vec3 & v1,
			const Vec3 & v2,
			const Vec3 & v3,
			const Vec3 & v4);
			
	void SphereAround4(Sphere * pS,
			const Vec3 & rkP0,
			const Vec3 & rkP1,
			const Vec3 & rkP2,
			const Vec3 & rkP3);

	//-----------------------------------------
//};

END_CB

#endif // GALAXY_VOLUMEBUILDER_H

#include "AxialBox.h"

#include "VolumeBuilder.h"
#include "cblib/Log.h"
#include "Vec3.h"
#include "Vec3U.h"
#include "Vec2.h"
#include "Vec2U.h"
#include "Vec3i.h"
#include "Vec2i.h"
#include "Mat3.h"
#include "Rect.h"
#include "Mat3Util.h"
#include "OrientedBox.h"
#include "AxialBox.h"
#include "Sphere.h"
#include "IntGeometry.h"
#include "ConvexHullBuilder2d.h"
//#include "EigenSolver.h"
#include "VolumeUtil.h"
#include "SphereNormals.h"
#include <limits.h>
#include "cblib/vector.h"
#include "External/MiniBall.h"

START_CB

//=============================================================

#ifdef DO_ASSERTS
static bool SphereContainsAll(const Sphere & s,
				const Vec3 * pVerts,
				const int numVerts,
				const float tolerance = CB_EPSILON)
{
	for(int i=0;i<numVerts;i++)
	{
		ASSERT( s.Contains(pVerts[i],tolerance) );
	}
	return true;
}

static bool OBBContainsAll(const OrientedBox & ob,
				const Vec3 * pVerts,
				const int numVerts,
				const float tolerance = CB_EPSILON)
{
	for(int i=0;i<numVerts;i++)
	{
		ASSERT( ob.Contains(pVerts[i],tolerance) );
	}
	return true;
}
#endif // DO_ASSERTS

//=============================================================

void MakeFastSphere(Sphere * pS,
				const Vec3 * pVerts,
				const int numVerts)
{
	ASSERT(pS);
	ASSERT(pVerts);
	ASSERT(numVerts>0);

	int i;
	Vec3 center(pVerts[0]);
	for(i=1;i<numVerts;i++)
	{
		center += pVerts[i];
	}
	center *= 1.f / float(numVerts);

	float maxDSqr = 0.f;
	for(i=0;i<numVerts;i++)
	{
		const float dSqr = DistanceSqr(center,pVerts[i]);
		maxDSqr = MAX(maxDSqr,dSqr);
	}

	pS->Set(center, sqrtf(maxDSqr) );

	ASSERT( SphereContainsAll(*pS,pVerts,numVerts) );
}


void MakeMiniSphere(Sphere * pS,
				const Vec3 * pVerts,
				const int numVerts)
{
	MiniBall::Make(pS,pVerts,numVerts);
}

//=============================================================

void BoundingRectangle(RectF * pR,
				const Vec2 * pVerts,
				const int numVerts)
{
	ASSERT(pR);
	ASSERT(pVerts);
	ASSERT(numVerts>0);
	
	pR->SetToPointV(pVerts[0]);
	for(int i=1;i<numVerts;i++)
	{
		pR->ExtendToPointV(pVerts[i]);
	}
}

//=============================================================

void MakeAxialBox(AxialBox * pBox,
				const Vec3 * pVerts,
				const int numVerts)
{
	ASSERT(pBox);
	ASSERT(pVerts);
	ASSERT(numVerts>0);

	pBox->SetToPoint(pVerts[0]);
	
	for(int i=1;i<numVerts;i++)
	{
		pBox->ExtendToPoint(pVerts[i]);
	}
}

//=============================================================

void AxialOBB(OrientedBox * pBox,
			const Vec3 * pVerts,
			const int numVerts)
{
	ASSERT(pBox);

	AxialBox ab;
	
	MakeAxialBox(&ab,pVerts,numVerts);

	pBox->SetMatrixToIdentity();
	pBox->SetCenter( ab.GetCenter() );
	pBox->SetRadii( ab.GetDiagonal() * 0.5f );

	ASSERT( OBBContainsAll(*pBox,pVerts,numVerts) );
}

//-----------------------------------------------------
void BuildBoxGivenAxes(
			OrientedBox * pBox,
			const Vec3 * pVerts,
			const int numVerts,
			const Mat3 & axes)
{
	ASSERT(pBox);
	ASSERT( pVerts );
	ASSERT( numVerts > 0 );
	ASSERT( axes.IsOrthonormal() );

	const Vec3 pt0 = axes * pVerts[0];

	AxialBox ab(pt0,pt0);
	
	int i;
	for(i=1;i<numVerts;i++)
	{
		const Vec3 boxSpaceVert = axes * pVerts[i];
		ab.ExtendToPoint(boxSpaceVert);
	}

	const Vec3 boxSpaceCenter = ab.GetCenter();
	const Vec3 worldSpaceCenter = axes.RotateByTranspose(boxSpaceCenter);

	pBox->SetCenter( worldSpaceCenter );
	// expand a bit to make sure we're conservative
	//pBox->SetRadii( ab.GetDiagonal() * (0.5f + 1e-5f) + Vec3(EPSILON,EPSILON,EPSILON) );
	pBox->SetRadii( ab.GetDiagonal() * (0.5f + 1e-9f) );
	pBox->SetMatrix( axes );
	
	ASSERT( OBBContainsAll(*pBox,pVerts,numVerts) );
}

//-----------------------------------------------------
// O(NlogN) ; limited by FindOptimalRectangle
static void BuildBox(OrientedBox * pBox,
			const Vec3 * pVerts,
			const int numVerts,
			const AxialBox & ab,
			const Vec3 & normal1)
{
	ASSERT(pBox);
	ASSERT( pVerts );
	ASSERT( numVerts > 0 );

	// normal is once axis
	// the others are determined by the optimal
	//	rectange in the Plane perp to normal

	Vec3 normal2,normal3;
	
	FindOptimalRectangle(pVerts,numVerts,ab,
						normal1,&normal2,&normal3);

	Mat3 axes(Mat3::eRows,normal1,normal2,normal3);
	if ( axes.GetDeterminant() < 0.f )
	{
		axes.Row(2) *= -1.f;
	}

	ASSERT( axes.IsOrthonormal() );

	// axes is the world-to-box rotation

	BuildBoxGivenAxes(pBox,pVerts,numVerts,axes);
}

//=============================================================

// lower rating is better
float RateOBB(const OrientedBox & obb,
					EOBBCriterion eCrit)
{
	switch(eCrit)
	{
	case eOBB_Volume:
		return obb.GetVolume();
	case eOBB_SurfaceArea:
		return obb.GetSurfaceArea();
	case eOBB_ShortestMajorAxis:
		return GetLargestComponentValue(obb.GetRadii());
	default:
		FAIL("Unknown!");
		return 0.f;
	}
}

/////////////////////////
//	the normals should be the normals of
//		the convex hull of the verts
//	O(N^2logN)
void OptimalOBB(OrientedBox * pBox,
			const Vec3 * pVerts,
			const int numVerts,
			const Vec3 * pNormals,
			const int numNormals,
			EOBBCriterion eCrit,
			AxialBox * pAB /*= NULL*/)
{
	ASSERT(pBox);
	ASSERT( pVerts );
	ASSERT( pNormals );
	ASSERT( numVerts > 0 );
	ASSERT( numNormals > 0 );

	// The optimal box must be aligned to one of the faces of
	//	the triangle.
	/* 12-04-02 : just found out this isn't true.
		in 3d you can "support" box faces in other ways, such
			as 1 edge + 1 vert off that edge,
		consider for example, extremely flat crushed Octahedrons.
		the "axial" box on the octahedron is supported by edges
			and verts, no faces.
	*/

	// initialize with OptimalOBBFixedDirections
	//	this gives us a very good box to start with
	AxialBox ab;
	OptimalOBBFixedDirections(pBox,pVerts,numVerts,eCrit,true,&ab);
	
	IterativeRefineOBB(pBox,ab,pVerts,numVerts,eCrit,false);
	
	if ( pAB ) *pAB = ab;

	for(int norm=0;norm<numNormals;norm++)
	{
		// one axis is the normal
		const Vec3 & n = pNormals[norm];

		OrientedBox testBox;
		BuildBox(&testBox,pVerts,numVerts,ab,n);
			
		IterativeRefineOBB(&testBox,ab,pVerts,numVerts,eCrit,false);
	
		/// take the box with the smallest rating :
		if ( RateOBB(testBox,eCrit) < RateOBB(*pBox,eCrit) )
		{
			*pBox = testBox;
		}
	}
}


void OptimalOBBFixedDirections(OrientedBox * pBox,
				const Vec3 * pVerts,
				const int numVerts,
				EOBBCriterion eCrit,
				bool optimize,
				AxialBox * pAB /*= NULL*/)
{
	ASSERT(pBox);
	ASSERT( pVerts );
	ASSERT( numVerts > 0 );

	// initialize with an axial box so we have a
	//	basis of comparsion :

	AxialBox ab;
	MakeAxialBox(&ab,pVerts,numVerts);
	if ( pAB ) *pAB = ab;

	MakeOrientedBoxFromAxialBox(pBox,ab);

	if ( optimize )
	{
		IterativeRefineOBB(pBox,ab,pVerts,numVerts,eCrit,true);
	}
		
	// then try various directions -
	//	only need to do one quadrant (?)

	for(int i=0;i< c_numSpherePositiveNormals;i++)
	{
		const Vec3 & n = GetSphereNormal(i);

		OrientedBox testBox;
		BuildBox(&testBox,pVerts,numVerts,ab,n);
		
		if ( optimize )
		{
			IterativeRefineOBB(&testBox,ab,pVerts,numVerts,eCrit,false);
		}
		
		/// take the box with the smallest rating :
		if ( RateOBB(testBox,eCrit) < RateOBB(*pBox,eCrit) )
		{
			*pBox = testBox;
		}
	}

}

void OptimalOBBBarequetHarPeled(OrientedBox * pBox,
			const Vec3 * pVerts,
			const int numVerts,
			EOBBCriterion eCrit,
			const int kLimit)
{
	// make the seed box :
	
	OBBByCovarianceOptimized(pBox,pVerts,numVerts,eCrit);
	//OBBByCovariance(pBox,pVerts,numVerts);
	
	const Mat3 axes = pBox->GetMatrix();

	Vec3 ax = axes.GetRowX();
	Vec3 ay = axes.GetRowY();
	Vec3 az = axes.GetRowZ();

	// @@ should thse be scaled by the box radii ?
	// perhaps a tiny bit better ?
	//	you're still searching a quadrant of axes, but the distribution is heavily biased by this
	/*
	ax *= pBox->GetRadii().x;
	ay *= pBox->GetRadii().y;	
	az *= pBox->GetRadii().z;
	/**/

	AxialBox ab;
	MakeAxialBox(&ab,pVerts,numVerts);
	
	// try all normals of the form
	//	 axes.x *i + axes.y * j + axes.z *k
	//	with (i+j+k) <= kLimit		
	
	vector<char> blocks;
	blocks.resize(kLimit*kLimit*kLimit,0);
	
	// no need to consider the two-zero cases cuz those give the original axes
	for(int k=0;k<kLimit;k++)
	{
		blocks[ k ] = 1;
		blocks[ k * kLimit ] = 1;
		blocks[ k * kLimit * kLimit ] = 1;
	}
	
	int numBuilds = 0;
	
	for(int i=0;i<kLimit;i++)
	{
		for(int j=0;j<(kLimit-i);j++)
		{
			for(int k=0;k<(kLimit-i-j);k++)
			{
				if ( blocks[ i + j * kLimit + k * kLimit * kLimit ] )
					continue;
			
				Vec3 normal = 
					float(i) * ax +
					float(j) * ay +
					float(k) * az;
				
				normal.NormalizeFast();
				
				numBuilds++;
				
				OrientedBox testBox;
				BuildBox(&testBox,pVerts,numVerts,ab,normal);
					
				// @@ option ? (this must be in the inner loop, not after)
				IterativeRefineOBB(&testBox,ab,pVerts,numVerts,eCrit,false);
	
				/// take the box with the smallest rating :
				if ( RateOBB(testBox,eCrit) < RateOBB(*pBox,eCrit) )
				{
					*pBox = testBox;
				}

				// block out multiples : (don't need to do self)	
				//	so when we do 1,1,1 then we block 2,2,2		
				for(int mul=2;;mul++)
				{
					int ti = i * mul;
					int tj = j * mul;
					int tk = k * mul;
					
					if ( ti >= kLimit || tj >= kLimit || tk >= kLimit )
						break;
					
					blocks[ ti + tj * kLimit + tk * kLimit * kLimit ] = 1;
				}
			}
		}
	}
	
	lprintf(" kLimit : %d , numBuilds : %d\n",kLimit,numBuilds);
}

/**

IterativeRefineOBB : step by using one of the calipers axis
 as the new fixed axis, then do calipers again

**/
void IterativeRefineOBB_Sub(OrientedBox * pBox,
				const AxialBox & ab,
				const Vec3 * pVerts,
				const int numVerts,
				EOBBCriterion eCrit,
				const int firstAxis)
{
	int nextAxis = firstAxis;

	// @@ 3 steps or 4 ?
	const int c_numSteps = 3;
	for(int step=0;step<c_numSteps;step++)
	{
		Vec3 axis = pBox->GetMatrix().GetRow(nextAxis);
		
		// use row Y or Z (never X because it was the last fixed axis)
		nextAxis = (nextAxis+1)%3;
		if ( nextAxis == 0 ) nextAxis++;
	
		OrientedBox testBox;
		BuildBox(&testBox,pVerts,numVerts,ab,axis);
			
		/// take the box with the smallest rating :
		if ( RateOBB(testBox,eCrit) < RateOBB(*pBox,eCrit) )
		{
			*pBox = testBox;
		}		
	}
}

void IterativeRefineOBB(OrientedBox * pBox,
				const AxialBox & ab,
				const Vec3 * pVerts,
				const int numVerts,
				EOBBCriterion eCrit,
				const bool tryAxes)
{
	if ( tryAxes )
	{
		OrientedBox startBox(*pBox);
		
		for(int firstAxis=0;firstAxis<3;firstAxis++)
		{
			OrientedBox testBox(startBox);
			IterativeRefineOBB_Sub(&testBox,ab,pVerts,numVerts,eCrit,firstAxis);
			
			/// take the box with the smallest rating :
			if ( RateOBB(testBox,eCrit) < RateOBB(*pBox,eCrit) )
			{
				*pBox = testBox;
			}		
		}
	}
	else
	{
		IterativeRefineOBB_Sub(pBox,ab,pVerts,numVerts,eCrit,1);		
	}
}

/*
// LinearExtent
//	find the min/max distance of a vert set in the "normal" direction
// O(N)
static float LinearExtent(const Vec3 & normal, 
			const Vec3 * pVerts,
			const int numVerts)
{
	ASSERT( pVerts );
	ASSERT( numVerts > 0 );

	float lo,hi;
	lo = hi = normal * pVerts[0];
	
	for(int i=1;i<numVerts;i++)
	{
		float dot = normal * pVerts[i];
		lo = min(lo,dot);
		hi = max(hi,dot);
	}

	return (hi - lo);
}
*/

void OBBGivenCOV(OrientedBox * pBox,
			const Vec3 * pVerts,
			const int numVerts,
			const Mat3 & givenCOV,
			EOBBCriterion eCrit,
			bool optimize)
{
	ASSERT(pBox);
	ASSERT( pVerts );
	ASSERT( numVerts > 0 );

	Vec3 eigenValues;
	Mat3 eigenVectors;
	EigenSolveSymmetric(givenCOV,&eigenValues,&eigenVectors);
	
	// @@ ??
	/*
	Mat3 axes;
	GetTranspose(eigenVectors,&axes);
	/*/
	Mat3 axes(eigenVectors);
	/**/
	
	// get largest 
	
	//	I want my largest-eigenvalue vector to be preserved as much as possible

	// largest-E is in X, swap it to Z :
	Swap(axes.RowX(),axes.RowZ());
	
	// Orthonormalize preserves the Z axis :
	Orthonormalize(&axes);

	Swap(axes.RowX(),axes.RowZ());
	
	BuildBoxGivenAxes(pBox,pVerts,numVerts,axes);
	
	if ( optimize )
	{
		AxialBox ab;
		MakeAxialBox(&ab,pVerts,numVerts);

		{
			OrientedBox testBox;
			MakeOrientedBoxFromAxialBox(&testBox,ab);
			
			/// take the box with the smallest rating :
			if ( RateOBB(testBox,eCrit) < RateOBB(*pBox,eCrit) )
			{
				*pBox = testBox;
			}
		}
		
		for(int axis=0;axis<3;axis++)
		{
			OrientedBox testBox;
			BuildBox(&testBox,pVerts,numVerts,ab,axes[axis]);
				
			IterativeRefineOBB(&testBox,ab,pVerts,numVerts,eCrit,false);

			/// take the box with the smallest rating :
			if ( RateOBB(testBox,eCrit) < RateOBB(*pBox,eCrit) )
			{
				*pBox = testBox;
			}
		}
	}
}

void OBBByCovariance(OrientedBox * pBox,
			const Vec3 * pVerts,
			const int numVerts)
{
	ASSERT(pBox);
	ASSERT( pVerts );
	ASSERT( numVerts > 0 );

	Mat3 axes;
	ComputeCovariantBasis(&axes,pVerts,numVerts);
	
	BuildBoxGivenAxes(pBox,pVerts,numVerts,axes);
}

void OBBByCovarianceOptimized(OrientedBox * pBox,
			const Vec3 * pVerts,
			const int numVerts,
			EOBBCriterion eCrit)
{
	ASSERT(pBox);
	ASSERT( pVerts );
	ASSERT( numVerts > 0 );

	Mat3 axes;
	ComputeCovariantBasis(&axes,pVerts,numVerts);
	
	AxialBox ab;
	MakeAxialBox(&ab,pVerts,numVerts);

	// initialize with an axial box so we have a
	//	basis of comparsion :
	MakeOrientedBoxFromAxialBox(pBox,ab);

	OrientedBox testBox;
	BuildBox(&testBox,pVerts,numVerts,ab,axes.GetRowX());
		
	IterativeRefineOBB(&testBox,ab,pVerts,numVerts,eCrit,true);

	/// take the box with the smallest rating :
	if ( RateOBB(testBox,eCrit) < RateOBB(*pBox,eCrit) )
	{
		*pBox = testBox;
	}
}

void OBBGoodHeuristic(OrientedBox * pBox,
			const Vec3 * pVerts,
			const int numVerts,
			EOBBCriterion eCrit)
{
	OBBByCovarianceOptimized(pBox,pVerts,numVerts,eCrit);
	
	OrientedBox testBox;
	AxialBox ab;
	OptimalOBBFixedDirections(&testBox,pVerts,numVerts,eCrit,true,&ab);
	
	/// take the box with the smallest rating :
	if ( RateOBB(testBox,eCrit) < RateOBB(*pBox,eCrit) )
	{
		*pBox = testBox;
	}	
}
				
//=====================================================================================

void ComputeCovariance(Mat3 * pMat,
			const Vec3 * pVerts,
			const int numVerts)
{
	ASSERT( pMat );
	ASSERT( pVerts );
	ASSERT( numVerts > 0 );
	
	// accumulate these sums :
	Mat3 sumMat = Mat3::zero;
	Vec3 sumVec = Vec3::zero;
	
	for(int i=0;i<numVerts;i++)
	{
		const Vec3 & v = pVerts[i];

		sumVec += v;

		for(int j=0;j<3;j++)
			for(int k=0;k<3;k++)
				sumMat.Element(j,k) += v[j] * v[k];
	}
	
	// now fill out pMat with the covariance :
	const float invNumVerts = 1.f/numVerts;
	const Vec3 center = sumVec * invNumVerts;
	
	for(int j=0;j<3;j++)
	{
		for(int k=0;k<3;k++)
		{
			pMat->Element(j,k) = sumMat.GetElement(j,k) * invNumVerts - center[j] * center[k];
		}
	}
}

void ComputeCovariantBasis(Mat3 * pMat,
			const Vec3 * pVerts,
			const int numVerts)
{
	Mat3 covariance;
	ComputeCovariance(&covariance,pVerts,numVerts);

	Vec3 eigenValues;
	EigenSolveSymmetric(covariance,&eigenValues,pMat);

	//	I want my largest-eigenvalue vector to be preserved as much as possible

	// largest-E is in X, swap it to Z :
	Swap(pMat->RowX(),pMat->RowZ());
	
	// preserves Z :
	Orthonormalize(pMat);

	Swap(pMat->RowX(),pMat->RowZ());
}

//=====================================================================================

static void RectangleInBasis(
						RectF * pRect,
						const Vec2 & curBasisX,
						const Vec2 & curBasisY,
						const Vec2i * pVerts,
						const int numVerts,
						const int support[4])
{
	ASSERT(pVerts);

	Vec2 abLo( FLT_MAX, FLT_MAX);
	Vec2 abHi(-FLT_MAX,-FLT_MAX);

	for(int i=0;i<4;i++)
	{
		ASSERT( support[i] >= 0 && support[i] <= numVerts );
		const Vec2i & vi = pVerts[ support[i] ];
		const Vec2 v( (float) vi.x, (float) vi.y );
		const Vec2 inBasis( curBasisX * v, curBasisY * v );

		abLo.SetMin(inBasis);
		abHi.SetMax(inBasis);
	}

	pRect->SetLoHiV( abLo, abHi );
}

static void RectangleInBasis_Slow(
						RectF * pRect,
						const Vec2 & curBasisX,
						const Vec2 & curBasisY,
						const Vec2i * pVerts,
						const int numVerts)
{
	ASSERT(pVerts);

	Vec2 abLo( FLT_MAX, FLT_MAX);
	Vec2 abHi(-FLT_MAX,-FLT_MAX);

	for(int i=0;i<numVerts;i++)
	{
		const Vec2i & vi = pVerts[ i ];
		const Vec2 v( (float) vi.x, (float) vi.y );
		const Vec2 inBasis( curBasisX * v, curBasisY * v );

		abLo.SetMin(inBasis);
		abHi.SetMax(inBasis);
	}

	pRect->SetLoHiV( abLo, abHi );
}

/*
static float RectArea(	const Vec2 & curBasisX,
						const Vec2 & curBasisY,
						const Vec2i * pVerts,
						const int numVerts,
						const int support[4])
{
	ASSERT(pVerts);

	Vec2 abLo( FLT_MAX, FLT_MAX);
	Vec2 abHi(-FLT_MAX,-FLT_MAX);

	for(int i=0;i<4;i++)
	{
		ASSERT( support[i] >= 0 && support[i] <= numVerts );
		const Vec2i & vi = pVerts[ support[i] ];
		const Vec2 v( (float) vi.x, (float) vi.y );
		const Vec2 inBasis( curBasisX * v, curBasisY * v );

		abLo.SetMin(inBasis);
		abHi.SetMax(inBasis);
	}

	const float area = (abHi.x - abLo.x) * (abHi.y - abLo.y);

	return area;
}
*/

static float RateRectangle(const RectF & rect, EOptimalCriterion eOptimal)
{
	switch(eOptimal)
	{
		case eOptimal_Area:
			return rect.Area();
		case eOptimal_Perimeter:
			return rect.Width() + rect.Height();
		case eOptimal_MinimumLargeDimension:
			return MAX(rect.Width(),rect.Height());
		case eOptimal_MinimumSmallDimension:
			return MIN(rect.Width(),rect.Height());
		default:
			FAIL("unknown Optimal!");
			return 0.f;
	}
}

static const Vec2 MakeVec2Normalized(const Vec2i & vi)
{
	Vec2 v( (float) vi.x, (float) vi.y );
	v.NormalizeFast();
	return v;
}

// pVerts/numVerts is the convex hull; verts sorted
//	in order, COUNTER-clockwise winding !
static void FindOptimalRectangle_RotateCalipers(
									const Vec2i * pVerts,
									const int numVerts,
									Vec2 * pBestRectBasis1,
									Vec2 * pBestRectBasis2,
									RectF * pRectInBasis = NULL,
									EOptimalCriterion eOptimal = eOptimal_Area)
{
	// start with the axial box
	Vec2i abLo( INT_MAX, INT_MAX);
	Vec2i abHi(-INT_MAX,-INT_MAX);
	int support[4] = { 0 };	
	int i;

	enum ESupport
	{
		eLoX=0,
		eHiX=1,
		eLoY=2,
		eHiY=3
	};

	for(i=0;i<numVerts;i++)
	{
		const Vec2i & v = pVerts[i];

		if ( v.x < abLo.x )
		{
			abLo.x = v.x;
			support[eLoX] = i;
		}
		if ( v.x > abHi.x )
		{
			abHi.x = v.x;
			support[eHiX] = i;
		}
		if ( v.y < abLo.y )
		{
			abLo.y = v.y;
			support[eLoY] = i;
		}
		if ( v.y > abHi.y )
		{
			abHi.y = v.y;
			support[eHiY] = i;
		}
	}

	// now we have an axial box and four verts on that box.
	// that gives us our starting basis of reference :

	RectF baseRect( (float) abLo.x, (float) abHi.x, (float) abLo.y, (float) abHi.y ); 
	float bestQ = RateRectangle(baseRect,eOptimal);
	RectF bestRect( (float)abLo.x,(float)abHi.x , (float)abLo.y,(float)abHi.y );
	*pBestRectBasis1 = Vec2::unitX;
	*pBestRectBasis2 = Vec2::unitY;

	if ( numVerts <= 4 )
	{
		// for very small cases, we have degeneracies, so just special-case it

		for(int i=0;i<numVerts;i++)
		{
			int next = (i+1)%numVerts;

			Vec2i v0 = pVerts[i];
			Vec2i v1 = pVerts[next];
			Vec2i edge = v1 - v0;

			if (edge.LengthSqr() == 0)
			{
				// degenerate edge; skip it!
				continue;
			}

			Vec2 curBasisX = MakeVec2Normalized(edge);
			Vec2 curBasisY = MakePerpCW(curBasisX);

			RectF curRect(0,0);
			RectangleInBasis_Slow(
						&curRect,curBasisX,curBasisY,
						pVerts,numVerts);

			float q = RateRectangle(curRect,eOptimal);
			if ( q < bestQ )
			{
				bestQ = q;
				bestRect = curRect;
				*pBestRectBasis1 = curBasisX;
				*pBestRectBasis2 = curBasisY;
			}
		}

		if ( pRectInBasis != NULL )
		{
			*pRectInBasis = bestRect;
		}

		return;
	}

	Vec2i curBasisXi = Vec2i::unitX;
	Vec2i curBasisYi = Vec2i::unitY;

	// should never happen, but prevent infinite loops :
	int steps = 0;
	int maxSteps = numVerts * 4;

	// once curBasisX is pointing in negative X, we've see a 90 degree round of bases
	while( curBasisXi.x >= 0 )
	{
		Vec2i edges[4];
		int next[4];
		int j;
		
		for(j=0;j<4;j++)
		{
			redo:
			next[j] = support[j] + 1;
			// and wrap :
			if ( next[j] == numVerts )
				next[j] = 0;
			edges[j] = pVerts[ next[j] ] - pVerts[ support[j] ];
			if ( edges[j] == Vec2i::zero )
			{
				// degenerate !
				support[j] = next[j];
				goto redo;
			}
		}

		// at "HiY" you should be running in -X
		// at "LoY" you should be running in +X
		// at "HiX" you should be running in +Y
		// at "LoX" you should be running in -Y

		bool colinear[4];

		colinear[eHiY] = ColinearEdges(edges[eHiY] , curBasisXi);
		colinear[eLoX] = ColinearEdges(edges[eLoX] , curBasisYi);
		colinear[eLoY] = ColinearEdges(edges[eLoY] , curBasisXi);
		colinear[eHiX] = ColinearEdges(edges[eHiX] , curBasisYi);

		// now advance the support of any colinear edges
		bool didAdvance = false;
		for(j=0;j<4;j++)
		{
			if ( colinear[j] )
			{
				support[j] = next[j];
				didAdvance = true;
			}
		}

		if ( didAdvance )
			continue;

		// @@@@ TODO : all I need to know is which edge is most-aligned
		//	with the current Basis; is there a way to do that exactly in ints?
		// -> using floats here is a robustness leak
		int largestDotI = 0;
		// scope for stuff to compute largestDotI
		{
			int64 dot[4];
			dot[eHiY] = -(edges[eHiY] * curBasisXi);
			dot[eLoX] = -(edges[eLoX] * curBasisYi);
			dot[eLoY] = +(edges[eLoY] * curBasisXi);
			dot[eHiX] = +(edges[eHiX] * curBasisYi);

			// if there are only three verts in the convex hull,
			//	then one of these dots can be negative
			//ASSERT( dot[0] >= 0.f && dot[1] >= 0.f && dot[2] >= 0.f && dot[3] >= 0.f );
			double basisLenSqr[4];
			basisLenSqr[eHiY] = (double) curBasisXi.LengthSqr();
			basisLenSqr[eLoY] = (double) curBasisXi.LengthSqr();
			basisLenSqr[eLoX] = (double) curBasisYi.LengthSqr();
			basisLenSqr[eHiX] = (double) curBasisYi.LengthSqr();

			double compare[4];

			for(j=0;j<4;j++)
			{
				if ( dot[j] < 0 )
				{
					compare[j] = 0;	
				}
				else
				{
					// compare dot-squared, since it has the same ordering,
					//	and it avoids the use of sqrt
					compare[j] = double( dot[j] ) * double( dot[j] ) /
								double( edges[j].LengthSqr() ) * double( basisLenSqr[j] );
				}
			}
			
			for(j=0;j<4;j++)
			{
				// if dot[j] is nearly -1 then we have a super-bad degeneracy
				//	this happens when all the verts are very nearly colinear

				if ( compare[j] > compare[largestDotI] )
				{
					largestDotI = j;
				}
			}
		}

		// now rotate the basis to have one edge along edges[largestDotI]
		// and advance the support in largestDotI

		Vec2i newBasisXi,newBasisYi;

		// at "HiY" you should be running in -X
		// at "LoY" you should be running in +X
		// at "HiX" you should be running in +Y
		// at "LoX" you should be running in -Y

		switch(largestDotI)
		{
		case eHiY:
			newBasisXi = -1 * edges[eHiY];
			newBasisYi = Vec2i::MakePerpCCW(newBasisXi);
			break;
		case eLoY:
			newBasisXi = edges[eLoY];
			newBasisYi = Vec2i::MakePerpCCW(newBasisXi);
			break;
		case eHiX:
			newBasisYi = edges[eHiX];
			newBasisXi = Vec2i::MakePerpCW(newBasisYi);
			break;
		case eLoX:
			newBasisYi = -1 * edges[eLoX];
			newBasisXi = Vec2i::MakePerpCW(newBasisYi);
			break;
		default:
			FAIL("Bad case!");
			return;
		}

		// make sure we're not turning more than 90 degrees :
		ASSERT( (curBasisXi * newBasisXi) >= 0.f );
		ASSERT( (curBasisYi * newBasisYi) >= 0.f );

		curBasisXi = newBasisXi;
		curBasisYi = newBasisYi;

		const Vec2 curBasisX = MakeVec2Normalized(curBasisXi);
		const Vec2 curBasisY = MakeVec2Normalized(curBasisYi);

		// measure the Q in this new basis
		RectF curRect(0.f,0.f);
		RectangleInBasis(&curRect,curBasisX,curBasisY,
							pVerts, numVerts,
							support );

		float q = RateRectangle(curRect,eOptimal);

		if ( q < bestQ )
		{
			bestQ = q;
			bestRect = curRect;
			*pBestRectBasis1 = curBasisX;
			*pBestRectBasis2 = curBasisY;			
		}

		support[largestDotI] = next[largestDotI];

		if ( ++steps > maxSteps )
		{
			lprintf("WARNING : broke out of FindOptimalRectangle_RotateCalipers\n");
			break;
		}
	}

	// end FindOptimalRectangle_RotateCalipers

	if ( pRectInBasis != NULL )
	{
		*pRectInBasis = bestRect;
	}
}

static RectF MakeRectProjection(const AxialBox & ab,const Vec3 & basisX,const Vec3 & basisY)
{
	// don't worry about some level of overestimate;
	RectF rect( MakeProjection( ab.GetCenter() , basisX , basisY ) );

	for(int i=0;i<8;i++)
	{
		rect.ExtendToPointV( MakeProjection( ab.GetCorner(i) , basisX , basisY ) );
	}

	return rect;
}

//---------------------------------------------------------------
// O(N) version using the 2d convex hull + rotating calipers
void FindOptimalRectangle(
			const Vec3 * pVerts,
			const int numVerts,
			const AxialBox & ab,
			const Vec3 & normal1,
			Vec3 * pNormal2,
			Vec3 * pNormal3,
			EOptimalCriterion eOptimal /*= eOptimal_Area*/
			)
{
	Vec3 basisX,basisY;
	GetTwoPerpNormals(normal1,&basisX,&basisY);

	// in the 2d space of basis1/basis2 , 
	//	find the optimal rectangle.

	// first make the 2d projection for convenience

	typedef vector<Vec2i> v_Vec2i;

	// make a Rect that encloses the projection of the verts onto basisX/basisY
	RectF rect = MakeRectProjection(ab,basisX,basisY);

	// quantizationRect must be square, or we stretch some dimension!
	RectF quantizationRect(rect);
	quantizationRect.GrowToSquare();

	v_Vec2i projection;

	projection.resize(numVerts);
	int i;
	for(i=0;i<numVerts;i++)
	{
		projection[i] = IntGeometry::Quantize( MakeProjection( pVerts[i], basisX, basisY ) , quantizationRect );
	}

	v_Vec2i hull;
	
	ConvexHullBuilder2d::Make2d(&projection[0],numVerts,hull);
	
	projection.clear();

	Vec2 bestRectBasis1,bestRectBasis2;

	// now use rotating calipers
	FindOptimalRectangle_RotateCalipers(hull.data(),(int)hull.size(),
										&bestRectBasis1,&bestRectBasis2,NULL,eOptimal);

	// turn the bases in 2d into real 3d vecs perp to normal1
	pNormal2->SetWeightedSum(	bestRectBasis1.x, basisX,
								bestRectBasis1.y, basisY );
	pNormal3->SetWeightedSum(	bestRectBasis2.x, basisX,
								bestRectBasis2.y, basisY );
}

//---------------------------------------------------------------
// O(N) version using the 2d convex hull + rotating calipers
void FindOptimalRectangle(
			const Vec2 * pVerts,
			const int numVerts,
			const RectF & axialRect,
			Vec2 * pBasis1,
			Vec2 * pBasis2,
			RectF * pRectInBasis /*= NULL*/,
			EOptimalCriterion eOptimal /*= eOptimal_Area*/
			)
{
	// in the 2d space of basis1/basis2 , 
	//	find the optimal rectangle.

	// first make the 2d projection for convenience

	typedef vector<Vec2i> v_Vec2i;

	// quantizationRect must be square, or we stretch some dimension!
	RectF quantizationRect(axialRect);
	quantizationRect.GrowToSquare();

	v_Vec2i projection;

	projection.resize(numVerts);
	int i;
	for(i=0;i<numVerts;i++)
	{
		projection[i] = IntGeometry::Quantize( pVerts[i], quantizationRect );
	}

	v_Vec2i hull;
	
	ConvexHullBuilder2d::Make2d(&projection[0],numVerts,hull);
	
	projection.clear();

	// now use rotating calipers
	FindOptimalRectangle_RotateCalipers(hull.data(),hull.size32(),
										pBasis1,pBasis2,pRectInBasis,eOptimal);

	// @@ pRectInBasis is still in the quantized coords!!!

}

//=====================================================================================

void SphereExact1(Sphere * pS,
			const Vec3 & v1)
{
	ASSERT(pS);
	pS->Set(v1,0.f);
}
void SphereExact2(Sphere * pS,
			const Vec3 & v1,
			const Vec3 & v2)
{
	ASSERT(pS);
	pS->SetCenter( MakeAverage(v1,v2) );
	pS->SetRadius( Distance(v1,v2) * 0.5f );
}
void SphereExact3(Sphere * pS,
			const Vec3 & v0,
			const Vec3 & v1,
			const Vec3 & v2)
{
	// (from Magic software)
	//
    // Compute the circle (in 3D) containing p0, p1, and p2.  The Center() in
    // barycentric coordinates is K = u0*p0+u1*p1+u2*p2 where u0+u1+u2=1.
    // The Center() is equidistant from the three points, so |K-p0| = |K-p1| =
    // |K-p2| = R where R is the radius of the circle.
    //
    // From these conditions,
    //   K-p0 = u0*A + u1*B - A
    //   K-p1 = u0*A + u1*B - B
    //   K-p2 = u0*A + u1*B
    // where A = p0-p2 and B = p1-p2, which leads to
    //   r^2 = |u0*A+u1*B|^2 - 2*Dot(A,u0*A+u1*B) + |A|^2
    //   r^2 = |u0*A+u1*B|^2 - 2*Dot(B,u0*A+u1*B) + |B|^2
    //   r^2 = |u0*A+u1*B|^2
    // Subtracting the last equation from the first two and writing
    // the equations as a linear system,
    //
    // +-                 -++   -+       +-        -+
    // | Dot(A,A) Dot(A,B) || u0 | = 0.5 | Dot(A,A) |
    // | Dot(B,A) Dot(B,B) || u1 |       | Dot(B,B) |
    // +-                 -++   -+       +-        -+
    //
    // The following code solves this system for u0 and u1, then
    // evaluates the third equation in r^2 to obtain r.

    Vec3 kA = v0 - v2;
    Vec3 kB = v1 - v2;
    float fAdA = kA * kA;
    float fAdB = kA * kB;
    float fBdB = kB * kB;
    float fDet = fAdA*fBdB-fAdB*fAdB;

    if ( fabsf(fDet) > EPSILON )
    {
        float fHalfInvDet = 0.5f/fDet;
        float fU0 = fHalfInvDet*fBdB*(fAdA-fAdB);
        float fU1 = fHalfInvDet*fAdA*(fBdB-fAdB);
        float fU2 = 1.0f-fU0-fU1;
        pS->SetCenter( fU0*v0 + fU1*v1 + fU2*v2 );
        Vec3 kTmp = fU0*kA + fU1*kB;
        pS->SetRadius( kTmp.Length() );
    }
    else
    {
		// coplanar case :
        pS->SetCenter( (v0+v1+v2)*(1.f/3.f) );
		pS->SetRadius( 0.f );
		pS->ExtendToPoint(v0);
		pS->ExtendToPoint(v1);
		pS->ExtendToPoint(v2);
    }

}

void SphereAround4(Sphere * pS,
			const Vec3 & rkP0,
			const Vec3 & rkP1,
			const Vec3 & rkP2,
			const Vec3 & rkP3)
{
	pS->SetCenter( (rkP0+rkP1+rkP2+rkP3)*0.25f );
	pS->SetRadius( 0.f );
	pS->ExtendToPoint(rkP0);
	pS->ExtendToPoint(rkP1);
	pS->ExtendToPoint(rkP2);
	pS->ExtendToPoint(rkP3);
}

void SphereExact4(Sphere * pS,
			const Vec3 & rkP0,
			const Vec3 & rkP1,
			const Vec3 & rkP2,
			const Vec3 & rkP3)
{
	// (from Magic software)
	//
    // Compute the sphere containing p0, p1, p2, and p3.  The Center() in
    // barycentric coordinates is K = u0*p0+u1*p1+u2*p2+u3*p3 where
    // u0+u1+u2+u3=1.  The Center() is equidistant from the three points, so
    // |K-p0| = |K-p1| = |K-p2| = |K-p3| = R where R is the radius of the
    // sphere.
    //
    // From these conditions,
    //   K-p0 = u0*A + u1*B + u2*C - A
    //   K-p1 = u0*A + u1*B + u2*C - B
    //   K-p2 = u0*A + u1*B + u2*C - C
    //   K-p3 = u0*A + u1*B + u2*C
    // where A = p0-p3, B = p1-p3, and C = p2-p3 which leads to
    //   r^2 = |u0*A+u1*B+u2*C|^2 - 2*Dot(A,u0*A+u1*B+u2*C) + |A|^2
    //   r^2 = |u0*A+u1*B+u2*C|^2 - 2*Dot(B,u0*A+u1*B+u2*C) + |B|^2
    //   r^2 = |u0*A+u1*B+u2*C|^2 - 2*Dot(C,u0*A+u1*B+u2*C) + |C|^2
    //   r^2 = |u0*A+u1*B+u2*C|^2
    // Subtracting the last equation from the first three and writing
    // the equations as a linear system,
    //
    // +-                          -++   -+       +-        -+
    // | Dot(A,A) Dot(A,B) Dot(A,C) || u0 | = 0.5 | Dot(A,A) |
    // | Dot(B,A) Dot(B,B) Dot(B,C) || u1 |       | Dot(B,B) |
    // | Dot(C,A) Dot(C,B) Dot(C,C) || u2 |       | Dot(C,C) |
    // +-                          -++   -+       +-        -+
    //
    // The following code solves this system for u0, u1, and u2, then
    // evaluates the fourth equation in r^2 to obtain r.

    Vec3 kE10 = rkP0 - rkP3;
    Vec3 kE20 = rkP1 - rkP3;
    Vec3 kE30 = rkP2 - rkP3;

    float aafA[3][3];
    aafA[0][0] = kE10 * kE10;
    aafA[0][1] = kE10 * kE20;
    aafA[0][2] = kE10 * kE30;
    aafA[1][0] = aafA[0][1];
    aafA[1][1] = kE20 * kE20;
    aafA[1][2] = kE20 * kE30;
    aafA[2][0] = aafA[0][2];
    aafA[2][1] = aafA[1][2];
    aafA[2][2] = kE30 * kE30;

    float afB[3];
    afB[0] = 0.5f*aafA[0][0];
    afB[1] = 0.5f*aafA[1][1];
    afB[2] = 0.5f*aafA[2][2];

    float aafAInv[3][3];
    aafAInv[0][0] = aafA[1][1]*aafA[2][2]-aafA[1][2]*aafA[2][1];
    aafAInv[0][1] = aafA[0][2]*aafA[2][1]-aafA[0][1]*aafA[2][2];
    aafAInv[0][2] = aafA[0][1]*aafA[1][2]-aafA[0][2]*aafA[1][1];
    aafAInv[1][0] = aafA[1][2]*aafA[2][0]-aafA[1][0]*aafA[2][2];
    aafAInv[1][1] = aafA[0][0]*aafA[2][2]-aafA[0][2]*aafA[2][0];
    aafAInv[1][2] = aafA[0][2]*aafA[1][0]-aafA[0][0]*aafA[1][2];
    aafAInv[2][0] = aafA[1][0]*aafA[2][1]-aafA[1][1]*aafA[2][0];
    aafAInv[2][1] = aafA[0][1]*aafA[2][0]-aafA[0][0]*aafA[2][1];
    aafAInv[2][2] = aafA[0][0]*aafA[1][1]-aafA[0][1]*aafA[1][0];

    float fDet =	aafA[0][0]*aafAInv[0][0] + 
					aafA[0][1]*aafAInv[1][0] +
					aafA[0][2]*aafAInv[2][0];

    if ( fabsf(fDet) > EPSILON )
    {
        float fInvDet = 1.0f/fDet;
        int iRow, iCol;
        for (iRow = 0; iRow < 3; iRow++)
        {
            for (iCol = 0; iCol < 3; iCol++)
                aafAInv[iRow][iCol] *= fInvDet;
        }
        
        float afU[4];
        for (iRow = 0; iRow < 3; iRow++)
        {
            afU[iRow] = 0.0f;
            for (iCol = 0; iCol < 3; iCol++)
                afU[iRow] += aafAInv[iRow][iCol]*afB[iCol];
        }
        afU[3] = 1.0f - afU[0] - afU[1] - afU[2];
        
        pS->SetCenter( afU[0]*rkP0 + afU[1]*rkP1 + afU[2]*rkP2 + afU[3]*rkP3 );
        Vec3 kTmp = afU[0]*kE10 + afU[1]*kE20 + afU[2]*kE30;
        pS->SetRadius( kTmp.Length() );
    }
    else
    {
		// coplanar case :
        pS->SetCenter( (rkP0+rkP1+rkP2+rkP3)*0.25f );
		pS->SetRadius( 0.f );
		pS->ExtendToPoint(rkP0);
		pS->ExtendToPoint(rkP1);
		pS->ExtendToPoint(rkP2);
		pS->ExtendToPoint(rkP3);
    }
}

END_CB

//=====================================================================================

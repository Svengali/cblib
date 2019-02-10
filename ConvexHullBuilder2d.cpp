#include "ConvexHullBuilder2d.h"
#include "Log.h"
#include <limits.h> // for INT_MAX

START_CB

//}{=====================================================================================================================

// pHull is *counterclockwise*
bool ConvexHullBuilder2d::IsConvexHull2d(
					const Vec2i * pVerts,
					const int numVerts,
					const Vec2i * pHull,
					const int numHullVerts)
{
	for(int h=0;h<numHullVerts;h++)
	{
		int next_h = h + 1;
		if ( next_h == numHullVerts ) next_h = 0;
		int next_h2 = next_h + 1;
		if ( next_h2 == numHullVerts ) next_h2 = 0;
		
		const Vec2i & v0 = pHull[h];
		const Vec2i & v1 = pHull[next_h];
		DURING_ASSERT( const Vec2i & v2 = pHull[next_h2] );

		// assert on no degenerates in Hull
		ASSERT( v0 != v1 );
		ASSERT( ! Colinear(v0,v1,v2) );

		// can use this "2d plane" formulation if you like, but using Left() is neater
		//const Vec2i edge = v1 - v0;
		//const Vec2i normal = Vec2i::MakePerpCCW(edge);
		//const int64 planeConstant = normal * pHull[h];

		// we have a 2d gPlane : {normal,planeConstant}

		// check that all the verts are on the "front" side of this ecge 
		for(int i=0;i<numVerts;i++)
		{
			const Vec2i & v = pVerts[i];

			//const int64 dist = v * normal - planeConstant;
			//if ( dist < 0 )
			//	return false;
			if ( ! LeftOrOn(v0,v1,v) )
				return false;
		}
	}

	return true;
}

//---------------------------------------------------------------------------

static Vec2i s_base;

static bool CounterClockwiseWithBase(const Vec2i &a,const Vec2i &b)
{
	const int64 area = Area(s_base,a,b);
	if ( area == 0 )
	{
		// s_base,a,b are colinear
		// make the one closer to s_base first
		return DistanceSqr(s_base,a) < DistanceSqr(s_base,b);
	}
	else
	{
		return area > 0; // "left"
	}
}

//---------------------------------------------------------------------------

// optimal convex hull builder; O(NlogN) due to a sort step
// This is "Graham's walk" or something like that
//	(see the cool 2d Convex Hull page).  It's semi-obvious if
//	you think about it for a bit.
// This implementation uses temp-storage and a call to std::sort
//  it would be preferable to avoid those, of course.
void ConvexHullBuilder2d::Make2d(
					const Vec2i * pVerts,
					const int numVerts,
					vector<Vec2i> & vHullPolygon)
{
	ASSERT( pVerts );
	ASSERT( numVerts > 1 );

	vHullPolygon.clear();

	// special case very small sets :
	if ( numVerts <= 3 )
	{
		for(int i=0;i<numVerts;i++)
		{
			vHullPolygon.push_back(pVerts[i]);
		}

		// result should be counter-clockwise
		if ( numVerts == 3 )
		{
			const int64 area = Area( vHullPolygon[0], vHullPolygon[1], vHullPolygon[2] );
			if ( area < 0 )
			{
				Swap( vHullPolygon[0], vHullPolygon[1] );
			}
		}
	
		ASSERT( IsConvexHull2d(pVerts,numVerts,vHullPolygon.data(),vHullPolygon.size()) );

		return;
	}

	// first find the point with lowest X to start the hull
	int iBase = 0;
	
	for(int i=0;i<numVerts;i++)
	{
		if ( pVerts[i].x < pVerts[iBase].x )
		{
			iBase = i;
		}
		else if ( pVerts[i].x == pVerts[iBase].x &&
				pVerts[i].y < pVerts[iBase].y )
		{
			iBase = i;
		}
	}
	// now the vert at "iBase" must be on the hull !
	// set up "base" for VecAndAngle::Less to see
	s_base = pVerts[iBase];

	// now sort all other points by their angle to this point
	vector<Vec2i> vas( pVerts, pVerts + numVerts );

	std::sort(vas.begin(),vas.end(),CounterClockwiseWithBase);

	// make sure the first one is "base" :
	ASSERT( vas[0] == s_base );

	// "unique" + erase removes duplicates
	vector<Vec2i>::iterator it = std::unique(vas.begin(),vas.end());
	vas.erase(it,vas.end());

	// now walk in order, building the hull :
	vHullPolygon.resize(2);
	vHullPolygon[0] = vas[0];
	vHullPolygon[1] = vas[1];

	for(int i = 2;i<vas.size();)
	{
		const int numHullVerts = vHullPolygon.size();
		ASSERT(numHullVerts>=2);
		ASSERT( vas[i-1] != vas[i] ); // degenerates should be gone
		ASSERT( LeftOrOn(s_base,vas[i-1],vas[i]) ); // check sort order

		const Vec2i & curVert = vas[i];

		// check if curVert is to the CCW of the previous edge
		const int64 area = Area( vHullPolygon[numHullVerts-2], vHullPolygon[numHullVerts-1], curVert );

		if ( area == 0 ) // colinear
		{
			// just replace [numHullVerts-1] with curVert
			ASSERT( DistanceSqr(vHullPolygon[numHullVerts-2],curVert) > DistanceSqr(vHullPolygon[numHullVerts-2],vHullPolygon[numHullVerts-1]) );
			vHullPolygon.back() = curVert;
			i++;
		}
		else if ( area > 0 ) // counterclockwise, good
		{
			// push onto the stack !
			vHullPolygon.push_back( curVert );
			i++;
		}
		else
		{
			// pop from the stack !
			// (and don't advance i, so we'll consider curVert again)
			ASSERT(numHullVerts>=3);
			vHullPolygon.pop_back();
		}
	}

	// s_base must have been on the hull
	ASSERT( vHullPolygon[0] == s_base );

	// you can get tiny hulls for degenerate cases :
	if ( vHullPolygon.size() < 3 )
	{
		int i=1;
		while( pVerts[i] == pVerts[0] && i < numVerts )
		{
			i++;
		}
		if ( i == numVerts )
		{
			// all verts identical !!
			lprintf("Totally degenerate hull - it's a point!!");
			return;
		}
		for(int j=i;j<numVerts;j++)
		{
			ASSERT( Colinear( pVerts[0], pVerts[i], pVerts[j] ) ); 
		}
		lprintf("Totally degenerate hull - it's a line!!");
	}
	else
	{
		ASSERT( vHullPolygon.size() >= 3 );
		ASSERT( ! Colinear( vHullPolygon.back(), vHullPolygon[0], vHullPolygon[1] ) );
		ASSERT( IsConvexHull2d(pVerts,numVerts,vHullPolygon.data(),vHullPolygon.size()) );
	}
}

//}{===================================================================================================================

#if 0 //{

#include "Math/gRand.h"
#include "Core/gLog.h"

void ConvexHullBuilder2d_Test()
{
	static const int NUM_VERTS = 1024;
	static const int COORD_RANGE = 8192;
	static const int REPEAT = 1000;

	vector<Vec2i> verts(NUM_VERTS);
	vector<Vec2i> vHull;

	for(int r=0;r<REPEAT;r++)
	{
		// make some random verts to test
		int i;
		for(i=0;i<NUM_VERTS;i++)
		{
			verts[i].x = gRand::Ranged(-COORD_RANGE,COORD_RANGE);
			verts[i].y = gRand::Ranged(-COORD_RANGE,COORD_RANGE);
		}

		ConvexHullBuilder2d::Make2d( verts.data(), verts.size(), vHull );

		lprintf("num verts in hull = %d\n",vHull.size());
	}
}

#endif //}

//}======================================================================================================================

END_CB

#include "ConvexHullBuilder3d.h"
#include "IntGeometry.h"
#include "Log.h"
#include "Rand.h"

START_CB
//}{=====================================================================================================================

static bool AllVertsFront(const Facei & face,const Vec3i * pVerts,const int numVerts,const int allowedError)
{
	for(int i=0;i<numVerts;i++)
	{
		const Vec3i & v = pVerts[i];
		const Facei::ESide eCurSide = face.Side(v);
		if ( eCurSide == Facei::eBack )
		{
			if ( allowedError == 0 )
			{
				return false;
			}
			else
			{
				const double dist = face.Distance(v);
				ASSERT( dist <= 0.f );
				if ( dist <= - allowedError )
					return false;
			}
		}
	}
	return true;
}

/*
static Facei::ESide AllVertsSide(const Facei & face,const Vec3i * pVerts,const int numVerts)
{
	Facei::ESide eTotalSide = Facei::eIntersecting;
	for(int i=0;i<numVerts;i++)
	{
		const Vec3i & v = pVerts[i];
		const Facei::ESide eCurSide = face.Side(v);
		if ( eCurSide == Facei::eIntersecting )
			continue;
		if ( eTotalSide == Facei::eIntersecting )
		{
			eTotalSide = eCurSide;
		}
		else
		{
			if ( eTotalSide != eCurSide )
				return Facei::eIntersecting;
		}
	}
	return eTotalSide;
}
*/

//---------------------------------------------------------------------------

#ifdef DO_ASSERTS
static bool HullIsValid(const vector<Facei> & vHullFaces)
{
	int size = vHullFaces.size32();

	int f;
	for(f=0;f<size;f++)
	{
		const Facei & face = vHullFaces[f];

		const Edgei e0( face.a, face.b );
		const Edgei e1( face.b, face.c );
		const Edgei e2( face.c, face.a );

		int matches_e0 = 0;
		int matches_e1 = 0;
		int matches_e2 = 0;

		for(int g=0;g<size;g++)
		{
			if ( f == g ) continue;
			const Facei & vs = vHullFaces[g];
	
			// every face should be on the front side of every other face :
			Facei::ESide sidea = face.Side(vs.a);
			Facei::ESide sideb = face.Side(vs.b);
			Facei::ESide sidec = face.Side(vs.c);
			ASSERT( sidea != Facei::eBack );
			ASSERT( sideb != Facei::eBack );
			ASSERT( sidec != Facei::eBack );
			// actually, coplanar faces are needed
			//ASSERT( sidea == Facei::eFront || sideb == Facei::eFront || sidec == Facei::eFront );

			const Edgei ve0( vs.a, vs.b );
			const Edgei ve1( vs.b, vs.c );
			const Edgei ve2( vs.c, vs.a );

			// none of the edges should ever match
			ASSERT( e0 != ve0 && e0 != ve1 && e0 != ve2 );
			ASSERT( e1 != ve0 && e1 != ve1 && e1 != ve2 );
			ASSERT( e2 != ve0 && e2 != ve1 && e2 != ve2 );

			// but they may match flipped
			int num0 =	( Edgei::EqualFlipped(e0,ve0) ? 1 : 0 ) +
						( Edgei::EqualFlipped(e0,ve1) ? 1 : 0 ) +
						( Edgei::EqualFlipped(e0,ve2) ? 1 : 0 );
			int num1 =	( Edgei::EqualFlipped(e1,ve0) ? 1 : 0 ) +
						( Edgei::EqualFlipped(e1,ve1) ? 1 : 0 ) +
						( Edgei::EqualFlipped(e1,ve2) ? 1 : 0 );
			int num2 =	( Edgei::EqualFlipped(e2,ve0) ? 1 : 0 ) +
						( Edgei::EqualFlipped(e2,ve1) ? 1 : 0 ) +
						( Edgei::EqualFlipped(e2,ve2) ? 1 : 0 );
			ASSERT( (num0 + num1 + num2) < 2 );

			matches_e0 += num0;
			matches_e1 += num1;
			matches_e2 += num2;
		}

		// each edge should share exactly one other edge
		ASSERT( matches_e0 == 1 );
		ASSERT( matches_e1 == 1 );
		ASSERT( matches_e2 == 1 );
	}
	return true;
}
#endif // DO_ASSERTS

//---------------------------------------------------------------------------

bool ConvexHullBuilder3d::IsConvexHull3d(
					const Vec3i * pVerts,
					const int numVerts,
					const vector<Facei> & vHullFaces,
					const int allowedError)
{
	ASSERT( HullIsValid(vHullFaces) );

	const int numFaces = vHullFaces.size32();
	for(int h=0;h<numFaces;h++)
	{
		if ( ! AllVertsFront(vHullFaces[h],pVerts,numVerts,allowedError) )
			return false;
	}

	return true;
}

//---------------------------------------------------------------------------

static bool FindSeedTetrahedron(
				const Vec3i * pVerts,
				const int numVerts,
				vector<Facei> & vHullFaces)
{
	ASSERT( vHullFaces.size() == 0 );

	#define REQUIRE(exp)	if ( ! (exp) ) return false

	// just find any 4 non-degenerate verts to make 4 faces to start the hull

	// find the vert closest to {-inf,-inf,-inf} and {+inf,+inf,+inf}
	int iLoX = 0;
	int iHiX = 0;

	int i;

	for(i=0;i<numVerts;i++)
	{
		const Vec3i & v = pVerts[i];
		if ( v.x < pVerts[iLoX].x )
			iLoX = i;	
		else if ( v.x == pVerts[iLoX].x && v.y < pVerts[iLoX].y )
			iLoX = i;	
		else if ( v.x == pVerts[iLoX].x && v.y == pVerts[iLoX].y && v.z < pVerts[iLoX].z )
			iLoX = i;	
			
		if ( v.x > pVerts[iHiX].x )
			iHiX = i;	
		else if ( v.x == pVerts[iHiX].x && v.y > pVerts[iHiX].y )
			iHiX = i;	
		else if ( v.x == pVerts[iHiX].x && v.y == pVerts[iHiX].y && v.z > pVerts[iHiX].z )
			iHiX = i;	
	}

	// degeneracies :
	REQUIRE( iLoX != iHiX );
	REQUIRE( pVerts[iLoX] != pVerts[iHiX] );

	// now find the vert farthest from the edge Lo,Hi
	//	that's the same as the largest Area of (Lo,Hi,Vert)
	
	int iLoX2 = -1;
	double best_area = 0.0;

	for(i=0;i<numVerts;i++)
	{
		const Vec3i & v = pVerts[i];

		double area = AreaSqrD(pVerts[iLoX],pVerts[iHiX],v);
		area = ABS( area );
		if ( area > best_area )
		{
			best_area = area;
			iLoX2 = i;
		}
	}

	REQUIRE( iLoX2 != -1 );
	REQUIRE( pVerts[iLoX] != pVerts[iHiX] );
	REQUIRE( pVerts[iLoX] != pVerts[iLoX2] );
	REQUIRE( pVerts[iLoX2] != pVerts[iHiX] );
	REQUIRE( ! Colinear(pVerts[iLoX],pVerts[iLoX2],pVerts[iHiX]) );
	
	// now find the next vert farthest from the 3 of them; that's the biggest volume
	//	of the tetrahedron
	
	int iLoX3 = -1;
	int64 best_volume = 0;

	for(i=0;i<numVerts;i++)
	{
		const Vec3i & v = pVerts[i];

		int64 vol = Volume(pVerts[iLoX],pVerts[iLoX2],pVerts[iHiX],v);
		vol = ABS( vol );
		if ( vol > best_volume )
		{
			best_volume = vol;
			iLoX3 = i;
		}
	}

	REQUIRE( best_volume > 0 );
	REQUIRE( iLoX3 != -1 );
	REQUIRE( pVerts[iLoX3] != pVerts[iHiX] );
	REQUIRE( pVerts[iLoX3] != pVerts[iHiX] );
	REQUIRE( pVerts[iLoX3] != pVerts[iLoX2] );

	// so, make 4 faces from these guys

	const int indices[4] = { iLoX, iLoX2, iLoX3, iHiX };

	for(int base=0;base<4;base++)
	{	
		Facei face(	pVerts[ indices[(0+base)&3]], 
						pVerts[ indices[(1+base)&3]],
						pVerts[ indices[(2+base)&3]] );
		const Vec3i & other = pVerts[ indices[(3+base)&3]];

		// if other is on the back side of face, then flip it :
		Facei::ESide side = face.Side(other);
		REQUIRE( side != Facei::eIntersecting );
		if ( side != Facei::eFront )
		{
			face.Flip();
			ASSERT( face.Side(other) == Facei::eFront );
		}

		vHullFaces.push_back( face );
	}
	
	#undef REQUIRE

	ASSERT( HullIsValid(vHullFaces) );

	return true;
}

//---------------------------------------------------------------------------

struct EqualFlippedFunc : public std::binary_function<Edgei,Edgei,bool>
{
	bool operator() (const Edgei &a,const Edgei &b) const
	{
		return Edgei::EqualFlipped(a,b);
	}
	EqualFlippedFunc() { }
};

static void AddEdgeToHangingList(vector<Edgei> & hanging,
								const Edgei & edge)
{
	// this is O(N^2) in the number of hanging edges
	//	could improve that by using some better data structure
	//	for the hull, like Winged Edge or something.

	// see if it's in there already :
	ASSERT( hanging.find(edge) == hanging.end() );
	
	vector<Edgei>::iterator it = hanging.find_if( std::bind1st( EqualFlippedFunc(), edge) );
	if ( it == hanging.end() )
	{
		hanging.push_back(edge);
	}
	else
	{
		// just delete the old one :
		hanging.erase_u(it);
	}
}

#ifdef DO_ASSERTS
bool HanginEdgeListIsValid(const vector<Edgei> & edges,const vector<Facei> & faces)
{
	// hanging should now list all the edges which are on faces in the hull
	//	which have no matching face
	// hanging should also form a closed polygon
	
	// first check that "edges" forms a closed loop :
	const int numEdges = edges.size32();
	const int numFaces = faces.size32();

	for(int i=0;i<numEdges;i++)
	{
		const Edgei & edge = edges[i];
		const Vec3i & vert = edge.b;
		// vert should occur as someone else's "a"
		int numFound = 0;
		for(int j=0;j<numEdges;j++)
		{
			if ( i == j ) continue;
			ASSERT( edges[j].b != vert );
			if ( edges[j].a == vert )
				numFound ++;
		}
		ASSERT( numFound == 1 );
		
		// now check that all the "edges" in "edges" occur in "faces", reversed and not the other way
		numFound = 0;
		for(int f=0;f<numFaces;f++)
		{
			const Facei & face = faces[f];
			for(int e=0;e<3;e++)
			{
				static const int plus1mod3[3] = { 1, 2, 0 };
				const Vec3i & a = *(&face.a + e);
				const Vec3i & b = *(&face.a + plus1mod3[e]);
				// edge should not occur in faces in order
				ASSERT( ! (a == edge.a && b == edge.b) );
				if ( a == edge.b && b == edge.a )
					numFound++;
			}
		}
		ASSERT( numFound == 1 );
	}

	return true;
}
#endif // DO_ASSERTS

void AddVert(vector<Facei> & vHullFaces, const Vec3i & vert,
				const int allowedError)
{	
	bool reallyOutside = false;

	vector<int> backFaces;

	// walk the faces ; backwards, cuz we're deleting :
	for(int f = 0;f<vHullFaces.size32();f++)
	{
		const Facei & face = vHullFaces[f];
		Facei::ESide side = face.Side(vert);
		if ( side != Facei::eBack )
			continue; // vert should be on the front side (or on)

		// if it was colinear with any of my edges, I shoulda gotten a Facei::On , not a Back
		// true asserts, but silly now that the bug is fixed
		//ASSERT( ! Colinear(vert, face.a, face.b) );
		//ASSERT( ! Colinear(vert, face.c, face.a) );
		//ASSERT( ! Colinear(vert, face.b, face.c) );

		// I'm on the back; if I'm only slightly outside, then don't count it as being 
		//	"really" outside
		if ( allowedError == 0 )
		{
			reallyOutside = true;
		}
		else
		{
			const double dist = face.Distance(vert);
			ASSERT( dist <= 0.f );
			if ( dist <= - allowedError )
				reallyOutside = true;
			// otherwise, vert is nearly on face
		}

		backFaces.push_back(f);
	}

	// this is the fast path; no changes made to the hull
	if ( ! reallyOutside )
		return;

	vector<Edgei> hanging;

	// walk the faces ; backwards, cuz we're deleting :
	for(int bf = backFaces.size32() - 1; bf >= 0; bf--)
	{
		const int f = backFaces[bf];
		const Facei & face = vHullFaces[f];

		// vert's on the back side ! this face is no good
		// remove it, and push all its edges to the hanging list
		// (winding matters)
		AddEdgeToHangingList(hanging, Edgei(face.a, face.b) );
		AddEdgeToHangingList(hanging, Edgei(face.b, face.c) );
		AddEdgeToHangingList(hanging, Edgei(face.c, face.a) );

		vHullFaces.erase_u( vHullFaces.begin() + f );
	}

	// vert will end up being added to the hull

	// hanging should now list all the edges which are on faces in the hull
	//	which have no matching face
	// hanging should also form a closed polygon
	ASSERT( HanginEdgeListIsValid(hanging,vHullFaces) );

	// add faces from all the edges in "hanging" to "vert"
	vHullFaces.reserve( vHullFaces.size() + hanging.size() );
	for(int e=0;e<hanging.size32();e++)
	{
		const Edgei & edge = hanging[e];
		// add [edge,vert]
		vHullFaces.push_back( Facei(edge.a,edge.b,vert) );
	}

	// this is too expensive :
	//@@ turn it on if something goes bad
	if ( 0 )
	{
		ASSERT( HullIsValid(vHullFaces) );
	}
}

//---------------------------------------------------------------------------

/*******************

10-1-01 :

Idea for O(H*N) exact convex hull in 3d :
Build incrementally.

Start with four (non-degenerate) points.
This constitutes your current hull.
Add the remaining points one by one and
 update the hull at each step.

How to update the hull for a new vert :

Walk over all faces in the hull (this is O(H))
  1. If the point is in front of the face (inside the hull)
		then do nothing
  2. If it's behind that face, then remove that face
	    and push all the edges on that face to a list

After all that, walk over all the edges in the pushed list,
remove any that don't lie on a face still in the hull (the
result is a list of all edges which are on only one face in
the hull).  Make triangles from the new vert to all those
edges and add them to the hull.

H is typically O(logN) but can be as large as N in the
worst case.

********************/

// Convex Hull in 3d by Incremental Addition
//	this is O(N*H) ;
// returns an empty list of faces if the input verts are degenerate
void ConvexHullBuilder3d::Make3d_Incremental(	
				const Vec3i * pVerts,
				const int numVerts,
				vector<Facei> & vHullFaces,
				const int allowedError /*= 0.f*/)
{	
	vHullFaces.clear();

	ASSERT( pVerts );
	if ( numVerts < 4 )
	{
		lprintf("ConvexHullBuilder3d::Make3d_Incremental called on degenerate set\n");
		return; // degenerate!
	}
			
	if ( ! FindSeedTetrahedron(pVerts,numVerts,vHullFaces) )
	{
		lprintf("ConvexHullBuilder3d::Make3d_Incremental called on degenerate set\n");
		vHullFaces.clear();
		return; // degenerate!
	}

	ASSERT( vHullFaces.size() >= 4 );
	
	// make a random permutation of indices
	
	vector<int> permutation((vecsize_t)numVerts,0);
	int i;
	for(i=0;i<numVerts;i++)
	{
		permutation[i] = i;
	}

	// do a bunch of swaps : 
	for(i=0;i<numVerts;i++)
	{
		/*
		const int a = irandranged(0,numVerts-1);
		const int b = irandranged(0,numVerts-1);
		Swap( permutation[a] , permutation[b] );
		*/
		const int b = irandranged(i,numVerts-1);
		Swap( permutation[i] , permutation[b] );
	}

	//@@ is there a way to favor adding more extreme verts first ?

	// add verts one by one :

	int logCounter=0;
	int logQuantum = numVerts/10;
	#ifdef _DEBUG
	logQuantum = MIN(100,logQuantum);
	#else
	logQuantum = MIN(10000,logQuantum);
	#endif

	lprintf("ConvexHullBuilder3d::Make3d_Incremental starting\n");
	lprintf("0/%d verts \r",numVerts);

	for(i=0;i<numVerts;i++)
	{
		AddVert(vHullFaces, pVerts[ permutation[i] ], allowedError);

		// at this point I should be the hull for verts 0 to i (permuted!)

		if ( ++ logCounter >= logQuantum )
		{
			//ASSERT( HullIsValid(vHullFaces) ); //@@
			logCounter = 0;
			lprintf("%d/%d verts \r",i,numVerts);
		}
	}
	lprintf("%d/%d verts\n",numVerts,numVerts);

	ASSERT( IsConvexHull3d(pVerts,numVerts,vHullFaces,allowedError) );
}

//}{===================================================================================================================

void ConvexHullBuilder3d::GetHullVerts(const vector<Facei> & vHullFaces,
						vector<Vec3i> & vHullVerts)
{
	vHullVerts.clear();
	vHullVerts.reserve( vHullFaces.size() * 3 );

	for(int f=0;f<vHullFaces.size32();f++)
	{
		vHullVerts.push_back( vHullFaces[f].a );
		vHullVerts.push_back( vHullFaces[f].b );
		vHullVerts.push_back( vHullFaces[f].c );
	}
	
	RemoveDegenerateVerts(vHullVerts);
}

static bool SortVertOnX(const Vec3i & a,const Vec3i & b)
{
	return a.x < b.x;
}

void ConvexHullBuilder3d::RemoveDegenerateVerts(vector<Vec3i> & verts)
{
	// collapse degenerates :
	
	// sort :

	std::sort( verts.begin(), verts.end(), SortVertOnX );

	// now run through the verts and see if they snap :

	// go backwards, cuz we're deleting:
	for(int vertI= verts.size32()-1;vertI>=0;vertI--)
	{
		const Vec3i & vert = verts[vertI];

		// search down from there :

		for(int vertJ=vertI-1;vertJ>=0;vertJ--)
		{
			const Vec3i & vsVert = verts[vertJ];

			// this is the key to the speed of this search :
			//	the list is sorted on x,
			//	so early out if we see a vert out of our x range
			if ( vert.x != vsVert.x )
			{
				// no snap :
				break;
			}

			if ( vert == vsVert )
			{
				// snap 'em !
				// just delete vertI
				verts.erase_u( verts.begin() + vertI);
				break;
			}
		}
	}
}

//}{===================================================================================================================

#if 1 //{

void ConvexHullBuilder3d_Test()
{
	static const int NUM_VERTS = 512;
	static const int COORD_RANGE = 8192;
	static const int REPEAT = 1000;

	vector<Vec3i> verts(NUM_VERTS,Vec3i(0,0,0));
	vector<Facei> vHullFaces;
	vector<Vec3i> vHullVerts;

	for(int r=0;r<REPEAT;r++)
	{
		// make some random verts to test
		int i;
		for(i=0;i<NUM_VERTS;i++)
		{
			verts[i].x = irandranged(-COORD_RANGE,COORD_RANGE);
			verts[i].y = irandranged(-COORD_RANGE,COORD_RANGE);
			verts[i].z = irandranged(-COORD_RANGE,COORD_RANGE);
		}

		ConvexHullBuilder3d::Make3d_Incremental( verts.data(), verts.size32(), vHullFaces );

		if ( ! ConvexHullBuilder3d::IsConvexHull3d(verts.data(), verts.size32(),vHullFaces) )
			lprintf("FAILURE!\n");

		lprintf("num faces in hull = %d\n",vHullFaces.size32());

		ConvexHullBuilder3d::GetHullVerts(vHullFaces,vHullVerts);
		
		lprintf("num verts in hull = %d\n",vHullVerts.size32());

		ASSERT( (vHullVerts.size32()-2)*2 == vHullFaces.size32() );
	}
}

#endif //}

//}======================================================================================================================

END_CB

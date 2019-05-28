#include "Triangulate2d.h"
#include "Log.h"
#include "Vec2U.h"
#include "Rect.h"
#include "VolumeBuilder.h"
#include "IntGeometry.h"
#include "ConvexHullBuilder2d.h"

START_CB

/*

This Triangulator is a simple "Ear clip" algorithm.

It doesn't handle self-intersection.

It's pretty easy to handle self-overlap, which is what I want for triangulating
3d polys projected into 2d.  You just use the recursive spit method.  For your
N-point polyline, find two verts such that the segment between them is inside
the polyline and doesn't intersect any of the edges.  Use that segment to cut
the polyline into two new ones and recurse.


*/

/*
  InsideTriangle decides if a point P is Inside of the triangle
  defined by A, B, C.
*/
bool Triangulate2d_InsideTriangle(
								const Vec2 & A,
								const Vec2 & B,
								const Vec2 & C,
								const Vec2 & P)

{
  float ax, ay, bx, by, cx, cy, apx, apy, bpx, bpy, cpx, cpy;
  float cCROSSap, bCROSScp, aCROSSbp;

  ax = C.x - B.x;  ay = C.y - B.y;
  bx = A.x - C.x;  by = A.y - C.y;
  cx = B.x - A.x;  cy = B.y - A.y;
  apx= P.x - A.x;  apy= P.y - A.y;
  bpx= P.x - B.x;  bpy= P.y - B.y;
  cpx= P.x - C.x;  cpy= P.y - C.y;

  aCROSSbp = ax*bpy - ay*bpx;
  cCROSSap = cx*apy - cy*apx;
  bCROSScp = bx*cpy - by*cpx;

  return ((aCROSSbp >= 0.0f) && (bCROSScp >= 0.0f) && (cCROSSap >= 0.0f));
};

bool Triangulate2d_Snip(const vector<Vec2> &contour,int u,int v,int w,int n,int *V)
{
	const Vec2 & A = contour[ V[u] ];
	const Vec2 & B = contour[ V[v] ];
	const Vec2 & C = contour[ V[w] ];

	if ( TriangleAreaCCW(A,B,C) <= 0.f )
		return false;

	for(int p=0;p<n;p++)
	{
		if ( (p == u) || (p == v) || (p == w) )
			continue;
	
		const Vec2 & P = contour[ V[p] ];

		if ( Triangulate2d_InsideTriangle(A,B,C,P) )
			return false;
	}

	return true;
}

/*
  InsideTriangle decides if a point P is Inside of the triangle
  defined by A, B, C.

  what about the on-edge cases?  I shouldn't generally allow them
	-> right
*/
bool Triangulate2d_InsideTriangle(
								const Vec2i & A,
								const Vec2i & B,
								const Vec2i & C,
								const Vec2i & P)
{
	ASSERT( Area(A,B,C) > 0 );

	if ( Area(A,B,P) < 0 )
		return false;
	if ( Area(B,C,P) < 0 )
		return false;
	if ( Area(C,A,P) < 0 )
		return false;

	return true;
}

bool Triangulate2d_Snip(const vector<Vec2i> &contour,int u,int v,int w,int n,int *V)
{
	const Vec2i & A = contour[ V[u] ];
	const Vec2i & B = contour[ V[v] ];
	const Vec2i & C = contour[ V[w] ];

	if ( Area(A,B,C) <= 0 )
		return false;

	for(int p=0;p<n;p++)
	{
		if ( (p == u) || (p == v) || (p == w) )
			continue;
	
		const Vec2i & P = contour[ V[p] ];

		if ( Triangulate2d_InsideTriangle(A,B,C,P) )
			return false;
	}

	return true;
}

bool Triangulate2d_Approx(const vector<Vec2> & contour,vector<int> * pTriangles)
{
	vector<int> & triangles = *pTriangles;
	triangles.clear();
	
	/* allocate and initialize list of Vertices in polygon */

	int n = contour.size32();
	ASSERT( n >= 3 );

	vector<int> V;
	V.resize(n);

	int v;
	for (v=0; v<n; v++)
	{
		V[v] = v;
	}

	int nv = n;

	/*  remove nv-2 Vertices, creating 1 triangle every time */
	int count = 2*nv;   /* error detection */

	for(v=nv-1; nv>2; )
	{
		/* if we loop, it is probably a non-simple polygon */
		count--;
		if (count < 0)
		{
			//** Triangulate: ERROR - probable bad polygon!
			return false;
		}

		/* three consecutive vertices in current polygon, <u,v,w> */
		int u = v % nv;
		v = (u+1)%nv;
		int w = (v+1)%nv;

		if ( Triangulate2d_Snip(contour,u,v,w,nv,V.data()) )
		{
			int a,b,c,s,t;

			/* true names of the vertices */
			a = V[u]; b = V[v]; c = V[w];

			/* output Triangle */
			triangles.push_back( a );
			triangles.push_back( b );
			triangles.push_back( c );

			/* remove v from remaining polygon */
			for(s=v,t=v+1;t<nv;s++,t++)
			{
				V[s] = V[t];
			}

			nv--;

			/* resest error detection counter */
			count = 2*nv;
		}
	}

	return true;
}

bool Triangulate2d_ViaInts(const vector<Vec2> & contour,vector<int> * pTriangles)
{
	int s = contour.size32();
	
	vector<Vec2i> contouri;
	contouri.resize(s);

	RectF rect(0,0,0,0);
	BoundingRectangle(&rect,contour.data(),s);
	IntGeometry::Quantize(contouri.data(),contour.data(),s,rect);

	return Triangulate2d(contouri,pTriangles);
}

bool Triangulate2d(const vector<Vec2i> & contour,vector<int> * pTriangles)
{
	vector<int> & triangles = *pTriangles;
	triangles.clear();
	
	/* allocate and initialize list of Vertices in polygon */

	int n = contour.size32();
	ASSERT( n >= 3 );

	vector<int> V;
	V.resize(n);

	int v;
	for (v=0; v<n; v++)
	{
		V[v] = v;
	}

	int nv = n;

	/*  remove nv-2 Vertices, creating 1 triangle every time */
	int count = 2*nv;   /* error detection */

	for(v=nv-1; nv>2; )
	{
		/* if we loop, it is probably a non-simple polygon */
		count--;
		if (count < 0)
		{
			//** Triangulate: ERROR - probable bad polygon!
			return false;
		}

		/* three consecutive vertices in current polygon, <u,v,w> */
		int u = v % nv;
		v = (u+1)%nv;
		int w = (v+1)%nv;

		if ( Triangulate2d_Snip(contour,u,v,w,nv,V.data()) )
		{
			int a,b,c,s,t;

			/* true names of the vertices */
			a = V[u]; b = V[v]; c = V[w];

			/* output Triangle */
			if ( Area(contour[a],contour[b],contour[c]) != 0 )
			{
				ASSERT( Area(contour[a],contour[b],contour[c]) > 0 );
				triangles.push_back( a );
				triangles.push_back( b );
				triangles.push_back( c );
			}

			/* remove v from remaining polygon */
			for(s=v,t=v+1;t<nv;s++,t++)
			{
				V[s] = V[t];
			}

			nv--;

			/* resest error detection counter */
			count = 2*nv;
		}
	}

	return true;
}

//===========================================================================

// is d in the circle around [a,b,c] ?
bool InCircle(
	const Vec2i & a,
	const Vec2i & b,
	const Vec2i & c,
	const Vec2i & d)
{

	Vec2i ad = a - d;
	Vec2i bd = b - d;
	Vec2i cd = c - d;

	int64 abdet = ad.x * bd.y - bd.x * ad.y;
	int64 bcdet = bd.x * cd.y - cd.x * bd.y;
	int64 cadet = cd.x * ad.y - ad.x * cd.y;
	int64 alift = ad.x * ad.x + ad.y * ad.y;
	int64 blift = bd.x * bd.x + bd.y * bd.y;
	int64 clift = cd.x * cd.x + cd.y * cd.y;

	// this all exact still, but now we're getting into trouble :
	// @@ this requires enough bits to do
	//	[Val]^4 * 12
	// my int is 20 bits, so that's 84 bits needed

	double ta = double(alift)*double(bcdet);
	double tb = double(blift)*double(cadet);
	double tc = double(clift)*double(abdet);

	double result = ta + tb + tc;

	// if result is very small compared to ta,tb,tc , then
	//	something may have gone horribly wrong

	#ifdef DO_ASSERTS
	{
		double r = fabs(result);
		double t = fabs(ta) + fabs(tb) + fabs(tc);
		double large = 1.0e018; // about 2^60
		if ( (r * large) < t )
		{
			// r is tiny compared to t, bad news !!
			FAIL("Need more precision!");
		}
	}
	#endif

	return (result > 0.f); // @@@@ right sign?
}

inline bool Match(int a,int b,int c,int d)
{
	ASSERT( ! (a== d && b == c) );
	return (a==c && b==d);
}

__forceinline int64 abs64(int64 a)
{
	if ( a < 0 ) return -a;
	return a;
}

bool Greater(int64 num1,int64 den1, int64 num2,int64 den2)
{
	// compare (num1/den1) > (num2/den2)
	// @@@@ can I do this exactly?
	double left  = double(num1)*double(den2);
	double right = double(num2)*double(den1);
	return left > right;
}

// cotangent goes from infinity (zero angle) to -infinity (180)
// cotangent(90) = 0
// cotangent(60) = sqrt(1/3)
void GetSmallestAngleCot(
	const Vec2i & a,
	const Vec2i & b,
	const Vec2i & c,
	const Vec2i & d,
	int64 * pNum,
	int64 * pDen)
{
	int64 ab = DistanceSqr(a,b);
// int64 cd = DistanceSqr(c,d);
	int64 ad = DistanceSqr(a,d);
	int64 db = DistanceSqr(d,b);
	int64 bc = DistanceSqr(b,c);
	int64 ca = DistanceSqr(c,a);
	
	// larger cot = smaller angle
	int64 cot_bca = bc + ca - ab;
	int64 cot_cab = ca + ab - bc;
	int64 cot_abc = ab + bc - ca;

	int64 max_abc = MAX3(cot_bca,cot_cab,cot_abc);
	int64 area_abc = Area(a,b,c);
	ASSERT( area_abc > 0 );

	int64 cot_dba = db + ab - ad;
	int64 cot_bad = ab + ad - db;
	int64 cot_adb = ad + db - ab;

	int64 max_adb = MAX3(cot_dba,cot_bad,cot_adb);
	int64 area_adb = Area(a,d,b);
	ASSERT( area_adb > 0 );

	// what's bigger , (max_abc/a_abc) or (max_adb/a_adb) ?
	
	if ( Greater(max_abc,area_abc, max_adb,area_adb) )
	{
		*pNum = max_abc;
		*pDen = area_abc;
	}
	else
	{
		*pNum = max_adb;
		*pDen = area_adb;
	}
}

double Sliverness(
	const Vec2i & a,
	const Vec2i & b,
	const Vec2i & c)
{
	// Sliverness is Perimeter^2 / Area
	//	it's infinite for a nasty sliver
	//	min is 20.78 for an equilateral triangle
	int64 area = Area(a,b,c);
	ASSERT(area > 0.f);

	// approximate with sum of distance squares instead
	//	of square of sum of distances ;
	// doing this actually biases sliverness even more towards
	//	non-equilateral triangles.
	// in fact you can do "sliverness" just by doing
	//	[Sum of Sides Squared]/[Square of Sum of Sides]
	// like an s-dev type of thing

	int64 perimSqr = DistanceSqr(a,b) + DistanceSqr(b,c) + DistanceSqr(c,a);

	//double perim = sqrt((double)DistanceSqr(a,b)) + sqrt((double)DistanceSqr(b,c)) + sqrt((double)DistanceSqr(c,a));
	//double perimSqr = perim*perim;

	return double(perimSqr)/double(area);
}

double WorstSliverness(
	const Vec2i & a,
	const Vec2i & b,
	const Vec2i & c,
	const Vec2i & d)
{
	// of a,b,c and b,a,d
	double s1 = Sliverness(a,b,c);
	double s2 = Sliverness(b,a,d);
	return MAX(s1,s2);
}

bool DelaunayFlip(
	const Vec2i & a,
	const Vec2i & b,
	const Vec2i & c,
	const Vec2i & d)
{
	ASSERT( Area(a,b,c) > 0 );
	ASSERT( Area(a,d,b) > 0 );

	// check for star-trek symbol :
	int64 cad = Area(c,a,d);
	if ( cad <= 0 )
		return false;
	int64 cdb = Area(c,d,b);
	if ( cdb <= 0 )
		return false;

/*
	// I can't make this work; it has illegal loops

	bool cir1 = InCircle(a,b,c,d);
	bool cir2 = InCircle(a,d,b,c);

	//#ifdef DO_ASSERTS
	//ASSERT( cir1 == cir2 );
	//bool cir3 = InCircle(a,d,c,b);
	//bool cir4 = InCircle(c,d,b,a);
	//bool cir34 = cir3 || cir4;
	//ASSERT( cir3 == cir4 );
	//ASSERT( ! ( cir12 && cir34 ) );
	//#endif
	
	return cir1 && cir2;

*/

/*
	// are we improving the *smallest* area ?
	// this is a nice test because it's exact, and doesn't reverse
	//	not so great because it encourages slivers

	int64 abc = Area(a,b,c);
	int64 adb = Area(a,d,b);

	int64 minSrc = MIN( abc,adb );
	int64 minDst = MIN( cad,cdb );

	return ( minDst > minSrc );		

/**/

//*
	// are we improving the *worst* sliverness

	double src = WorstSliverness(a,b,c,d);
	double dst = WorstSliverness(c,d,b,a);

	// big Sliverness is bad, so take it if it
	//	went down

	return ( dst < src );		

/**/

/*
	// are we improving the smallest angle ?
	int64 src_num,src_den;
	GetSmallestAngleCot(a,b,c,d,&src_num,&src_den);
	int64 dst_num,dst_den;
	GetSmallestAngleCot(c,d,b,a,&dst_num,&dst_den);

	// larger cot = smaller angle
	// we want a bigger angle = smaller cot
	//
	// we want the lower value of src or dst
	//	do a flip if dst is lower (src is Greater)

	// for debug, make the angles :
	double src_cot = double(src_num) / (4.0 * double(src_den));
	double dst_cot = double(dst_num) / (4.0 * double(dst_den));
	float src_ang = fradtodeg( (float) atan(1.0/src_cot) );
	float dst_ang = fradtodeg( (float) atan(1.0/dst_cot) );

	bool ret = Greater(src_num,src_den,dst_num,dst_den);
	return ret;
/**/

}

bool DelaunayFlip(const vector<Vec2i> & contour, int e0,int e1,int a,int b)
{
	const Vec2i & A = contour[e0];
	const Vec2i & B = contour[e1];
	const Vec2i & C = contour[a];
	const Vec2i & D = contour[b];

	if ( DelaunayFlip(A,B,C,D) )
	{
		// make sure we won't try to do the opposite !
		ASSERT( ! DelaunayFlip(C,D,B,A) );
		return true;
	}
	return false;
}

bool DelaunayFlip(const vector<Vec2i> & contour, int * tri1,int * tri2)
{
	// this mutates tri1 and tri2 if a flip is found

	int u0 = tri2[0];
	int u1 = tri2[1];
	int u2 = tri2[2];

	ASSERT( Area( contour[u0],contour[u1],contour[u2] ) > 0 );

	for(int e=0;e<3;e++)
	{
		static const int plus1mod3[3] = { 1, 2, 0 };
		static const int plus2mod3[3] = { 2, 0, 1 };

		int v0 = tri1[e];
		int v1 = tri1[plus1mod3[e]];
		int v2 = tri1[plus2mod3[e]];
		
		ASSERT( Area( contour[v0],contour[v1],contour[v2] ) > 0 );

		if ( Match(v0,v1, u1,u0) )
		{
			if ( DelaunayFlip(contour,v0,v1,v2,u2) )
			{
				tri1[0] = v0;
				tri1[1] = u2;
				tri1[2] = v2;
				tri2[0] = v2;
				tri2[1] = u2;
				tri2[2] = v1;
				return true;
			}
			return false;
		}
		else if ( Match(v0,v1, u2,u1) )
		{
			if ( DelaunayFlip(contour,v0,v1,v2,u0) )
			{
				tri1[0] = v0;
				tri1[1] = u0;
				tri1[2] = v2;
				tri2[0] = v2;
				tri2[1] = u0;
				tri2[2] = v1;
				return true;
			}
			return false;
		}
		else if ( Match(v0,v1, u0,u2) )
		{
			if ( DelaunayFlip(contour,v0,v1,v2,u1) )
			{
				tri1[0] = v0;
				tri1[1] = u1;
				tri1[2] = v2;
				tri2[0] = v2;
				tri2[1] = u1;
				tri2[2] = v1;
				return true;
			}
			return false;
		}
	}
	return false;
}

void DelaunayImprove2d(const vector<Vec2i> & contour,const vector<int> & sourceTriangles,vector<int> * pTriangles)
{
	// I really need a little winged-edge, but fuck it for now

	pTriangles->assignv( sourceTriangles );
	int numTris = sourceTriangles.size32()/3;

	vector<int> & cur = *pTriangles;
	
	int repeat = 0;
	for(;;)
	{
		bool didFlipAnyTri = false;
		for(int t=0;t<numTris;t++)
		{
			int * tri1 = cur.data() + t*3;
				
			// find the edge [v0,v1] in the list to see what the other vert is :
			// only search upward in t
			for(int t2=(t+1);t2<numTris;t2++)
			{
				int * tri2 = cur.data() + t2*3;
				
				if ( DelaunayFlip(contour,tri1,tri2) )
				{
					// make sure it won't undo what I just did !!
					ASSERT( ! DelaunayFlip(contour,tri1,tri2) );
					didFlipAnyTri = true;
					//break;
				}
			}
		}
		if ( ! didFlipAnyTri )
			return; // all done
		repeat++;
		if ( repeat > numTris )
		{
			// crapper, didn't terminate ?
			lprintf("Crapper\n");
			return;
		}
	}
}


bool DelaunayTriangulate2d(const vector<Vec2i> & contour,vector<int> * pTriangles)
{
	vector<int> temp;
	if ( ! Triangulate2d(contour,&temp) )
		return false;

	DelaunayImprove2d(contour,temp,pTriangles);
	return true;
}

bool DelaunayTriangulate2d_ViaInts(const vector<Vec2> & contourf,vector<int> * pTriangles)
{
	int s = contourf.size32();
	
	vector<Vec2i> contour;
	contour.resize(s);

	RectF rect(0,0,0,0);
	BoundingRectangle(&rect,contourf.data(),s);
	IntGeometry::Quantize(contour.data(),contourf.data(),s,rect);

	vector<int> temp;
	if ( ! Triangulate2d(contour,&temp) )
		return false;

	DelaunayImprove2d(contour,temp,pTriangles);
	return true;
}

bool DelaunayTriangulate2d_PointCloudHull(const vector<Vec2i> & points,vector<int> * pTriangles)
{
	vector<Vec2i> vHull;
	ConvexHullBuilder2d::Make2d(points.data(),points.size32(),vHull);
	
	vector<int> tris;
	if ( ! Triangulate2d(vHull,&tris) )
		return false;
		
	// tris now indexes into vHull , not points, need to fix that
	vector<int> hullToPoints( vHull.size(), -1 );
	for(int h=0;h<vHull.size32();h++)
	{
		const Vec2i * pFound = std::find( points.begin(), points.end(), vHull[h] );
		ASSERT( pFound != points.end() );
		hullToPoints[h] = ptr_diff_32( pFound - points.begin() );
	}
	
	for(int t=0;t<tris.size32();t++)
	{
		tris[t] = hullToPoints[ tris[t] ];
	}	
		
	// add the points :
	
	for(int p=0;p<points.size32();p++)
	{
		const Vec2i & P = points[p];
		
		if ( std::find(vHull.begin(),vHull.end(),P) != vHull.end() )
			continue;
		
		// add the point, look for the tri(s) it's on :
		//	(can be more than one if it's on an edge)
		
		int numInTris = 0;
		
		for(int t=0;t<tris.size32();t+=3)
		{
			int a = tris[t+0];
			int b = tris[t+1];
			int c = tris[t+2];
			
			const Vec2i & A = points[ a ];
			const Vec2i & B = points[ b ];
			const Vec2i & C = points[ c ];
			
			if ( A == P || B == P || C == P )
				continue;
			
			if ( Triangulate2d_InsideTriangle(A,B,C,P) )
			{
				// remove ABC
				// make ABP,PBC,APC
				
				// change ABC to ABP :
				tris[t+2] = p;
				
				tris.push_back( a );
				tris.push_back( p );
				tris.push_back( c );
				
				tris.push_back( p );
				tris.push_back( b );
				tris.push_back( c );
				
				numInTris++;
			}
		}
		
		ASSERT( numInTris >= 1 && numInTris <= 2 );
	}

	DelaunayImprove2d(points,tris,pTriangles);
	return true;	
}

bool DelaunayTriangulate2d_PointCloudHull(const vector<Vec2> & contourf,vector<int> * pTriangles)
{
	int s = contourf.size32();
	
	vector<Vec2i> contour;
	contour.resize(s);

	RectF rect(0,0,0,0);
	BoundingRectangle(&rect,contourf.data(),s);
	IntGeometry::Quantize(contour.data(),contourf.data(),s,rect);

	return DelaunayTriangulate2d_PointCloudHull(contour,pTriangles);
}

END_CB

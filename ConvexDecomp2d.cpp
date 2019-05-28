#include "Triangulate2d.h"

START_CB

// A,B,C should be counter-clockwise
bool IsReflex(const Vec2i & A,const Vec2i & B,const Vec2i & C)
{
	return ( Area(A,B,C) < 0 );
}

bool IsReflex(const vector<Vec2i> & verts,const int a,const int b, const int c)
{
	return ( Area(verts[a],verts[b],verts[c]) < 0 );
}

bool IsReflex(const vector<Vec2i> & verts,const vector<int> & indeces,const int a,const int b, const int c)
{
	return ( Area(verts[indeces[a]],verts[indeces[b]],verts[indeces[c]]) < 0 );
}

bool IsConvex(const vector<Vec2i> & verts,const vector<int> & indeces)
{
	int ixsize = indeces.size32();
	int cur;
	for(cur = 0;cur<ixsize;cur++)
	{
		int prev = (cur + ixsize-1)%ixsize;
		int next = (cur + 1)%ixsize;
	
		if ( IsReflex(verts,indeces,prev,cur,next) )
			return false;
	}
	
	return true;
}

// return |a-b| on the mod loop [0,m-1]
int ModDifference(int a,int b,int m)
{
	int d = a - b;
	if ( d > (m/2) )
		d -= m;
	else if ( d < -(m/2) )
		d += m;
	ASSERT( abs(d) <= (m+1)/2 );
	return d;
}

bool ModDifferenceIs1(int a,int b,int m)
{
	ASSERT( a >= 0 && a < m );
	ASSERT( b >= 0 && b < m );

	if ( (a == b+1) || (a == b-1) )
		return true;

	if ( a == 0 && b == m-1 )
		return true;
	if ( b == 0 && a == m-1 )
		return true;
	
	return false;	
}

inline bool SignsDiffer(int64 s1,int64 s2)
{
	return	(s1 > 0 && s2 < 0) ||
			(s1 < 0 && s2 > 0);
}

inline bool Between(const Vec2i & A,const Vec2i & B,const Vec2i & C)
{
	ASSERT( A != B );
	ASSERT( Area(A,B,C) == 0 );

	// I know C is on [AB]
	if ( B.x > A.x )
	{
		return ( C.x >= A.x && C.x <= B.x );
	}
	else if ( B.x < A.x )
	{
		return ( C.x <= A.x && C.x >= B.x );
	}
	else if ( B.y > A.y )
	{
		return ( C.y >= A.y && C.y <= B.y );
	}
	else
	{
		ASSERT( B.y < A.y );
		return ( C.y <= A.y && C.y >= B.y );
	}
}

// return if [AB] intersects [CD]
bool IntersectsOrTouches(const Vec2i & A,const Vec2i & B,const Vec2i & C,const Vec2i & D)
{
	int64 abc = Area(A,B,C);
	int64 abd = Area(A,B,D);
	int64 cda = Area(C,D,A);
	int64 cdb = Area(C,D,B);

	if ( abc == 0 && Between(A,B,C) )
	{
		return true;	
	}
	if ( abd == 0 && Between(A,B,D) )
	{
		return true;	
	}
	if ( cda == 0 && Between(C,D,A) )
	{
		return true;	
	}
	if ( cdb == 0 && Between(C,D,B) )
	{
		return true;	
	}

	return
		SignsDiffer(abc,abd) &&
		SignsDiffer(cda,cdb);	
}

bool IntersectsAndCrosses(const Vec2i & A,const Vec2i & B,const Vec2i & C,const Vec2i & D)
{
	int64 abc = Area(A,B,C);
	int64 abd = Area(A,B,D);
	int64 cda = Area(C,D,A);
	int64 cdb = Area(C,D,B);

	return
		SignsDiffer(abc,abd) &&
		SignsDiffer(cda,cdb);	
}

// return if [AB] intersects [CD]
bool Intersects(const Vec2 & f1,const Vec2 & t1,const Vec2 & f2,const Vec2 & t2)
{
	// check segment-segment intersection

	const Vec2 d1 = t1 - f1;
	const Vec2 & b1 = f1;

	const Vec2 d2 = t2 - f2;
	const Vec2 & b2 = f2;

	const double d1d2 = d1*d2;

	const Vec2 b2Mb1 = b2 - b1;

	const double A = d1.LengthSqr();
	const double B = d2.LengthSqr();
	const double C = d1*b2Mb1;
	const double D = d2*b2Mb1;

	const double denom = A*B - d1d2*d1d2;
	if ( denom <= 0.0 )
		return false; // that means d1 and d2 are parallel

	const double s = (B*C - D*d1d2);
	const double t = (C*d1d2 - A*D);

	// divide s & t by "denom" to make unit parameters

	if ( s >= 0.0 && s <= denom && t >= 0.0 && t <= denom )
	{
		// that's an interesction !!
		// check it :
		#ifdef DO_ASSERTS
		const double ss = s / denom;
		const double tt = t / denom;
		const Vec2 p1 = b1 + d1 * float(ss);
		const Vec2 p2 = b2 + d2 * float(tt);
		ASSERT( Vec2::Equals(p1,p2) );
		#endif

		return true;
	}

	return false;
}

bool CheckSelfIntersection2d(const vector<Vec2i> & contour)
{
	int s = contour.size32();
	for(int i=0;i<s;i++)
	{
		const Vec2i & A = contour[i];
		const Vec2i & B = contour[(i+1)%s];
		for(int j=i+1;j<s;j++)
		{
			const Vec2i & C = contour[j];
			const Vec2i & D = contour[(j+1)%s];
			if ( IntersectsAndCrosses(A,B,C,D) )
				return true;
		}
	}
	return false;
}

bool CheckSelfIntersection2d(const vector<Vec2> & contour)
{
	int s = contour.size32();
	for(int i=0;i<s;i++)
	{
		const Vec2 & A = contour[(i+s)%s];
		const Vec2 & B = contour[(i+1)%s];
		for(int j=i+2;j<s;j++)
		{
			const Vec2 & C = contour[j];
			const Vec2 & D = contour[(j+1)%s];
			if ( i == 0 && j == (s-1) )
				continue;
			if ( Intersects(A,B,C,D) )
				return true;
		}
	}
	return false;
}

bool CheckIntersection(const Vec2i & A,const Vec2i & B, const vector<Vec2i> & verts,const vector<int> & indeces)
{
	int ixsize = indeces.size32();
	for(int i=0;i<(ixsize-1);i++)
	{
		const Vec2i & C = verts[ indeces[i  ] ];
		const Vec2i & D = verts[ indeces[i+1] ];
	
		if ( IntersectsAndCrosses(A,B,C,D) )
			return true;
	}

	// loop case :
	{
		const Vec2i & C = verts[ indeces.front() ];
		const Vec2i & D = verts[ indeces.back() ];
	
		if ( IntersectsAndCrosses(A,B,C,D) )
			return true;
	}

	return false;
}

bool IsOutside(const Vec2i & AP,const Vec2i & A,const Vec2i & AN, const Vec2i & B)
{
	bool r1 = Right(AP,A,B);
	bool r2 = Right(A,AN,B);
	if ( IsReflex(AP,A,AN) )
	{
		return (r1 && r2);
	}
	else
	{
		return (r1 || r2);
	}
}	

// can recurse:
void MakeConvexPieces(const vector<Vec2i> & verts,const vector<int> & indeces,
						const vector<int> & allIndeces,
						vector< vector<int> > * pPolygons);

void Break(const vector<Vec2i> & verts,
						const vector<int> & indeces,
						const vector<int> & allIndeces,
						vector< vector<int> > * pPolygons, int v1,int v2)
{
	// recurse
	// v1 & v2 cur go into both halves,
	//	all other verts go into one or the other
	vector<int>	ix1;
	vector<int>	ix2;
	int ixsize = indeces.size32();
	int i;
	for(i=v1;;i++)
	{
		if ( i == ixsize )
			i = 0;
		ix1.push_back( indeces[i] );
		if ( i == v2 )
			break;
	}
	for(i=v2;;i++)
	{
		if ( i == ixsize )
			i = 0;
		ix2.push_back( indeces[i] );
		if ( i == v1 )
			break;
	}
	ASSERT( ix1.size() < indeces.size() );
	ASSERT( ix2.size() < indeces.size() );
	ASSERT( (ix1.size() + ix2.size()) == (indeces.size()+2) );
	MakeConvexPieces(verts,ix1,allIndeces,pPolygons);
	MakeConvexPieces(verts,ix2,allIndeces,pPolygons);
}

bool PinchReflexReflex(const vector<Vec2i> & verts,const vector<int> & indeces,
						const vector<int> & allIndeces,
						vector< vector<int> > * pPolygons)
{
	// consider two reflexes that are adjacent in the loop
	// the poly between them must be convex
	// 

	int lastReflexI = -1;
	int firstReflexI = -1;
	bool looped = false;
	int ixsize = indeces.size32();
	for(int cur = 0;;cur++)
	{
		if ( cur == ixsize )
		{
			cur = 0;
			looped = true;
		}
		if ( cur >= firstReflexI && looped )
		{
			// we looped and connected tail to head
			// (firstReflexI can be -1 if we saw none)
			break;
		}

		int prev = (cur + ixsize-1)%ixsize;
		int next = (cur + 1)%ixsize;

		// is [prev,cur,next] reflex ?
		const Vec2i & vCur = verts[ indeces[cur] ];
		const Vec2i & vCurP = verts[ indeces[prev] ];
		const Vec2i & vCurN = verts[ indeces[next] ];
		if ( IsReflex(vCurP,vCur,vCurN) )
		{
			if ( lastReflexI == -1 )
			{
				lastReflexI = cur;
				firstReflexI = cur;
			}
			else
			{
				// try an edge from lastReflex to cur
				if ( ModDifferenceIs1(lastReflexI,cur,ixsize) )
				{
					// adjacent, ignore
				}
				else
				{
					// check for intersection with the contour
					// if it doesn't intersect, we've got a convex loop from lastReflexI to cur
					//	(actually, we know the bit from lastReflex to cur is fine, we only need
					//	to consider the bit from cur to lastReflex; also, we actually only need
					//	to consider other reflexes; any convex bit can't give us trouble)
					ASSERT( lastReflexI != cur );
					const Vec2i & vOther = verts[ indeces[lastReflexI] ];
					int otherprev = (lastReflexI + ixsize-1)%ixsize;
					int othernext = (lastReflexI + 1)%ixsize;
					const Vec2i & vOtherP = verts[ indeces[otherprev] ];
					const Vec2i & vOtherN = verts[ indeces[othernext] ];

					// make sure the new edge we're making is on the inside of the poly
					if ( IsOutside(vCurP,vCur,vCurN,vOther) )
					{
						continue;
					}
					if ( IsOutside(vOtherP,vOther,vOtherN,vCur) )
					{
						continue;
					}

					if ( ! CheckIntersection(vOther,vCur,verts,allIndeces) )
					{
						// ok, the bit from lastReflexI to cur is convex, so add it straight to pPolygons
						// not necessarilly, it can be nonconvex at the new edge
						/*
						pPolygons->push_back();
						vector<int> & poly = pPolygons->back();
						{for(int j=lastReflexI;;j++)
						{
							if ( j == ixsize)
								j = 0;
							poly.push_back( indeces[j] );
							if ( j == cur )
								break;
						}}

						// could just change indeces in place, but don't for debugging
						vector<int> remaining;
						{for(int k=cur;;k++)
						{
							if ( k == ixsize)
								k = 0;
							remaining.push_back( indeces[k] );
							if ( k == lastReflexI )
								break;
						}}

						ASSERT( (poly.size() + remaining.size()) == (indeces.size()+2) );

						ASSERT( IsConvex(verts,poly) );

						MakeConvexPieces(verts,remaining,allIndeces,pPolygons);
						*/

						Break(verts,indeces,allIndeces,pPolygons, lastReflexI,cur);
						return true;
					}
				}
			}
		}
	}

	if ( firstReflexI == -1 )
	{
		// saw no reflexes - we're done
		ASSERT( IsConvex(verts,indeces) );
		// just copy this last poly
		pPolygons->push_back();
		vector<int> & poly = pPolygons->back();
		poly.assignv(indeces);
		return true;
	}

	return false;
}

bool BreakAtReflex(const vector<Vec2i> & verts,const vector<int> & indeces,
						const vector<int> & allIndeces,
						vector< vector<int> > * pPolygons, int cur, bool force)
{
	// try another vert to break at
	// start at the opposite and walk out to either side
	const Vec2i & A = verts[ indeces[cur] ];

	int ixsize = indeces.size32();
	int base = (cur + (ixsize/2))%ixsize;
	for(int step=0;step<ixsize;step++)
	{
		int offset = ((step+1)>>1) * ( (step&1) ? -1 : 1 );
		// offset goes 0,1,-1,2,-2,...
		// start positive since "base" rounded down]
		int ix = (base + offset + ixsize)%ixsize;
		if ( ix == cur )
			continue;
		if ( ModDifferenceIs1(ix,cur,ixsize) )
			continue;
		// try a cut from [cur,ix]

		const Vec2i & B = verts[ indeces[ix] ];
		if ( CheckIntersection(A,B,verts,allIndeces) )
			continue;

		// no intersect, free to try it
		// only do the cut if it reduces the number of reflexes
		// that's a local measure

		const Vec2i & AP = verts[ indeces[ (cur+ixsize-1)%ixsize ] ];
		const Vec2i & AN = verts[ indeces[ (cur+1)%ixsize ] ];
		const Vec2i & BP = verts[ indeces[ (ix+ixsize-1)%ixsize ] ];
		const Vec2i & BN = verts[ indeces[ (ix+1)%ixsize ] ];
	
		ASSERT( IsReflex(AP,A,AN) );
		
		// make sure the new edge we're making is on the inside of the poly
		if ( IsOutside(AP,A,AN,B) )
		{
			continue;
		}
		if ( IsOutside(BP,B,BN,A) )
		{
			continue;
		}

		bool otherIsReflex = IsReflex(BP,B,BN);

		// check if we're making reflexes :
		bool is1 = IsReflex(AP,A,B);
		bool is2 = IsReflex(B,A,AN);
		bool is3 = IsReflex(BP,B,A);
		bool is4 = IsReflex(A,B,BN);

		int numSrc = 1 + (otherIsReflex?1:0);
		int numDst = (is1?1:0) + (is2?1:0) + (is3?1:0) + (is4?1:0);
		
		if ( force || numDst < numSrc )
		{
			// we improved the total, take it
			Break(verts,indeces,allIndeces,pPolygons,cur,ix);
			return true;
		}
	}
	return false;
}

const Vec2 i2f(const Vec2i & i)
{
	return Vec2( (float)i.x, (float)i.y );
}

void MakeConvexPieces(const vector<Vec2i> & verts,const vector<int> & indeces,
						const vector<int> & allIndeces,
						vector< vector<int> > * pPolygons)
{
	// first try adjacent reflexes :
	if ( PinchReflexReflex(verts,indeces,allIndeces,pPolygons) )
		return;

	// must have been at least one reflex that PinchReflexReflex could not handle
	//	(if poly was convex, it would have returned)

	// could try non-adjacent reflex-reflex edges here

	// so, try reflex ray shooting
	// @@ which reflex should I shoot from ?
	// @@ don't ray shoot from the same reflex more than once
	/*
	int ixsize = indeces.size();
	for(int cur = 0;cur<ixsize;cur++)
	{
		int prev = (cur + ixsize-1)%ixsize;
		int next = (cur + 1)%ixsize;
	
		if ( ! IsReflex(verts,indeces,prev,cur,next) )
			continue;

		// shoot a ray from cur
	
		const Vec2i & v = verts[ indeces[cur] ];

		Vec2 e1 = i2f(v - verts[ indeces[ prev] ]);
		Vec2 e2 = i2f(verts[ indeces[ next] ] - v);
		// take the average of the two :
	}
	*/
	
	int ixsize = indeces.size32();
	ASSERT( ixsize > 3 );
	// should not have a triangle that's non-convex !!!

	int cur;
	for(cur = 0;cur<ixsize;cur++)
	{
		int prev = (cur + ixsize-1)%ixsize;
		int next = (cur + 1)%ixsize;
	
		if ( ! IsReflex(verts,indeces,prev,cur,next) )
			continue;

		// we have a reflex, try to make a break here
		if ( BreakAtReflex(verts,indeces,allIndeces,pPolygons,cur,false) )
			return;
	}

	// force a break :
	
	for(cur = 0;cur<ixsize;cur++)
	{
		int prev = (cur + ixsize-1)%ixsize;
		int next = (cur + 1)%ixsize;
	
		if ( ! IsReflex(verts,indeces,prev,cur,next) )
			continue;

		// we have a reflex, try to make a break here
		if ( BreakAtReflex(verts,indeces,allIndeces,pPolygons,cur,true) )
			return;
	}

	FAIL("Couldn't fix reflexes!!??");
}

inline bool Match(int a,int b,int c,int d)
{
	ASSERT( ! (a== d && b == c) );
	return (a==c && b==d);
}

bool CanMerge(const vector<Vec2i> & verts,const vector<int> & allIndeces, 
				const vector<int> & poly1,
				int v1,int n1,
				const vector<int> & poly2,
				int v2,int n2,
				vector<int> * pMerged)
{
	// check for convexity; it's a local check :
	int size1 = poly1.size32();
	int size2 = poly2.size32();

	int p1 = (v1 + size1-1)%size1;
	int p2 = (v2 + size2-1)%size2;
	int nn1 = (n1 + 1)%size1;
	int nn2 = (n2 + 1)%size2;

	ASSERT( poly1[v1] == poly2[n2] );
	ASSERT( poly1[n1] == poly2[v2] );

	const Vec2i & AP = verts[ poly1[p1] ];
	const Vec2i & A  = verts[ poly1[v1] ];
	const Vec2i & AN = verts[ poly2[nn2] ];
	const Vec2i & BP = verts[ poly2[p2] ];
	const Vec2i & B  = verts[ poly2[v2] ];
	const Vec2i & BN = verts[ poly1[nn1] ];

	ASSERT( ! IsReflex(AP,A,B) );
	ASSERT( ! IsReflex(A,B,BN) );
	ASSERT( ! IsReflex(BP,B,A) );
	ASSERT( ! IsReflex(B,A,AN) );

	if ( IsReflex(AP,A,AN) )
		return false;
	if ( IsReflex(BP,B,BN) )
		return false;

	pMerged->clear();

	for(int i2=n2;;i2++)
	{
		if ( i2 == size2 )
			i2 = 0;
		if ( i2 == v2 )
			break; // and don't add v2
		pMerged->push_back( poly2[i2] );
	}
	for(int i1=n1;;i1++)
	{
		if ( i1 == size1 )
			i1 = 0;
		if ( i1 == v1 )
			break; // and don't add v1
		pMerged->push_back( poly1[i1] );
	}
	ASSERT( pMerged->size32() == (size1+size2 -2) );

	ASSERT( IsConvex(verts,*pMerged) );

	// shouldn't be able to make any intersections
	
	return true;	
}

void TryMerges(const vector<Vec2i> & verts,const vector<int> & indeces,vector< vector<int> > * pPolygons)
{
	bool didMerge;
	do
	{
		didMerge = false;
		int polys = pPolygons->size32();
		for(int p1=0;p1<polys;p1++)
		{
			for(int p2=p1+1;p2<polys;p2++)
			{
				// see if p1 and p2 are adjacent :
				vector<int> & poly1 = pPolygons->at(p1);
				vector<int> & poly2 = pPolygons->at(p2);
				int size1 = poly1.size32();
				int size2 = poly2.size32();
				for(int v1=0;v1<size1;v1++)
				{
					int n1 = (v1+1)%size1;
					for(int v2=0;v2<size2;v2++)
					{
						int n2 = (v2+1)%size2;
						if ( Match(poly1[v1],poly1[n1],poly2[n2],poly2[v2]) )
						{
							// they're adjacent
							// try a merge :
							vector<int> merged;
							if ( CanMerge(verts,indeces, poly1,v1,n1,poly2,v2,n2, &merged) )
							{
								pPolygons->erase_u(p2);
								pPolygons->at(p1).assignv(merged);
								didMerge = true;
								goto allOut;
							}
						}
					}
				}
			}
		}
		allOut:
		;
	} while(didMerge);
}

void ConvexDecomposition2d(const vector<Vec2i> & verts,vector< vector<int> > * pPolygons)
{
	pPolygons->clear();
	pPolygons->reserve( verts.size() );

	// currently O(N^3)
	//
	// first try to break off chunks using reflex-reflex edges
	//  only take ones that reduce the number of reflexes
	// O(R*R)
	//	need some way to decide which ones are best; any one that's
	//	from adjacent reflexes will detach a convex piece, so that's good
	//	and still fast.
	//
	// from each reflex (R), shoot a ray at bisecting angle (N)
	// it should hit a segment.  look at the two verts on that segment.
	// consider the two reflex-vert splits.  If either makes one or
	//	both halves convex, take it (N).  If either reduces the total number
	//	of reflex verts, it's still better than nothing (1), but do the
	//	splits greedy, eg. best first, repeating
	//
	// O(N*N*R) , with the try-to-make convex pieces goal, but
	//	only O(N*R) without it; actually it can still be done in O(N*R)
	//	by merging the ray-segment walk with the is-convex walk.  The
	//	is-convex walk is just the question of whether there's another
	//	reflex between the ray source and the target vert

	// make the identity indexing :
	vector<int>	loop;
	loop.resize(verts.size32());
	for(int i=0;i<verts.size32();i++)
	{
		loop[i] = i;
	}

	MakeConvexPieces(verts,loop,loop,pPolygons);

	// check it :
	#ifdef DO_ASSERTS
	{
		for(int i=0;i<pPolygons->size32();i++)
		{
			vector<int> & poly = pPolygons->at(i);
			ASSERT( IsConvex(verts,poly) );
			// check intersection :
			for(int v=0;v<poly.size32();v++)
			{
				int n = (v+1)%poly.size32();
				const Vec2i & A = verts[ poly[v] ];
				const Vec2i & B = verts[ poly[n] ];
				ASSERT( ! CheckIntersection(A,B,verts,loop) );
			}
		}
	}
	#endif

	// Now try to merge any neighbors that are still convex

	TryMerges(verts,loop,pPolygons);
	
	// check it :
	#ifdef DO_ASSERTS
	{
		for(int i=0;i<pPolygons->size32();i++)
		{
			vector<int> & poly = pPolygons->at(i);
			ASSERT( IsConvex(verts,poly) );
			// check intersection :
			for(int v=0;v<poly.size32();v++)
			{
				int n = (v+1)%poly.size32();
				const Vec2i & A = verts[ poly[v] ];
				const Vec2i & B = verts[ poly[n] ];
				ASSERT( ! CheckIntersection(A,B,verts,loop) );
			}
		}
	}
	#endif
}

END_CB

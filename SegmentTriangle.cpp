#include "SegmentTriangle.h"
#include "Segment.h"
#include "Sphere.h"
#include "Vec3U.h"
#include "FloatUtil.h"
#include "Rand.h"
#include "Log.h"

START_CB

struct CrossCylinderTemps
{
	//Vec3	cross;
	//float	dot;
	//float	crossLenSqr;
	
	CrossCylinderTemps()
	{
	//	finvalidatedbg(dot);
	//	finvalidatedbg(crossLenSqr);
	}
};

struct CylinderTemps2d
{
	Vec3 destToV0;
	float dPerpTo;
	float edgePerpLenSqr;
	
	CylinderTemps2d()
	{
		finvalidatedbg(dPerpTo);
		finvalidatedbg(edgePerpLenSqr);
	}
};

static bool IntersectInfiniteCylinder(
			const Vec3 & rayBaseFromCylinderBase,const Vec3 & rayNormal,
			const Vec3 & cylinderDirection,const float cylinderRadius,
			CrossCylinderTemps * pTemps);

static bool IntersectInfiniteCylinder2d(
			const Vec3 & edgePerp,
			const float dPerpFm,
			const Vec3 & segTo,
			const Vec3 & cylinderBase,
			const float cylinderRadius,
			CylinderTemps2d * pTemps);
			
static bool CollideCylinder(
			const Segment& seg, SegmentResults* result,
			const Vec3 & cylinderDirection, // cylinderDirection = End1 - End0
			const Vec3 & cylinderEnd0,
			const Vec3 & cylinderEnd1,
			const CrossCylinderTemps & temps);
			
static bool CollideCylinder2d(
			const Segment& seg, SegmentResults* result,
			const Vec3 & cylinderDirection, // cylinderDirection = End1 - End0
			const Vec3 & cylinderEnd0,
			const Vec3 & cylinderEnd1,
			const CylinderTemps2d & temps);

static bool CollideCylinder(
			const Segment& seg, SegmentResults* result,
			const Vec3 & cylinderEnd0,
			const Vec3 & cylinderEnd1);

static bool CollideCylinder2d(
			const Segment& seg, SegmentResults* result,
			const Vec3 & cylinderEnd0,
			const Vec3 & cylinderEnd1);
			
static bool CollideVertex(const Segment& seg, SegmentResults* result,
								const Vec3 &v)
{
	// @@ this could make use of
	//	delta = (v - seg.GetFm()) which I generally already have
	//	also Sphere m_radius is always 0, so that's redundant math
	//	if I had (seg.Normal() * delta) that would help
	Sphere s(v,0.f);
	SegmentResults temp;
	bool hit = s.IntersectSurface(seg,&temp);
	result->TakeEarlier(temp);
	return hit;
}
	
//===============================================================================================================
//===============================================================================================================

		
#define INSIDE_EDGE0	1
#define INSIDE_EDGE1	2
#define INSIDE_EDGE2	4
	
#define LSSTRI_MASK_CAPSULE0	( LSSTRI_MASK_EDGE0 | LSSTRI_MASK_VERT0 | LSSTRI_MASK_VERT1 )
#define LSSTRI_MASK_CAPSULE1	( LSSTRI_MASK_EDGE1 | LSSTRI_MASK_VERT1 | LSSTRI_MASK_VERT2 )
#define LSSTRI_MASK_CAPSULE2	( LSSTRI_MASK_EDGE2 | LSSTRI_MASK_VERT2 | LSSTRI_MASK_VERT0 )

#define IFHITVERT(a) \
/**/ if ( (edgeVertCheckFlags & LSSTRI_MASK_VERT##a) && CollideVertex(seg,result,v##a) )

#define IFHITCYL(a,b) \
/**/	if ( (edgeVertCheckFlags & LSSTRI_MASK_EDGE##a) && CollideCylinder(seg,result,e##a,v##a,v##b,temps##a) )
			
static bool CollideFaceStartEmbedded(
					const Segment& seg, SegmentResults* result, int edgeVertCheckFlags,
					const Vec3& v0, const Vec3& v1, const Vec3& v2,
					const Vec3& faceNormalUn,
					const float faceNormalLengthSqr,
					const Vec3 & e0,
					const Vec3 & e2,
					const float tMax)
{
			
	// seg.Fm() is behind the original face !
	//	any hit in the main triangle part is not a hit, only edges & verts are valid candidates
	
	// @@ very early reject - does this ever actually happen ?
	//	we'll catch it in a moment anyway
	//if ( ! edgeVertCheckFlags )
	//	return false;
		
	const Vec3 & segNormal = seg.GetNormal();
	const float segR = seg.GetRadius();
	const Vec3 & segFm = seg.GetFm();

	const Vec3 v0toFm( segFm - v0 ); 

	// the Bary check is REQUIRED, because if you just check edges & verts,
	//	you may collide with the INSIDE parts of the Edges and Verts.
	//	those shouldn't collide, they should let you out!!
		
	//const float bary0 = TripleProduct(faceNormalUn,e0,v0toFm);
	//const float bary2 = TripleProduct(faceNormalUn,e2,v0toFm);
	
	const Vec3 baryCross( v0toFm ^ faceNormalUn );
	const float bary0 = e0 * baryCross;
	const float bary2 = e2 * baryCross;
	const float bary1 = faceNormalLengthSqr - bary0 - bary2;

	#ifdef DO_ASSERTS	
	{
		const Vec3 e1(v2 - v1);
		const float realBary1 = TripleProduct(faceNormalUn,e1,segFm - v1);
		ASSERT( fequal(bary1/faceNormalLengthSqr,realBary1/faceNormalLengthSqr) ); 
	}
	#endif
		
	// positive bary means inside :
	ASSERT( TripleProduct(faceNormalUn,e0,v2 - v0) > 0.f );

	// only need to check the "capsule" (edge+vert) of the sides we're out of
	int needCheckFlags = 0;
	//*
	if ( bary0 < 0.f ) needCheckFlags |= LSSTRI_MASK_CAPSULE0;
	if ( bary1 < 0.f ) needCheckFlags |= LSSTRI_MASK_CAPSULE1;
	if ( bary2 < 0.f ) needCheckFlags |= LSSTRI_MASK_CAPSULE2;
	/**/
	// @@ CB is mul or cmov faster?
	// with fsignbit_I , the handling of exact 0.0 is semi-random
	/*
	needCheckFlags |= LSSTRI_MASK_CAPSULE0 * fsignbit_I(bary0);
	needCheckFlags |= LSSTRI_MASK_CAPSULE1 * fsignbit_I(bary1);
	needCheckFlags |= LSSTRI_MASK_CAPSULE2 * fsignbit_I(bary2);
	*/
	
	// they should not all be set :
	ASSERT( needCheckFlags != (LSSTRI_MASK_CAPSULE0|LSSTRI_MASK_CAPSULE1|LSSTRI_MASK_CAPSULE2) );

	// if we were inside the triangle body, needCheckFlags is now ZERO
	edgeVertCheckFlags &= needCheckFlags;

	if ( ! edgeVertCheckFlags )
	{
		// all barys IN the original triangle !
		return false;
	}

	CrossCylinderTemps temps0;
	CrossCylinderTemps temps1;
	CrossCylinderTemps temps2;

	//if ( (edgeVertCheckFlags & LSSTRI_MASK_CAPSULE0) && 
	if ( (edgeVertCheckFlags & LSSTRI_MASK_EDGE0) && 
		! IntersectInfiniteCylinder(v0toFm,segNormal,e0,segR,&temps0) )
	{
		// can't hit this edge or its end verts
		edgeVertCheckFlags &= ~LSSTRI_MASK_CAPSULE0;
		if ( ! edgeVertCheckFlags )
			return false;
	}
	
	const Vec3 e1(v2 - v1);
	
	//if ( (edgeVertCheckFlags & LSSTRI_MASK_CAPSULE1) && 
	if ( (edgeVertCheckFlags & LSSTRI_MASK_EDGE1) && 
		! IntersectInfiniteCylinder(segFm - v1,segNormal,e1,segR,&temps1) )
	{
		// can't hit this edge or its end verts
		edgeVertCheckFlags &= ~LSSTRI_MASK_CAPSULE1;
		if ( ! edgeVertCheckFlags )
			return false;
	}
	
	//if ( (edgeVertCheckFlags & LSSTRI_MASK_CAPSULE2) && 
	if ( (edgeVertCheckFlags & LSSTRI_MASK_EDGE2) && 
		! IntersectInfiniteCylinder(segFm - v2,segNormal,e2,segR,&temps2) )
	{
		// can't hit this edge or its end verts
		edgeVertCheckFlags &= ~LSSTRI_MASK_CAPSULE2;
		if ( ! edgeVertCheckFlags )
			return false;
	}
	
	ASSERT( edgeVertCheckFlags != 0 );
	
	// any hit on a cylinder body is the one collision I want, so return
	IFHITCYL(0,1) return true;
	IFHITCYL(1,2) return true;
	IFHITCYL(2,0) return true;
	
	// you can't just return true for the vert hits unless you sort them;
	//	eg. if you're in the "vert region" outside vert 2, you need to check vert 2,0,1
	
	bool hit = false;
	IFHITVERT(0) hit = true;
	IFHITVERT(1) hit = true;
	IFHITVERT(2) hit = true;
	
	return hit;
}

bool SegmentTriangleIntersect(
					const Segment& seg, SegmentResults* result,
					const int edgeVertCheckFlags,
					const Vec3& v0, const Vec3& v1, const Vec3& v2,
					const float tMax)
{
	// edge 0 = vert 0->1
	// edge 1 = vert 1->2
	// edge 2 = vert 2->0
	
	const Vec3 e0(v1 - v0);
	const Vec3 e2(v0 - v2);
	
	// Need to compute our own normal, from our verts.
	Vec3 faceNormal;
	//faceNormal.SetCross(v1 - v0, v2 - v0);
	faceNormal.SetCross(e2,e0);

	const Vec3 & segNormal = seg.GetNormal();

// tulrich 8/14/2004: alternative early-out.  However, it may upset later
// early-outs, because it allows out-facing queries to proceed.
//
// 	// Early out on facing-away queries...  This works, provided we
// 	// are not responsible for any edge/vert checks!
// 	const float dotUnn = segNormal * faceNormal;
// 	if ( edgeVertCheckFlags == 0
// 		 && dotUnn >= 0.f )
// 	{
// 		// Ray points out of the triangle -- no possible legitimate face hit.
// 		return false;
// 	}

// tulrich 8/14/2004: comment is wrong -- this *only* works if we never share
// convex edges, regardless of their interior angle!
// (CB - yeah, there are problems with this, but actually they're very rare; more to come...)

	// Early out on facing-away queries...  This works, provided we
	// never share edges between faces that have less than 90-degrees
	// in their interior angle.  Exporter should be guaranteeing this.
	const float dotUnn = segNormal * faceNormal;
	if ( dotUnn >= 0.f)
	{
		// Ray points out of the triangle -- no possible legitimate hit.
		return false;
	}

	// Next big reject is : do we start too far behind the plane?
	//	try to do these without normalizing faceNormal yet

	const Vec3 & segFm = seg.GetFm();
	const float & segR = seg.GetRadius();
	const float & segL = seg.GetLength();
	
	const float d0Unn = faceNormal * v0;
	const float dFmUnn = segFm * faceNormal - d0Unn;
	
	const float faceNormalLengthSqr = faceNormal.LengthSqr();
	
	if ( dFmUnn <= 0.f )
	{
		// Ray starts behind *original* triangle face and points in -
		//	we're starting *way* embedded

		// tulrich 8/14/2004: if we early-reject pointing-out queries,
		// then I think we can return right now, with no further test.
		// If you picture the original mesh, this essentially says the
		// query point starts inside the mesh --> no hit.  No need to
		// check edges, since we're guaranteed to be embedded further
		// than segR. 
		// CB 8/16/04 - this is certainly not true if you're doing
		//	anything with the edge sharing flags, but if you are putting
		//	all convex edges on BOTH faces, then I think it is true; if
		//	you start behind one face plane and go towards its edge, you
		//	would be in front of the other face plane if you were in a valid
		//	spot at all

		// tulrich 8/14/2004: I think this is a valid early-out, if we
		// *do* decide to allow pointing-out queries.  The edges only
		// need to cover 90 degrees, and if the from-point is behind
		// the original face plane and points in, it can't possibly
		// see the face or any edges.
		//
		//if (dotUnn < 0.f) { return false; }

		// tulrich 8/14/2004: I don't think this is right.  If we
		// reject facing-out queries, then we can early out right now,
		// with no further test.
		//  If we allow facing-out queries, then
		// this test will reject some valid hitting queries.
		// (I.e. queries that start way behind and outside the face,
		// but point back and hit an edge.)
		
		if ( fsquare(dFmUnn) > fsquare(segR) * faceNormalLengthSqr )
		{
			// Ray starts behind sphere-swept triangle radius
			return false;
		}
		
		// now handle embedded case here
		//	-> we just collide with edges/verts here
		//	if you're inside the bary tri, there's no collision, only need to check for being outside and going towards edges/verts
		//	the ray normal has got to be extremely grazing here

		// normalize our normal :		
		//const float faceNormalRecipSqrt = FloatUtil::ReciprocalSqrt(faceNormalLengthSqr);
		//faceNormal *= faceNormalRecipSqrt;

		return CollideFaceStartEmbedded(seg,result,edgeVertCheckFlags,v0,v1,v2,faceNormal,faceNormalLengthSqr,e0,e2,tMax);
	}

	// tulrich 8/14/2004: a possible early-out, if we *do* allow
	// pointing-out queries: query points outwards, and starts further
	// than segR from the face plane.
	//
	//if (dotUnn >= 0.f && fsquare(dFmUnn) > fsquare(segR) * faceNormalLengthSqr) { return false; }

	//const float tMax = 1.f;
	
	// tMax should already be at MIN() of any previous hit result
	ASSERT( tMax <= result->time && tMax <= 1.f );
	
	// start recipsqrt
	// KDTreeDynamic must ensure that no crappy
	// faces get into KDTree, which might cause NormalizeFast to
	// return garbage.
	const float faceNormalRecipSqrt = ReciprocalSqrt(faceNormalLengthSqr);
	
	const float dFm = dFmUnn * faceNormalRecipSqrt;
	
	const float activeLength = tMax * segL;
	//const Vec3 to = segFm + activeLength * segNormal;
	//const float dToUnn = to * faceNormal - d0Unn;
	const float dot = dotUnn * faceNormalRecipSqrt;
	const float dToActiveLength = dFm + activeLength * dot;

	ASSERT( dToActiveLength <= dFm ); // must be closer	// tulrich 8/14/2004: fails if we allow pointing-out queries

	if ( dToActiveLength >= segR )
	{
		// To point doesn't get close enough to hit -
		return false;
	}
	
	// dTo and dToActiveLength are different cuz of tMax ; annoying
	const float dTo = dFm + segL * dot;
	
	ASSERT( dTo < segR );
	
	float tHitPlane;
	Vec3 hitPlane;
	
	const float dFmR = dFm - segR;
	float dHitPlane; // dHitPlane = distance from "hitPlane" to the original triangle plane
	
	if ( dFmR <= 0.f )
	{
		// Fm point starts embedded within seg radius
		
		// @@ this could be faster, but it's not super important, is it?
		//	-> just test 2d point inclusion for the triangle part
		//	-> and test edges & verts
		
		tHitPlane = 0.f;
		hitPlane = segFm;
		dHitPlane = dFm;
	}
	else
	{
		// clip the point to the plane -
		// dFm and dTo are distances to the actual triangle plane, not the pushed-forward-by-radius bit
		
		/*const float dFmR = dFm - segR;
		const float dToR = dTo - segR;
		ASSERT( dFmR >= 0.f && dToR <= 0.f );*/
		ASSERT( dFm > segR && dTo < segR );
		ASSERT( dFm > dTo );
		
		tHitPlane = dFmR / ( dFm - dTo );
		// this is the "t" up to our "active To" , not up to the original query's To
		ASSERT( tHitPlane >= 0.f && tHitPlane <= 1.f );

		hitPlane = (1.f - tHitPlane) * segFm + tHitPlane * seg.GetTo();
		
		dHitPlane = segR;
		
		#ifdef DO_ASSERTS
		{
			Vec3 n(faceNormal * faceNormalRecipSqrt);
			float d = hitPlane * n - v0 * n;	
			ASSERT( fequal(d,segR) );
		}
		#endif
	}
	
	ASSERT( tHitPlane <= tMax );
	
	// now check the edge planes and find the region "hitPlane" is in
	
	// edge 0 = vert 0->1
	// edge 1 = vert 1->2
	// edge 2 = vert 2->0
					
	// now Normalize our normal.  
	//  (actually this could be deferred even a little more, but it's just 3 muls, so do it)
	//faceNormal *= faceNormalRecipSqrt;
	
	//	-> the problem with the Bary test is the triangle verts are in the original triangle plane,
	//		what we want is the triangle plane pushed forward by the segment radius
	//	if we had those verts originally, we could do the back-facing test and find the bary all
	//		using the Moller test without ever making faceNormal, etc.
	//	it actually still might be better; you can compute "det" to do back-facing, then you
	//		need the face normal to push one vert, then you proceed with Bary as usual
		
	int inFlags = 0;
	
	const Vec3 hitToV0 = hitPlane - v0;	 // ("tvec")
	
	// to compute the barys here we're using the cyclic invariance of the triple product
	//	that is Triple(A,B,C) = Triple(C,A,B)
	// so, you can think in terms of edge perps, like "edge0 ^ faceNormal" , and think
	//	bary0 = hitToV0 * (faceNormalUn ^ e0);
	//	bary2 = hitToV0 * (faceNormalUn ^ e2);
	// but that's just the same as :
	//	bary0 = e0 * (hitToV0 ^ faceNormalUn);
	//	bary2 = e2 * (hitToV0 ^ faceNormalUn);
	// which lets us do one cross product (slower) and share the result and just do two dot products (faster)
		
	//const float bary0 = TripleProduct(faceNormalUn,e0,v0toFm);
	//const float bary2 = TripleProduct(faceNormalUn,e2,v0toFm);
	
	// faceNormal is still un-normalized at this point
	const Vec3 baryCross( hitToV0 ^ faceNormal );
		
	// positive bary means inside :
	ASSERT( TripleProduct(faceNormal,e0,v2 - v0) > 0.f );

	CrossCylinderTemps temps0;
	const float bary0 = e0 * baryCross;
	
	// only need to check the "capsule" (edge+vert) of the sides we're out of
	if ( bary0 >= 0.f )
	{
		inFlags |= INSIDE_EDGE0;
	}
	else
	{
		if ( ! IntersectInfiniteCylinder(hitToV0,segNormal,e0,segR,&temps0) )
		{
			// can't hit this edge or its end verts
			return false;
		}
	}
	
	CrossCylinderTemps temps2;
	const float bary2 = e2 * baryCross;
	
	// only need to check the "capsule" (edge+vert) of the sides we're out of
	if ( bary2 >= 0.f )
	{
		inFlags |= INSIDE_EDGE2;
	}
	else
	{
		// @@ ACHTUNG! - v0toFm makes the wrong temps
		//	but is ok for the infinite quick reject
		if ( ! IntersectInfiniteCylinder(hitToV0,segNormal,e2,segR,&temps2) )
		{
			// can't hit this edge or its end verts
			return false;
		}
	}
	
	CrossCylinderTemps temps1;
	
	// ("det") = faceNormalLengthSqr
	const float bary1 = faceNormalLengthSqr - bary0 - bary2;

	#ifdef DO_ASSERTS	
	{
		const Vec3 e1(v2 - v1);
		const float realBary1 = TripleProduct(faceNormal,e1,hitPlane - v1);
		ASSERT( fequal(bary1/faceNormalLengthSqr,realBary1/faceNormalLengthSqr) ); 
	}
	#endif

	Vec3 e1;

	if ( bary1 >= 0.f )
	{
		inFlags |= INSIDE_EDGE1;
		// e1 is never set in this case, but it can't be used
	}
	else
	{
		e1 = v2 - v1;
		
		// out of this edge
		// do infinite-cylinder check to reject here & make temps
		const Vec3 hitToV1 = hitPlane - v1;
		if ( !  IntersectInfiniteCylinder(hitToV1,segNormal,e1,segR,&temps1) )
		{
			return false;
		}
	}
	
	// (we also check edge & vert flags here) :
	
	// if a vert collision is possible, it will always be sooner if it happens,
	//	so we just check it first and get out
	//	-> not true
	// I think in the one-edge cases I may need to also check the two end verts
	//	->indeed

	switch(inFlags)
	{
		#ifdef _DEBUG
		default:
		case 0:
			// impossible
			FAIL("should not get here");
			return false;
		#endif
	
		case INSIDE_EDGE0: // inside edge 0, outside edge 1 & 2 ;check vert 2
		{
//			#pragma PRAGMA_MESSAGE("@@ CB - use cylinder collides to help vert collides")
			// I already know that I go through *both* infinite cylinders
			//	if I go through the intersection of the two, I may hit the leading wedge vertex
			//	the *second* one that I enter is the only one I need to consider for cylinder body collisions
			//	if I hit neither the leading wedge vert nor a cylinder body, I may still hit a tail vertex
			//	only need to check the tail vertex on the cylinder body that I considered
					
			// can early out if I hit the cylinder *body* but not if I hit an end-cap !!
			// -> change cylinder check so it doesn't collide with end-caps at all and fix this !!
			IFHITCYL(1,2) return true;
			IFHITCYL(2,0) return true;
			
			// the adjoining vert :
			IFHITVERT(2) return true;
			
			// technically I need to check the two other verts too because they're visible;
			//	in reality hitting these in a useful way would be incredibly rare
			//	I could skip them altogether except that the darn vert-flags thing means I might not
			//	check them later on some face where they were better candidates
			// @@@@ I should use the cylinder body collides to help with all that
			IFHITVERT(0) return true;
			IFHITVERT(1) return true;
				
			return false;
		}
		
		case INSIDE_EDGE1: // inside edge 1, outside edge 0 & 2 ;check vert 0
		{
			IFHITCYL(2,0) return true;
			IFHITCYL(0,1) return true;
			IFHITVERT(0) return true;

			// rare verts:			
			IFHITVERT(1) return true;
			IFHITVERT(2) return true;
			
			return false;
		}
	
		case INSIDE_EDGE0|INSIDE_EDGE1: // inside edge 0 & 1, outside edge 2
		{			
			// cylinder end-check could tell me which one vertex to check (or none)
			IFHITCYL(2,0) return true;
					
			IFHITVERT(0) return true;
			IFHITVERT(2) return true;
				
			return false;
		}
			
		case INSIDE_EDGE2: // inside edge 2, outside edge 0 & 1 ;check vert 1
		{			
			IFHITCYL(0,1) return true;
			IFHITCYL(1,2) return true;
			
			// main vert:
			IFHITVERT(1) return true;				
			
			// rare verts :
			IFHITVERT(0) return true;
			IFHITVERT(2) return true;
			
			return false;
		}
		
		case INSIDE_EDGE0|INSIDE_EDGE2: // inside edge 0 & 2, outside edge 1
		{
			// cylinder end-check could tell me which one vertex to check
			IFHITCYL(1,2) return true;
					
			IFHITVERT(1) return true;
			IFHITVERT(2) return true;
				
			return false;
		}
			
		case INSIDE_EDGE1|INSIDE_EDGE2: // inside edge 1 & 2, outside edge 0
		{
			// cylinder end-check could tell me which one vertex to check
			IFHITCYL(0,1) return true;
					
			IFHITVERT(0) return true;
			IFHITVERT(1) return true;
				
			return false;
		}
			
		case INSIDE_EDGE0|INSIDE_EDGE1|INSIDE_EDGE2:
		{
			// inside the triangle face
			// we're done

			ASSERT( tHitPlane <= result->time );
			
			// Ensure that any hit on this face is valid in world space.
			// faceNormal is still not normalized yet
			faceNormal *= faceNormalRecipSqrt;
			ASSERT( faceNormal.IsNormalized() );
			
			/*
			const Vec3 worldUnitNormal = query.GetQueryToWorldFrame().Rotate(faceNormal);
			if ( ( worldUnitNormal * query.GetWorldNormal() ) >= 0.f)
			{
				// Not really valid -- query is pointing away in world space.
				// @@@@ CB - bad shite!!! - should we check edges here ?
				return false;	// dunno about edges -- might hit.
			}
			*/
			
			#ifdef DO_ASSERTS
			const Vec3 segHitPoint = seg.GetHitPoint(tHitPlane);
			ASSERT( Vec3::Equals(segHitPoint,hitPlane) );
			#endif
			
			// "collidedPoint" is on the actual face 
			const Vec3 collidedPoint = hitPlane - faceNormal * segR;
							
			//result->m_collided++;
			result->time = tHitPlane;
			result->normal = faceNormal;
			//result->collidedPoint = query.GetQueryToWorldFrame() * collidedPoint;
			result->collidedPoint = collidedPoint;
			return true;
		}
	}
	
	//return true;
	__assume(0);
}

bool SegmentTriangleIntersect(const Segment& seg, SegmentResults* result,
								const Vec3 &v0,
								const Vec3 &v1,
								const Vec3 &v2)
{
	return SegmentTriangleIntersect(seg,result,LSSTRI_MASK_ALL,v0,v1,v2,1.f);
}
								
static bool IntersectInfiniteCylinder(
			const Vec3 & rayBaseFromCylinderBase,const Vec3 & rayNormal,
			const Vec3 & cylinderDirection,const float cylinderRadius,
			CrossCylinderTemps * pTemps)
{
	const Vec3 cross = rayNormal ^ cylinderDirection;
	const float dot = rayBaseFromCylinderBase * cross;
	const float crossLenSqr = cross.LengthSqr();
	if ( crossLenSqr < 1e-9f )
	{
		// if crossLenSqr is tiny, ray is parallel to cylinder - do special test!
		// @@ for a real Cylinder test, we should check collision here,
		//	but for LSS-Triangle, we don't need to collide with the end caps at all, so allow it
		//	(but I may collide with the end spheres, so check this right)
		const float dSqr = rayBaseFromCylinderBase.LengthSqr();
		if ( dSqr < fsquare(cylinderRadius) )
		{
			return true;
		}
		return false;
	}
	else
	{
		if ( fsquare(dot) <= fsquare(cylinderRadius)*crossLenSqr )
		{
			//pTemps->cross = cross;
			/*
			pTemps->dot = dot;
			pTemps->crossLenSqr = crossLenSqr;
			*/
			return true;
		}
		else
		{
			return false;
		}
	}
}

// CollideCylinder is a weird type of "cylinder" collide in that we do NOT collide with the end caps
//	(since there will be spheres there plugging those holes anyway), but we DO consider the cylinder "solid"
//	for the purpose of rays that start inside the cylinder.
static bool CollideCylinder(
			const Segment& seg, SegmentResults* result,
			const Vec3 & cylinderDirection, // cylinderDirection = End1 - End0
			const Vec3 & cylinderEnd0,
			const Vec3 & cylinderEnd1,
			const CrossCylinderTemps & temps)
{
//	#pragma PRAGMA_MESSAGE("@@ CB - CollideCylinder could be faster!")
	
	// in the LSS-Tri usage, we've already made sure that the *ray* (not the capped segment) goes through the infinite-length cylinder
	//	I need to make sure -
	//		A) the hit on the infinite cylinder is within the end ranges
	//		B) the hit is within the length of the segment
	
	// @@@@ lazy version for now :
	//	just start out normalizing cylinderDirection
	
	Vec3 axis(cylinderDirection);
	axis.NormalizeFast();
	
	// first do end-cap distances :
	const float dEnd0 = cylinderEnd0 * axis;
	const float dEnd1 = cylinderEnd1 * axis;
	const float cylLength = dEnd1 - dEnd0;
	ASSERT( cylLength > 0.f );
	
	const float dFm = seg.GetFm() * axis - dEnd0;
	const float dTo = seg.GetTo() * axis - dEnd0;
	//const float dotN = rayNormal * axis;
	//const float dTo = dFm + seg.GetLength() * dotN;
	
	float tEnd0,tEnd1;
	
	if ( dFm < 0.f )
	{
		if ( dTo < 0.f )
		{
			// off end
			return false;
		}
		else if ( dTo > cylLength )
		{
			// crosses across the whole thing
			// what times?
			const float inv = 1.f / (dTo - dFm);
			tEnd0 = -dFm * inv;
			tEnd1 = (cylLength-dFm) * inv;
		}
		else
		{
			// To is inside	
			tEnd0 = -dFm / (dTo - dFm);
			tEnd1 = 1.f;
		}
	}
	else if ( dFm > cylLength )
	{
		if ( dTo > cylLength )
		{
			// off end
			return false;
		}
		else if ( dTo < 0.f )
		{
			// crosses
			// what times?
			const float inv = 1.f / (dFm - dTo);
			tEnd0 = (dFm-cylLength) * inv;
			//tTo = tFm + cylLength * inv;
			tEnd1 = dFm * inv;
		}
		else
		{
			// To is inside
			tEnd0 = (dFm-cylLength) / (dFm - dTo);
			tEnd1 = 1.f;
		}
	}
	else
	{
		// Fm starts inside
		tEnd0 = 0.f;
		if ( dTo < 0.f )
		{
			tEnd1 = dFm / (dFm - dTo);
		}
		else if ( dTo > cylLength )
		{
			tEnd1 = (cylLength-dFm) / (dTo - dFm);
		}
		else
		{
			// To is also inside
			tEnd1 = 1.f;
		}
	}
	
	ASSERT( fiszerotoone(tEnd0) );
	ASSERT( fiszerotoone(tEnd1) );
	ASSERT( tEnd1 >= tEnd0 );
	
	const Vec3 & rayNormal = seg.GetNormal();
	const float R = seg.GetRadius();
	const float L = seg.GetLength();
	
	// @@ verify :
	#ifdef DO_ASSERTS
	{
		const Vec3 p0 = seg.GetFm() + rayNormal * L * tEnd0;
		const Vec3 p1 = seg.GetFm() + rayNormal * L * tEnd1;
		const float d0 = ((p0 * axis) - dEnd0);
		const float d1 = ((p1 * axis) - dEnd0);
		ASSERT( fisinrangesloppy(d0,0.f,cylLength) );
		ASSERT( fisinrangesloppy(d1,0.f,cylLength) );
	}
	#endif
	
	// @@ n = temps.cross * axisInvLength
	Vec3 n( rayNormal ^ axis );
	const float lnSqr = n.LengthSqr();
	
	if ( lnSqr <= 1e-9f )
	{
		// ray is parallel to axis!
		// @@ for a real Cylinder test, we should check collision here,
		//	but for LSS-Triangle, we don't need to collide with the end caps at all, so allow it
		return false;
		/*
		const float dSqr = RC.LengthSqr();
		if ( dSqr > R*R )
		{
			// ray is outside
			return false;
		}
		else
		{
			// ray starts inside
		}
		*/
	}
	
	// @@ RC usually already exists
	const Vec3 RC( seg.GetFm() - cylinderEnd0 );

	// @@ do I need to make sure that the ray is going towards the cylinder !?
	//	the "t" we compute below could be negative, no ?	

	const float invLN = ReciprocalSqrt(lnSqr);
	n *= invLN;		
	
	// @@ d (scaled by axisLength) was already made in the infinite test
	const float d = fabsf(RC * n);
	// should've been checked by the previous InfiniteCylinder check
	//ASSERT( d <= R );	
	// this can happen cuz of damn floats
	if ( d > R )
	{		
		return false;
	}
		
	//Vec3 O1 = RC ^ axis;
	//const float t = - (O1 * n) * invLN;
	
	const Vec3 O = n ^ axis;
	
	const float t = (RC * O) * invLN;

	if ( t < 0.f )
	{
		// we're going away from further penetration
		//	(for the LSS-Tri test, this case could only ever occur if we're starting embedded badly)
		return false;
	}

	//O.NormalizeFast();
	const float OLenSqr = O.LengthSqr();
	
	const float s = sqrtf( (R*R - d*d) * OLenSqr ) / fabsf(rayNormal * O);
	
	// @@ these are distances not times
	//	dist0 and dist1 are the distances from Fm where we hit the infinite cylinder
	const float dist0 = t - s; 
	const float dist1 = t + s;
	
	ASSERT( dist1 >= dist0 );
	
	// verify :
	#ifdef DO_ASSERTS
	{
		const Vec3 p0 = seg.GetFm() + rayNormal * dist0;
		const Vec3 p1 = seg.GetFm() + rayNormal * dist1;
		const Vec3 c0 = ((p0 * axis) - dEnd0)*axis + cylinderEnd0;
		const Vec3 c1 = ((p1 * axis) - dEnd0)*axis + cylinderEnd0;
		const float d0 = Distance(p0,c0);
		const float d1 = Distance(p1,c1);
		ASSERT( fequal(d0,R,0.001f) );
		ASSERT( fequal(d1,R,0.001f) );
	}
	#endif
	
	
	// we'll catch this in the test below :
	/*	
	if ( t1 < 0.f || t0 > L )
	{
		//collides outside range finite segment
		return false;
	}
	*/
	
	// distances along where we go through the end caps :
	const float distEnd0 = tEnd0*L;
	const float distEnd1 = tEnd1*L;
	
	// valid hit range is the intersection of the two ranges
	//	dist0/dist1 is the cylinder body , distEnd0/distEnd1 are the end caps
	
	float int0;
	//const float int0 = OwMax(dist0,distEnd0);
	
	if ( dist0 < distEnd0 )
	{
		// we are colliding with an end-cap
		//	do NOT count that!
		if ( dist0 > 0.f || distEnd0 > 0.f )
		{
			return false;
		}
		// we started inside
		int0 = 0.f;
	}
	else
	{
		int0 = dist0;
	}
	
	//const float int1 = OwMin(dist1,distEnd1);
	//if ( int1 < int0 )
	
	if ( int0 > dist1 || int0 > distEnd1 )
	{
		// no overlap!
		return false;
	}
	
	// int0 is the first hit distance
	ASSERT( int0 >= 0.f && int0 <= L );
	
	// good hit
	//float hitTime = int0 / L;
	
	if ( ! result->Collided() || int0 < L * result->time )
	{
		const Vec3 hitPoint = seg.GetFm() + rayNormal * int0;
		const float hitTime = int0 / L;
		
		float dHit = hitPoint * axis - dEnd0;
		ASSERT( fisinrangesloppy(dHit,0.f,cylLength) );

		result->collidedPoint = cylinderEnd0 + dHit * axis;
		result->time = hitTime;
		result->normal = seg.GetHitPoint(hitTime) - result->collidedPoint;
		result->normal.NormalizeSafe();
	}
	
	return true;
}

static bool CollideCylinder(
			const Segment& seg, SegmentResults* result,
			const Vec3 & cylinderEnd0,
			const Vec3 & cylinderEnd1)
{
	Vec3 cylinderDirection(cylinderEnd1 - cylinderEnd0);
	Vec3 rayBaseFromCylinderBase( seg.GetFm() - cylinderEnd0 );
	CrossCylinderTemps temps;
	if ( ! IntersectInfiniteCylinder(rayBaseFromCylinderBase,seg.GetNormal(),cylinderDirection,seg.GetRadius(),&temps) )
		return false;
	
	return CollideCylinder(seg,result,cylinderDirection,cylinderEnd0,cylinderEnd1,temps);
}
			
//===============================================================================================================
//===============================================================================================================

#if 0

static bool IntersectInfiniteCylinder2d(
			const Vec3 & edgePerp,
			const float dPerpFm,
			const Vec3 & segTo,
			const Vec3 & cylinderBase,
			const float cylinderRadius,
			CylinderTemps2d * pTemps)
{
	pTemps->destToV0 = segTo - cylinderBase;
	const float dPerpTo = pTemps->destToV0 * edgePerp;
	pTemps->edgePerpLenSqr = edgePerp.LengthSqr();
	if ( dPerpTo < 0.f )
	{
		const float rhs = pTemps->edgePerpLenSqr * fsquare(cylinderRadius);
		if ( fsquare(dPerpFm) > rhs && fsquare(dPerpTo) > rhs )
		{
			// hitPlane and To are both farther than Radius past this edge plane
			return false;
		}
		//const float edgePerp0inv = ReciprocalSqrt(edgePerp0LenSqr);
	}
	pTemps->dPerpTo = dPerpTo;
	
	return true;	
}

static bool CollideCylinder2d(
			const Segment& seg, SegmentResults* result,
			const Vec3 & cylinderEnd0,
			const Vec3 & cylinderEnd1,
			const float dFm_Y,
			const float dTo_Y,
			const float dFm_X_un,
			const CylinderTemps2d & temps)
{
	// the _un distances are scaled by edgePerpLen
	const float invEdgePerpLen = ReciprocalSqrt(temps.edgePerpLenSqr);
	const float dFm_X = dFm_X_un * invEdgePerpLen;
	const float dTo_X = temps.dPerpTo * invEdgePerpLen;
	// do 2d segment-circle intersection
	const float R = seg.GetRadius();
	const float RSqr = R*R;
	const float dFmSqr = dFm_X*dFm_X + dFm_Y*dFm_Y;
	const float delta_X = dTo_X - dFm_X;
	const float delta_Y = dTo_Y - dFm_Y;
	if ( dFmSqr <= RSqr )
	{
		// from point starts inside the cylinder (in 2d)
		const float deltaDotFm = delta_X * dFm_X + delta_Y * dFm_Y;
		if ( deltaDotFm > 0.f )
		{
			// going away from axis! no collision
			return false;
		}
		// collide with end caps
		// @@@@
		return true;	
	}
	
	const float delta_LenSqr = delta_X*delta_X + delta_Y*delta_Y;
	// perp is { delta_Y, -delta_X }
	const float dotFmPerp = delta_Y * dFm_X - delta_X * dFm_Y;
	// dotFmPerp is the distance of closest approach, scaled by |delta|
	const float dSqr_deltaLenSqr = dotFmPerp*dotFmPerp;
	if ( dSqr_deltaLenSqr >= RSqr * delta_LenSqr )
	{
		// no intersection
		return false;
	}
	const float dSqr = dSqr_deltaLenSqr / delta_LenSqr;
	ASSERT( dSqr <= RSqr );
	ASSERT( dFmSqr >= RSqr );
	const float tSqr = dFmSqr - dSqr;
	ASSERT( tSqr >= 0.f );
	const float sSqr = RSqr - dSqr;
	ASSERT( sSqr >= 0.f && tSqr >= sSqr );
	// entry collision is at (t-s)
	// if ( (t-s) > deltaLen ) , no collision
	// @@ t is also just dFm dot delta
	const float t = sqrtf(tSqr);
	const float s = sqrtf(sSqr);
	const float dAlong = t-s;
	if ( fsquare(dAlong) > delta_LenSqr )
	{
		// can't reach collision
		return false;
	}
	const float delta_InvLen = ReciprocalSqrt(delta_LenSqr);
	const float tHit = dAlong * delta_InvLen;
	// @@@@ check end caps
	return true;
}

static bool CollideCylinder2d(
			const Segment& seg, SegmentResults* result,
			const Vec3 & cylinderEnd0,
			const Vec3 & cylinderEnd1)
{
	const Vec3 rayBaseFromCylinderBase = seg.GetFm() - cylinderEnd0;
	const Vec3 cylinderDirection = cylinderEnd1 - cylinderEnd0;
	
	Vec3 edgePerp;
	Vec3 faceNormal;
	GetTwoPerp(cylinderDirection,&edgePerp,&faceNormal);
	faceNormal.NormalizeFast();
	
	const float dPerpFm = edgePerp * seg.GetFm();
	
	CylinderTemps2d temps;
	
	if ( ! IntersectInfiniteCylinder2d(edgePerp,dPerpFm,seg.GetTo(),cylinderEnd0,seg.GetRadius(),&temps) )
	{
		return false;
	}
	
	const float dNormalFm = faceNormal * (seg.GetFm() - cylinderEnd0);
	const float dNormalTo = faceNormal * (seg.GetTo() - cylinderEnd0);
	
	return CollideCylinder2d(seg,result,cylinderEnd0,cylinderEnd1,
						dNormalFm,dNormalTo,dPerpFm,temps);
}

#endif

//===============================================================================================================
//===============================================================================================================
#if 1
// Regressions :

#define NUM_TESTS	(10000)

static void Test_CollideVertex()
{
	for(int i=0;i<NUM_TESTS;i++)
	{
		Vec3 vertPos;
		SetRandomInUnitCube(&vertPos);
		vertPos *= 1000;
		float radius = frandranged(1.f,2.f);
		float distance = frandranged(0.f,4.f);
		if ( fequal(distance,radius) )
			distance += EPSILON*4.f;
		Vec3 offset;
		SetRandomNormal(&offset);
		Vec3 p1,p2;
		GetTwoPerpNormals(offset,&p1,&p2);
		Vec3 base = vertPos + distance*offset;
		float segOff1 = frandranged(1.f,100.f);
		Vec3 e1 = base + segOff1 * p1;
		float segOff2 = frandranged(1.f,100.f);
		Vec3 e2 = base - segOff2 * p1;
		
		Segment seg(e1,e2,radius);
		SegmentResults result;
		bool hit = CollideVertex(seg,&result,vertPos);
	
		bool shouldHit = distance <= radius;
		if ( hit != shouldHit )
		{
			FAIL("bad collide!\n");
		}
	}
}

static void Test_CollideCylinder()
{
	// hits : 
	for(int i=0;i<NUM_TESTS;i++)
	{
		Vec3 c0;
		SetRandomInUnitCube(&c0);
		c0 *= 100.f;
		Vec3 c1;
		SetRandomInUnitCube(&c1);
		c1 *= 100.f;
		Vec3 cdir = c1 - c0;
		cdir.NormalizeFast();
		Vec3 p1,p2;
		GetTwoPerpNormals(cdir,&p1,&p2);
		
		float radius = frandranged(1.f,2.f);
		float distance = frandranged(0.f,radius-EPSILON);
		float angle = frandunit() * TWO_PIf;
		float t = frandunit();
		Vec3 oncyl = t * c0 + (1-t)*c1;
		Vec3 base = oncyl + distance * ( cosf(angle)*p1 + sinf(angle)*p2 );
		
		// base is inside the cylinder
		
		// end caps don't collide, so make sure we don't go through an end-cap
		//SetRandomNormal(&dir);
		float dirAngle = frandunit() * TWO_PIf;
		Vec3 dir = ( cosf(dirAngle)*p1 + sinf(dirAngle)*p2 );
		ASSERT( dir.IsNormalized() );
		
		float segOff1 = frandranged(3.f,100.f);
		Vec3 e1 = base - segOff1 * dir;
		float segOff2 = frandranged(3.f,100.f);
		Vec3 e2 = base + segOff2 * dir;
		
		Segment seg(e1,e2,radius);
		SegmentResults result;
		bool hit = CollideCylinder(seg,&result,c0,c1);
	
		if ( hit != true )
		{
			FAIL(" should hit!\n");
		}
	}
	
	// misses : 
	for(int i=0;i<NUM_TESTS;i++)
	{
		Vec3 c0;
		SetRandomInUnitCube(&c0);
		c0 *= 100.f;
		Vec3 c1;
		SetRandomInUnitCube(&c1);
		c1 *= 100.f;
		Vec3 cdir = c1 - c0;
		cdir.NormalizeFast();
		Vec3 p1,p2;
		GetTwoPerpNormals(cdir,&p1,&p2);
		
		float radius = frandranged(1.f,2.f);
		float distance = frandranged(radius+EPSILON,10.f);
		float t = frandunit();
		Vec3 oncyl = t * c0 + (1-t)*c1;
		Vec3 base = oncyl + distance * p1;
		Vec3 dir = p2;
		
		float segOff1 = frandranged(1.f,100.f);
		Vec3 e1 = base + segOff1 * dir;
		float segOff2 = frandranged(1.f,100.f);
		Vec3 e2 = base - segOff2 * dir;
		
		Segment seg(e1,e2,radius);
		SegmentResults result;
		bool hit = CollideCylinder(seg,&result,c0,c1);
	
		if ( hit != false )
		{
			FAIL(" should miss!\n");
		}
	}
	
	// misses : 
	for(int i=0;i<NUM_TESTS;i++)
	{
		Vec3 c0;
		SetRandomInUnitCube(&c0);
		c0 *= 100.f;
		Vec3 c1;
		SetRandomInUnitCube(&c1);
		c1 *= 100.f;
		Vec3 cdir = c1 - c0;
		cdir.NormalizeFast();
		Vec3 p1,p2;
		GetTwoPerpNormals(cdir,&p1,&p2);
		
		float radius = frandranged(1.f,2.f);
		float distance = frandranged(radius+EPSILON,10.f);
		float angle = frandunit() * TWO_PIf;
		float t = frandunit();
		Vec3 oncyl = t * c0 + (1-t)*c1;
		Vec3 dir = ( cosf(angle)*p1 + sinf(angle)*p2 );
		Vec3 base = oncyl + distance * dir;
		
		Vec3 e1 = base;
		float segOff2 = frandranged(1.f,100.f);
		Vec3 e2 = base + segOff2 * dir;
		
		Segment seg(e1,e2,radius);
		SegmentResults result;
		bool hit = CollideCylinder(seg,&result,c0,c1);
	
		if ( hit != false )
		{
			FAIL(" should miss!\n");
		}
	}
}

static void Test_CollideTriangle()
{
	// hits :
	for(int i=0;i<NUM_TESTS;i++)
	{
		// random triangle :
		Vec3 t0,t1,t2;
		SetRandomInUnitCube(&t0); t0 *= 100.f;
		SetRandomInUnitCube(&t1); t1 *= 100.f;
		SetRandomInUnitCube(&t2); t2 *= 100.f;
		Vec3 n;
		if ( SetTriangleNormal(&n,t0,t1,t2) < EPSILON )
			continue;
		
		float u = frandranged(EPSILON*2.f,1.f-EPSILON*2.f);
		float v = frandranged(EPSILON,1.f-EPSILON-u);
		Vec3 base = u * t0 + v * t1 + (1.f - u - v) * t2;
	
		float radius = frandranged(EPSILON,1.f);
	
		// make a dir that's pointing at the triangle
		Vec3 dir;
		do
		{
			SetRandomNormal(&dir);
		} while ( (dir * n) > - 0.1f );
		
		float segOff1 = frandranged(2.f,100.f);
		Vec3 e1 = base - segOff1 * dir;
		float segOff2 = frandranged(2.f,100.f);
		Vec3 e2 = base + segOff2 * dir;
		
		Segment seg(e1,e2,radius);
		SegmentResults result;
		bool hit = SegmentTriangleIntersect(seg,&result,t0,t1,t2);
	
		if ( hit != true )
		{
			FAIL(" should hit!\n");
		}		
	}
	
	// hits :
	for(int i=0;i<NUM_TESTS;i++)
	{
		// random triangle :
		Vec3 t0,t1,t2;
		SetRandomInUnitCube(&t0); t0 *= 100.f;
		SetRandomInUnitCube(&t1); t1 *= 100.f;
		SetRandomInUnitCube(&t2); t2 *= 100.f;
		Vec3 n;
		if ( SetTriangleNormal(&n,t0,t1,t2) < EPSILON )
			continue;
		
		float u = frandranged(EPSILON*2.f,1.f-EPSILON*2.f);
		float v = frandranged(EPSILON,1.f-EPSILON-u);
		Vec3 base = u * t0 + v * t1 + (1.f - u - v) * t2;
	
		float radius = frandranged(0.5f,1.f);
	
		// make a dir that's pointing at the triangle
		Vec3 dir;
		do
		{
			SetRandomNormal(&dir);
		} while ( (dir * n) > - 0.1f );
		
		// make a dir that's pointing at the triangle
		Vec3 offset;
		do
		{
			SetRandomNormal(&offset);
		} while ( (offset * n) > 0.f );
		
		base -= offset * frandranged(0.f,radius-EPSILON);
		
		float segOff1 = frandranged(2.f,100.f);
		Vec3 e1 = base - segOff1 * dir;
		float segOff2 = frandranged(2.f,100.f);
		Vec3 e2 = base + segOff2 * dir;
		
		Segment seg(e1,e2,radius);
		SegmentResults result;
		bool hit = SegmentTriangleIntersect(seg,&result,t0,t1,t2);
	
		if ( hit != true )
		{
			FAIL(" should hit!\n");
		}		
	}	
}

void SegmentTriangle_Test()
{
	Test_CollideVertex();
	Test_CollideCylinder();
	Test_CollideTriangle();
}

#endif
//===============================================================================================================
//===============================================================================================================

END_CB

#include "Poly3.h"
#include "cblib/Log.h"
#include "cblib/Vec3U.h"
#include "cblib/Poly2.h"
#include "cblib/Mat3.h"
#include "cblib/Frustum.h"
#include "cblib/Sphere.h"
#include "cblib/Segment.h"
#include "cblib/VolumeBuilder.h"
#include <algorithm>

START_CB

void Poly3::Clear()
{
	m_verts.clear(); 
}

void Poly3::Reverse()
{
	// this reverses the vert order and DOES NOT FIX THE PLANE
	//	it's used to fix the winding if you got it wrong

	std::reverse(m_verts.begin(),m_verts.end());
}

float Poly3::ComputeArea() const
{
	if ( GetNumVerts() == 0 )
		return 0.f;

	ASSERT( GetNumVerts() >= 3 );
	const Vec3 & a = m_verts[0];
	float area = 0.f;
	for(int i=2;i<GetNumVerts();i++)
	{
		const Vec3 & b = m_verts[i-1];
		const Vec3 & c = m_verts[i];
		area += TriangleArea(a,b,c);
	}
	return area;
}

const Vec3 Poly3::ComputeCenter() const
{
	Vec3 ret = m_verts[0];
	for(int i=1;i<GetNumVerts();i++)
	{
		ret += m_verts[i];
	}
	ret *= (1.f/GetNumVerts());
	return ret;
}

void Poly3::ComputeBBox(AxialBox * pBox) const
{
	pBox->SetToPoint( m_verts[0] );
	for(int i=1;i<GetNumVerts();i++)
	{
		pBox->ExtendToPoint( m_verts[i] );
	}
}
void Poly3::ComputeSphereFast(Sphere * pSphere) const
{
	ASSERT(pSphere);

	MakeFastSphere(pSphere,
					m_verts.data(),
					m_verts.size32());

	ASSERT(pSphere->IsValid());
}
	
void Poly3::ComputeSphereGood(Sphere* pSphere) const
{
	ASSERT(pSphere);

	MakeMiniSphere(pSphere,
				   m_verts.data(),
				   m_verts.size32());

	ASSERT(pSphere->IsValid());
}


Plane::ESide Poly3::PlaneSide(const Plane & plane,const float epsilon) const
{
	int numF=0,numOn=0,numB=0;	
	const int numVerts = GetNumVerts();
	ASSERT( numVerts > 0 );
	for(int i=0;i<numVerts;i++)
	{
		Plane::ESide pointSide = plane.PointSideOrOn(m_verts[i],epsilon);
		switch(pointSide)
		{
		case Plane::eFront:
			numF++;
			break;
		case Plane::eBack:
			numB++;
			break;
		default:
			numOn++;
			break;
		}
	}

	if ( numF > 0 && numB > 0 )
	{
		return Plane::eIntersecting;
	}
	else if ( numF == 0 && numB == 0 )
	{
		return Plane::eOn;		
	}
	else if ( numF > 0 )
	{
		// may have some "on" as well
		return Plane::eFront;
	}
	else
	{
		ASSERT( numB > 0 );
		// may have some "on" as well
		return Plane::eBack;
	}
}
/*
	CleanPoly removes colinear segments, and tries
	to makes poly convex if it's gotten little wiggles
*/
bool CleanPoly(const Poly3 & from,Poly3 * pTo)
{
	// copy all verts
	pTo->m_verts = from.m_verts;

	// @@@@ TODO : fit a Plane to to the Poly3 ;
	//	then project to 2d and use CleanPoly in 2d

	/*

	// remove verts from "to" in place, sliding down
	for(int i=0;i<pTo->GetNumVerts();)
	{
		ASSERT( pTo->GetNumVerts() >= 3 );

		int in1 = (i+1)%(pTo->GetNumVerts());
		int in2 = (i+2)%(pTo->GetNumVerts());
		const Vec3 & a = pTo->m_verts[i];
		const Vec3 & b = pTo->m_verts[in1];
		const Vec3 & c = pTo->m_verts[in2];

		// a,b,c should be counterclockwise
		float area = TriangleArea(a,b,c);

		if ( area <= EPSILON )
		{
			if ( pTo->GetNumVerts() <= 3 )
			{
				pTo->m_verts.clear();
				return false;
			}

			// a,b,c is degenerate, colinear, or concave
			// either way, just remove "b"
			pTo->m_verts.erase(pTo->m_verts.begin() + in1);
		}
		else
		{
			i++;
		}
	}
	*/

	return true;
}

/*
	CleanPoly removes colinear segments, and tries
	to makes poly convex if it's gotten little wiggles

	CleanPolySerious uses rigorous integer 2d convex hull
*/
bool CleanPolySerious(const Poly3 & from,const Plane & plane,Poly3 * pTo)
{
	const Vec3 center = from.ComputeCenter();
	
	Poly2 poly2;
	Project2d(&poly2,from,plane,center);

	Poly2 poly2c;
	if ( ! CleanPolySerious(poly2,&poly2c) )
	{
		pTo->Clear();
		return false;
	}

	Lift2d(pTo,poly2c,plane,center);

	return true;
}

void Project2d(Poly2 * pInto, const Poly3 & from,const Plane & plane,const Vec3 & basePos)
{
	Vec3 n1,n2;
	GetTwoPerpNormals(plane.GetNormal(),&n1,&n2);
	pInto->Clear();
	int numVerts = from.GetNumVerts();
	pInto->m_verts.resize( numVerts );
	for(int i=0;i<numVerts;i++)
	{
		const Vec3 v = from.m_verts[i] - basePos;
		pInto->m_verts[i].x = v * n1;
		pInto->m_verts[i].y = v * n2;
	}
}

void Lift2d(Poly3 * pInto, const Poly2 & from, const Plane & onPlane,const Vec3 & basePos)
{
	Vec3 n1,n2;
	GetTwoPerpNormals(onPlane.GetNormal(),&n1,&n2);

	pInto->Clear();
	int numVerts = from.GetNumVerts();
	pInto->m_verts.resize( numVerts );
	for(int i=0;i<numVerts;i++)
	{
		pInto->m_verts[i] = basePos + from.m_verts[i].x * n1 + from.m_verts[i].y * n2;
	}
}


Plane::ESide ClipPoly(const Poly3 & from,Poly3 * pToF,Poly3 * pToB,const Plane & plane,const float epsilon /*= EPSILON*/)
{
	ASSERT( from.m_verts.size() >= 3 );
	int numVerts = from.GetNumVerts();

	STACK_ARRAY(dists,float,numVerts+1);
	STACK_ARRAY(sides,Plane::ESide,numVerts+1);
	//float				dists[POLY_MAX_VERTS+1];
	//Plane::ESide		sides[POLY_MAX_VERTS+1];
	int					counts[3] = { 0 };
	COMPILER_ASSERT( Plane::eFront == 0 );
	COMPILER_ASSERT( Plane::eBack == 1 );
	COMPILER_ASSERT( Plane::eIntersecting == 2 );
	
	// determine sides for each point
	{for (int i=0 ; i< numVerts; i++)
	{
		dists[i] = plane.DistanceToPoint(from.m_verts[i]);

		if (dists[i] > epsilon)			sides[i] = Plane::eFront;
		else if (dists[i] < -epsilon)	sides[i] = Plane::eBack;
		else							sides[i] = Plane::eIntersecting;

		counts[sides[i]]++;
	}}
	// wrap for convenience
	sides[numVerts] = sides[0];
	dists[numVerts] = dists[0];
	
	if ( pToF ) { pToF->Clear(); }
	if ( pToB ) { pToB->Clear(); }

	if ( counts[Plane::eFront] == 0 && counts[Plane::eBack] == 0 )
	{
		// all verts are *ON* the plane
		if ( pToF ) pToF->m_verts = from.m_verts;
		if ( pToB ) pToB->m_verts = from.m_verts;
		return Plane::eOn;
	}
	else if ( counts[Plane::eBack] == 0 )
	{
		if ( pToF ) pToF->m_verts = from.m_verts;
		return Plane::eFront;
	}
	else if ( counts[Plane::eFront] == 0 )
	{
		if ( pToB ) pToB->m_verts = from.m_verts;
		return Plane::eBack;
	}

	// counts[Front] and [Back] are both > 0 

	for (int i=0 ; i<numVerts ; i++)
	{
		const Vec3 & p1 = from.m_verts[i];
		
		if (sides[i] == Plane::eIntersecting)
		{
			if ( pToF ) pToF->m_verts.push_back(p1);
			if ( pToB ) pToB->m_verts.push_back(p1);
			continue;
		}
	
		if (sides[i] == Plane::eFront)
		{
			if ( pToF ) pToF->m_verts.push_back(p1);
		}
		if (sides[i] == Plane::eBack)
		{
			if ( pToB ) pToB->m_verts.push_back(p1);
		}

		if (sides[i+1] == Plane::eIntersecting || sides[i+1] == sides[i])
			continue;

		// sides[i] is front or back and sides[i+1] is the opposite
		// generate a split point
		// Be consistent when generating split point.  Depend only on
		// vertex data, so if we split the same edge in a different
		// order, we get the exact same result.
		int	first = i;
		int	second = (i+1) % numVerts;
		if (sides[first] == Plane::eFront)
		{
			Swap(first, second);
		}
		float denom = dists[first] - dists[second];
		ASSERT( fabsf(denom) > epsilon );
		float t = dists[first] / denom;
		ASSERT( fiszerotoone(t,0.f) );
		Vec3 mid = (1.f-t)*from.m_verts[first] + t*from.m_verts[second];

		ASSERT( plane.PointSideOrOn(mid,epsilon*2) == Plane::eIntersecting );
			
		if ( pToF ) pToF->m_verts.push_back(mid);
		if ( pToB ) pToB->m_verts.push_back(mid);
	}
	
	return Plane::eIntersecting;
}

Plane::ESide ClipEdge(const Edge &edge,
										Edge * pFront,
										Edge * pBack,
										const Plane & plane,
										const float epsilon /*= EPSILON*/)
{
	//STACK_ARRAY(dists,float,numVerts+1);
	float				dists[2];
	Plane::ESide		sides[2];
	int					counts[3] = { 0 };
	COMPILER_ASSERT( Plane::eFront == 0 );
	COMPILER_ASSERT( Plane::eBack == 1 );
	COMPILER_ASSERT( Plane::eIntersecting == 2 );
	
	// determine sides for each point
	{for (int i=0 ; i< 2; i++)
	{
		dists[i] = plane.DistanceToPoint(edge.verts[i]);

		if (dists[i] > epsilon)			sides[i] = Plane::eFront;
		else if (dists[i] < -epsilon)	sides[i] = Plane::eBack;
		else							sides[i] = Plane::eIntersecting;

		counts[sides[i]]++;
	}}
	
	if ( counts[Plane::eFront] == 0 && counts[Plane::eBack] == 0 )
	{
		// all verts are *ON* the plane
		if ( pFront ) *pFront = edge;
		if ( pBack  ) *pBack = edge;
		return Plane::eOn;
	}
	else if ( counts[Plane::eBack] == 0 )
	{
		if ( pFront ) *pFront = edge;
		return Plane::eFront;
	}
	else if ( counts[Plane::eFront] == 0 )
	{
		if ( pBack ) *pBack = edge;
		return Plane::eBack;
	}

	// counts[Front] and [Back] are both > 0 
	//	one is front, one is back

	ASSERT( sides[0] == Plane::eFront || sides[0] == Plane::eBack );
	ASSERT( sides[1] == Plane::eFront || sides[1] == Plane::eBack );
	
	// generate a split point
			
	float denom = dists[0] - dists[1];
	// denom should actually be >= 2*epsilon, but allow some slip
	ASSERT( fabsf(denom) > epsilon );
	float t = dists[0] / denom;
	ASSERT( fiszerotoone(t,0.f) );

	//Vec3 mid = p1 + t*(p2-p1);
	Vec3 mid = (1.f-t)*edge.verts[0] + t*edge.verts[1];
	
	ASSERT( plane.PointSideOrOn(mid,epsilon*2) == Plane::eIntersecting );
		
	if ( pFront ) 
	{
		pFront->verts[0] = ( sides[0] == Plane::eFront ) ? edge.verts[0] : edge.verts[1];; 
		pFront->verts[1] = mid;
	}
	if ( pBack )
	{
		pBack->verts[0] = mid;
		pBack->verts[1] = ( sides[0] == Plane::eFront ) ? edge.verts[1] : edge.verts[0];;
	}
	
	return Plane::eIntersecting;
}

bool ClipToBox(Edge * pInto, const Edge & from, const AxialBox & _refBox)
{
	AxialBox refBox(_refBox);
	refBox.Expand(EPSILON);

	Plane plane;

	plane.SetFromNormalAndPoint( Vec3::unitX, refBox.GetMin() );
	if ( ! ClipEdgeF(from,pInto,plane) )
		return false;
		
	Edge work;
	Edge * p1 = pInto;
	Edge * p2 = &work;
	
	plane.SetFromNormalAndPoint( Vec3::unitY, refBox.GetMin() );
	if ( ! ClipEdgeF(*p1,p2,plane) )
		return false;
	Swap(p1,p2);
	
	plane.SetFromNormalAndPoint( Vec3::unitZ, refBox.GetMin() );
	if ( ! ClipEdgeF(*p1,p2,plane) )
		return false;
	Swap(p1,p2);
	
	plane.SetFromNormalAndPoint( Vec3::unitXneg, refBox.GetMax() );
	if ( ! ClipEdgeF(*p1,p2,plane) )
		return false;
	Swap(p1,p2);
	
	plane.SetFromNormalAndPoint( Vec3::unitYneg, refBox.GetMax() );
	if ( ! ClipEdgeF(*p1,p2,plane) )
		return false;
	Swap(p1,p2);
	
	plane.SetFromNormalAndPoint( Vec3::unitZneg, refBox.GetMax() );
	if ( ! ClipEdgeF(*p1,p2,plane) )
		return false;

	if ( p2 != pInto )
		*pInto = *p2;

	return true;
}

bool ClipToFrustum(Poly3*         pInto,
							  const Poly3&   from,
							  const Frustum& frustum)
{
	Poly3 work;
	const Poly3* p1 = &from;
	Poly3* p2 = pInto;

	for (int i = 0; i < frustum.GetNumPlanes(); i++)
	{
		if ( ! ClipPolyF(*p1,p2, frustum.GetPlane(i)) )
		return false;
		
		if ( p2 == pInto )
		{
			p1 = pInto;
			p2 = &work;
		}
		else
		{
			p2 = pInto;
			p1 = &work;
		}
	}
	// If we popped out on a odd iteration, then we need to
	//  copy into the output variable...
	if (p1 != pInto)
	{
		*pInto = *p1;
	}

	return true;
}

void PutPointsOnPlane(Poly3 * pInto,const Plane & plane)
{
	for(int v=0;v<pInto->GetNumVerts();v++)
	{
		pInto->m_verts[v] = plane.GetClosestPointOnPlane(pInto->m_verts[v]);
	}
}

void MakeQuadOnPlane(Poly3 * pInto, const Plane & onPlane, const AxialBox & refBox)
{
	const Vec3 c = refBox.GetCenter();
	const Vec3 p = onPlane.GetClosestPointOnPlane(c);
	const float radius = sqrtf( refBox.GetRadiusSqr() );

	Vec3 n1,n2;
	GetTwoPerpNormals(onPlane.GetNormal(),&n1,&n2);

	const float d = radius * 3.f;

	pInto->m_verts.resize(4);
	pInto->m_verts[0] = onPlane.GetClosestPointOnPlane( p + n1 * d + n2 * d );
	pInto->m_verts[1] = onPlane.GetClosestPointOnPlane( p + n1 * d - n2 * d );
	pInto->m_verts[2] = onPlane.GetClosestPointOnPlane( p - n1 * d - n2 * d );
	pInto->m_verts[3] = onPlane.GetClosestPointOnPlane( p - n1 * d + n2 * d );

	// now get the winding right :
	Vec3 cross;
	SetTriangleCross(&cross,pInto->m_verts[0],pInto->m_verts[1],pInto->m_verts[2]);
	if ( (cross * onPlane.GetNormal()) < 0.f )
	{
		pInto->Reverse();
	}

	//pInto->m_plane = onPlane;
}


bool ClipToBox(Poly3 * pInto, const Poly3 & from, const AxialBox & _refBox)
{
	AxialBox refBox(_refBox);
	refBox.Expand(EPSILON);

	/*
	if ( refBox.PlaneSide(pInto->m_plane) != Plane::eIntersecting )
	{
		pInto->Clear();
		return false;
	}
	*/

	Plane plane;

	plane.SetFromNormalAndPoint( Vec3::unitX, refBox.GetMin() );
	if ( ! ClipPolyF(from,pInto,plane) )
		return false;
		
	Poly3 work;
	Poly3 * p1 = pInto;
	Poly3 * p2 = &work;
	
	plane.SetFromNormalAndPoint( Vec3::unitY, refBox.GetMin() );
	if ( ! ClipPolyF(*p1,p2,plane) )
		return false;
	Swap(p1,p2);
	
	plane.SetFromNormalAndPoint( Vec3::unitZ, refBox.GetMin() );
	if ( ! ClipPolyF(*p1,p2,plane) )
		return false;
	Swap(p1,p2);
	
	plane.SetFromNormalAndPoint( Vec3::unitXneg, refBox.GetMax() );
	if ( ! ClipPolyF(*p1,p2,plane) )
		return false;
	Swap(p1,p2);
	
	plane.SetFromNormalAndPoint( Vec3::unitYneg, refBox.GetMax() );
	if ( ! ClipPolyF(*p1,p2,plane) )
		return false;
	Swap(p1,p2);
	
	plane.SetFromNormalAndPoint( Vec3::unitZneg, refBox.GetMax() );
	if ( ! ClipPolyF(*p1,p2,plane) )
		return false;

	if ( p2 != pInto )
		*pInto = *p2;

	return true;
}



// fit a plane to 4 points
// could easily support any number of points
bool FitPlane(const Poly3 & poly,Plane * pPlane)
{
	ASSERT( poly.GetNumVerts() >= 3 );
	const int numVerts = poly.GetNumVerts();

	const Vec3 center = poly.ComputeCenter();

	Plane fallbackPlane(Vec3::unitZ,0.f);
	{
		bool ok = false;
		int i;
		for(i=1;!ok && i<numVerts;i++)
		{
			for(int j=i+1;!ok && j<numVerts;j++)
			{
				if ( fallbackPlane.SetFromThreePoints(poly.m_verts[0],poly.m_verts[i],poly.m_verts[j]) > 0.f )
				{
					ok = true;
					break;
				}
			}
		}
		ASSERT(ok);
		*pPlane = fallbackPlane;
	
		// check for an exact match
		
		for(i=0;i<numVerts;i++)
		{
			if ( fallbackPlane.PointSideOrOn(poly.m_verts[i]) != Plane::eIntersecting )
				break;
		}
		if ( i == numVerts )
		{
			// all on
			return true;
		}
	}

	Mat3 mat;
	int r;
	for(r=0;r<3;r++)
	{
		for(int c=0;c<3;c++)
		{
			float & el = mat.Element(r,c);
			el = 0.f;
			for(int v=0;v<numVerts;v++)
			{
				Vec3 d = poly.m_verts[v] - center;
				el += d[r] * d[c];
			}
		}
	}

	// find the vector that makes mat*vec = 0

	// make the factor between u&v ; can do any two rows to do this :
	// choose the two rows that make the denom of cUV the biggest
	
	const Vec3 cross12 = MakeCross( mat.GetRowX() , mat.GetRowY() );
	const Vec3 cross23 = MakeCross( mat.GetRowY() , mat.GetRowZ() );
	const Vec3 cross31 = MakeCross( mat.GetRowZ() , mat.GetRowX() );

	double cUV;
	if ( fabsf(cross31.y) > fabsf(cross23.y) &&
		 fabsf(cross31.y) > fabsf(cross12.y) )
	{
		cUV = cross31.x/(double)cross31.y;
	}
	else if ( fabsf(cross23.y) > fabsf(cross12.y) )
	{
		cUV = cross23.x/(double)cross23.y;
	}
	else
	{
		if ( cross12.y == 0.f )
		{
			return false;
		}
		cUV = cross12.x/(double)cross12.y;
	}

	// again can use any row here :
	//  choose the row with the biggest z component
	int rowBigZ = 0;
	for(r=1;r<3;r++)
	{
		if ( fabsf(mat.GetRow(r).z) >= fabsf(mat.GetRow(rowBigZ).z) )
		{
			rowBigZ = r;
		}
	}
	const Vec3 & rq = mat.GetRow(rowBigZ);
	if ( fabsf(rq.z) == 0.f )
	{
		return false;
	}

	const double qA = cUV*cUV + 1.0;
	const double qB = - ( rq.x * cUV + rq.y ) / rq.z;
	const double qC = - 1.0;

	// solve a quadratic :
	const double qInSqrt = qB*qB - 4.f*qA*qC;
	if ( qInSqrt < 0.0 )
	{
		return false;
	}

	const double qSqrt = sqrt(qInSqrt);

	const double v_pos = (-qB + qSqrt)/(2.0*qA);
	const double v_neg = (-qB - qSqrt)/(2.0*qA);

	const double u_pos = cUV * v_pos;
	const double u_neg = cUV * v_neg;

	const double w_pos_sqr = 1.0 - v_pos*v_pos - u_pos*u_pos;
	const double w_neg_sqr = 1.0 - v_neg*v_neg - u_neg*u_neg;

	// take the pos or neg answer ?

	if ( w_pos_sqr < 0.0 && w_neg_sqr < 0.0 )
	{
		 // bad news, no solution
		 return false;
	}

	Vec3 n1(fallbackPlane.GetNormal()),n2(fallbackPlane.GetNormal());

	if ( w_pos_sqr >= 0.0 )
	{
		n1.x = (float) u_pos;
		n1.y = (float) v_pos;
		n1.z = (float) sqrt(w_pos_sqr);

		// mat * n1 should be zero
	}
	if ( w_neg_sqr >= 0.0 )
	{
		n2.x = (float) u_neg;
		n2.y = (float) v_neg;
		n2.z = (float) sqrt(w_neg_sqr);
		
		// mat * n2 should be zero
	}
	ASSERT( n1.IsNormalized() );
	ASSERT( n2.IsNormalized() );

	Plane plane1(n1,center);
	Plane plane2(n2,center);

	float d1 = 0.f;
	float d2 = 0.f;
	
	for(int v=0;v<numVerts;v++)
	{
		d1 += fsquare( plane1.DistanceToPoint(poly.m_verts[v]) );
		d2 += fsquare( plane2.DistanceToPoint(poly.m_verts[v]) );
	}

	if ( d1 <= d2 )
	{
		*pPlane = plane1;
	}
	else
	{
		*pPlane = plane2;
	}

	return true;
}

bool LineSegmentIntersects(const Poly3 & poly,const Plane & polyPlane,const Vec3& start,const Vec3& end)
{
	const Plane & plane = polyPlane;
	const float ds = plane.DistanceToPoint(start);
	const float de = plane.DistanceToPoint(end);
	if ( ds > 0.f && de > 0.f )	return false;
	if ( ds < 0.f && de < 0.f )	return false;
	Vec3 intersection;
	if ( ds == de )
	{
		// degenerate segment right on the poly plane
		ASSERT( ds == 0.f );
		intersection = start;
	}
	else
	{
		float t = ds / ( ds - de );
		intersection = MakeLerp(start,end,t); // Lerp = start * (1-t) + end * t
	}
	ASSERT( plane.PointSideOrOn(intersection) == Plane::eIntersecting );

	return IsProjectedPointInPoly(poly,polyPlane, intersection);
}

// IsProjectedPointInPoly tests if "point" is inside "poly" in a 2d sense (eg. using the edge perp planes)
bool IsProjectedPointInPoly(const Poly3& poly,
										const Plane & polyPlane,
									   const Vec3&  point)
{
	ASSERT(poly.IsValid());
	ASSERT(point.IsValid());

	const Vec3 & normal =polyPlane.GetNormal();
	const int numVerts  = poly.m_verts.size32();

	int prev = numVerts-1;
	for (int v = 0; v < numVerts; v++)
	{
		// edge from prev to v
		const Vec3 & v0 = poly.m_verts[prev];
		const Vec3 & v1 = poly.m_verts[v];
		
		// cheap, just make a plane -
		const Vec3 edge = v1 - v0;
		const Vec3 outwards = edge ^ normal;
		
		ASSERT( fequal( outwards*v0 , outwards*v1 , 0.01f) );
	
		// no need to normalize since we're just checking sign :
		const float d = (outwards * point) - (outwards * v0);

		if ( d > 0.f )
		{
			// that's outside the poly
			return false;
		}
	
		prev = v;
	}

	// inside the poly :
	return true;
}


bool PolyIntersectsSphere(const Poly3& poly,
										const Plane & polyPlane, const Sphere& sphere)
{
	// Test is: find the closest point on the polygon to the center of the sphere, then
	//  check the distance.  Very simplistic, we do this by:
	//    1. Project the point
	//    2. For all edges, if the point is in front of the edge, clamp it to the edge in the plane
	//    3. check radius

	ASSERT(poly.IsValid());
	ASSERT(sphere.IsValid());

	const Vec3 & normal = polyPlane.GetNormal();
	const int numVerts  = poly.m_verts.size32();

	Vec3 workPoint = polyPlane.GetClosestPointOnPlane(sphere.GetCenter());

	int prev = numVerts-1;
	for (int v = 0; v < numVerts; v++)
	{
		// edge from prev to v
		const Vec3 & v0 = poly.m_verts[prev];
		const Vec3 & v1 = poly.m_verts[v];
		
		// cheap, just make a plane -
		const Vec3 edge = v1 - v0;
		Vec3 outwards = edge ^ normal;
		if (outwards.NormalizeSafe() == 0)
		{
			DO_N_TIMES(30, lprintf("Very suspicious poly normalization bug\n"));
			continue;
		}
		ASSERT( fequal( outwards*v0 , outwards*v1 , 0.01f) );
	
		Plane p(outwards, v0);

		if ( p.DistanceToPoint(workPoint) > 0.0f )
		{
			workPoint = p.GetClosestPointOnPlane(workPoint);
		}
	
		prev = v;
	}

	const float distSq = DistanceSqr(sphere.GetCenter(),workPoint);
	const float RSq = fsquare(sphere.GetRadius());
	return distSq < RSq;
}

/**

CB 8-24-04 :

A better way to do SegmentIntersects would be with a direct distance computation.

I think you can compute the distance quickly like this :

Find the closest point on "seg" to the plane of the poly "P"
Clip that point into the poly by checking vs. edge planes
For any edge the point is outside of,
	check edge-seg distance ; if that's <= r , return true
If the point is inside all edges,
	the seg intersects the poly, return true
Else, take the clipped point and check distance to the (new) closest point on the seg
	if that's <= r , return true
return false

**/

bool SegmentIntersects(const Poly3& poly,const Plane & polyPlane, const Segment & seg)
{
	ASSERT( poly.IsValid() );
	ASSERT( seg.IsValid() );
	
	// should've been rejected at a higher level:
	ASSERT( ! seg.IsZero() );

	// use SAT test :
	
	const Vec3 segCenter = seg.GetCenter();
	const Vec3 & polyNormal = polyPlane.GetNormal();
	const int numVerts  = poly.m_verts.size32();
	const float segR = seg.GetRadius();
	const float segL = seg.GetLength();

	// axis 1 : poly normal :
	{
		const float d = fabsf( polyPlane.DistanceToPoint(segCenter) );
		const float r = seg.GetRadiusInDirection(polyNormal);
		if ( d > r )
		{
			return false;
		}
	}
	
	// axis 2&3 : seg Normal and seg Normal cross poly normal
	{
		const Vec3 & segNormal = seg.GetNormal();
		
		// if crossNormal is degenerate, it's an unnecessary test
		Vec3 crossNormal = segNormal ^ polyNormal;
		bool hasCrossNormal = ( crossNormal.NormalizeSafe() != 0.f );
		
		float segNormalDMin =  FLT_MAX;
		float segNormalDMax = -FLT_MAX;
		float crossNormalDMin =  FLT_MAX;
		float crossNormalDMax = -FLT_MAX;
		
		for (int v = 0; v < numVerts; v++)
		{
			// would be a little faster to not subtract segCenter off here, but it makes the float math much more accurate
			const Vec3 V = poly.m_verts[v] - segCenter;
			const float segNormalD = segNormal * V;
			const float crossNormalD = crossNormal * V;
			segNormalDMin = MIN(segNormalDMin,segNormalD);
			segNormalDMax = MAX(segNormalDMax,segNormalD);
			crossNormalDMin = MIN(crossNormalDMin,crossNormalD);
			crossNormalDMax = MAX(crossNormalDMax,crossNormalD);
		}
		
		float segNormalSeparation;
		if ( segNormalDMin > 0.f ) segNormalSeparation = segNormalDMin;
		else if ( segNormalDMax < 0.f ) segNormalSeparation = -segNormalDMax;
		else segNormalSeparation = 0.f;
		
		const float segNormalRadius = segL*0.5f + segR;
		if ( segNormalSeparation > segNormalRadius )
		{
			return false;
		}
		
		if ( hasCrossNormal )
		{
			float crossNormalSeparation;
			if ( crossNormalDMin > 0.f ) crossNormalSeparation = crossNormalDMin;
			else if ( crossNormalDMax < 0.f ) crossNormalSeparation = -crossNormalDMax;
			else crossNormalSeparation = 0.f;
			
			if ( crossNormalSeparation > segR )
			{
				return false;
			}
		}
	}

	// (we're almost surely colliding at this point)
	
	// edge perp axes : 
	{
		int prev = numVerts-1;
		for (int v = 0; v < numVerts; v++)
		{
			// edge from prev to v
			const Vec3 & v0 = poly.m_verts[prev];
			const Vec3 & v1 = poly.m_verts[v];
			
			// cheap, just make a plane -
			const Vec3 edge = v1 - v0;
			Vec3 outwards = edge ^ polyNormal;

			const float dUnn = (outwards * segCenter) - (outwards * v0);
			
			if ( dUnn <= 0.f )
			{
				// behind
				prev = v;
				continue;
			}
			
			const float invLen = ReciprocalSqrt( outwards.LengthSqr() );
			const float d = dUnn * invLen;
			outwards *= invLen;
			
			const float r =seg.GetRadiusInDirection(outwards);
			
			if ( d > r )
			{
				return false;
			}			
		
			prev = v;
		}
	}
		
	// CB 11-18-04 : check sphere of the whole segment
	Sphere seSphere(segCenter,segR + seg.GetLength()*0.5f);
	if ( ! PolyIntersectsSphere(poly,polyPlane,seSphere) )
	{
		return false;
	}
	
	// no axis rules out intersection :

	// @@@@ CB 08-23-04 :
	//	this is not right, I think.  I think there are some more axes we need to test.
	//	but 99% of the time it's right, and the case we get wrong are nearly intersecting anyway,
	//	so it's just a form of having a big epsilon, he he ? ;)

	// CB 11-18-04 : the main case we still get wrong is the case of segments that are fully outside the poly, moving tangentially across a
	//	vertex voronoi region.  We should be able to take the normal that's from the vert and perp to the segment normal and use that to reject these cases
	
	return true;
}

END_CB

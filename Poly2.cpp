#include "Base.h"
#include "Poly2.h"
#include "ConvexHullBuilder2d.h"
#include "IntGeometry.h"
#include "Vec2U.h"
#include <algorithm>

START_CB

void Poly2::Clear()
{
	m_verts.clear(); 
}

void Poly2::Reverse()
{
	std::reverse(m_verts.begin(),m_verts.end());
}

float Poly2::GetArea() const
{
	ASSERT( GetNumVerts() >= 0 );
	const Vec2 & a = m_verts[0];
	float area = 0.f;
	for(int i=2;i<GetNumVerts();i++)
	{
		const Vec2 & b = m_verts[i-1];
		const Vec2 & c = m_verts[i];
		area += TriangleAreaCCW(a,b,c);
	}
	return area;
}

const Vec2 Poly2::GetCenter() const
{
	Vec2 ret = m_verts[0];
	for(int i=1;i<GetNumVerts();i++)
	{
		ret += m_verts[i];
	}
	ret *= (1.f/GetNumVerts());
	return ret;
}

void Poly2::GetBBox(RectF * pBox) const
{
	pBox->SetToPointV( m_verts[0] );
	for(int i=1;i<GetNumVerts();i++)
	{
		pBox->ExtendToPointV( m_verts[i] );
	}
}

Plane::ESide Poly2::PlaneSide(const Plane2 & plane) const
{
	int numF=0,numOn=0,numB=0;
	ASSERT( GetNumVerts() > 0 );
	for(int i=0;i<GetNumVerts();i++)
	{
		Plane::ESide pointSide = plane.PointSideOrOn(m_verts[i]);
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
bool CleanPoly(const Poly2 & from,Poly2 * pTo)
{
	// copy all verts
	pTo->m_verts = from.m_verts;

	// remove verts from "to" in place, sliding down
	for(int i=0;i<pTo->GetNumVerts();)
	{
		ASSERT( pTo->GetNumVerts() >= 3 );

		int in1 = (i+1)%(pTo->GetNumVerts());
		int in2 = (i+2)%(pTo->GetNumVerts());
		const Vec2 & a = pTo->m_verts[i];
		const Vec2 & b = pTo->m_verts[in1];
		const Vec2 & c = pTo->m_verts[in2];

		// a,b,c should be counterclockwise
		float cross = TriangleCrossCCW(a,b,c);

		if ( cross <= 0.f ) // @@ <= EPSILON ?
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

	return true;
}

/*
	CleanPoly removes colinear segments, and tries
	to makes poly convex if it's gotten little wiggles

	CleanPolySerious uses rigorous integer 2d convex hull
*/
bool CleanPolySerious(const Poly2 & from,Poly2 * pTo)
{
	RectF box(0,0);
	from.GetBBox(&box);
	box.GrowToSquare();

	if ( box.Width() < EPSILON || box.Height() < EPSILON )
	{
		pTo->Clear();
		return false;
	}

	// quantize to ints
	vector<Vec2i>	vi;
	vi.resize(from.GetNumVerts());
	IntGeometry::Quantize(vi.data(),from.m_verts.data(),from.GetNumVerts(),box);

	// build the convex hull in ints
	vector<Vec2i> hullPolygon;
	ConvexHullBuilder2d::Make2d(vi.data(),vi.size32(),hullPolygon);

	if ( hullPolygon.size() < 3 )
	{
		pTo->Clear();
		return false;
	}

	// dequantize back to floats
	pTo->m_verts.resize(hullPolygon.size32());
	IntGeometry::Dequantize(pTo->m_verts.data(),hullPolygon.data(),hullPolygon.size32(),box);

	return true;
}



Plane::ESide ClipPoly(const Poly2 & from,Poly2 * pToF,Poly2 * pToB,const Plane2 & plane,const float epsilon /*= EPSILON*/)
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
	
	if ( pToF ) pToF->Clear();
	if ( pToB ) pToB->Clear();

	if ( counts[Plane::eFront] == 0 && counts[Plane::eBack] == 0 )
	{
		// all verts are *ON* the plane
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
		const Vec2 & p1 = from.m_verts[i];
		
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
		
		const Vec2 & p2 = from.m_verts[(i+1)%numVerts];
		
		float denom = dists[i] - dists[i+1];
		// denom should actually be >= 2*epsilon, but allow some slip
		ASSERT( fabsf(denom) > epsilon );
		float t = dists[i] / denom;
		ASSERT( fiszerotoone(t,0.f) );

		//Vec2 mid = p1 + t*(p2-p1);
		Vec2 mid = (1.f-t)*p1 + t*p2;
		
		ASSERT( plane.PointSideOrOn(mid,epsilon*2) == Plane::eIntersecting );
			
		if ( pToF ) pToF->m_verts.push_back(mid);
		if ( pToB ) pToB->m_verts.push_back(mid);
	}
	
	return Plane::eIntersecting;
}

END_CB

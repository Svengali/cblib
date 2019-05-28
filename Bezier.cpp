
#include "cblib/Bezier.h"
#include "cblib/VolumeBuilder.h"
#include "cblib/Sphere.h"
#include "cblib/AxialBox.h"
//#include "cblib/vector_a.h"
#include "cblib/vector.h"
#include <string.h> // for memcpy

START_CB

//-------------------------------------
/*

Controls are B0,B1,B2,B3

The Curve function is :

s = (1-t)

B(t) = B0 * s^3 + B1 * s^2 * t * 3 + B2 * s * t^2 * 3 + B3 * t^3

B'(t) = 3 * [ (B1-B0) * s^2 + (B2-B1) * st + (B3 - B2) * t^2 ]

---------

Bezier is invariant under this exchange :

B0 -> B3
B1 -> B2
B2 -> B1
B3 -> B0

t -> (1-t)

------------

The convex hull of {B0,B1,B2,B3} is gauranteed to enclose the curve

*/

#define B0	m_controls[0]
#define B1	m_controls[1]
#define B2	m_controls[2]
#define B3	m_controls[3]

//-------------------------------------

void Bezier::SetFromControls(const Vec3 * pControls)
{
	memcpy(m_controls,pControls,4*sizeof(Vec3));
}

void Bezier::SetFromControls(const Vec3 & b0,const Vec3 & b1,const Vec3 & b2,const Vec3 & b3)
{
	B0 = b0;
	B1 = b1;
	B2 = b2;
	B3 = b3;
}

void Bezier::SetFromEnds(const Vec3 & v0,const Vec3 & d0,
			const Vec3 & v1,const Vec3 & d1)
{
	B0 = v0;
	B1 = v0 + d0*(1.f/3.f);
	B2 = v1 - d1*(1.f/3.f);
	B3 = v1;
}

void Bezier::FitWithDuration(const Vec3 & pos0,const Vec3 & pos1,
						const Vec3 & vel0,const Vec3 & vel1,const float duration)
{
	B0 = pos0;
	B1 = pos0 + vel0*(duration/3.f);
	B2 = pos1 - vel1*(duration/3.f);
	B3 = pos1;
}

//-------------------------------------


const Vec3 Bezier::GetDerivative0() const
{
	return 3.f*(B1 - B0);
}

const Vec3 Bezier::GetDerivative1() const
{
	return 3.f*(B3 - B2);
}

const Vec3 Bezier::Get2ndDerivative0() const
{
	return 6.f*(B2 - B0);
}

const Vec3 Bezier::Get2ndDerivative1() const
{
	return 6.f*(B3 - B1);
}

const Vec3 Bezier::Get3rdDerivative() const
{
	// 3 * ( (B0+B3)/2 - (B1+B2)/2 )
	return 6.f*( B0 - B1 - B2 + B3 );
}

const Vec3 Bezier::GetValue(const float t) const
{
	const float s = (1.f-t);
	return B0* (s*s*s) + B1*(s*s*t*3.f) + B2*(t*t*s*3.f) + B3*(t*t*t);
}

const Vec3 Bezier::GetDerivative(const float t) const
{
	const float s = (1.f-t);
	return (B1-B0) * (3.f*s*s) + (B2-B1) * (6.f*s*t) + (B3 - B2) * (3.f*t*t);
}

const Vec3 Bezier::Get2ndDerivative(const float t) const
{
	return MakeLerp( Get2ndDerivative0(),Get2ndDerivative1(), t );
}

void Bezier::GetBoundinSphere(Sphere * pInto) const
{
	SphereAround4(pInto,B0,B1,B2,B3);
}

void Bezier::GetBoundingBox(AxialBox * pInto) const
{
	pInto->SetEnclosing(B0,B1);
	pInto->ExtendToPoint(B2);
	pInto->ExtendToPoint(B3);
}

//-------------------------------------

float Bezier::ComputePartialLength(const float start,const float end,const float dt /*= 0.01f*/) const
{
	// Length requires an integration!!
	ASSERT( fiszerotoone(start) );
	ASSERT( fiszerotoone(end) );

	Vec3 pos[2];
	int iPos=1;
	pos[0] = GetValue(start);

	float length = 0.f;

	float t;
	for(t=start+dt;t<=end;t+=dt)
	{
		// sample at t into pos[iPos];
		pos[iPos] = GetValue(t);
		// add on current distance
		const float d = Distance(pos[0],pos[1]);
		length += d;
		// swap iPos
		iPos ^= 1;
	}

	// add on the last bit :
	if ( t < end )
	{
		pos[iPos] = GetValue(end);
		const float d = Distance(pos[0],pos[1]);
		length += d;		
	}

	return length;
}

float Bezier::ComputeParameterForLength(const float lengthParam,const float dt /*= 0.01f*/) const
{
	if ( lengthParam <= 0.f )
		return 0.f;

	Vec3 pos[2];
	int iPos=1;
	pos[0] = GetValue(0.f);

	float length = 0.f;

	for(float t = 0.f; t <= 1.f; t += dt)
	{
		// sample at t into pos[iPos];
		pos[iPos] = GetValue(t+dt);
		// add on current distance
		const float d = Distance(pos[0],pos[1]);
		const float nextLength = length + d;

		if ( lengthParam <= nextLength )
		{
			// I'm somewhere in between (t) and (t+dt)
			float lerper = fmakelerperclamped(length,nextLength,lengthParam);
			return flerp(t,t+dt,lerper);
		}

		length = nextLength;
		// swap iPos
		iPos ^= 1;
	}

	return 1.f;
}



//-------------------------------------
// deCasteljau Bezier subdivision :

void Bezier::Subdivide(Bezier *pBez1,Bezier *pBez2) const
{
	Vec3 b01,b12,b23,b012,b123,b0123;

	b01.SetAverage(B0,B1);
	b12.SetAverage(B1,B2);
	b23.SetAverage(B2,B3);
	b012.SetAverage(b01,b12);
	b123.SetAverage(b12,b23);
	b0123.SetAverage(b012,b123);
	
	pBez1->SetFromControls(B0,b01,b012,b0123);
	pBez2->SetFromControls(b0123,b123,b23,B3);
}

void Bezier::Subdivide(Bezier *pBez1,Bezier *pBez2,const float t) const
{
	Vec3 b01,b12,b23,b012,b123,b0123;

	b01.SetLerp(B0,B1,t);
	b12.SetLerp(B1,B2,t);
	b23.SetLerp(B2,B3,t);
	b012.SetLerp(b01,b12,t);
	b123.SetLerp(b12,b23,t);
	b0123.SetLerp(b012,b123,t);
	
	pBez1->SetFromControls(B0,b01,b012,b0123);
	pBez2->SetFromControls(b0123,b123,b23,B3);
}

//-------------------------------------

//! tells how far the Bezier differs from a line
//!		in units squared
float Bezier::ComputeLinearErrorBoundSqr() const
{
	/*
		Error is bounded by
		[ B1 - lerp(B0,B3,(1/3)) ] or [ B2 - ..(2/3) ]

		Actually, there's a tighter bound, which is
		[ B1 - projection of B1 onto (B0,B3) ]
		but this is much cheaper
	*/

	const Vec3 d1 = B0 * (2.f/3.f) + B3 * (1.f/3.f) - B1;
	const Vec3 d2 = B0 * (1.f/3.f) + B3 * (2.f/3.f) - B2;

	const float errSqr = MAX( d1.LengthSqr(), d2.LengthSqr() );

	return errSqr;
}

//-------------------------------------

struct ComputeClosestPointRecurser
{
	Bezier sub;
	float start,end;
};

void Bezier::ComputeClosestPointAccurate(const Vec3 & query,const float maxErrSqr,float * pDistSqr, float * pTime) const
{
	//gMemoryArenaAutoScopeHead scoper;

	// use a manual stack on the Arena
	//vector_a<ComputeClosestPointRecurser>	stack;
	vector<ComputeClosestPointRecurser>	stack;

	// seed the search :
	float curBestParameter = 0.f;
	float curBestDSqr = DistanceSqr(query,B0);

	// push the root ;
	stack.push_back();
	stack.back().sub = *this;
	stack.back().start = 0.f;
	stack.back().end = 1.f;

	while( stack.size() > 0 )
	{
		const ComputeClosestPointRecurser & cur = stack.back();
		
		// first try to early out using the axial-bound
		//	of the current curve
		AxialBox ab;
		cur.sub.GetBoundingBox(&ab);
		float minDsqr = ab.DistanceSqr(query);

		if ( minDsqr >= curBestDSqr )
		{
			// can't possibly be closer than what we already found
			stack.pop_back();
			continue;
		}

		const float dHeadSqr = DistanceSqr(query,cur.sub.GetValue0());
		const float dTailSqr = DistanceSqr(query,cur.sub.GetValue1());

		// if it's nearly a line, we're done recursing
		// just do linear distance
		if ( cur.sub.ComputeLinearErrorBoundSqr() <= maxErrSqr )
		{
			float dSqr,t;
			ComputeClosestPointToEdge(query, cur.sub.GetValue0(),cur.sub.GetValue1(), &dSqr,&t);
			ASSERT( (minDsqr/dSqr) <= 1.f+EPSILON );
			if ( dSqr < curBestDSqr )
			{
				curBestDSqr = dSqr;
				curBestParameter = flerp(cur.start,cur.end,t);
			}
			stack.pop_back();
			continue;
		}

		// pop one and push two
		// sucky copy
		ComputeClosestPointRecurser parent = stack.back();
		stack.pop_back();
		
		stack.push_back();
		stack.push_back();
		int st0 = stack.size32()-2;
		int st1 = stack.size32()-1;

		//  try to make the back of the stack the closer one
		// default order makes the second half of the curve get
		//	subdivided first, eg. it's closer
		if ( dHeadSqr < dTailSqr )
			Swap(st0,st1);

		parent.sub.Subdivide(&(stack[st0].sub),&(stack[st1].sub));

		stack[st0].start = parent.start;
		stack[st0].end   = (parent.start+parent.end)*0.5f;
		stack[st1].start = stack[st0].end;
		stack[st1].end   = parent.end;
	}

	*pDistSqr = curBestDSqr;
	*pTime = curBestParameter;
}


void Bezier::ComputeClosestPointLinear(const Vec3 & query,float * pDistSqr, float * pTime) const
{
	ComputeClosestPointToEdge(query, GetValue0(), GetValue1(), pDistSqr,pTime);
}


//-------------------------------------

END_CB

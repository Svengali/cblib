#pragma once

#include "cblib/Vec3.h"

START_CB

class Sphere;
class AxialBox;

//=========================================================================

// cubic Bezier curve
// parameterized in 0->1
class Bezier
{
public:
	 Bezier() { }
	~Bezier() { }

	//-------------------------------------

	void SetFromControls(const Vec3 * pControls);
	void SetFromControls(const Vec3 & b0,const Vec3 & b1,const Vec3 & b2,const Vec3 & b3);

	// "d" is the parametric derivative	
	void SetFromEnds(const Vec3 & v0,const Vec3 & d0,
				const Vec3 & v1,const Vec3 & d1);

	// "vel" is the velocity in real time
	//	duration is the length of the curve in real time; eg. [0,1] parametric, [0,duration] real time
	void FitWithDuration(const Vec3 & pos0,const Vec3 & pos1,
							const Vec3 & vel0,const Vec3 & vel1,const float duration);
		// to get velocity back out, do GetDerivative()/duration
		// to get acceleration do Get2ndDerivative() /duration^2

	//-------------------------------------

	const Vec3 * GetControls() const { return m_controls; }

	const Vec3 GetValue(const float t) const;
	const Vec3 GetDerivative(const float t) const;
	const Vec3 Get2ndDerivative(const float t) const;
	const Vec3 Get3rdDerivative() const;

	const Vec3 GetValue0() const { return m_controls[0]; }
	const Vec3 GetValue1() const { return m_controls[3]; }
	const Vec3 GetDerivative0() const;
	const Vec3 GetDerivative1() const;
	const Vec3 Get2ndDerivative0() const;
	const Vec3 Get2ndDerivative1() const;

	void GetBoundinSphere(Sphere * pInto) const;
	void GetBoundingBox(AxialBox * pInto) const;

	// length is computed with an iterative integration !
	float ComputePartialLength(const float start,const float end,const float dt = 0.01f) const;
	float ComputeLength(const float dt = 0.01f) const { return ComputePartialLength(0.f,1.f,dt); }

	float ComputeParameterForLength(const float length,const float dt = 0.01f) const;

	// subdivide makes two beziers for the [0,0.5] and [.5,1] ranges
	void Subdivide(Bezier *pBez1,Bezier *pBez2) const;
	
	// breaks [0,t][t,1]
	void Subdivide(Bezier *pBez1,Bezier *pBez2,const float t) const;

	//! tells how far the Bezier differs from a line
	//!		in units squared
	float ComputeLinearErrorBoundSqr() const;

	//! finds the closest point on the curve to "query"
	//!	 within an error of maxErrSqr
	void ComputeClosestPointAccurate(const Vec3 & query,const float maxErrSqr,float * pDistSqr, float * pTime) const;

	void ComputeClosestPointLinear(const Vec3 & query,float * pDistSqr, float * pTime) const;

	//-------------------------------------

private:
	Vec3	m_controls[4];
	// 3*4*4 = 48 bytes
};

//=========================================================================

END_CB

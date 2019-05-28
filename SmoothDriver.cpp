#include "SmoothDriver.h"
#include "cblib/inc.h"
#include "cblib/Log.h"
#include "cblib/vec3u.h"
#include "cblib/FloatUtil.h"

START_CB

//-----------------------------------------------------------

/*
	damping is usually 1.0 (critically damped)
	frequency is usually in the range 1..16
		freq has units [1/time]
		damping is unitless
		
	ComputePDAccel is NOT an exact step, it's an implicit Euler step,
	which is more stable and has some artificial damping
	
	It should be used to step velocity first, then step pos with the new velocity :
	float v1 = v0 + a*dt;
	float x1 = x0 + v1 * dt;
*/
/*
inline float ComputePDAccel(const float fmValue,const float fmVelocity,
					const float toValue,const float toVelocity,
					const float frequency, const float damping,
					const float dt )
{
	const float ks = fsquare(frequency)*(36.f);
	const float kd = frequency*damping*(9.f);

	// scale factor for implicit integrator :
	//	usually just slightly less than one
	const float scale = 1.0f / ( 1.0f  + kd* dt + ks*dt*dt );

	const float ksI = ks  *  scale;
	const float kdI = ( kd + ks* dt ) * scale;

	return ksI*(toValue-fmValue) + kdI*(toVelocity-fmVelocity);
}
*/

inline void ApplyPDClamping(
					float * pValue,
					float * pVelocity,
					const float toValue,
					const float toVelocity,
					const float frequency,
					const float damping,
					const float minVelocity,
					const float dt )
{
	float x0 = *pValue;
	float v0 = *pVelocity;

	float a = ComputePDAccel(x0,v0,toValue,toVelocity,frequency,damping,dt);
	
	// step value with the velocity from the end of the frame :
	float v1 = v0 + a*dt;
	float x1 = x0 + v1 * dt;

	float smallStep = minVelocity * dt;

	if ( fabsf(x1-x0) <= smallStep )
	{
		if ( fabsf(x0 - toValue) <= smallStep )
		{
			*pValue = toValue;
			*pVelocity = toVelocity;
		}
		else
		{
			// tiny velocities, do min step
			float sign = fsign(toValue - x0); 
			*pVelocity = sign * minVelocity;
			*pValue = x0 + (*pVelocity) * dt;
		}
	}
	else
	{
		*pValue = x1;
		*pVelocity = v1;
	}
	
}

inline void ApplyPDClamping(
					Vec3 * pValue,
					Vec3 * pVelocity,
					const Vec3 toValue,
					const Vec3 toVelocity,
					const float frequency,
					const float damping,
					const float minVelocity,
					const float dt )
{
	Vec3 x0 = *pValue;
	Vec3 v0 = *pVelocity;

	Vec3 a;

	a.x = ComputePDAccel(x0.x ,v0.x ,toValue.x ,toVelocity.x ,frequency,damping,dt);
	a.y = ComputePDAccel(x0.y ,v0.y ,toValue.y ,toVelocity.y ,frequency,damping,dt);
	a.z = ComputePDAccel(x0.z ,v0.z ,toValue.z ,toVelocity.z ,frequency,damping,dt);
	
	// step value with the velocity from the end of the frame :
	Vec3 v1 = v0 + a*dt;
	Vec3 x1 = x0 + v1 * dt;

	float smallStep = minVelocity * dt;

	if ( DistanceSqr(x1,x0) <= fsquare(smallStep) )
	{
		if ( DistanceSqr(x0,toValue) <= fsquare(smallStep) )
		{
			*pValue = toValue;
			*pVelocity = toVelocity;
		}
		else
		{
			// tiny velocities, do min step
			*pVelocity = toValue - x0;
			ClampToMinLen(pVelocity,minVelocity);
			*pValue = x0 + (*pVelocity) * dt;
		}
	}
	else
	{
		*pValue = x1;
		*pVelocity = v1;
	}
	
}
					
struct Bezier
{
	Vec3 B0,B1,B2,B3;
	
	void Fit(const Vec3 &x0, const Vec3 &x1, const Vec3 &v0, const Vec3 &v1, const float T)
	{
		// bezier controls :
		B0 = x0;
		B3 = x1;
		//d0 = 3*(B1-B0);
		B1 = x0 + v0*(T/3.f);
		B2 = x1 - v1*(T/3.f);
	}
	
	const Vec3 X(const float t) const
	{
		const float s = 1.f - t;
		return B0* (s*s*s) + B1*(s*s*t*3.f) + B2*(t*t*s*3.f) + B3*(t*t*t);
	}
	
	const Vec3 dX(const float t) const
	{
		const float s = 1.f - t;
		return (B1-B0) * (3.f*s*s) + (B2-B1) * (6.f*s*t) + (B3 - B2) * (3.f*t*t);
	}
	
	const Vec3 ddX(const float t) const
	{
		const Vec3 A0 = 6.f*B0 + 6.f*B2 - 12.f*B1;
		const Vec3 A1 = 3.f*B1 - 3.f*B2 - B0 + B3;
		
		return A0 + (6.f*t) * A1;
	}
	
};


// how close are we to having the max accel be what we want :
inline float ErrorBezierWithMaxAccel(const Vec3 &x0, const Vec3 &x1, const Vec3 &v0, const Vec3 &v1, const float T, const float maxA)
{
	Bezier bez;
	bez.Fit(x0,x1,v0,v1,T);
	
	float a0 = bez.ddX(0).Length()/fsquare(T);
	float a1 = bez.ddX(1).Length()/fsquare(T);
	float a = MAX(a0,a1);
	
	return fsquare( a - maxA );
}

inline float MakeBezierWithMaxAccel_BinSearch(Bezier * pBez,const Vec3 &x0, const Vec3 &x1, const Vec3 &v0, const Vec3 &v1, const float minT, const float maxA)
{
	/**
	
	CB note : there's an alternative way to do this, you can solve it analytically.
	It's a quartic equation in T, so you then have to use Newton's Method or something to solve for the roots of the quartic.
	
	**/

	// first see if minT gives us a low enough acceleration :

	pBez->Fit(x0,x1,v0,v1,minT);
	
	float a0 = pBez->ddX(0).Length()/fsquare(minT);
	float a1 = pBez->ddX(1).Length()/fsquare(minT);

	if ( a0 < maxA && a1 < maxA )
	{
		return minT;
	}
	
	// no ; well then find a bound where loT is too short and hiT is too long :
	
	float loT = minT;
		
	for(;;)
	{
		float hiT = loT*2;
	
		pBez->Fit(x0,x1,v0,v1,hiT);
		
		float a0 = pBez->ddX(0).Length()/fsquare(hiT);
		float a1 = pBez->ddX(1).Length()/fsquare(hiT);
	
		if ( a0 < maxA && a1 < maxA )
		{
			break;
		}
		
		loT = hiT;
	}
	
	// somewhere between loT and hiT is good.
	// binary search

	float curT = loT*1.5f; //faverage(loT,loT*2);
	float Tstep = loT*0.5f;
	
	float curErr = ErrorBezierWithMaxAccel(x0,x1,v0,v1,curT,maxA);
		
	for(int i=0;i<16;i++)
	{
		float upT = curT + Tstep;
		float dnT = curT - Tstep;
	
		float upErr = ErrorBezierWithMaxAccel(x0,x1,v0,v1,upT,maxA);
		float dnErr = ErrorBezierWithMaxAccel(x0,x1,v0,v1,dnT,maxA);

		if ( upErr < curErr && upErr < dnErr )
		{
			curT = upT;
		}		
		else if ( dnErr < curErr )
		{
			curT = dnT;
		}
		
		Tstep *= 0.5f;
	}
	
	pBez->Fit(x0,x1,v0,v1,curT);
	return curT;
}

struct QuarticRoots // cb::vector_s<double,4>
{
	double data[4];
	int	count;
	
	QuarticRoots() : count(0) { }
	
	void clear() { count = 0; }
	void push_back(double d) { ASSERT( count < 4 ); data[count++] = d; }
	int size() const { return count; }
	double operator [] (const int i) { return data[i]; }
};

inline void SolveQuarticReal(QuarticRoots * roots,double A,double C,double D,double E)
{
	// ax^4 + cx^2 + dx + e = 0

	ComplexDouble x0,x1,x2,x3;
	SolveDepressedQuartic(A,C,D,E,&x0,&x1,&x2,&x3);	
	
	// one or more of these solutions may be bogus if they should've been imaginary
	//	so, check them to make sure they are real solutions :
	
	roots->clear();
	
	if ( x0.IsReal() ) roots->push_back(x0.re);
	if ( x1.IsReal() ) roots->push_back(x1.re);
	if ( x2.IsReal() ) roots->push_back(x2.re);
	if ( x3.IsReal() ) roots->push_back(x3.re);
	
	return;
}

inline double SolveQuarticNonNegative(double A,double C,double D,double E, double C2,double D2)
{
	// ax^4 + cx^2 + dx + e = 0
	
	QuarticRoots vals;
	SolveQuarticReal(&vals,A,C,D,E);
		
	// return the lowest non-negative *valid* solution :
	
	double ret = FLT_MAX;
	
	for(int i=0;i<vals.size();i++)
	{		
		double t= vals[i];
	
		if ( t <= 0.0 ) // zero or negative, no good
			continue;
		
		if ( t > ret ) // later than current best, no need to validate
			continue;
		
		// we solved with C,D , check vs C2,D2
		//	the one we solved should be the larger acceleration
		// max accel should be at A0 (it's actually A0/t^4)
		double A0 = -(C  * t*t + D *t + E);
		double A2 = -(C2 * t*t + D2*t + E);
		ASSERT( A0 >= 0.0 && A2 >= 0.0 );
		
		if ( A2 > A0 + EPSILON )
			continue; // invalid
		
		ret = t;
	}
	
	// there should be one !!
	//ASSERT( ret != FLT_MAX );
	
	return ret;
}

inline float MakeBezierWithMaxAccel_Quartic(Bezier * pBez,const Vec3 &x0, const Vec3 &x1, const Vec3 &v0, const Vec3 &v1, const float minT, const float maxA)
{
	// first see if minT gives us a low enough acceleration :

	pBez->Fit(x0,x1,v0,v1,minT);
	
	float a0 = pBez->ddX(0).Length()/fsquare(minT);
	float a1 = pBez->ddX(1).Length()/fsquare(minT);

	if ( a0 <= maxA && a1 <= maxA )
	{
		return minT;
	}

	/*
	
	CB note : BTW in 1d you can just solve a quadratic which is way easier
	the quartic comes up because we have to square the quadratic to turn the vectors into lengths
	
	maxA occurs either at t = 0 or t = 1
	
	solve both cases and take the shorter ?
	
	*/
	
	Vec3 xd = x1 - x0;
	
	double A = maxA*maxA;
	double D = (-36.0) * xd.LengthSqr();
	
	Vec3 vvv0 = v0 + v0 + v1;
	double B0 = (-4.0)* vvv0.LengthSqr();
	double C0 = (24.0) * ( xd * vvv0 );

	Vec3 vvv1 = v1 + v1 + v0;
	double B1 = (-4.0)* vvv1.LengthSqr();
	double C1 = (24.0) * ( xd * vvv1 );
	
	// solve for constraint at t=0 and t=1, and take the shorter time

	double t0 = SolveQuarticNonNegative(A,B0,C0,D, B1,C1);
	double t1 = SolveQuarticNonNegative(A,B1,C1,D, B0,C0);
	
	float t = (float) MIN(t0,t1);
	
	// should have a solution :
	ASSERT( t != FLT_MAX );
	
	if ( t <= minT )	
	{
		// will just step to end
		return minT;
	}
	
	pBez->Fit(x0,x1,v0,v1,t);
	
	// just to check :
	//*
	  a0 = pBez->ddX(0).Length()/fsquare(t);
	  a1 = pBez->ddX(1).Length()/fsquare(t);
	  // as t gets very tiny the accuracy gets quite bad
	  ASSERT( a0 <= maxA + 1.f );
	  ASSERT( a1 <= maxA + 1.f );
	/**/
	
	return t;
}

//===========================================================================

// cubic curve with max accel
//	max_accel should be somewhere around (typical_distance/coverge_time^2)
void DriveCubic(Vec3 * pos,Vec3 * vel,const Vec3 & toPos,const Vec3 & toVel,
		float max_accel,float time_step)
{
	// solve cubic
	Bezier curve;
	
	// these both work :			
	// float T = MakeBezierWithMaxAccel_BinSearch(&curve,
	float T = MakeBezierWithMaxAccel_Quartic(&curve,
		*pos,
		toPos,
		*vel,
		toVel,
		time_step,
		max_accel);
	
	// step along by dt :
	float t = time_step / T;
			
	if ( t >= 1.f - EPSILON )
	{
		*pos = toPos;
		*vel = toVel;
	}
	else
	{				
		*pos = curve.X(t);
		*vel = curve.dX(t) * (1.f / T);
	}
}

//===========================================================================
			
struct Bezier1d
{
	double B0,B1,B2,B3;
	
	void Fit(const double x0, const double x1, const double v0, const double v1, const double T)
	{
		// bezier controls :
		B0 = x0;
		B3 = x1;
		//d0 = 3*(B1-B0);
		B1 = x0 + v0*(T/3.0);
		B2 = x1 - v1*(T/3.0);
	}
	
	const double X(const double t) const
	{
		const double s = 1.0 - t;
		return B0* (s*s*s) + B1*(s*s*t*3.0) + B2*(t*t*s*3.0) + B3*(t*t*t);
	}
	
	const double dX(const double t) const
	{
		const double s = 1.0 - t;
		return (B1-B0) * (3.0*s*s) + (B2-B1) * (6.0*s*t) + (B3 - B2) * (3.0*t*t);
	}
	
	const double ddX(const double t) const
	{
		const double A0 = 6.0*B0 + 6.0*B2 - 12.0*B1;
		const double A1 = 3.0*B1 - 3.0*B2 - B0 + B3;
		
		return A0 + (6.0*t) * A1;
	}
	
};

//inline double fsquare(const double f) { return f*f; }

// notes about issues :
// http://www.nezumi.demon.co.uk/consult/quadrati.htm

// A t^2 + B t + C = 0;
// returns number of solutions
inline int SolveQuadraticReal(const double A,const double B,const double C, double * pT0,double * pT1)
{
	// first invalidate :
	*pT0 = *pT1 = FLT_MAX;
		
	if ( A == 0.0 )
	{
		if ( B == 0.0 )
		{
			if ( C == 0.0 )
			{
				// degenerate - any value of t is a solution
				*pT0 = *pT1 = 0.0;
				return -1;
			}
			else
			{		
				// no solution
				return 0;
			}
		}
		
		double t = - C / B;
		*pT0 = *pT1 = t;
		return 1;
	}
	else if ( B == 0.0 )
	{
		// B is 0 but A isn't
		double discriminant = -C / A;
		if ( discriminant < 0.0 )
		{
			return 0;
		}
		else if ( discriminant == 0.0 )
		{
			*pT0 = *pT1 = 0.0;
			return 1;
		}
		double t = sqrt(discriminant);
		*pT0 = t;
		*pT1 = - t;
		return 2;
	}
	else if ( C == 0.0 )
	{
		// A and B are not zero
		// t = 0 is one solution
		*pT0 = 0.0;
		// A t + B = 0;
		*pT1 = -B / A;
		return 2;
	}

	// Numerical Recipes 5.6 : 

	double discriminant = ( B*B - 4.0 * A * C );
	if ( discriminant < 0.0 )
	{
		return 0;
	}
	else if ( discriminant == 0.0 )
	{
		double t = - 0.5 * B / A;
		*pT0 = *pT1 = t;
		return 1;
	}
	
	double Q = - 0.5 * ( B + fsign(B) * sqrt( discriminant ) );
	
	// Q cannot be zero
	
	*pT0 = Q / A;
	*pT1 = C / Q;
	
	// @@ - may also want to refine roots by Newton-Raphson
	
	return 2;
}

inline double SolveQuadratic_LowestPositive(const double A,const double B,const double C, const double B2,const double C2)
{
	double r1,r2;
	if ( SolveQuadraticReal(A,B,C,&r1,&r2) == 0 )
	{
		// no solutions
		return FLT_MAX;
	}
	
	// don't accept negative durations :
	if ( r1 <= 0.0 ) r1 = FLT_MAX;
	if ( r2 <= 0.0 ) r2 = FLT_MAX;
		
	// check validity :
	// good solution should've been the max A at B,C ; check vs. alt B2,C2
	// note I'm not checking the acceleration vs maxA here,
	//	what I'm testing is whether the end that I was trying to make the max is the max
	// don't put epsilon in these compares, values can be arbitrarily small here
	if ( fabs( C2 + B2 * r1 ) > fabs( C + B * r1 ) )
	{
		r1 = FLT_MAX;
	}		
	if ( fabs( C2 + B2 * r2 ) > fabs( C + B * r2 ) )
	{
		r2 = FLT_MAX;
	}
	
	// take the lower time :
	// MIN rejects the FLT_MAX bad solutions
	double ret = MIN(r1,r2);

	return ret;
}

inline double SolveBezierMaxAccel1d_Sub(const float x0, const float x1, const float v0, const float v1,const float maxA)
{
	double A = maxA;
	
	// B0 is the quadratic if maxA occurs at t=0
	double B0 = 2.0 * (v1 + v0 + v0);
	double C0 = 6.0 * (x0 - x1);
	
	// B1 is the quadratic if maxA occurs at t=1
	double B1 = - 2.0 * (v0 + v1 + v1);
	double C1 = - C0;

	// try both and take the faster way :
	double t0 = SolveQuadratic_LowestPositive(A,B0,C0 ,B1,C1);
	double t1 = SolveQuadratic_LowestPositive(A,B1,C1 ,B0,C0);
		
	// take the shorter time :
	// MIN rejects the FLT_MAX bad solutions
	return MIN(t0,t1);
}

inline double MakeBezierWithMaxAccel1d(Bezier1d * pBez,const float x0, const float x1, const float v0, const float v1, const float minT, const float maxA)
{
	// first see if minT gives us a low enough acceleration :

	pBez->Fit(x0,x1,v0,v1,minT);
	
	double a0 = pBez->ddX(0)/fsquare(minT);
	double a1 = pBez->ddX(1)/fsquare(minT);

	if ( fabs(a0) <= maxA && fabs(a1) <= maxA )
	{
		return minT;
	}	

	// we can either have an accel of +a or -a
	//	(we're doing the actual acceleration, not the magnitude, to avoid squaring the equation)
	double plusT = SolveBezierMaxAccel1d_Sub(x0,x1,v0,v1, maxA);
	double negT  = SolveBezierMaxAccel1d_Sub(x0,x1,v0,v1,-maxA);

	// take the shorter time :
	// MIN rejects the FLT_MAX bad solutions
	double t = MIN(plusT,negT);
	//float t = (float) MAX(plusT,negT);
	
	ASSERT( t < FLT_MAX ); // we should have a solution
	
	if ( t < minT )
	{
		// doesn't really matter what's in pBez, we're going to jump to the end
		return minT;
	}
	
	pBez->Fit(x0,x1,v0,v1, t);
	
	// to check :
	DURING_ASSERT( a0 = (pBez->ddX(0))/fsquare(t) );
	DURING_ASSERT( a1 = (pBez->ddX(1))/fsquare(t) );
	ASSERT( fabs(a0) <= maxA * 1.01f + EPSILON );
	ASSERT( fabs(a1) <= maxA * 1.01f + EPSILON );
	
	// one accel is always max !
	DURING_ASSERT( double ma = MAX( fabs(a0), fabs(a1) ) );
	ASSERT( fequal((float)ma , (float)maxA) );
	
	return t;
}

inline void SolveQuadratic_All(const double A,const double B,const double C, vector<double> * pVec)
{
	double r1,r2;
	if ( SolveQuadraticReal(A,B,C,&r1,&r2) == 0 )
	{
		// no solutions
		return;
	}
	
	// don't accept negative durations :
	if ( r1 > 0.0 ) pVec->push_back(r1);
	if ( r2 > 0.0 ) pVec->push_back(r2);
}

inline void SolveBezierMaxAccel1d_All(const double x0, const double x1, const double v0, const double v1,const double maxA, 
										vector<double> * pVec)
{
	double A = maxA;
	
	// B0 is the quadratic if maxA occurs at t=0
	double B0 = 2.0 * (v1 + v0 + v0);
	double C0 = 6.0 * (x0 - x1);
	
	// B1 is the quadratic if maxA occurs at t=1
	double B1 = - 2.0 * (v0 + v1 + v1);
	double C1 = - C0;

	SolveQuadratic_All(A,B0,C0,pVec);
	SolveQuadratic_All(A,B1,C1,pVec);
		
	return;
}

inline bool Bez1d_IsValidSolution(const double x0, const double x1, const double v0, const double v1, const double T,const double maxA)
{
	Bezier1d bez;
	bez.Fit(x0,x1,v0,v1,T);
	
	double a0 = bez.ddX(0)/fsquare(T);
	double a1 = bez.ddX(1)/fsquare(T);

	if ( fabs(a0) <= maxA+EPSILON && fabs(a1) <= maxA+EPSILON )
	{
		return true;
	}
	
	return false;
}

inline double MakeBezierWithMaxAccel1d_All(Bezier1d * pBez,const double x0, const double x1, const double v0, const double v1, const double minT, const double maxA)
{
	// first see if minT gives us a low enough acceleration :

	pBez->Fit(x0,x1,v0,v1,minT);
	
	double a0 = pBez->ddX(0)/fsquare(minT);
	double a1 = pBez->ddX(1)/fsquare(minT);

	if ( fabs(a0) <= maxA && fabs(a1) <= maxA )
	{
		return minT;
	}	

	// we can either have an accel of +a or -a
	//	(we're doing the actual acceleration, not the magnitude, to avoid squaring the equation)
	vector<double> solutions;
	SolveBezierMaxAccel1d_All(x0,x1,v0,v1, maxA,&solutions);
	SolveBezierMaxAccel1d_All(x0,x1,v0,v1,-maxA,&solutions);

	ASSERT( ! solutions.empty() );

	std::sort(solutions.begin(),solutions.end());
	vector<double>::iterator it = std::unique(solutions.begin(),solutions.end());
	solutions.erase(it,solutions.end());
	
	DURING_ASSERT( double maxT = solutions.back() );
	ASSERT( Bez1d_IsValidSolution(x0,x1,v0,v1,maxT,maxA) );

	int numSolutions = solutions.size32();
	numSolutions;
	
	// remove invalid :
	for(int i=0;i<solutions.size32();i++)
	{
		double T = solutions[i];
		if ( ! Bez1d_IsValidSolution(x0,x1,v0,v1,T,maxA) )
		{
			solutions.erase(solutions.begin() + i);
			i--;
		}
	}
	
	int numValidSolutions = solutions.size32();
	
	if ( numValidSolutions == 0 )
		return FLT_MAX;
	
	/*
	if ( numValidSolutions > 1 )
	{
		lprintf("x0: %f, x1 : %f, v0 : %f, v1 : %f\n",x0,x1,v0,v1);
	
		lprintf("  ns : %d , nvs : %d \n",numSolutions,numValidSolutions);
		
		for(int sol=0;sol<numValidSolutions;sol++)
		{
			double t = solutions[sol];
			
			Bezier1d bez0;
			bez0.Fit(x0,x1,v0,v1,t);
			double a0 = bez0.ddX(0)/fsquare(t);
			double a1 = bez0.ddX(1)/fsquare(t);
		
			lprintf("  %d : %f : %f,%f \n",sol,t,a0,a1);
		}
	}
	*/
	
	// take lowest time :
	double T = solutions[0];
	
	pBez->Fit(x0,x1,v0,v1,T);
	
	return T;
}

// cubic curve with max accel
//	max_accel should be somewhere around (typical_distance/coverge_time^2)
void DriveCubic(float * pos,float * vel,const float toPos,const float toVel,
		float max_accel,float time_step)
{
	// solve cubic
	Bezier1d curve;

	//*
	double T = MakeBezierWithMaxAccel1d(&curve,
		*pos,
		toPos,
		*vel,
		toVel,
		time_step,
		max_accel);
	
	/*/
	
	double T = MakeBezierWithMaxAccel1d_All(&curve,
		*pos,
		toPos,
		*vel,
		toVel,
		time_step,
		max_accel);
	
	/**/
	
	// step along by dt :
	double t = time_step / T;
			
	if ( t >= 1.f - EPSILON ) // this importantly also catches time_step > T
	{
		*pos = toPos;
		*vel = toVel;
	}
	else
	{				
		*pos = (float) curve.X(t);
		*vel = (float) ( curve.dX(t) / T );
	}
}

//-------------------------------------------------------------------------

/*
	PD damping should generally be between 1.0 and 1.1
	frequency should be like 1.0/coverge_time
*/
void DrivePDClamped(Vec3 * pos,Vec3 * vel,const Vec3 & toPos,const Vec3 & toVel,
			const float converge_time_scale,
			const float damping,
			const float minVelocity,
			float time_step)
{
	ApplyPDClamping(pos,vel,toPos,toVel,1.f/converge_time_scale,damping,minVelocity,time_step);
}					

void DrivePDClamped(float * pos,float * vel,const float toPos,const float toVel,
			const float converge_time_scale,
			const float damping,
			const float minVelocity,
			float time_step)
{
	ApplyPDClamping(pos,vel,toPos,toVel,1.f/converge_time_scale,damping,minVelocity,time_step);
}					

#if 0
//-------------------------------------------------------------------------
// "Hybrid" cubic is like max accel, but is gauranteed to reach a stationary target in max_time
//	C1 smooth

struct HybridCubicState
{
	Vec3	toPos,toVel;
	double	time_remaining;
	
	HybridCubicState() : time_remaining(-1.f) { }

	void SetTarget(const Vec3 & _toPos,const Vec3 & _toVel)
	{
		if ( toPos == _toPos && toVel == _toVel )
		{
		}
		else
		{
			toPos = _toPos;
			toVel = _toVel;
			time_remaining = -1.f;
		}
	}	
};

// cubic curve with max accel
//	max_accel should be somewhere around (typical_distance/coverge_time^2)
void DriveCubicHybrid(Vec3 * pos,Vec3 * vel,HybridCubicState * pState,
		float max_accel,float max_time,float time_step)
{
	// hybrid cubic :
	// max time is set from max_time
	// min time is set by maxaccel

	if ( pState->time_remaining < 0.f )
	{
		// solve cubic
		Bezier curve;
		
		// these both work :			
		// float T = MakeBezierWithMaxAccel_BinSearch(&curve,
		float maxaccel_time = MakeBezierWithMaxAccel_Quartic(&curve,
			*pos,
			pState->toPos,
			*vel,
			pState->toVel,
			time_step,
			max_accel);
		
		pState->time_remaining = fclamp(maxaccel_time,0,max_time);
	}
	else
	{
		pState->time_remaining -= time_step;
	}
	
	if ( pState->time_remaining <= time_step )
	{
		*pos = pState->toPos;
		*vel = pState->toVel;
	}
	else
	{
		// solve cubic
		Bezier curve;
		
		const float T = (float) pState->time_remaining;
		
		curve.Fit(*pos,pState->toPos,
			*vel,pState->toVel,
			T);
		
		// step along by dt :
		float t = time_step / T;
		
		*pos = curve.X(t);
		*vel = curve.dX(t) * (1.f / T);
	}
}
#endif

//=================================================================================

/**

To do an "intercept" we simply transform into the frame of the target such that the target is stationary in
	that frame

We do the normal "drive" in that frame

Then transition back to the original frame by applying the target velocity

---

intercept_confidence should be in [0,1]
something around 0.5 is nice

it roughly blends off how strongly we assume that the target will continue at toVel exactly

@@ NOTEZ :

really what we'd like is to have intercept_confidence of 1.0 at t=0 and blend it away as
	time goes forward, but that makes all the solves even more complex

if you just set intercept_confidence = 1 , it can lead to some ugly behavior because
the chaser will make no effort to go at the current position


***/

void DriveCubicIntercept(Vec3 * pos,Vec3 * vel,const Vec3 & toPos,const Vec3 & toVel,
		float max_accel,float time_step,
		float intercept_confidence)
{
	Vec3 intercept_vel = toVel * intercept_confidence;
	*vel -= intercept_vel;
	DriveCubic(pos,vel,toPos,(toVel - intercept_vel),max_accel,time_step);

	*vel += intercept_vel;	
	*pos += time_step * intercept_vel;
}
		

void DriveCubicIntercept(float * pos,float * vel,const float toPos,const float toVel,
		float max_accel,float time_step,
		float intercept_confidence)
{
	float intercept_vel = toVel * intercept_confidence;
	*vel -= intercept_vel;
	DriveCubic(pos,vel,toPos,(toVel - intercept_vel),max_accel,time_step);

	*vel += intercept_vel;	
	*pos += time_step * intercept_vel;
}
		

void DrivePDClampedIntercept(Vec3 * pos,Vec3 * vel,const Vec3 & toPos,const Vec3 & toVel,
			const float converge_time_scale,
			const float damping,
			const float minVelocity,
			float time_step,
			float intercept_confidence)
{
	Vec3 intercept_vel = toVel * intercept_confidence;
	*vel -= intercept_vel;
	DrivePDClamped(pos,vel,toPos,(toVel - intercept_vel),converge_time_scale,damping,minVelocity,time_step);

	*vel += intercept_vel;	
	*pos += time_step * intercept_vel;
}

void DrivePDClampedIntercept(float * pos,float * vel,const float toPos,const float toVel,
			const float converge_time_scale,
			const float damping,
			const float minVelocity,
			float time_step,
			float intercept_confidence)
{
	float intercept_vel = toVel * intercept_confidence;
	*vel -= intercept_vel;
	DrivePDClamped(pos,vel,toPos,(toVel - intercept_vel),converge_time_scale,damping,minVelocity,time_step);

	*vel += intercept_vel;	
	*pos += time_step * intercept_vel;
}

//=================================================================================

END_CB

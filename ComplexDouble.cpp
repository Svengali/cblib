#include "ComplexDouble.h"

START_CB

const ComplexDouble ComplexDouble::zero(0,0);
const ComplexDouble ComplexDouble::one(1,0);
const ComplexDouble ComplexDouble::I(0,1);
	

const ComplexDouble ComplexSqrt(const ComplexDouble & v)
{
	return ComplexPow(v,0.5);
}

const ComplexDouble ComplexSqrt(const double v)
{
	if ( v >= 0 )
	{
		return ComplexDouble( sqrt(v) , 0 );
	}
	else
	{
		return ComplexDouble( 0 , sqrt(-v) );
	}
}

const ComplexDouble ComplexPow(const ComplexDouble & v, const double p)
{
	// @@ this is pretty super non-robust and inefficient
	//	would be nice to avoid going through polar form
	//	as it creates a lot of drift even when p = 1

	double m = v.GetMagnitude();
	double a = v.GetAngle();
	
	double mp = pow(m,p);
	double ma = a*p;
	
	return ComplexDouble::MakePolar(mp,ma);
}

const ComplexDouble ComplexPow(const double v, const double p)
{
	double m,a;
	
	if ( v >= 0 )
	{
		m = v;
		a = 0; // or 2PI , or 4PI , etc.
	}
	else
	{
		m = -v;
		a = PI;
	}
	
	double mp = pow(m,p);
	double ma = a*p;
	
	return ComplexDouble::MakePolar(mp,ma);	
}

/*
void SolveQuadratic(const double a,const double b,const double c,
					ComplexDouble * root1,ComplexDouble * root2)
{
	// ( -b +/- sqrt( b*b - 4*a*c )) / 2a
	
	// @@ CB : this is extremely non-robust
	
	double discriminant = b*b - 4.0*a*c;
	ComplexDouble sqrtpart = ComplexSqrt( discriminant );
	sqrtpart /= (2.0*a);
	double realpart = -b / (2.0*a);
	 
	*root1 = realpart + sqrtpart;
	*root2 = realpart - sqrtpart;
}
*/

// A t^2 + B t + C = 0;
// returns number of solutions
int SolveQuadratic(const double A,const double B,const double C,
					ComplexDouble * pT0,ComplexDouble * pT1)
{
	// first invalidate :
	*pT0 = FLT_MAX;
	*pT1 = FLT_MAX;
		
	if ( A == 0.0 )
	{
		if ( B == 0.0 )
		{
			if ( C == 0.0 )
			{
				// degenerate - any value of t is a solution
				*pT0 = 0.0;
				*pT1 = 0.0;
				return -1;
			}
			else
			{		
				// no solution
				return 0;
			}
		}
		
		double t = - C / B;
		*pT0 = t;
		*pT1 = t;
		return 1;
	}
	else if ( B == 0.0 )
	{
		if ( C == 0.0 )
		{
			// A t^2 = 0;
			*pT0 = 0.0;
			*pT1 = 0.0;
			return 1;
		}
		
		// B is 0 but A isn't
		double discriminant = -C / A;
		ComplexDouble t = ComplexSqrt(discriminant);
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
	
	if ( discriminant == 0.0 )
	{
		double t = - 0.5 * B / A;
		*pT0 = t;
		*pT1 = t;
		return 1;
	}
	
	ComplexDouble sqrtpart = ComplexSqrt( discriminant );
	
	sqrtpart *= - 0.5 * fsign(B);
	
	ComplexDouble Q = sqrtpart + (- 0.5 * B);
	
	// Q cannot be zero
	
	*pT0 = Q / A;
	*pT1 = C / Q;
	
	// @@ - may also want to refine roots by Newton-Raphson
	
	return 2;
}

void SolveCubic(double a,double b,double c,double d,
					ComplexDouble * px0,
					ComplexDouble * px1,
					ComplexDouble * px2)
{
	ASSERT( a != 0.0 );
	
	double q = (3.0*a*c - b*b)/(9.0*a*a);
	double r = (9.0*a*b*c - 27.0*a*a*d - 2.0*b*b*b)/(54.0*a*a*a);
	
	ComplexDouble u = ComplexSqrt( q*q*q + r*r );
	
	ComplexDouble s = ComplexPow( r + u, 1.0/3.0 ); 
	ComplexDouble t = ComplexPow( r - u, 1.0/3.0 );
	
	*px0 = s + t - b/(3.0*a);

	ComplexDouble v1 = -0.5*(s + t) - b/(3.0*a);
	ComplexDouble v2 = 0.5*sqrt(3.0)*(s - t)*ComplexDouble::I;
	
	*px1 = v1 + v2;

	*px2 = v1 - v2;
}

void SolveDepressedQuartic(double A,double C,double D,double E,
					ComplexDouble * px0,
					ComplexDouble * px1,
					ComplexDouble * px2,
					ComplexDouble * px3)
{
	// try to solve the quartic analytically and get all the roots
	
	ASSERT( A != 0.0 );
	
	// if E == 0 there is one root at 0, and then we solve a depressed cubic
	
	if ( E == 0.0 )
	{
		*px3 = 0.0;
		
		SolveCubic(A,0.0,C,D,px0,px1,px2);
		return;
	}
	
	if ( D == 0.0 )
	{
		// just solve a quadratic for t^2
		
		ComplexDouble r1,r2;
		SolveQuadratic(A,C,E,&r1,&r2);
		
		*px0 = ComplexSqrt( r1 );
		*px1 = - *px0;
		*px2 = ComplexSqrt( r2 );
		*px3 = - *px2;

		return;
	}

	// if C == 0 ??	.. the general case math below works okay

	double alpha = C/A;
	double beta = D/A;
	double gamma = E/A;
	
	double P = - alpha*alpha/12.0 - gamma;
	double Q = - (alpha*alpha*alpha)/108.0 + (alpha*gamma)/3.0 - (beta*beta)/8.0;
	double forR = Q*Q/4.0 + P*P*P/27.0;
	ComplexDouble R = (Q/2.0) + ComplexSqrt( forR );
	
	ComplexDouble U = ComplexPow(R,1.0/3.0);
	
	ComplexDouble y( - (5.0/6.0)*alpha );
	if ( U != 0.0 )
		y += -U + P/(3.0*U);
	
	ComplexDouble W = ComplexSqrt( alpha + 2.0*y );
	
	ASSERT( W != 0.0 ); // possible ?
	
	ComplexDouble p1 = 3.0*alpha + 2.0*y;
	ComplexDouble p2 = 2.0*beta/W;
	
	ComplexDouble sqrtpos = ComplexSqrt( - (p1 + p2) );
	ComplexDouble sqrtneg = ComplexSqrt( - (p1 - p2) );
	
	*px0 = 0.5 * ( + W + sqrtpos );
	*px1 = 0.5 * ( - W + sqrtneg );
	*px2 = 0.5 * ( + W - sqrtpos );
	*px3 = 0.5 * ( - W - sqrtneg );
}

//---------------------------------


static void test()
{
	ComplexDouble a;
	a = ComplexDouble::MakePolar(2.0,PI);

	a += 3.f;
	
	a *= ComplexDouble::I;

	ComplexDouble b = a / ComplexDouble(2,2);

}

END_CB

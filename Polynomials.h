#pragma once

#include "Base.h"
#include <Log.h>

START_CB

template < int n >
struct Legendre
{
	double operator () ( double x )
	{
		double L_n1 = Legendre<n-1>()(x);
		double L_n2 = Legendre<n-2>()(x);
		const double B = (n-1)/(double)n; // n-1/n
		return (1+B)*x*L_n1 - B*L_n2;
	}
};

template< > struct Legendre<0>
{
	double operator () ( double x ) { return 1.0; }
};

template< > struct Legendre<1>
{
	double operator () ( double x ) { return x; }
};

template < int n >
struct ShiftedLegendre
{
	double operator () ( double x )
	{
		return Legendre<n>() ( 2*x - 1 );
	}
};

template < int n >
struct NormalizedLegendre
{
	double operator () ( double x )
	{
		const double N = sqrt( n + 0.5 );
		return Legendre<n>() ( x ) * N;
	}
};

template < int n >
struct NormalizedShiftedLegendre
{
	double operator () ( double x )
	{
		return NormalizedLegendre<n>() ( 2*x - 1 ) * sqrt(2.0);
	}
};

__forceinline double legendre_ryg(int n, double x)
{
  if (n == 0) return 1.0;
  if (n == 1) return x;

  double t = x, t1 = 1, t2;
  // if you don't use a for here, the vc++ loop optimizer doesn't get it!
  for (int i=1; i<n; i++) {
    t2 = t1;
    t1 = t;
	const double B = (i)/(double)(i+1);
	t = (1+B)*x*t1 - B*t2;
  }
  return t;
}

__forceinline double shiftedlegendre_ryg(int n, double x)
{
	return legendre_ryg(n,2*x-1);
}

__forceinline double chebychev_ryg(int n, double x)
{
  if (n == 0) return 1.0;
  if (n == 1) return x;

  double t = x, t1 = 1, t2;
  // if you don't use a for here, the vc++ loop optimizer doesn't get it!
  for (int i=1; i<n; i++) {
    t2 = t1;
    t1 = t;
    t = 2.0 * x * t1 - t2;
  }
  return t;
}

template < int n >
struct Tchebychev
{
	double operator () ( double x )
	{
		return 2.0 * x * Tchebychev<n-1>()(x) - Tchebychev<n-2>()(x);
	}
};

template< > struct Tchebychev<0>
{
	double operator () ( double x ) { return 1.0; }
};

template< > struct Tchebychev<1>
{
	double operator () ( double x ) { return x; }
};

template < int n >
struct ShiftedTchebychev
{
	double operator () ( double x ) // x in [0,1]
	{
		return Tchebychev<n>()( 2 * x - 1 );
	}
};

template < int n >
struct TchebychevMetric
{
	double operator () ( double x ) // x in [0,1]
	{
		if ( x <= -1.0 || x >= 1.0 ) return 0.0;
		//double ret = Tchebychev<n>()( x ) / ( PI * sqrt( 1.0 - x*x ) );
		//if ( n == 0 ) ret *= 0.5;
		double ret = Tchebychev<n>()( x ) / ( sqrt( 1.0 - x*x ) );
		return ret;
	}
};

template < int n >
struct ShiftedTchebychevMetric
{
	double operator () ( double x ) // x in [0,1]
	{
		//return 4.0 * 
		return TchebychevMetric<n>()( 2 * x - 1 );
	}
};

/*
FindLegendreCoefficients : functor in [0,1]
*/
template < typename functor >
void FindLegendreCoefficients(int steps = 65536)
{
	#define LEGENDRE_FUNCTOR	ShiftedLegendre
//	#define LEGENDRE_FUNCTOR	NormalizedShiftedLegendre

	//FunctorProduct< LEGENDRE_FUNCTOR<0> , LEGENDRE_FUNCTOR<0> > l0_l0;
	
    RemapUnitTo< Legendre<0> > ShiftedLegendre0(-1,1);
	double n0 = Integrate(0.0,1.0,steps,MakeFunctorProduct(ShiftedLegendre0,ShiftedLegendre0));
	//FunctorProduct< RemapUnitTo< Legendre<0> > , RemapUnitTo< Legendre<0> > > l0_l0 = MakeFunctorProduct(ShiftedLegendre0,ShiftedLegendre0);
	//double n0 = Integrate(0.0,1.0,steps,l0_l0);

	FunctorProduct< LEGENDRE_FUNCTOR<1> , LEGENDRE_FUNCTOR<1> > l1_l1;
	double n1 = Integrate(0.0,1.0,steps,l1_l1);

	FunctorProduct< LEGENDRE_FUNCTOR<2> , LEGENDRE_FUNCTOR<2> > l2_l2;
	double n2 = Integrate(0.0,1.0,steps,l2_l2);

	lprintfvar(n0);
	lprintfvar(n1);
	lprintfvar(n2);

	//FunctorProduct< functor , LEGENDRE_FUNCTOR<0> > f_l0;
	double c0 = Integrate(0.0,1.0,steps,MakeFunctorProduct(functor(),ShiftedLegendre0));

	FunctorProduct< functor , LEGENDRE_FUNCTOR<1> > f_l1;
	double c1 = Integrate(0.0,1.0,steps,f_l1);

	FunctorProduct< functor , LEGENDRE_FUNCTOR<2> > f_l2;
	double c2 = Integrate(0.0,1.0,steps,f_l2);
	
	double t0 = c0/n0;
	double t1 = c1/n1;
	double t2 = c2/n2;
	
	lprintf("%f\n",t0);
	lprintf("%f\n",t1);
	lprintf("%f\n",t2);
	
	double errSqr = 0;
	double errMax = 0;
	for(int i=0;i<steps;i++)
	{
		double x = i/steps;
		
		double y = functor()(x);
		double t = 
			t0 * ShiftedLegendre<0>()( x ) +
			t1 * ShiftedLegendre<1>()( x ) +
			t2 * ShiftedLegendre<2>()( x );
		errSqr += fsquare(y-t);
		errMax = MAX( errMax , fabs(y-t) );
	}
	errSqr/=steps;
	//lprintfvar(errSqr);
	lprintf("errSqr = %g errMax = %g\n",errSqr,errMax);
	
	{		
		double sum;
		//const int num_steps = 1<<24;
		int num_steps = 1<<24;
		num_steps += irandmod(10);
	
		sum = 0;
		{
			TSCScopeLog tsc("shiftedlegendre_ryg",num_steps);

			double xs = 2.0 / num_steps;
			double x = -1 + xs * 0.5;
			for(int i=0;i<num_steps;i++)
			{
				sum += 
					c0 * shiftedlegendre_ryg(0,x) +
					c1 * shiftedlegendre_ryg(1,x) +
					c2 * shiftedlegendre_ryg(2,x);
				x += xs;
			}		
		}
		lprintfvar(sum);

		sum = 0;			
		{
			TSCScopeLog tsc("ShiftedLegendre",num_steps);

			double xs = 2.0 / num_steps;
			double x = -1 + xs * 0.5;
			for(int i=0;i<num_steps;i++)
			{
				sum += 
					c0 * ShiftedLegendre<0>()(x) +
					c1 * ShiftedLegendre<1>()(x) +
					c2 * ShiftedLegendre<2>()(x);
				x += xs;
			}
		
		}
		lprintfvar(sum);
	
		
	}
}

static inline double ShiftedTchebychevN(int n,double x)
{
	switch(n) {
	case 0 : return ShiftedTchebychev<0>()(x); 
	case 1 : return ShiftedTchebychev<1>()(x); 
	case 2 : return ShiftedTchebychev<2>()(x); 
	case 3 : return ShiftedTchebychev<3>()(x); 
	case 4 : return ShiftedTchebychev<4>()(x); 
	case 5 : return ShiftedTchebychev<5>()(x); 
	case 6 : return ShiftedTchebychev<6>()(x); 
	case 7 : return ShiftedTchebychev<7>()(x); 
	case 8 : return ShiftedTchebychev<8>()(x); 
	case 9 : return ShiftedTchebychev<9>()(x); 
	NO_DEFAULT_CASE
	}
	return 0;
}

static inline double TchebychevN(int n,double x)
{
	switch(n) {
	case 0 : return Tchebychev<0>()(x); 
	case 1 : return Tchebychev<1>()(x); 
	case 2 : return Tchebychev<2>()(x); 
	case 3 : return Tchebychev<3>()(x); 
	case 4 : return Tchebychev<4>()(x); 
	case 5 : return Tchebychev<5>()(x); 
	case 6 : return Tchebychev<6>()(x); 
	case 7 : return Tchebychev<7>()(x); 
	case 8 : return Tchebychev<8>()(x); 
	case 9 : return Tchebychev<9>()(x); 
	NO_DEFAULT_CASE
	}
	return 0;
}

/*
template <typename functor>
double CallIntFunctor(int n,double x)
{
	switch(n) {
	case 0 : return functor<0>()(x); 
	case 1 : return functor<1>()(x); 
	case 2 : return functor<2>()(x); 
	case 3 : return functor<3>()(x); 
	case 4 : return functor<4>()(x); 
	case 5 : return functor<5>()(x); 
	case 6 : return functor<6>()(x); 
	case 7 : return functor<7>()(x); 
	case 8 : return functor<8>()(x); 
	case 9 : return functor<9>()(x); 
	NO_DEFAULT_CASE
	}
	return 0;
}
*/

/*
FindTchebychevCoefficients : functor in [0,1]
*/
template < typename functor >
void FindTchebychevCoefficients(functor f,double lo = 0.0, double hi = 1.0, int N = (1<<20))
{
	const double PI_over_N = PI/N;

	#define NUM_T	6

	double t[NUM_T] = { 0 };
	
	for(int k=0;k<N;k++)
	{
		double pk = PI_over_N * (k + 0.5);
		double x_k = cos(pk);
		double farg = lo + (hi - lo) * 0.5 * (x_k+1.0);
		double fval = f( farg );
		for(int j=0;j<NUM_T;j++)
		{
			t[j] += fval * cos( pk * j );
		}
	}
	for(int j=0;j<NUM_T;j++)
	{
		t[j] *= 1.0 / N;
		if ( j != 0 )
			t[j] *= 2.0;

		//lprintfvar(t[j]);
		lprintf("t %d: %16.9f , %16.8g\n",j,t[j],t[j]);
	}
	
	double errSqr[NUM_T] = { 0 };
	double errMax[NUM_T] = { 0 };
	for(int i=0;i<N;i++)
	{
		double xunit = (i+0.5)/N;
		double farg = lo + (hi - lo) * xunit;
		double xt = xunit*2.0 - 1.0;
		double y = f(farg);
		double p = 0.0;
		for(int j=0;j<NUM_T;j++)
		{
			//p += t[j] * CallIntFunctor<Tchebychev>(j,xt);
			p += t[j] * TchebychevN(j,xt);
			errSqr[j] += fsquare(y-p);
			errMax[j] = MAX( errMax[j] , fabs(y-p) );
		}
	}
	for(int j=0;j<NUM_T;j++)
	{
		lprintf("%d : errSqr = %g errMax = %g\n",j,errSqr[j]/N,errMax[j]);
	}
	
	{
		double c0 = t[0];
		double c1 = t[1];
		double c2 = t[2];
		
		double sum;
		const int num_steps = 1<<24;
	
		sum = 0;
		{
			TSCScopeLog tsc("chebychev_ryg",num_steps);

			double xs = 2.0 / num_steps;
			double x = -1 + xs * 0.5;
			for(int i=0;i<num_steps;i++)
			{
				sum += 
					c0 * chebychev_ryg(0,x) +
					c1 * chebychev_ryg(1,x) +
					c2 * chebychev_ryg(2,x);
				x += xs;
			}		
		}
		lprintfvar(sum);

		sum = 0;			
		{
			TSCScopeLog tsc("Tchebychev",num_steps);

			double xs = 2.0 / num_steps;
			double x = -1 + xs * 0.5;
			for(int i=0;i<num_steps;i++)
			{
				sum += 
					c0 * Tchebychev<0>()(x) +
					c1 * Tchebychev<1>()(x) +
					c2 * Tchebychev<2>()(x);
				x += xs;
			}
		
		}
		lprintfvar(sum);
	
		
	}
}

END_CB

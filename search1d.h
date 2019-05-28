#pragma once

#include <cblib/Base.h>

START_CB

//===================================================================
// root finders :

typedef double (tfunc_doubledouble)( double x);

double FindRoot_BrentsMethod(tfunc_doubledouble * function, double lo, double hi, double tolerance);

// note you can also use GoldenSearch on func^2 to find the root of func
//	(Search1d_FixedStepAndDownEach is the safest)
double FindRoot_GoldenMinSqr(tfunc_doubledouble * function, double lo, double hi, int fixedSteps, double tolerance);

//===================================================================
// minimum finders :

/**

Note : these searches find the spot where the function is *minimum*
That is, function should be a cost or error you want to minimize

Functor should take one parameter , double -> double , eg:

struct CostFunctor
{
	double operator() ( double param ) const
	{
		return cost;
	}
};

**/

/*
QuadraticExtremum gives you the extremum (either minimum or maximum)
of the quadratic that passes through the three points x0y0,x1y1,x2y2
*/
inline double QuadraticExtremum(
	double x0,double y0,
	double x1,double y1,
	double x2,double y2)
{	
	// warning : this form is pretty but it's numerically very bad
	//	when the three points are near each other
	
	double s2 = x2*x2;
	double s1 = x1*x1;
	double s0 = x0*x0;
	
	double numer = y0*(s1-s2) + y1*(s2-s0) + y2*(s0-s1);
	double denom = y0*(x1-x2) + y1*(x2-x0) + y2*(x0-x1);
	
	double ret = 0.5 * numer / denom;
	
	return ret;
}
	
template< typename t_functor >	
double BinarySearch1d( t_functor & func, double start, double step, double minstep )
{
	double cur = start;
	double curV = func( cur );
	
	while( step > minstep )
	{
		double up = cur + step;
		double dn = cur - step;
		
		double upV = func( up );
		double dnV = func( dn );
		
		if ( upV < curV )
		{
			cur = up;
			curV = upV;
		}
		if ( dnV < curV )
		{
			cur = dn;
			curV = dnV;
		}
		
		step *= 0.5;
	}
	
	return cur;
}

template< typename t_functor >	
double GoldenSearch1d( t_functor & func, double lo, double v_lo, double m1, double v_m1, double hi, double v_hi, double minstep )
{
	ASSERT( minstep > 0 );
	
	const double rho = 0.381966;
	const double irho = 1.0 - rho; // = (sqrt(5)-1)/2 

	// four points :
	// [lo,m1,m2,hi]

	DURING_ASSERT( double check_m1 = irho*lo + rho*hi );
	ASSERT( fequal(m1,check_m1) );
	double m2 = irho*hi + rho*lo;

	double v_m2 = func( m2 );
	
	while( (m1-lo) > minstep )
	{
		// step to [lo,m1,m2] or [m1,m2,hi]
		// only one func eval per iteration :
		if ( MIN(v_lo,v_m1) < MIN(v_hi,v_m2) )
		{
			hi = m2; v_hi = v_m2;
			m2 = m1; v_m2 = v_m1;
			m1 = irho*lo + rho*hi;
			v_m1 = func( m1 );
		}
		else
		{
			lo = m1; v_lo = v_m1;
			m1 = m2; v_m1 = v_m2;
			m2 = irho*hi + rho*lo;
			v_m2 = func( m2 );
		}
		
		ASSERT( fequal(m2, irho*hi + rho*lo) );
		ASSERT( fequal(m1, irho*lo + rho*hi) );
	}
	
	// could pick the better triple and use QuadraticExtremum here
	
	/*
	//return (lo+hi)/2.0;
	
	if ( v_m1 < v_m2 ) return m1;
	else return m2;
	
	/*/
	// return best of the 4 samples :
	if ( v_lo < v_m1 ) { v_m1 = v_lo; m1 = lo; }
	if ( v_hi < v_m2 ) { v_m2 = v_hi; m2 = hi; }
	
	if ( v_m1 < v_m2 ) return m1;
	else return m2;
	/**/
}

template< typename t_functor >	
double GoldenSearch1d( t_functor & func, double lo, double v_lo, double hi, double v_hi, double minstep )
{
	const double rho = 0.381966;
	const double irho = 1.0 - rho; // = (sqrt(5)-1)/2 

	double m1 = irho*lo + rho*hi;
	double v_m1 = func( m1 );
	
	return GoldenSearch1d(func,lo,v_lo,m1,v_m1,hi,v_hi,minstep);
}

template< typename t_functor >	
double GoldenSearch1d( t_functor & func, double lo, double hi, double minstep )
{
	double v_lo = func( lo );
	double v_hi = func( hi );
	
	return GoldenSearch1d( func, lo, v_lo, hi, v_hi, minstep );
}

/*
template< typename t_functor >	
double Search1d_BinaryExpandingThenDown( t_functor func, double start, double step, double minstep , const int min_steps = 8)
{
	double cur = start;
	double curV = func( cur );
		
	int steps = 0;
	double best = cur;
	double bestV = curV;
	double bestStep = step;
	
	for(;;)
	{
		if ( curV < bestV )
		{
			bestV = curV;
			best = cur;
			bestStep = step;
		}
		
		double next = cur + step;
		double nextV = func( next );
		
		if ( nextV > curV && steps > min_steps )
			break;
				
		cur = next;
		curV = nextV;
		step *= 1.5;
		steps++;
	}
	
	//lprintfvar(best);
	
	//return BinarySearch1d(func,best,bestStep*0.5,minstep);	
	return GoldenSearch1d(func,best-bestStep*0.5,best+bestStep,minstep);
}
/**/

template< typename t_functor >	
double Search1d_ExpandingThenGoldenDown( t_functor & func, double start, double step, double minstep , const int min_steps = 8)
{
	struct Triple
	{
		double t0,f0;
		double t1,f1;
		double t2,f2;
	};
	
	Triple cur;
	cur.t2 = cur.t1 = cur.t0 = start;
	cur.f2 = cur.f1 = cur.f0 = func( start );
		
	int steps = 0;
	
	Triple best = cur;
	
	for(;;)
	{
		cur.t0 = cur.t1; 
		cur.f0 = cur.f1;
		cur.t1 = cur.t2; 
		cur.f1 = cur.f2;
		
		cur.t2 = cur.t1 + step;
		cur.f2 = func( cur.t2 );
		
		if ( cur.f1 <= best.f1 )
		{
			best = cur;
		}
		
		// if we got worst and we're past min_steps :
		if ( cur.f2 > cur.f1 && steps > min_steps )
			break;
			
		// golden growth is a pretty nice ratio to grow (less than 2.0 is safe)
		//	and it saves me one func eval on the descent
		const double golden_growth = 1.618034; // 1/rho - 1
		step *= golden_growth; // grow step by some amount
		steps++;
	}
		
	// best is at t1 , bracketed in [t0,t2]	
	
	/*
	lprintfvar(best.t0);
	lprintfvar(best.f0);
	lprintfvar(best.t1);
	lprintfvar(best.f1);
	lprintfvar(best.t2);
	lprintfvar(best.f2);
	/**/
	
	/*
	// I could do a quadratic interpolation here, I have 3 points
	
	// in practice this is often very bad, when the function shape is not like a quadratic
	//	this can lead you far away from the real minimum
	
	// you can really only use this once the interval gets smaller
	
	double qe = QuadraticExtremum(best.t0,best.f0,best.t1,best.f1,best.t2,best.f2);
	lprintfvar(qe);
	*/
	
	// save one function eval here, I have best.t1 and f1
	
	if ( best.t1 != best.t0 )
		return GoldenSearch1d(func,best.t0,best.f0,best.t1,best.f1,best.t2,best.f2,minstep);
	else
		return GoldenSearch1d(func,best.t0,best.f0,best.t2,best.f2,minstep);
}

template< typename t_functor >	
double Search1d_FixedStepThenDown( t_functor & func, double start, double step, int numsteps , double minstep)
{
	double cur = start;
	double curV = func( cur );
	
	double best = cur;
	double bestV = curV;
	
	for LOOP(steps,numsteps)
	{
		cur += step;
		curV = func( cur );

		if ( curV < bestV )
		{
			bestV = curV;
			best = cur;
		}
	}
	
	//lprintfvar(best);
	
	return GoldenSearch1d(func,best-step,best+step,minstep);
}

template< typename t_functor >	
double Search1d_FixedStepAndDownEach( t_functor & func, double start, double step, int numsteps , double minstep)
{
	double best = 0;
	double bestV = FLT_MAX;
	
	for LOOP(steps,numsteps)
	{
		double lo = start + steps * step;
		double hi = lo + step;

		double cur = GoldenSearch1d(func,lo,hi,minstep);
		double curV = func(cur); // dumb extra eval
		
		if ( curV < bestV )
		{
			bestV = curV;
			best = cur;
		}
	}
	
	//lprintfvar(best);
	
	return best;
}

END_CB

#pragma once

#include "Base.h"

START_CB

#define MAKE_FUNCTOR(type,func) \
struct STRING_JOIN(func,_functor) { \
  type operator() (type x) { return func(x); } \
};


template < typename functor >
double Integrate( double lo, double hi, int steps, functor f )
{
    double sum = 0.0;
    double last_val = f(lo);
    
    double step_size = (hi - lo)/steps;
    double t = lo + step_size;

    for(int i=1;i <= steps;i++, t += step_size)
    {
		// t is at the end of the current interval
        //double t = lo + i * (hi - lo) / steps;
        //double t = ( (steps - i) * lo + i * hi ) / steps;
        
        double cur_val = f(t);
        
        // trapezoid :
        double cur_int = (1.0/2.0) * (cur_val + last_val);
        
        // Simpson :
        // double mid_val = f(t + step_size * 0.5);
        //double cur_int = (1.0/6.0) * (cur_val + 4.0 * mid_val + last_val);
        
        sum += cur_int;
        
        last_val = cur_val;
    }
    
    sum *= step_size;
    
    return sum;
}


template < typename f1, typename f2 >
struct FunctorProduct
{
	FunctorProduct() { }
	FunctorProduct(f1 _f1, f2 _f2) : m_f1(_f1), m_f2(_f2) { }
	
	double operator () ( double x )
	{
		return m_f1(x) * m_f2(x);
	}
	
	f1 m_f1;
	f2 m_f2;
};


template < typename f1, typename f2 >
FunctorProduct<f1,f2> MakeFunctorProduct( f1 _f1 , f2 _f2 ) { return FunctorProduct<f1,f2>(_f1,_f2); }

template < typename functor >
struct RemapFmTo
{
	double m_fmLo; double m_fmHi;
	double m_toLo; double m_toHi;
			
	RemapFmTo( 
			double fmLo, double fmHi,
			double toLo, double toHi )
	: m_fmLo(fmLo), m_fmHi(fmHi), m_toLo(toLo), m_toHi(toHi)
	{
	}
	
	double operator () ( double x )
	{
		double t = (x - m_fmLo) / (m_fmHi - m_fmLo);
		double y = t * (m_toHi - m_toLo) + m_toLo;
		return functor()(y);
	}
};

template < typename functor >
struct RemapUnitTo
{
	double m_toLo; double m_toHi;
			
	RemapUnitTo( 
			double toLo, double toHi )
	:  m_toLo(toLo), m_toHi(toHi)
	{
	}
	
	double operator () ( double x )
	{
		double y = x * (m_toHi - m_toLo) + m_toLo;
		return functor()(y);
	}
};

END_CB

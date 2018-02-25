#pragma once

#include "cblib/Base.h"
#include "cblib/FloatUtil.h"


//---------------------------------------------------------------------------

/**

The Filter is symmetric around
	x = Width()/2 - 0.5
it has its max values at
	W/2-1 and W/2
it has its min values at
	0 and W-1
all the weights sum to 1.0 (normalized)

The normal "even" filter is centered half way between samples, so it has 2 values at max
An "odd" filter is centerd on a sample, so it has only one sample which is exactly at max

NOTE :

The Cubic or Gaussian filter is real good for making mip maps, but it is more "blurring" that you'd really like for image resizing.
	(the blurring is actually useful in mip maps and can be compensated with mip bias if you like)

The SincFilter is a more correct resizer and works well for high res photographic images, 
	but can create very bad ringing in low res or synthetic images
	The sinc is sharpest possible filter that doesn't cause aliasing

**/


START_CB

class Filter
{
public :
	
	int GetWidth() const	{ return m_width; }
	int GetLevels() const	{ return m_levels;}
	bool IsOdd() const	{ return !!m_odd; }
	int GetCenterWidth() const { return (1<<m_levels) + m_odd; }

	float GetWeight(const int i) const
	{
		ASSERT( i >= 0 && i < m_width );
		return m_data[i];
	}

	//-----------------------------------------------------------------------------------------

	typedef double (*gFilter_func)(const double & x);

	Filter(	const int levels,
			const int base_range,
			gFilter_func pulser,
			gFilter_func windower);

	enum EOdd { eOdd };

	Filter(	EOdd odd,
			const int levels,
			const int base_range,
			gFilter_func pulser,
			gFilter_func windower);
			
	~Filter();

	//-----------------------------------------------------------------------------------------

//protected :

	//-----------------------------------------------------------------------------------------

	void SetWeight(const int i,const float v)
	{
		ASSERT( i >= 0 && i < m_width );
		m_data[i] = v;
	}

	void Normalize();
	
	void Fill(gFilter_func pulser,gFilter_func windower,
		const int levels_to_reduce);

	// what width should we use for some # of levels
	static int WidthFromLevels(const int start_width,const int levels_to_reduce);

	//-----------------------------------------------------------------------------------------

	#define EPSILON_RATIO	(1E-16)
	#define KAISER_ALPHA	(4.0)
	#define D_PI			(3.14159265358979323846)


	static double bessel0(const double & x) 
	{
		double xh, sum, pow, ds;
		int k;

		xh = 0.5 * x;
		sum = 1.0;
		pow = 1.0;
		k = 0;
		ds = 1.0;
		while (ds > sum * EPSILON_RATIO) 
		{
			++k;
			pow = pow * (xh / k);
			ds = pow * pow;
			sum = sum + ds;
			ASSERT( k < 32 );
		}

		return sum;
	}

	// pulses have maxes at 0.0
	// pulses may be evaluated outside of 1.0
	//	but -0.5 to 0.5 is the base domain

	static double pulse_sinc(const double & x) 
	{
		if (x == 0.0) return 1.0;
		const double pix = D_PI * x;
		ASSERT( pix != 0.0 );
		return sin(pix) / pix;
	}

	static double pulse_gauss(const double & x)
	{
		return exp(- 2.0 * x * x);
	}

	static double pulse_box(const double &x)
	{
		double ax = fabs(x);
		if ( ax <= 0.5 )
			return 1.0;
		else
			return 0.0;
	}

	static double pulse_linear(const double &signed_x)
	{
		// this is the quadratic approximation of a Guassian
		const double ax = fabs(signed_x);
		//if ( ax >= 0.5 ) return 0.0;
		//else return 1.0 - 2 * ax;
		return 1.0 - ax;
	}
	
	static double pulse_quadratic(const double &signed_x)
	{
		// this is the quadratic approximation of a Guassian
		const double ax = fabs(signed_x);
		if (ax<.5) return 0.75-ax*ax;
		else if (ax<1.5) { const double t = ax-1.5; return 0.5*t*t;}
		else return 0.0;
	}

	static double pulse_cubic(const double &signed_x)
	{
		// this is the cubic approximation of a Guassian
		//	(this is a (1,0) Mitchell filter, B=1,C=0)
		double ax = fabs(signed_x);
		if (ax<1.0) return (4.0+ax*ax*(-6.0+ax*3.0))*(1.0/6.0);
		else if (ax<2.0) { const double t = 2.0-ax; return t*t*t*(1.0/6.0);}
		else return 0.0;
	}

	static double pulse_cubic_mitchell_B(const double &signed_x,const double B)
	{
		//const double C = (1.0-B)*0.5;
		double ax = fabs(signed_x);
		if (ax<1.0) return (  ax*ax* ( (9.0-6.0*B)*ax - 15.0 + 9.0*B ) + 6.0 - 2.0*B ) *(1.0/6.0);
		else if (ax<2.0) return ( ax*ax* ((2.0*B - 3.0)*ax + (15.0 - 9.0*B)) + (12.0*B - 24.0)*ax + (12.0 - 4.0*B) ) *(1.0/6.0);
		else return 0.0;
	}
	
	static double pulse_cubic_mitchell1(const double &signed_x)
	{
		// compromise Mitchell, may have ringing, etc.
		return pulse_cubic_mitchell_B(signed_x,(1.0/3.0));
	}
	static double pulse_cubic_mitchell2(const double &signed_x)
	{
		// very blurry Mitchell , excellent for antialiasing
		return pulse_cubic_mitchell_B(signed_x,1.5);
	}
	
	static double pulse_unity(const double &)
	{
		// unit pulse lets you see the window function
		return 1.0;
	}

	// window(0) = 1
	// window(1) = 0
	// window(-1) = 0
	// an implicit box window from -1 to 1 is also applied
	//	based on the finite size of the data range

	static double window_unity(const double &x)
	{
		ASSERT( x >= -1.0 && x <= 1.0 );
		return 1.0;
	}

	static double window_cos(const double & x)
	{
		ASSERT( x >= -1.0 && x <= 1.0 );
		return 0.5 + 0.5*cos(D_PI*x);
	}

	static double window_blackman(const double & x)
	{
		ASSERT( x >= -1.0 && x <= 1.0 );
		return 0.42+0.50*cos(D_PI*x)+0.08*cos(2.*D_PI*x);
	}

	static double window_kaiser(const double & x)
	{
		ASSERT( x >= -1.0 && x <= 1.0 );
		//static const double iBessel0A = 1.0 / bessel0(KAISER_ALPHA);
		const double iBessel0A = 0.088480526076450;
		return bessel0(KAISER_ALPHA * sqrt(1.0 - x * x)) * iBessel0A;
	}

private :

	int		m_levels;
	int		m_width;
	float	* m_data;
	int		m_odd; // 1 if odd, 0 if not
};

template <int B>	
double pulse_cubic_mitchell_t(const double &signed_x)
{
		// compromise Mitchell, may have ringing, etc.
	return Filter::pulse_cubic_mitchell_B(signed_x,B/(12.0));
}
	
//---------------------------------------------------------------------------

class BoxFilter : public Filter
{
public :

	// these are identical because the sample range implicitly gives you the box pulse
	BoxFilter(const int levels_to_reduce) : 
		Filter( levels_to_reduce,1,pulse_unity,window_unity )
		//Filter( levels_to_reduce,1,pulse_box,window_unity )
	{
	}
};

//---------------------------------------------------------------------------

class LinearFilter : public Filter
{
public :
	
	LinearFilter(const int levels_to_reduce) : 
		Filter( levels_to_reduce,2,pulse_linear,window_unity )
	{
	}
};

//---------------------------------------------------------------------------

class QuadraticFilter : public Filter
{
public :
	
	QuadraticFilter(const int levels_to_reduce) : 
		Filter( levels_to_reduce,3,pulse_quadratic,window_unity )
	{
	}
};

//---------------------------------------------------------------------------

class CubicFilter : public Filter
{
public :
	
	CubicFilter(const int levels_to_reduce) : 
		Filter( levels_to_reduce,4,pulse_cubic,window_unity )
	{
	}
};

class MitchellFilter1 : public Filter
{
public :
	
	MitchellFilter1(const int levels_to_reduce) : 
		Filter( levels_to_reduce,5,pulse_cubic_mitchell1,window_unity )
	{
	}
};

//---------------------------------------------------------------------------

class GaussianFilter : public Filter
{
public :
	
	// no point in making a GaussianFilter with a base_width smaller than 5,
	//	since you can just use a Cubic Filter then !
	GaussianFilter(const int levels_to_reduce) : 
		Filter( levels_to_reduce,5,pulse_gauss,window_blackman )
	{
	}
};

//---------------------------------------------------------------------------

class SincFilter : public Filter
{
public :
	
	SincFilter(const int levels_to_reduce) : 
		Filter( levels_to_reduce,6,pulse_sinc,window_blackman )
	{
	}
};

//---------------------------------------------------------------------------

// if you do an image double with an OddLinearFilter it will just be a bilinear filter result
class OddLinearFilter : public Filter
{
public :
	
	OddLinearFilter() :
		Filter(eOdd, 1,1,pulse_linear,window_unity )
	{
	}
};

class OddMitchellFilter1 : public Filter
{
public :
	
	OddMitchellFilter1() :
		Filter(eOdd, 1,5,pulse_cubic_mitchell1,window_unity )
	{
	}
};

template <int B>
class OddMitchellFilterT : public Filter
{
public :
	
	OddMitchellFilterT() :
		Filter(eOdd, 1,5,pulse_cubic_mitchell_t<B>,window_unity )
	{
	}
};

class OddSincFilter : public Filter
{
public :
	
	OddSincFilter() :
		Filter(eOdd, 1,6,pulse_sinc,window_blackman )
	{
	}
};

class OddGaussianFilter : public Filter
{
public :
	
	OddGaussianFilter() : 
		Filter(eOdd, 1,6,pulse_gauss,window_blackman )
	{
	}
};

//---------------------------------------------------------------------------

END_CB

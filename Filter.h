#pragma once

#include "Base.h"
#include "FloatUtil.h"


//---------------------------------------------------------------------------

/**

In my silly accidental nomenclature :

a "Filter" is a discrete filter
  it's just an array of weights
  usually pre-normalized

a "FilterGenerator" is the continuous version of a filter function
  it can be used to fill out a discrete "Filter"  
  it consists of a "pulse" and a "window"
  
a "pulser" is a continuous function shape for a mother filter function
  (eg. a gaussian, sinc, box, etc)
  
a "windower" is continuous windowing function

=======================

changed a bit 4/1/11
trying to get rid of the concept of the "center"

for an N level downsampling filter, the "center" is 2^N taps
the "center" is the home region
eg. if it were a box filter, those taps would be the non-zero ones

a filter is odd or even
if it's odd there is one central tap
if even, there are two in the middle

generally filters are symmetric, but that's not required
 (eg. I store all the taps, not half and mirror them)

a symmetric even filter of level N performs downsampling by 2^N

-----------

A level 1 even filter is symmetric around
	x = Width()/2 - 0.5
it has its max values at
	W/2-1 and W/2
it has its min values at
	0 and W-1
all the weights sum to 1.0 (normalized)

The normal "even" filter is centered half way between samples, so it has 2 values at max
	Even is good for minification
	eg. the plain 2-tap box filter for downsampling is Even
An "odd" filter is centered on a sample, so it has only one sample which is exactly at max
	Odd is good for in-place filtering

-----------

NOTE :

The Cubic or Gaussian filter is real good for making mip maps, but it is more "blurring" that you'd really like for image resizing.
	(the blurring is actually useful in mip maps and can be compensated with mip bias if you like)

The SincFilter is a more correct resizer and works well for high res photographic images, 
	but can create very bad ringing in low res or synthetic images
	The sinc is sharpest possible filter that doesn't cause aliasing

Odd filter with levels=1 takes the *halfwidth* as parameter
	eg. OddLanczosFilter(1,4) makes a width of 9

**/


START_CB

typedef double (*Filter_funcptr)(const double & x);
	
struct FilterGenerator
{
	const char *	name;
	Filter_funcptr	pulser;
	Filter_funcptr	windower;
	int				baseWidth;
};

class Filter
{
public :
	
	int GetWidth() const	{ return m_width; }
	int GetLevels() const	{ return m_levels;}
	bool IsOdd() const	{ return !!m_odd; }
	
	float GetWeight(const int i) const
	{
		ASSERT( i >= 0 && i < m_width );
		return m_data[i];
	}

	void LogWeights() const;

	//-----------------------------------------------------------------------------------------

	// default Filter is *even* :
	//	levels >=1 is the number of power of 2 size steps
	// base_range is the basic half-width of the filter (see below)
	//	(eg. for Box filter base_range = 1 , for Hat filter, base_range = 2)
	Filter(	const int levels,
			const int base_range,
			Filter_funcptr pulser,
			Filter_funcptr windower);
			
	enum EOdd { eOdd };

	Filter(	EOdd odd,
			const int levels,
			const int base_range,
			Filter_funcptr pulser,
			Filter_funcptr windower);

	Filter(	const int levels,
			const FilterGenerator & generator);

	Filter(	EOdd odd,
			const int levels,
			const FilterGenerator & generator);
								
	enum EEmpty { eEmpty };
			
	Filter(	
		EEmpty empty,
		const int levels,
		const int base_range);

	Filter(	
		EEmpty empty,
		EOdd odd,
		const int levels,
		const int base_range);
		
	Filter();
		
	~Filter();

	void Init(
		bool odd,
		const int levels,
		const int base_range,
		Filter_funcptr pulser,
		Filter_funcptr windower);
		
	void Init(
		int width,
		float centerWidth,
		Filter_funcptr pulser,
		Filter_funcptr windower);
		
	//-----------------------------------------------------------------------------------------

	void SetWeight(const int i,const float v)
	{
		ASSERT( i >= 0 && i < m_width );
		m_data[i] = v;
	}

	void Normalize();
	
	void ScaleMaxWeight1();
	
	// Scale & Add to do linear combos
	//	will produce non-normalized filters
	void Scale(float scale);
	void Add(const Filter & rhs);
	
	// make sure you Normalize before CutZeroTails :
	void CutZeroTails(const float tolerance = 0.00001f );
	
	// note : Fill does not normalize, you must call it
	void Fill(float center_width,Filter_funcptr pulser,Filter_funcptr windower);

	// support for old "center" ; don't use this :
	int OldCenterWidth() const { return (1<<m_levels) | m_odd; }
	
	//-----------------------------------------------------------------------------------------

	#define EPSILON_RATIO	(1E-16)
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
	// pulser is evaluated from [-base_range to +base_range]
	//	imagine pulser is working on pixels, [-0.5,0.5] is the range of the central pixel
	//	but wide pulses can go to several pixels on either side
	//
	// note : the pulses as written here are NOT normalized
	//		they are normalized manually by Fill()
	// these are only called in Filter construction to fill out a table

	static double pulse_sinc(const double & x) 
	{
		if (x == 0.0) return 1.0;
		const double pix = D_PI * x;
		ASSERT( pix != 0.0 );
		return sin(pix) / pix;
	}

	static double pulse_gauss(const double & x)
	{
		// sdev = 1/2
		return exp(- 2.0 * x * x);
	}

	static double pulse_sqrtgauss(const double & x)
	{
		// this is *wider* than gauss
		// sdev = sqrt(1/2)
		return exp(- x * x);
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
		// this is the linear approximation of a Guassian
		const double ax = fabs(signed_x);

		//return 1.0 - ax;
		
		if ( ax >= 1.0 ) return 0.0;
		else return 1.0 - ax;
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
	
	static double pulse_unity(const double &x)
	{
		// unit pulse lets you see the window function
		return 1.0;
	}

	// window(0) = 1
	// window(1) = 0
	// window(-1) = 0
	// an implicit box window from -1 to 1 is also applied
	//	based on the finite size of the data range

	static double window_box(const double &x)
	{
		if ( x < -0.5 || x > 0.5 ) return 0.0;
		else return 1.0;
	}
	
	static double window_unity(const double &x)
	{
		ASSERT( x >= -1.0 && x <= 1.0 );
		return 1.0;
	}

	// aka "Hann" window :
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

	// this is flatter than blackman :
	//	quite similar to sinc window actually
	static double window_blackman_sqrt(const double & x)
	{
		double w = Filter::window_blackman(x);
		return sqrt(w);
	}

	// sin jump ; not derivative continuous at edges
	static double window_sin(const double & x)
	{
		ASSERT( x >= -1.0 && x <= 1.0 );
		return sin(CB_HALF_PI*(x+1.0));
	}

	// nutall is even sharper than blackman , not useful
	static double window_nutall(const double & x)
	{
		ASSERT( x >= -1.0 && x <= 1.0 );
		return 0.355768 + 0.487396*cos(D_PI*x) + 0.144232*cos(2.*D_PI*x) + 0.012604*cos(3*D_PI*x);
	}

	/*
	static double window_kaiser_alpha(const double & x,const double & alpha)
	{
		ASSERT( x >= -1.0 && x <= 1.0 );
		double iBessel0A = 1.0 / bessel0(alpha);
		return bessel0(alpha * sqrt(1.0 - x * x)) * iBessel0A;
	}
	
	// CB 9-15-08 : changed based on visual inspection ; alpha = 3 looks way better to me
	// CB 2009 some time : this is not really what you want, you want KBD (Kaiser-Bessel-Derived)
	//	 (see LapIm) - the version here is broken broken
	//#define KAISER_ALPHA	(4.0)
	#define KAISER_ALPHA	(3.0)
	static double window_kaiser(const double & x)
	{
		ASSERT( x >= -1.0 && x <= 1.0 );
		// static const is only evaluated once
		static const double iBessel0A = 1.0 / bessel0(KAISER_ALPHA);
		//const double iBessel0A = 0.088480526076450;
		return bessel0(KAISER_ALPHA * sqrt(1.0 - x * x)) * iBessel0A;
	}
	*/

	void Swap(Filter & rhs);

public:

	int		m_levels;	// power of 2 levels
	int		m_width;	// width is the full # of taps in m_data
	float	* m_data;
	int		m_odd;		// 1 if odd, 0 if not

private :	
	FORBID_CLASS_STANDARDS(Filter);
};

/*
template <typename t_type>
class FilterContainer : public RefCounted
{
public:

	VFilter() { }
	virtual ~VFilter() { }

private :	
	FORBID_CLASS_STANDARDS(VFilter);
};
/**/

// B in [4,18] or so
template <int B>	
double pulse_cubic_mitchell_t(const double &signed_x)
{
		// compromise Mitchell, may have ringing, etc.
	return Filter::pulse_cubic_mitchell_B(signed_x,B/(12.0));
}
	
//---------------------------------------------------------------------------
// the normals here are "Even"
//	which means they are symmetric and have two identical taps at the center peak

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

template <int B>
class MitchellFilterT : public Filter
{
public :
	
	MitchellFilterT(const int levels_to_reduce) : 
		Filter( levels_to_reduce,5,pulse_cubic_mitchell_t<B>,window_unity )
	{
	}
};

//---------------------------------------------------------------------------

// The difference between Sinc and Lanczos is just the window
//	"Sinc" using blackman which is quite a compact window
//	lanczos uses sinc as the window which is much flatter

class SincFilter : public Filter
{
public :
	
	SincFilter(const int levels_to_reduce) : 
		Filter( levels_to_reduce,6,pulse_sinc,window_blackman )
	{
		CutZeroTails(); // will actually be width 5
	}
};

class LanczosFilter : public Filter
{
public :

	// pulse of sinc, window with main lobe of sinc
	//	see http://en.wikipedia.org/wiki/Lanczos_resampling
	// note the pulser is indexed from [-range,range] while the window is called from [-1,1]
	// width is in [4-6] generally
	//	wider = better frequency preservation but more ringing
	LanczosFilter(const int levels_to_reduce,const int width=5) :
		Filter(levels_to_reduce,width,Filter::pulse_sinc,Filter::pulse_sinc)
	{
	}
};

//---------------------------------------------------------------------------
// "Odd" filters that have a single-tap peak at the center

// these really only work right with levels = 1

class OddBoxFilter : public Filter
{
public :
	
	OddBoxFilter() :
		Filter(eOdd, 1,0,pulse_box,window_unity )
	{
	}
};

// if you do an image double with an OddLinearFilter it will just be a bilinear filter result
class OddLinearFilter : public Filter
{
public :
	
	OddLinearFilter(const int levels=1) :
		Filter(eOdd, levels,1,pulse_linear,window_unity )
	{
	}
};

class OddQuadraticFilter : public Filter
{
public :
	
	OddQuadraticFilter(const int levels=1) :
		Filter(eOdd, levels,2,pulse_quadratic,window_unity )
	{
	}
};

class OddCubicFilter : public Filter
{
public :
	
	OddCubicFilter(const int levels=1) :
		Filter(eOdd, levels,3,pulse_cubic,window_unity )
	{
	}
};

class OddMitchellFilter1 : public Filter
{
public :
	
	OddMitchellFilter1(const int levels=1) :
		Filter(eOdd, levels,5,pulse_cubic_mitchell1,window_unity )
	{
	}
};

class OddMitchellFilter2 : public Filter
{
public :
	
	OddMitchellFilter2(const int levels=1) :
		Filter(eOdd, levels,5,pulse_cubic_mitchell2,window_unity )
	{
	}
};

template <int B>
class OddMitchellFilterT : public Filter
{
public :
	
	OddMitchellFilterT(const int levels=1) :
		Filter(eOdd, levels,5,pulse_cubic_mitchell_t<B>,window_unity )
	{
	}
};

class OddSincFilter : public Filter
{
public :
	
	OddSincFilter(const int levels=1) :
		Filter(eOdd, levels,6,pulse_sinc,window_blackman )
	{
		CutZeroTails(); // will actually be width 5
	}
};

class OddLanczosFilter : public Filter
{
public :

	// this is a type of Lanczos
	//	 a bit weird to express in my system
	//	see http://en.wikipedia.org/wiki/Lanczos_resampling
	// note the pulser is indexed from [-range,range] while the window is called from [-1,1]
	// 5 seems to be the right width, not sure why I had 4 before
	OddLanczosFilter(const int levels=1,int base_width = 5) :
		Filter(Filter::eOdd, levels,base_width,Filter::pulse_sinc,Filter::pulse_sinc)
	{
	}
};

//---------------------------------------------------------------------------

// helper :
void FillGauss(Filter * filt,double sigma);

class GaussianFilter : public Filter
{
public :
	
	// no point in making a GaussianFilter with a base_width smaller than 5,
	//	since you can just use a Cubic Filter then !
	GaussianFilter(const int levels_to_reduce) : 
//		Filter( levels_to_reduce,6,pulse_gauss,window_blackman )
		Filter( levels_to_reduce,5,pulse_gauss,window_unity ) // no window needed
	{
	}
	
	GaussianFilter(const int levels_to_reduce,int width,double sigma) : 
		Filter(levels_to_reduce,width,pulse_unity,window_unity )
	{
		FillGauss(this,sigma);
	}
};

class OddGaussianFilter : public Filter
{
public :
	
	OddGaussianFilter(const int levels=1) : 
//		Filter(eOdd, levels,5,pulse_gauss,window_blackman_sqrt )
		Filter(eOdd, levels,5,pulse_gauss,window_unity )
// you certainly can run gauss with no window
//	I like the shape better actually with window, it sharpens it a bit
	{
	}
	
	// gaussian of arbitrary sigma :
	// this takes the fullWidth , not the halfWidth like most
	OddGaussianFilter(int fullWidth,double sigma) : 
		Filter(Filter::eEmpty, eOdd, 0,fullWidth)
	{
		ASSERT( (fullWidth&1) ==1 );
		FillGauss(this,sigma);
	}
};

struct OddDaub9Filter : public Filter
{
	OddDaub9Filter() :
		Filter( Filter::eEmpty, Filter::eOdd, 1,4 )
	{
		static const double c_daub9[9] = {
			0.037828455507260, -0.023849465019560,  -0.110624404418440,
			0.377402855612830, 0.852698679008890,   0.377402855612830,
			-0.110624404418440, -0.023849465019560, 0.037828455507260 };
	
		m_levels = 0;
		ASSERT( m_width == ARRAY_SIZE(c_daub9) );
		//ASSERT( GetCenterWidth() == 1 );
		for(int i=0;i<m_width;i++)
		{
			m_data[i] = (float) c_daub9[i];
		}
		Normalize();
	}
};

//---------------------------------------------------------------------------

extern const float c_downForUp_Linear[6];
extern const float c_downForUp_Mitchell1[8];
extern const float c_downForUp_Lanczos4[8];
extern const float c_downForUp_Lanczos5[10];

struct DownForUpFilter_Linear : public Filter
{
	DownForUpFilter_Linear() : Filter( Filter::eEmpty, 1,3  )
	{
		memcpy(m_data,c_downForUp_Linear,ARRAY_SIZE(c_downForUp_Linear)*sizeof(float));
		Normalize();
	}
};

struct DownForUpFilter_Mitchell1 : public Filter
{
	DownForUpFilter_Mitchell1() : Filter( Filter::eEmpty, 1,ARRAY_SIZE(c_downForUp_Mitchell1)/2  )
	{
		memcpy(m_data,c_downForUp_Mitchell1,ARRAY_SIZE(c_downForUp_Mitchell1)*sizeof(float));
		Normalize();
	}
};

struct DownForUpFilter_Lanczos4 : public Filter
{
	DownForUpFilter_Lanczos4() : Filter( Filter::eEmpty, 1,ARRAY_SIZE(c_downForUp_Lanczos4)/2  )
	{
		memcpy(m_data,c_downForUp_Lanczos4,ARRAY_SIZE(c_downForUp_Lanczos4)*sizeof(float));
		Normalize();
	}
};

struct DownForUpFilter_Lanczos5 : public Filter
{
	DownForUpFilter_Lanczos5() : Filter( Filter::eEmpty, 1,ARRAY_SIZE(c_downForUp_Lanczos5)/2  )
	{
		memcpy(m_data,c_downForUp_Lanczos5,ARRAY_SIZE(c_downForUp_Lanczos5)*sizeof(float));
		Normalize();
	}
};
		
//---------------------------------------------------------------------------

struct NamedFilter { const char * name; const Filter * filter; };

// returns the count :
int GetNamedFilters_Even(NamedFilter const * * ptr);
int GetNamedFilters_Odd( NamedFilter const * * ptr);
// returns -1 for not found
int NamedFilter_Find( const NamedFilter * filters, int count, const char* searchFor );

int GetFilterGenerators(FilterGenerator const * * ptr);
int FilterGenerators_Find( const FilterGenerator * filters, int count, const char* searchFor );
const FilterGenerator * StandardFilterGenerators_Find( const char* searchFor );
void StandardFilterGenerators_Log();

// in order from least ringy to most ringy :
extern const FilterGenerator c_filterGenerator_gauss;
extern const FilterGenerator c_filterGenerator_mitchell1;
extern const FilterGenerator c_filterGenerator_lanczos4;
extern const FilterGenerator c_filterGenerator_lanczos5;
	
extern const FilterGenerator c_goodFilterGenerators[];
extern const int c_goodFilterGenerators_count;


//---------------------------------------------------------------------------

double SampleFilter( double x_raw,
					Filter_funcptr pulser,Filter_funcptr windower,
					double centerWidth,double fullWidth);

double SampleFilterGenerator( double x_raw, const FilterGenerator & generator, float centerWidth );

double ApplyFilter(
					const float * rowData, int rowWidth,
					float filterCenterPos,
					float centerWidth,float fullWidth, 
					Filter_funcptr pulser,Filter_funcptr windower);

double ApplyFilterGenerator(
					const float * rowData, int rowWidth,
					float filterCenterPos,
					float centerWidth, const FilterGenerator & generator);
					
//---------------------------------------------------------------------------
					
END_CB

#include "Filter.h"
#include "Util.h"
#include "Log.h"

START_CB

/*

the shrinkers behave about the same as the expanders ?
on aikmi mitchell1 and lanczos5 look good

------------
for expansion :

lanczos5 is best *if* it doesn't have ringing artifacts
	which sometimes it does
sinc is similar to lanczos5 but has slightly fewer artifacts and is a bit blurrier
Mitchell1 is not bad, can ring a bit but not as much as sinc or lanczos5
Gauss is blurry but pretty reliable

sinc & lanczos4 are very similar
sqrtgauss is too blurry
mitchell2 is too blurry & mitchell0 is ringy
cubic,quadratic,etc. are similar to gauss but just worse


/**/

//---------------------------------------------------------------

const float c_downForUp_Linear[6] = { - 0.375f, 0.125f, 1.25f, 1.25f, 0.125f, - 0.375f };
const float c_downForUp_Mitchell1[8] = { 0.05611f, -0.19322f, 0.01013f, 0.62698f, 0.62698f, 0.01013f, -0.19322f, 0.05611f };
const float c_downForUp_Lanczos4[8] = { 0.04304f, -0.14175f, 0.05018f, 0.54854f, 0.54854f, 0.05018f, -0.14175f, 0.04304f };
const float c_downForUp_Lanczos5[10] = { 0.04598f, 0.00377f, -0.13690f, 0.06906f, 0.51809f, 0.51809f, 0.06906f, -0.13690f, 0.00377f, 0.04598f };

//---------------------------------------------------------------

const FilterGenerator c_filterGenerator_gauss = 
	{ "gauss", Filter::pulse_gauss, Filter::window_cos, 5 };

const FilterGenerator c_filterGenerator_mitchell1 = 
	{ "mitchell1", Filter::pulse_cubic_mitchell1, Filter::window_unity, 4 };
	
const FilterGenerator c_filterGenerator_lanczos4 = 
	{ "lanczos4", Filter::pulse_sinc, Filter::pulse_sinc, 4 };

const FilterGenerator c_filterGenerator_lanczos5 = 
	{ "lanczos5", Filter::pulse_sinc, Filter::pulse_sinc, 5 };
	
// in order from least ringy to most ringy :
const FilterGenerator c_goodFilterGenerators[] =
{
	c_filterGenerator_gauss,
	c_filterGenerator_mitchell1,
	c_filterGenerator_lanczos4,
	c_filterGenerator_lanczos5
};
const int c_goodFilterGenerators_count = ARRAY_SIZE(c_goodFilterGenerators);

static const FilterGenerator c_filterGenerators[] =
{
	{ "box", Filter::pulse_box, Filter::window_unity, 1 },
	{ "linear", Filter::pulse_linear, Filter::window_unity, 2 },
	{ "quadratic", Filter::pulse_quadratic, Filter::window_unity, 3 },
	{ "cubic", Filter::pulse_cubic, Filter::window_unity, 4 },
	
	// "mitchell" gives you mitchell1 :
	// mitchell 0,1,2 = sharp,compromise,blurry
	{ "mitchell1", Filter::pulse_cubic_mitchell1, Filter::window_unity, 4 },
	{ "mitchell0", pulse_cubic_mitchell_t<0>, Filter::window_unity, 4 },
	{ "mitchell2", Filter::pulse_cubic_mitchell2, Filter::window_unity, 4 },
	
	// "gauss" gives you this :
	{ "gauss-cos", Filter::pulse_gauss, Filter::window_cos, 5 },
	{ "gauss-unity", Filter::pulse_gauss, Filter::window_unity, 5 },
	//{ "sqrtgauss", Filter::pulse_sqrtgauss, Filter::window_blackman_sqrt, 5 },
	
	// "sinc" gives you this :
	// sinc-cos-5 and sinc-blackman-6 are nearly identical
	{ "sinc-blackman", Filter::pulse_sinc, Filter::window_blackman, 5 },
	{ "sinc-cos", Filter::pulse_sinc, Filter::window_cos, 5 },
	// "lanczos" gives you this one :
	{ "lanczos4", Filter::pulse_sinc, Filter::pulse_sinc, 4 },
	{ "lanczos5", Filter::pulse_sinc, Filter::pulse_sinc, 5 },
	{ "lanczos6", Filter::pulse_sinc, Filter::pulse_sinc, 6 }
};
static const int c_filterGenerators_count = ARRAY_SIZE(c_filterGenerators);

int GetFilterGenerators(FilterGenerator const * * ptr)
{
	*ptr = c_filterGenerators;
	return c_filterGenerators_count;
}

int FilterGenerators_Find( const FilterGenerator * filters, int count ,  const char* searchFor)
{
	for LOOP(i,count)
	{
		if ( stripresame(filters[i].name,searchFor) )
			return i;
	}
	return -1;
}

const FilterGenerator * StandardFilterGenerators_Find( const char* searchFor )
{
	const FilterGenerator * filters;
	int count = GetFilterGenerators(&filters);
	if ( isdigit(searchFor[0]) )
	{
		int fi = atoi(searchFor);
		if ( fi >= 0 && fi < count ) return &filters[fi];
		else return NULL;
	}
	else
	{
		int fi = FilterGenerators_Find(filters,count,searchFor);
		if ( fi == -1 ) return NULL;
		return &filters[fi];
	}
}

void StandardFilterGenerators_Log()
{
	const FilterGenerator * filters;
	int count = GetFilterGenerators(&filters);
	for LOOP(i,count)
	{
		lprintf("%d : %s\n",i,filters[i].name);
	}
}

int NamedFilter_Find( const NamedFilter * filters, int count , const char* searchFor)
{
	for LOOP(i,count)
	{
		if ( stripresame(searchFor,filters[i].name) )
			return i;
	}
	return -1;
}

#define XX(name)	{ #name, new name##Filter(1) }

int GetNamedFilters_Even(NamedFilter const * * ptr)
{
	static const struct NamedFilter c_even_filters[] = 
	{
		XX(Box),
		XX(Linear),
		XX(Quadratic),
		XX(Cubic),
		XX(Gaussian),
		{ "Mitchell1", new MitchellFilter1(1) },
		XX(Sinc),
		{ "Lanczos4", new LanczosFilter(1,4) },
		{ "Lanczos5", new LanczosFilter(1,5) },
		{ "Lanczos6", new LanczosFilter(1,6) },
		{ "DownForUp_Linear", new DownForUpFilter_Linear() },
		{ "DownForUp_Mitchell1", new DownForUpFilter_Mitchell1() },
		{ "DownForUp_Lanczos4", new DownForUpFilter_Lanczos4() },
		{ "DownForUp_Lanczos5", new DownForUpFilter_Lanczos5() }
	};
	int c_even_filters_count = ARRAY_SIZE(c_even_filters);

	*ptr = c_even_filters;
	return c_even_filters_count;
}

#undef XX

#define XX(name)	{ #name, new Odd##name##Filter() }

int GetNamedFilters_Odd(NamedFilter const * * ptr)
{
	static const struct NamedFilter c_odd_filters[] = 
	{
		XX(Box),
		XX(Linear),
		XX(Quadratic),
		XX(Cubic),
		XX(Gaussian),
		{ "Mitchell1", new OddMitchellFilter1() },
		XX(Sinc),
		{ "Lanczos4", new OddLanczosFilter(1,4) },
		{ "Lanczos5", new OddLanczosFilter(1,5) },
		{ "Lanczos6", new OddLanczosFilter(1,6) },
		{ "OddDaub9Filter", new OddDaub9Filter() }
	};
	int c_odd_filters_count = ARRAY_SIZE(c_odd_filters);

	*ptr = c_odd_filters;
	return c_odd_filters_count;
}

#undef XX

/*
		Filter( levels_to_reduce,1,pulse_unity,window_unity )
		Filter( levels_to_reduce,2,pulse_linear,window_unity )
		Filter( levels_to_reduce,3,pulse_quadratic,window_unity )
		Filter( levels_to_reduce,4,pulse_cubic,window_unity )
		Filter( levels_to_reduce,5,pulse_cubic_mitchell1,window_unity )
		Filter( levels_to_reduce,5,pulse_cubic_mitchell_t<B>,window_unity )
		Filter( levels_to_reduce,5,pulse_gauss,window_blackman )
		Filter( levels_to_reduce,6,pulse_sinc,window_blackman )
		Filter(levels_to_reduce,width,Filter::pulse_sinc,Filter::pulse_sinc)
		
		
		
		Filter(eOdd, 1,0,pulse_box,window_unity )
		Filter(eOdd, levels,1,pulse_linear,window_unity )
		Filter(eOdd, levels,5,pulse_cubic_mitchell1,window_unity )
		Filter(eOdd, levels,5,pulse_cubic_mitchell_t<B>,window_unity )
		Filter(eOdd, levels,6,pulse_sinc,window_blackman )
		Filter(Filter::eOdd, levels,5,Filter::pulse_sinc,Filter::pulse_sinc)
*/

//=====================================

void Filter::Swap(Filter & rhs)
{
	cb::Swap(m_levels,rhs.m_levels);
	cb::Swap(m_width,rhs.m_width);
	cb::Swap(m_odd,rhs.m_odd);
	cb::Swap(m_data,rhs.m_data);
}
	
// what width should we use for some # of levels
//	levels >= 0
static int WidthFromLevels(const int base_range,const int levels_to_reduce,const int odd)
{
	//ASSERT( levels_to_reduce >= 1 );
	ASSERT( levels_to_reduce >= 0 );

	int ret = base_range * (1<<levels_to_reduce);
	ret |= odd;
	
	return ret;
}

Filter::Filter() : m_odd(0), m_levels(0), m_width(0), m_data(NULL)
{
}
		
void Filter::Init(
		bool odd,
		const int levels,
		const int base_range,
		Filter_funcptr pulser,
		Filter_funcptr windower)
{
	m_odd = odd ? 1 : 0;
	m_levels = levels;
	m_width = WidthFromLevels(base_range,levels,m_odd);

	ASSERT( m_width > 0 );
	m_data = new float[m_width];

	float cw = (float)OldCenterWidth();
	
	Fill(cw,pulser,windower);

	Normalize();
}

void Filter::Init(
		int width,
		float centerWidth,
		Filter_funcptr pulser,
		Filter_funcptr windower)
{
	m_odd = (width&1);
	m_levels = 0;
	m_width = width;

	ASSERT( m_width > 0 );
	m_data = new float[m_width];

	Fill(centerWidth,pulser,windower);

	Normalize();
}
		
Filter::Filter(	const int levels,
		const FilterGenerator & generator)
{
	Init(false,levels,generator.baseWidth,generator.pulser,generator.windower);
}

Filter::Filter(	EOdd odd,
		const int levels,
		const FilterGenerator & generator)
{
	Init(true ,levels,generator.baseWidth,generator.pulser,generator.windower);
}								
								
Filter::Filter(	const int levels,
		const int base_range,
		Filter_funcptr pulser,Filter_funcptr windower)
{
	Init(false,levels,base_range,pulser,windower);
}

Filter::Filter(	
		EOdd odd,
		const int levels,
		const int base_range,
		Filter_funcptr pulser,Filter_funcptr windower)
{
	Init(true,levels,base_range,pulser,windower);
}

Filter::Filter(	
		EEmpty empty,
		const int levels,
		const int base_range)
{
	Init(false,levels,base_range,pulse_box,window_unity);
}

Filter::Filter(	
		EEmpty empty,
		EOdd odd,
		const int levels,
		const int base_range)
{
	Init(true ,levels,base_range,pulse_box,window_unity);
}

Filter::~Filter()
{
	delete [] m_data;
	m_data = NULL;
}

void Filter::Normalize()
{
	float sum = 0.f;
	for(int i=0;i<m_width;i++)
		sum += m_data[i];
	const float normalizer = 1.f / sum;
	for(int j=0;j<m_width;j++)
		m_data[j] *= normalizer;
}

void Filter::ScaleMaxWeight1()
{
	float big = 0.f;
	for(int i=0;i<m_width;i++)
		big = MAX(big, fabsf(m_data[i]));
		
	const float normalizer = 1.f / big;
	for(int j=0;j<m_width;j++)
		m_data[j] *= normalizer;
}

void Filter::CutZeroTails(const float tolerance)
{
	if ( m_width <= 2 )
		return;
		
	// cut off zeros at the end :
	// must cut off two to preserve oddness

	float ends = fabsf(m_data[0]) + fabsf(m_data[m_width-1]);
	if ( ends < tolerance )
	{
		float * oldData = m_data;
		//int oldWidth = m_width;
		m_width -= 2;
		m_data = new float [m_width];
		
		for LOOP(i,m_width)
		{
			m_data[i] = oldData[i+1];
		}
		
		delete [] oldData;
		Normalize();
		
		// and try again :
		CutZeroTails(tolerance);
	}
}


void Filter::Scale(float scale)
{
	for(int j=0;j<m_width;j++)
		m_data[j] *= scale;
}

void Filter::Add(const Filter & rhs)
{
	ASSERT( m_width == rhs.m_width );
	for(int j=0;j<m_width;j++)
		m_data[j] += rhs.m_data[j];
}
	
void Filter::Fill(float center_width,Filter_funcptr pulser,Filter_funcptr windower)
{
	const int w = GetWidth();
	// m_width must be even ! (for a symmetric reduction filter)
	ASSERT( ( w & 1) == m_odd );

	// 12-07-06 : "odd" mode is new and this is not tested well
	//	this indexing might not be too hot; hmm, seems okay

	const double half_width = w * 0.5;
	const double inv_half_width = 1.0 / half_width;

	//const int center_width = GetCenterWidth() - 1; // @@ WTF
	//const int center_width = GetCenterWidth();
    const double inv_center_width = 1.0 / center_width;
    
	for (int i = 0; i < w; i++) 
	{
		const double x_raw = ((i - half_width) + 0.5); // -half_width - half_width

		const double x_in_center = x_raw * inv_center_width; // (-half_width/center_width -> half_width/center_width)
		const double x_in_width  = x_raw * inv_half_width; // (-1 -> 1)

		// x_in_center goes from -base_range to base_range
		//	cuz of the way pulser is defined

		// x_in_width is = [-1,1] half a pel past the ends of my filter
		//  if you don't want the ends of the filer to be = -1,1 because then the end taps would always be zero

		const double pulse_value  = pulser(x_in_center);
		const double window_value = windower(x_in_width);

		m_data[i] = float( pulse_value * window_value );
	}
}

// sigma = sdev
void FillGauss(Filter * filt,double sigma)
{
	int width = filt->m_width;
	
	DURING_ASSERT( int odd = filt->m_odd );
	ASSERT( (odd&1) == odd ); // 0 or 1
	ASSERT( (width&odd) == odd ); // odd
	
	// works the same for even or odd, yes ?
	
	double offset = ( width - 1 )* 0.5f;
	// offset is the pos of the center tap
	// if ( odd ) , offset is integer
	// if even, offset is half integer
	
	for(int i=0;i<width;i++)
	{
		// the gaussian :
		double x = (i - offset) / sigma;
		double f = exp(- 0.5 * x * x);
		
		// a window function for the finite filter :
		/*
		// this makes the end points be t=[0,1] exactly :
		//  that's kind of silly because the window is exactly zero at those spots
		//	so you have always zeros in m_data
		//double t = (i - offset) / offset;
		// this puts the end points 0.5 past the end :
		double t = (i - offset) / (offset + 0.5);
		//double t = (i - offset) * 2 / width;
		//t *= t; // squaring t makes the window flatter in the middle
		double w = Filter::window_blackman_sqrt(t);
		/*/
		double w = 1.0;
		/**/
		
		filt->m_data[i] = (float)(w * f);
	}
	
	// normalize :
	filt->Normalize();
}

void Filter::LogWeights() const
{
	lprintf("const float c_filter[%d] = ",m_width);
	lprintf("{ ");
	for LOOP(i,m_width)
	{
		lprintf("%7.5f",m_data[i]);
		if ( i != m_width-1 )
			lprintf(", ");
	}
	lprintf(" };\n");
}




double SampleFilter( double x_raw,
					Filter_funcptr pulser,Filter_funcptr windower,
					double centerWidth,double fullWidth)
{
	const double half_width = fullWidth * 0.5;
	
	if ( x_raw < -half_width || x_raw > half_width ) return 0.f;
	
	const double inv_half_width   = 1.0 / half_width;
    const double inv_center_width = 1.0 / centerWidth;
    
	const double x_stretched = x_raw * inv_center_width; // (-half_width/center_width -> half_width/center_width)
	const double x_in_width  = x_raw * inv_half_width; // (-1 -> 1)

	// x_stretched goes from -base_range to base_range
	//	cuz of the way pulser is defined

	// two function pointer calls : slow!
	const double pulse_value  = pulser(x_stretched);
	const double window_value = windower(x_in_width);
	const double filter_value = pulse_value * window_value;
	
	return filter_value;
}

double SampleFilterGenerator( double x_raw, const FilterGenerator & generator, float centerWidth )
{
	const double fullWidth = generator.baseWidth * centerWidth;

	return SampleFilter(x_raw,generator.pulser,generator.windower,centerWidth,fullWidth);
}

double ApplyFilter(
					//const FloatRow & row, 
					const float * rowData, int rowWidth,
					float filterCenterPos,
					float centerWidth,float fullWidth, 
					Filter_funcptr pulser,Filter_funcptr windower)
{
	const double half_width = fullWidth * 0.5;
	const double inv_half_width   = 1.0 / half_width;
    const double inv_center_width = 1.0 / centerWidth;
    
    // hiX is NOT inclusive
    //	round because pixel centers are 0.5 off indexes
	int loX = ftoi_round( filterCenterPos - half_width );
	int hiX = ftoi_round( filterCenterPos + half_width );
	
	// make sure I got these right :
	#ifdef DO_ASSERTS
	{
		const double loX_raw = (loX  +0.5) - filterCenterPos;
		const double hiX_raw = (hiX-1+0.5) - filterCenterPos;
		ASSERT( loX_raw >= -half_width );
		ASSERT( hiX_raw <=  half_width );
		ASSERT( loX_raw-1 <= -half_width );
		ASSERT( hiX_raw+1 >=  half_width );
	}
	#endif
	
	// clip to row extents :
	loX = MAX(loX,0);
	hiX = MIN(hiX,rowWidth); // row.GetSize());

	//const float * rowData = row.GetData();

	// accumulate :
	double sum  = 0;
	double sumW = 0;
		
	for(int x=loX;x<hiX;x++)
	{
		const double x_raw = (x+0.5) - filterCenterPos;
		// @@ if I got my loX/hiX right this would go away :
		//if ( x_raw < -half_width || x_raw > half_width ) continue;
		ASSERT( x_raw >= -half_width && x_raw <= half_width );

		const double x_stretched = x_raw * inv_center_width; // (-half_width/center_width -> half_width/center_width)
		const double x_in_width  = x_raw * inv_half_width; // (-1 -> 1)

		// x_stretched goes from -base_range to base_range
		//	cuz of the way pulser is defined

		// two function pointer calls : slow!
		const double pulse_value  = pulser(x_stretched);
		const double window_value = windower(x_in_width);
		const double filter_value = pulse_value * window_value;
		
		sum  += filter_value * rowData[x];
		sumW += filter_value;
	}
	
	ASSERT( sumW > 0 );
	
	return (sum / sumW);
}

double ApplyFilterGenerator(
					const float * rowData, int rowWidth,
					float filterCenterPos,
					float centerWidth, const FilterGenerator & generator)
{
	return ApplyFilter(rowData,rowWidth,filterCenterPos,centerWidth,
			generator.baseWidth * centerWidth,generator.pulser,generator.windower);
}
					
END_CB

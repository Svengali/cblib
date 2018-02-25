#include "Filter.h"

START_CB

Filter::Filter(	const int levels,
		const int base_range,
		gFilter_func pulser,gFilter_func windower)
{
	m_odd = 0;
	m_levels = levels;
	m_width = WidthFromLevels(base_range,levels);

	ASSERT( m_width > 0 );
	m_data = new float[m_width];

	Fill(pulser,windower,levels);

	Normalize();
}

Filter::Filter(	
		EOdd,
		const int levels,
		const int base_range,
		gFilter_func pulser,gFilter_func windower)
{
	m_odd = 1;
	m_levels = levels;
	m_width = WidthFromLevels(base_range,levels) + 1;

	ASSERT( m_width > 0 );
	m_data = new float[m_width];

	Fill(pulser,windower,levels);

	Normalize();
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

void Filter::Fill(gFilter_func pulser,gFilter_func windower,
	const int)
{
	// m_width must be even ! (for a symmetric reduction filter)
	ASSERT( ( GetWidth() & 1) == m_odd );

	// 12-07-06 : "odd" mode is new and this is not tested well
	//	this indexing might not be too hot; hmm, seems okay

	const double half_width = GetWidth() * 0.5;
	const double inv_half_width = 1.0 / half_width;

	const int center_width = GetCenterWidth() - 1; // @@
    const double inv_center_width = 1.0 / center_width;

	for (int i = 0; i < GetWidth(); i++) 
	{
		const double x_raw = ((i - half_width) + 0.5);

		const double x_stretched = x_raw * inv_center_width; // (-half_width/center_width -> half_width/center_width)
		const double x_in_width = x_raw * inv_half_width; // (-1 -> 1)

		const double pulse_value = pulser(x_stretched);
		const double window_value = windower(x_in_width);

		SetWeight(i , float( pulse_value * window_value ) );
	}
}

// what width should we use for some # of levels
int Filter::WidthFromLevels(const int start_width,const int levels_to_reduce)
{
	ASSERT( levels_to_reduce >= 1 );

	// use a smaller base_width for higher levels to improve speed :
	//int base_width = (start_width * (9 - levels_to_reduce) + 5) / 8;
	//if ( base_width < 1 ) base_width = 1;

	// @@ : use this form ?
	//const int ret = ( start_width - 1 ) * 2 + GetCenterWidth();
    
	const int ret = start_width * (1<<levels_to_reduce);

	return ret;
}

END_CB

#pragma once

#include "cblib/Base.h"
#include "cblib/TextureInfo.h"
#include "cblib/Filter.h"

START_CB

class FloatRow
{
public:

	FloatRow(float * data, int size, int stride, TextureInfo::EEdgeMode eMode)
		: m_data(data), m_size(size), m_stride(stride), m_eEdgeMode(eMode)
	{
	}

	int GetSize() const { return m_size; }

	const float * GetData() const { return m_data; }
	float * MutableData() { return m_data; }

	float GetValue(const int i) const
	{
		const int iFixed = Index(i);
		ASSERT( iFixed >= 0 && iFixed < m_size );
		return m_data[iFixed * m_stride];
	}

	int Index(const int i) const
	{
		return TextureInfo::Index(i,m_size,m_eEdgeMode);
	}

	// the center of the Filter will start at centerIndex on row
	//	centerIndex and (centerIndex+1) will both get the max weight (for a level 1 Filter)
	//	(for a level 2 Filter, four pixels starting at center will get the max weight, etc.)
	// 04/1/11 : this is the old Apply :
	float ApplyAtCenter(const Filter & flt,const int centerIndex)
	{
		const int w = flt.GetWidth();
		const int centerWidth = flt.OldCenterWidth();
		const int offset = (w - centerWidth)/2;
		const int start = centerIndex - offset;
		
		return ApplyBase(flt,start);
	}
	
	// put the middle tap at centerIndex
	float ApplyCentered(const Filter & flt,const int centerIndex)
	{
		const int w = flt.GetWidth();
		const int offset = w/2;
		const int start = centerIndex - offset;
		
		return ApplyBase(flt,start);
	}
	
	// apply flt tap 0 at startIndex
	float ApplyBase(const Filter & flt,const int startIndex)
	{
		const int w = flt.GetWidth();
		double sum = 0.0;
		for(int i=0;i<w;i++)
		{
			sum += flt.GetWeight(i) * GetValue( i + startIndex );
		}
		return (float) sum;
	}

	/*
	// nah, you can Apply externally
	// do the Apply but make sure the output stays in [0,1]
	float ApplyAndClamp(const Filter & Filter,const int centerIndex)
	{
		const float out = Apply(Filter,centerIndex);
		if ( out < 0.f ) return 0.f;
		else if ( out > 1.f ) return 1.f;
		return out;
	}
	/**/

private:
	TextureInfo::EEdgeMode	m_eEdgeMode;
	float *		m_data;
	int			m_size;
	int			m_stride;
};

END_CB

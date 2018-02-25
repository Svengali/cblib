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

	float GetValue(const int i) const
	{
		const int iFixed = Index(i);
		ASSERT( iFixed >= 0 && iFixed < m_size );
		return m_data[iFixed * m_stride];
	}

	int Index(const int i) const
	{
		if ( i >= 0 && i < m_size )
			return i;
		
		switch(m_eEdgeMode)
		{
			case TextureInfo::eEdge_Wrap:
			{
				int iMod = i;
				while ( iMod < 0 )
					iMod += m_size;
				while ( iMod >= m_size )
					iMod -= m_size;
				return iMod;
			}
			case TextureInfo::eEdge_Mirror:
			{				
				if ( i < 0 )
					return Index( - i ); // -1 -> 1
				else
					return Index( - i + 2*m_size - 2 ); // size -> size-2
			}
			case TextureInfo::eEdge_Clamp:
			case TextureInfo::eEdge_Shared:
			{
				if ( i < 0 )
					return 0;
				else
					return m_size - 1;
			}
			default :
				break;
		}
		ASSERT(false);
		return 0;
	}

	// the center of the Filter will start at centerIndex on row
	//	centerIndex and (centerIndex+1) will both get the max weight (for a level 1 Filter)
	//	(for a level 2 Filter, four pixels starting at center will get the max weight, etc.)
	float Apply(const Filter & flt,const int centerIndex)
	{
		const int w = flt.GetWidth();
		const int centerWidth = flt.GetCenterWidth();
		const int offset = (w - centerWidth)/2;
		const int start = centerIndex - offset;
		
		float sum = 0.f;
		for(int i=0;i<w;i++)
		{
			sum += flt.GetWeight(i) * GetValue( i + start );
		}
		return sum;
	}

	// do the Apply but make sure the output stays in [0,1]
	float ApplyAndClamp(const Filter & Filter,const int centerIndex)
	{
		const float out = Apply(Filter,centerIndex);
		if ( out < 0.f ) return 0.f;
		else if ( out > 1.f ) return 1.f;
		return out;
	}

private:
	TextureInfo::EEdgeMode	m_eEdgeMode;
	float *		m_data;
	int			m_size;
	int			m_stride;
};

END_CB

#pragma once

#include "cblib/Base.h"
#include "cblib/TextureInfo.h"
#include "cblib/FloatRow.h"
#include "cblib/Filter.h"
#include "cblib/Color.h"
#include "cblib/SPtr.h"
#include "cblib/Rect.h"

START_CB

SPtrFwd(FloatImage);
SPtrFwd(BmpImage);
class Vec2;

//! \todo @@@@ support a few types of grey : Luminance and RGB average

class FloatImage : public RefCounted
{
public:

	FloatImage(const TextureInfo & info, int depth);
	virtual ~FloatImage();

	static FloatImagePtr CreateFromFile(const char * const fileName);
	static FloatImagePtr CreateFromBmp(const BmpImage & source);
	static FloatImagePtr CreateEmpty(const TextureInfo & info, int depth);
	static FloatImagePtr CreateCopy(const FloatImagePtr & src, int depth);
	static FloatImagePtr CreateTranspose(const FloatImagePtr & src);
	
	// src high end points are not inclusive, eg. [0,0,w,h] is the whole thing
	static FloatImagePtr CreateBilinearResize(const FloatImagePtr & src,const RectI & rect,int toW,int toH);
	
	BmpImagePtr CopyToBmp() const;
	
	const TextureInfo & GetInfo() const { return m_info; }
	int GetDepth() const { return m_depth; }

	// be careful with this !!
	TextureInfo & MutableInfo() { return m_info; }

	void DeGammaCorrect();
	void ReGammaCorrect();

	void Normalize();

	float * Plane(const int p)
	{
		ASSERT( p >= 0 && p < m_depth );
		return m_planes[p];
	}
	
	float & Sample(const int p,const int i)
	{
		ASSERT( p >= 0 && p < m_depth );
		ASSERT( i >= 0 && i < m_info.m_width*m_info.m_height );
		return m_planes[p][ i ];
	}
	float & Sample(const int p,const int x,const int y)
	{
		ASSERT( x >= 0 && x < m_info.m_width );
		ASSERT( y >= 0 && y < m_info.m_height );
		return m_planes[p][ y * m_info.m_width + x ];
	}
	float GetSample(const int p,const int i) const
	{
		ASSERT( p >= 0 && p < m_depth );
		ASSERT( i >= 0 && i < m_info.m_width*m_info.m_height );
		return m_planes[p][ i ];
	}
	float GetSample(const int p,const int x,const int y) const
	{
		ASSERT( x >= 0 && x < m_info.m_width );
		ASSERT( y >= 0 && y < m_info.m_height );
		return m_planes[p][ y * m_info.m_width + x ];
	}

	// uv are in [0,1] with the proper pixel centering and all that
	//	the center of the first pixel is at 0.5/W
	float SampleBilinearFiltered(int p,const Vec2 & uv) const;
	const ColorF GetPixelBilinearFiltered(const Vec2 & uv) const;

	void FillPlane(const int p,const float f);

	ColorF GetPixel(const int x,const int y) const;
	float GetGrey(const int x,const int y) const;
	
	ColorF GetPixel(const int index) const;
	void SetPixel(const int index,const ColorF & cf);
	float GetGrey(const int index) const;

	// using edgemode :
	int IndexAroundEdge(const int x,const int y) const;
	ColorF GetPixelAroundEdge(const int x,const int y) const;
	float GetGreyAroundEdge(const int x,const int y) const;

	// faster, doesn't use edgemode :
	int IndexMirror(const int x,const int y) const;

	void FillSolidColor(const ColorF & color);

	FloatRow GetRow(const int p,const int y) const
	{
		ASSERT( p >= 0 && p < m_depth );
		float * plane = m_planes[p];
		return FloatRow(plane + y * m_info.m_width,m_info.m_width,1,m_info.m_eEdgeMode);
	}

	FloatRow GetColumn(const int p,const int x) const
	{
		ASSERT( p >= 0 && p < m_depth );
		float * plane = m_planes[p];
		return FloatRow(plane + x,m_info.m_height,m_info.m_width,m_info.m_eEdgeMode);
	}

	// "DownFilter" = >>levels size ; should use normal "even" filter
	FloatImagePtr CreateDownFiltered(const Filter & Filter) const;
	
	// SameSize & Doubled should use "odd" filters, eg. centered on one pel, not between pels
	FloatImagePtr CreateSameSizeFiltered(const Filter & Filter) const;
	FloatImagePtr CreateDoubledFiltered(const Filter & Filter) const;

	FloatImagePtr CreateRandomDownSampled() const;
		// CreateRandomDownSampled makes an image of width/2 , height/2
		// all the pixels in the product occur in the original
		// useful for heightmaps

	FloatImagePtr CreateNormalMapFromHeightMap() const;

	void ConvertRGBtoYUV();
	void ConvertYUVtoRGB();

private:

	TextureInfo	m_info;
	int			m_depth;
	float **	m_planes;

	FORBID_CLASS_STANDARDS(FloatImage);
};


// for cube maps:

struct FloatImagePtr6
{
	FloatImagePtr data[6];

	FloatImagePtr6()
	{
	}
	~FloatImagePtr6()
	{
	}
	FloatImagePtr6(const FloatImagePtr6 & other)
	{
		for(int i=0;i<6;i++)
			data[i] = other.data[i];
	}
};

END_CB

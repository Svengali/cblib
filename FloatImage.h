#pragma once

#include "Base.h"
#include "TextureInfo.h"
#include "FloatRow.h"
#include "Filter.h"
#include "Color.h"
#include "SPtr.h"
#include "Rect.h"
#include "File.h"

START_CB

SPtrFwd(FloatImage);
SPtrFwd(BmpImage);
class Vec2;

class FloatImage : public RefCounted
{
public:

	FloatImage(const TextureInfo & info, int depth);
	virtual ~FloatImage();

	//---------------------------------------------------
	// static ::Create functions
	
	// CreateFromFile uses extension ; "fim" is a float source, else loads 8-bit bmp sources
	static FloatImagePtr CreateFromFile(const char * const fileName);
	static FloatImagePtr CreateFromBmp(BmpImagePtr source);
	static FloatImagePtr CreateEmpty(const TextureInfo & info, int depth);
	static FloatImagePtr CreateCopy(const FloatImagePtr & src, int depth);
	static FloatImagePtr CreateTranspose(const FloatImagePtr & src);
	//static FloatImagePtr CreateResized(const FloatImagePtr & src,int toW,int toH);
	static FloatImagePtr CreateGreyScale(const FloatImagePtr & src);
	
	// src high end points are not inclusive, eg. [0,0,w,h] is the whole thing
	static FloatImagePtr CreateBilinearResize(const FloatImagePtr & src,int toW,int toH,const RectI * pRect = NULL, const Vec2 * pOffset = NULL);

	static FloatImagePtr CreateCopyPortion(const FloatImagePtr & src,int xlo,int ylo,int w,int h);

	//---------------------------------------------------
	// basics :
	
	void SetCopy(const FloatImage * src);
	void CopyPlane(int top, const FloatImage * fm, int fmp);
	void CopyPlanePortion(int top, const FloatImage * fm, int fmp, int xlo, int ylo, int w, int h);

	// CopyToBmp clamps to range
	BmpImagePtr CopyToBmp() const;

	// Load/Save a "fim"
	static FloatImagePtr LoadFim(const FileRCPtr & stream); 
	void SaveFim(const FileRCPtr & stream) const;
	
	static FloatImagePtr LoadFim(const char * name); 
	bool SaveFim(const char * name) const; 	
	
	// Saves Fim or Bmp depending on name
	bool SaveByName(const char * fileName) const;
	// common sequence you should do is ClampUnit,ReGamma,SaveByName :
	bool Clamp_Gamma_SaveByName(const char * fileName);

	const TextureInfo & GetInfo() const { return m_info; }
	int GetDepth() const { return m_depth; }

	int GetWidth()  const { return m_info.m_width; }
	int GetHeight() const { return m_info.m_height; }

	// be careful with this !!
	TextureInfo & MutableInfo() { return m_info; }

	//---------------------------------------------------
	// sample access :
	
	float * Plane(const int p)
	{
		ASSERT( p >= 0 && p < m_depth );
		return m_planes[p];
	}
	const float * GetPlane(const int p) const
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
	
	float & SampleClamped(const int p,const int x,const int y)
	{
		return m_planes[p][ IndexClamp(x,y) ];
	}
	float GetSampleClamped(const int p,const int x,const int y) const
	{
		return m_planes[p][ IndexClamp(x,y) ];
	}
	float & SampleMirrored(const int p,const int x,const int y)
	{
		return m_planes[p][ IndexMirror(x,y) ];
	}
	float GetSampleMirrored(const int p,const int x,const int y) const
	{
		return m_planes[p][ IndexMirror(x,y) ];
	}

	// uv are in [0,1] with the proper pixel centering and all that
	//	the center of the first pixel is at 0.5/W
	float SampleBilinearFiltered(int p,const Vec2 & uv) const;
	const ColorF GetPixelBilinearFiltered(const Vec2 & uv) const;

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
	int IndexMirror(const int x,const int y) const; // does NOT duplicate edge pixel
	int IndexClamp(const int x,const int y) const; // does NOT duplicate edge pixel

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

	//---------------------------------------------------
	// making filtered copies :
	
	// box up filter:
	FloatImagePtr CreateDoubledBoxFilter() const;
	FloatImagePtr CreateHalvedBoxFilter() const; // simple box filter halving
	FloatImagePtr CreateHalvedPointSampled() const;

	// "DownFilter" = >>levels size ; should use normal "even" filter
	FloatImagePtr CreateDownFiltered(const Filter & Filter) const;
	
	// SameSize & Doubled should use "odd" filters, eg. centered on one pel, not between pels
	FloatImagePtr CreateSameSizeFiltered(const Filter & Filter) const;
	FloatImagePtr CreateDoubled_BoxThenSameSizeFiltered(const Filter & Filter) const;

	FloatImagePtr CreateSameSizeFilteredPlanes(const Filter * planeFilters) const;

	FloatImagePtr CreateRandomDownSampled() const;
		// CreateRandomDownSampled makes an image of width/2 , height/2
		// all the pixels in the product occur in the original
		// useful for heightmaps

	FloatImagePtr CreateNormalMapFromHeightMap() const;

	//---------------------------------------------------
	// utils :
	
	void FillLinearCombo(
		float w1,const FloatImage * fim1,
		float w2,const FloatImage * fim2,float C);
	
	// fim1 * fim2 + B
	void FillProduct(
		const FloatImage * fim1,const FloatImage * fim2,float B);
		
	void FillZero();
	void FillPlane(const int p,const float f);
	void FillSolidColor(const ColorF & color);
	
	// WARNING : you should generally ClampUnit before ReGamma !!
	// note : ReGamma is a nop if you're already gamma'ed ; this changes persistent state
	void DeGammaCorrect();
	void ReGammaCorrect();

	// this YUV is rec601 and Y goes in plane 0
	void ConvertRGBtoYUV();
	void ConvertYUVtoRGB();

	double ComputeVariance(ColorF * pAverage) const;

	void Normalize(); // Normalize like RGB is a Vec3
	
	void ClampUnit(); // clamp all in [0,1]
	
	// ScaleBiasUnit :
	//	rescale such that Lo -> 0 , Hi - > 1
	//	note : uses the same range for all planes, not a range per plane
	void ScaleBiasUnit();

	// this = scale * this + offset
	void ScaleAndOffset(float scale,float offset);

	//--------------------------------------------------

//private:
public:

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
	
	const TextureInfo & GetInfo() const { return data[0]->GetInfo(); }
	int GetDepth() const { return data[0]->GetDepth(); }
	
	void CreateEmpty(const TextureInfo & info, int depth)
	{
		for(int i=0;i<6;i++)
			data[i] = FloatImage::CreateEmpty(info,depth);
	}
	
	void ClampUnit()
	{
		for(int i=0;i<6;i++)
			data[i]->ClampUnit();
	}
	
	
	bool Load(const FileRCPtr & stream); 
	void Save(const FileRCPtr & stream) const; 
};


// LoadGeneric should be called without extension and we'll find it
bool LoadGeneric(const char *filename, BmpImage *result);

// WriteByName writes bmp or tga
bool WriteByName(BmpImagePtr bmp,const char *name);

double FloatImageMSE(const FloatImage * im1,const FloatImage * im2);

void FloatImage_DeGammaCorrect_Pow2(FloatImage * fim);
void FloatImage_ReGammaCorrect_Pow2(FloatImage * fim);

void FloatImage_DeGammaCorrect_SRGB(FloatImage * fim);
void FloatImage_ReGammaCorrect_SRGB(FloatImage * fim);

class Mat3;
void FloatImage_ApplyColorMatrix(FloatImage * fim,const Mat3 & mat);

/**

using a 0-level even filter gives you half-pel offsetting
eg.
LanczosFilter f(0,3);

**/
FloatImagePtr FloatImage_CreateHalfPelFilteredH(const FloatImage *src,const Filter & filter);
FloatImagePtr FloatImage_CreateHalfPelFilteredV(const FloatImage *src,const Filter & filter);

FloatImagePtr CreateProduct(const FloatImage * im1,const FloatImage * im2,float A = 0.f);
FloatImagePtr CreateProduct(FloatImagePtr im1,FloatImagePtr im2,float A = 0.f);
FloatImagePtr CreateSelectOnePlane(FloatImagePtr im1,int p);
FloatImagePtr CreateLinearCombo(float w1,FloatImagePtr im1,float w2,FloatImagePtr im2,float C);

cb::FloatImagePtr CreatePlaneSum(const cb::FloatImage * fim);
cb::FloatImagePtr CreatePlaneSumSqr(const cb::FloatImage * fim);
cb::FloatImagePtr CreatePlaneSumSqrSqrt(const cb::FloatImage * fim);

END_CB

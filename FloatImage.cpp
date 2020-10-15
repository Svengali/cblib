#include "FloatImage.h"
#include "Vec3.h"
#include "Mat3.h"
#include "Vec2.h"
#include "FloatUtil.h"
#include "Rand.h"
#include "BmpImage.h"
#include "File.h"
#include "Log.h"
#include "FileUtil.h"
#include "ColorConversions.h"
#include <string.h>

START_CB


//! \todo @@@@ support a few types of grey : Luminance and RGB average

FloatImage::FloatImage(const TextureInfo & info, int depth)
	: m_info(info), m_depth(depth)
{
	m_planes = new float * [m_depth];
	for(int d=0;d<m_depth;d++)
	{
		m_planes[d] = new float [m_info.m_width*m_info.m_height];
	}
}

FloatImage::~FloatImage()
{
	for(int d=0;d<m_depth;d++)
	{
		delete [] m_planes[d];
	}
	delete [] m_planes;
}

/*static*/ FloatImagePtr FloatImage::CreateEmpty(const TextureInfo & info, int depth)
{
	FloatImagePtr ret( new FloatImage(info,depth) );
	return ret;
}

FloatImagePtr FloatImage::CreateCopy(const FloatImagePtr & src, int depth)
{
	FloatImagePtr ret( new FloatImage(src->GetInfo(),depth) );
	
	int mdepth = MIN(depth,src->m_depth);
	int i;
	for(i=0;i<mdepth;i++)
	{
		memcpy( ret->m_planes[i], src->m_planes[i], src->m_info.m_width*src->m_info.m_height*sizeof(float));
	}
	for(i=mdepth;i<depth;i++)
	{
		memset( ret->m_planes[i], 0, src->m_info.m_width*src->m_info.m_height*sizeof(float));
	}

	return ret;
}

void FloatImage::CopyPlane(int top, const FloatImage * fm, int fmp)
{
	ASSERT_RELEASE( m_info.m_width  == fm->m_info.m_width );
	ASSERT_RELEASE( m_info.m_height == fm->m_info.m_height );
	
	memcpy( m_planes[top], fm->m_planes[fmp], fm->m_info.m_width*fm->m_info.m_height*sizeof(float));
}

void FloatImage::CopyPlanePortion(int top, const FloatImage * fm, int fmp, int xlo, int ylo, int w, int h)
{
	ASSERT_RELEASE( w <= m_info.m_width  );
	ASSERT_RELEASE( h <= m_info.m_height );
	ASSERT_RELEASE( xlo+w <= fm->m_info.m_width  );
	ASSERT_RELEASE( ylo+h <= fm->m_info.m_height );
	
	for(int y=0;y<h;y++)
	{
		for(int x=0;x<w;x++)
		{
			m_planes[top][x + y*m_info.m_width] = fm->m_planes[fmp][(x+xlo) + (y+ylo)*(fm->m_info.m_width)];
		}
	}		
}

void FloatImage::SetCopy(const FloatImage * src)
{
	ASSERT_RELEASE( m_info.m_width == src->m_info.m_width );
	ASSERT_RELEASE( m_info.m_height == src->m_info.m_height );
 
	int mdepth = MIN(m_depth,src->m_depth);
	int i;
	for(i=0;i<mdepth;i++)
	{
		memcpy( m_planes[i], src->m_planes[i], src->m_info.m_width*src->m_info.m_height*sizeof(float));
	}
	for(i=mdepth;i<m_depth;i++)
	{
		memset( m_planes[i], 0, src->m_info.m_width*src->m_info.m_height*sizeof(float));
	}
}

FloatImagePtr FloatImage::CreateTranspose(const FloatImagePtr & src)
{
	FloatImagePtr ret( new FloatImage(src->GetInfo(),src->GetDepth()) );
	
	Swap( ret->m_info.m_width, ret->m_info.m_height );	

	int d = src->GetDepth();	
	for(int p=0;p<d;p++)
	{
		int w = ret->m_info.m_width;
		int h = ret->m_info.m_height;

		for(int y=0;y<h;y++)
		{
			for(int x=0;x<w;x++)
			{
				ret->m_planes[p][x + y*w] = src->m_planes[p][y + x*h];
			}
		}		
	}

	return ret;
}

FloatImagePtr FloatImage::CreateCopyPortion(const FloatImagePtr & src,int xlo,int ylo,int w,int h)
{
	const TextureInfo & srcInfo = src->GetInfo();
	if ( xlo+w > srcInfo.m_width || ylo+h > srcInfo.m_height )
		return FloatImagePtr(NULL);
		
	int d = src->GetDepth();	
	TextureInfo newInfo = srcInfo;
	newInfo.m_width  = w;
	newInfo.m_height = h;
	FloatImagePtr ret( new FloatImage(newInfo,d) );
	
	for(int p=0;p<d;p++)
	{
		for(int y=0;y<h;y++)
		{
			for(int x=0;x<w;x++)
			{
				ret->m_planes[p][x + y*w] = src->m_planes[p][(x+xlo) + (y+ylo)*(srcInfo.m_width)];
			}
		}		
	}

	return ret;
}

FloatImagePtr FloatImage::CreateGreyScale(const FloatImagePtr & src)
{
	FloatImagePtr ret( new FloatImage(src->GetInfo(),1) );
	
	int w = ret->m_info.m_width;
	int h = ret->m_info.m_height;

	for(int y=0;y<h;y++)
	{
		for(int x=0;x<w;x++)
		{
			ret->m_planes[0][x + y*w] = src->GetGrey(x + y*w);
		}
	}		

	return ret;
}

// src high end points are not inclusive, eg. [0,0,w,h] is the whole thing
FloatImagePtr FloatImage::CreateBilinearResize(const FloatImagePtr & src,int toW,int toH,const RectI * pRect, const Vec2 * pOffset)
{
	TextureInfo dstInfo(src->GetInfo());
	dstInfo.m_width = toW;
	dstInfo.m_height = toH;
	FloatImagePtr dst( new FloatImage(dstInfo,src->GetDepth()) );
	FloatImage * pSrc = src.GetPtr();
	
	RectI srcRectAll( 0, src->m_info.m_width, 0, src->m_info.m_height );
	if ( pRect == NULL )
	{
		pRect = &srcRectAll;
	}
	float srcX = (float)pRect->LoX();
	float srcY = (float)pRect->LoY();
	float srcW = (float)pRect->Width();
	float srcH = (float)pRect->Height();
	
	float srcVscale = 1.f/(float)src->m_info.m_height;
	float srcUscale = 1.f/(float)src->m_info.m_width;
	
	float dstOffX = 0.5f;
	float dstOffY = 0.5f;
	
	if ( pOffset )
	{
		dstOffX += pOffset->x;
		dstOffY += pOffset->y;
	}
					
	int d = src->GetDepth();	
	for(int p=0;p<d;p++)
	{
		int dstW = dst->m_info.m_width;
		int dstH = dst->m_info.m_height;

		for(int y=0;y<dstH;y++)
		{
			float dstV = (y + dstOffY)/dstH;
			float srcV = (srcY + dstV * srcH) * srcVscale;
						
			for(int x=0;x<dstW;x++)
			{
				float dstU = (x + dstOffX)/dstW;
				float srcU = (srcX + dstU * srcW) * srcUscale;
			
				Vec2 uv(srcU,srcV);
				
				dst->m_planes[p][x + y*dstW] = pSrc->SampleBilinearFiltered(p,uv);
			}
		}		
	}

	return dst;
}

ColorF FloatImage::GetPixelAroundEdge(const int x,const int y) const
{
	const int i = IndexAroundEdge(x,y);
	return GetPixel(i);
}

float FloatImage::GetGreyAroundEdge(const int x,const int y) const
{
	const int i = IndexAroundEdge(x,y);
	return GetGrey(i);
}

int FloatImage::IndexAroundEdge(const int x,const int y) const
{
	int ix = TextureInfo::Index(x,m_info.m_width,m_info.m_eEdgeMode);
	int iy = TextureInfo::Index(y,m_info.m_height,m_info.m_eEdgeMode);
	
	return iy * m_info.m_width + ix;
}

int FloatImage::IndexMirror(const int x,const int y) const
{
	int xx = x, yy =y;
	
	if ( xx < 0 )
	{
		xx = -xx;
	}
	else if ( xx >= m_info.m_width )
	{
		xx = 2*m_info.m_width - xx - 2;
	}
	
	if ( yy < 0 )
	{
		yy = -yy;
	}
	else if ( yy >= m_info.m_height )
	{
		yy = 2*m_info.m_height - yy - 2;
	}
	
	ASSERT( xx >= 0 && xx < m_info.m_width );
	ASSERT( yy >= 0 && yy < m_info.m_height );

	return yy * m_info.m_width + xx;
}

int FloatImage::IndexClamp(const int x,const int y) const
{
	int xx = Clamp(x,0,m_info.m_width -1);
	int yy = Clamp(y,0,m_info.m_height-1);

	return yy * m_info.m_width + xx;
}

float FloatImage::GetGrey(const int x,const int y) const
{
	ASSERT( x >= 0 && x < m_info.m_width );
	ASSERT( y >= 0 && y < m_info.m_height );
	const int i = x + y * m_info.m_width;
	return GetGrey(i);
}

float FloatImage::GetGrey(const int i) const
{
	ASSERT( i >= 0 && i < m_info.m_width*m_info.m_height );

	if ( m_depth == 1 )
	{
		return m_planes[0][i];
	}
	else
	{
		//! \todo @@@@ support a few types of grey : Luminance and RGB average
		ColorF color = GetPixel(i);
		//return (color.GetR() + 2.f * color.GetG() + color.GetB()) / 4.f;
		return ( 0.25f * color.GetR() + 0.55f * color.GetG() + 0.20f * color.GetB());
	}
}

ColorF FloatImage::GetPixel(const int x,const int y) const
{
	ASSERT( x >= 0 && x < m_info.m_width );
	ASSERT( y >= 0 && y < m_info.m_height );
	
	const int i = x + y * m_info.m_width;

	return GetPixel(i);
}

ColorF FloatImage::GetPixel(const int i) const
{
	ASSERT( i >= 0 && i < m_info.m_width*m_info.m_height );

	switch(m_depth)
	{
		case 1:
		{
			const float f = m_planes[0][i];
			return ColorF( f,f,f );
		}
		case 2:
		{
			const float r = m_planes[0][i];
			const float g = m_planes[1][i];
			return ColorF( r,g,255.f );
		}
		case 3:
		{
			const float r = m_planes[0][i];
			const float g = m_planes[1][i];
			const float b = m_planes[2][i];
			return ColorF( r,g,b );
		}
		case 4:
		{
			const float r = m_planes[0][i];
			const float g = m_planes[1][i];
			const float b = m_planes[2][i];
			const float a = m_planes[3][i];
			return ColorF( r,g,b,a );
		}
		default:
		{
			FAIL("Can't GetPixel for planes > 4");
			CANT_GET_HERE();
			return ColorF::debug;
		}
	}
}

void FloatImage::SetPixel(const int i,const ColorF & cf)
{
	ASSERT( i >= 0 && i < m_info.m_width*m_info.m_height );

	switch(m_depth)
	{
		case 1:
		{
			m_planes[0][i] = cf.GetG();
			return;
		}
		case 2:
		{
			m_planes[0][i] = cf.GetR();
			m_planes[1][i] = cf.GetG();
			return;
		}
		case 3:
		{
			m_planes[0][i] = cf.GetR();
			m_planes[1][i] = cf.GetG();
			m_planes[2][i] = cf.GetB();
			return;
		}
		case 4:
		{
			m_planes[0][i] = cf.GetR();
			m_planes[1][i] = cf.GetG();
			m_planes[2][i] = cf.GetB();
			m_planes[3][i] = cf.GetA();
			return;
		}
		default:
		{
			FAIL("Can't GetPixel for planes > 4");
			return;
		}
	}
}

const ColorF FloatImage::GetPixelBilinearFiltered(const Vec2 & uv) const
{
	// sample pImage at "uv"

	int w = GetInfo().m_width;
	int h = GetInfo().m_height;

	float x = uv.x * w - 0.5f;
	float y = uv.y * h - 0.5f;

	int i,j;
	float fx,fy;

	if ( x <= 0.f )
	{
		i = 0; fx = 0.f;
	}
	else if ( x >= w-1 )
	{
		i = w-2;
		fx = 1.f;
	}
	else
	{
		i = (int) x;
		ASSERT( i <= (w-2) );
		fx = x - i;
	}
	
	if ( y <= 0.f )
	{
		j = 0; fy = 0.f;
	}
	else if ( y >= h-1 )
	{
		j = h-2;
		fy = 1.f;
	}
	else
	{
		j = (int) y;
		ASSERT( j <= (h-2) );
		fy = y - j;
	}

	ColorF LL = GetPixel(i,j);
	ColorF LH = GetPixel(i,j+1);
	ColorF HL = GetPixel(i+1,j);
	ColorF HH = GetPixel(i+1,j+1);

	float wLL = (1-fx)*(1-fy);
	float wLH = (1-fx)*fy;
	float wHL = fx*(1-fy);
	float wHH = fx*fy;

	ColorF bilerp = LL * wLL + LH * wLH + HL * wHL + HH * wHH;

	return bilerp;
}

float FloatImage::SampleBilinearFiltered(int p,const Vec2 & uv) const
{
	// sample pImage at "uv"

	int w = GetInfo().m_width;
	int h = GetInfo().m_height;

	float x = uv.x * w - 0.5f;
	float y = uv.y * h - 0.5f;

	int i,j;
	float fx,fy;

	if ( x <= 0.f )
	{
		i = 0; fx = 0.f;
	}
	else if ( x >= w-1 )
	{
		i = w-2;
		fx = 1.f;
	}
	else
	{
		i = (int) x;
		ASSERT( i <= (w-2) );
		fx = x - i;
	}
	
	if ( y <= 0.f )
	{
		j = 0; fy = 0.f;
	}
	else if ( y >= h-1 )
	{
		j = h-2;
		fy = 1.f;
	}
	else
	{
		j = (int) y;
		ASSERT( j <= (h-2) );
		fy = y - j;
	}

	float LL = GetSample(p,i,j);
	float LH = GetSample(p,i,j+1);
	float HL = GetSample(p,i+1,j);
	float HH = GetSample(p,i+1,j+1);

	float wLL = (1-fx)*(1-fy);
	float wLH = (1-fx)*fy;
	float wHL = fx*(1-fy);
	float wHH = fx*fy;

	float bilerp = LL * wLL + LH * wLH + HL * wHL + HH * wHH;

	return bilerp;
}

void FloatImage::FillZero()
{
	int size = m_info.m_width*m_info.m_height;
	for(int p = 0; p < m_depth;p++ )
	{
		float * plane = m_planes[p];
		memset(plane,0,size*sizeof(float));
	}
}

void FloatImage::FillPlane(const int p,const float f)
{
	ASSERT( p >= 0 && p < m_depth );

	float * plane = m_planes[p];
	int size = m_info.m_width*m_info.m_height;
	for(int i=0;i<size;i++)
	{
		plane[i] = f;
	}
}

void FloatImage::FillSolidColor(const ColorF & color)
{
	switch(m_depth)
	{
		case 1:
		{
			const float l = (color.GetR() + color.GetG() + color.GetB()) / 3.f;
			FillPlane(0,l);
			break;
		}
		case 2:
		{
			FillPlane(0,color.GetR());
			FillPlane(1,color.GetG());
			break;
		}
		case 3:
		{
			FillPlane(0,color.GetR());
			FillPlane(1,color.GetG());
			FillPlane(2,color.GetB());
			break;
		}
		case 4:
		{
			FillPlane(0,color.GetR());
			FillPlane(1,color.GetG());
			FillPlane(2,color.GetB());
			FillPlane(3,color.GetA());
			break;
		}
		default:
		{
			FAIL("Can't FillSolidColor for planes > 4");
			return;
		}
	}
}

void FloatImage::ReGammaCorrect()
{
	// @@ hmm.. you pretty much always want to ClampUnit before this
	//	but I guess not always (eg. for HDR) so it's not done automatically
	// I could do the clamp at zero though, since that's an assert/failure , not optional
	FloatImage_ReGammaCorrect_SRGB(this);
}

void FloatImage::DeGammaCorrect()
{
	FloatImage_DeGammaCorrect_SRGB(this);
}

void FloatImage::ClampUnit()
{
	for(int d=0;d<m_depth;d++)
	{
		float * plane = m_planes[d];
		int size = m_info.m_width*m_info.m_height;
		for(int i=0;i<size;i++)
		{
			plane[i] = fclampunit( plane[i] );
		}
	}
}


// ScaleBiasUnit :
//	rescale such that Lo -> 0 , Hi - > 1
//	note : uses the same range for all planes, not a range per plane
void FloatImage::ScaleBiasUnit()
{
	float lo = FLT_MAX;
	float hi = -FLT_MAX;
	
	for(int d=0;d<m_depth;d++)
	{
		float * plane = m_planes[d];
		int size = m_info.m_width*m_info.m_height;
		for(int i=0;i<size;i++)
		{
			lo = MIN(lo, plane[i] );
			hi = MAX(hi, plane[i] );
		}
	}
	
	for(int d=0;d<m_depth;d++)
	{
		float * plane = m_planes[d];
		int size = m_info.m_width*m_info.m_height;
		for(int i=0;i<size;i++)
		{
			plane[i] = fmakelerperclamped(lo,hi, plane[i] );
		}
	}
}

void FloatImage::ScaleAndOffset(float scale,float offset)
{
	for(int d=0;d<m_depth;d++)
	{
		float * plane = m_planes[d];
		int size = m_info.m_width*m_info.m_height;
		for(int i=0;i<size;i++)
		{
			plane[i] = plane[i] * scale + offset;
		}
	}
}

void FloatImage::Normalize()
{
	ASSERT( m_info.m_usage == TextureInfo::eUsage_Normalmap );
	ASSERT( m_depth >= 3 );
	
	int size = m_info.m_width*m_info.m_height;
	for(int i=0;i<size;i++)
	{
		ColorF color = GetPixel(i);
		Vec3 vec = color.GetNormal();
		vec.NormalizeSafe();
		color.SetFromNormal(vec);
	
		m_planes[0][i] = color.GetR();
		m_planes[1][i] = color.GetG();
		m_planes[2][i] = color.GetB();
	}
}

void FloatImage::FillLinearCombo(
		float w1,const FloatImage * fim1,
		float w2,const FloatImage * fim2,float C)
{
	for(int d=0;d<m_depth;d++)
	{
		float * plane = m_planes[d];
		int size = m_info.m_width*m_info.m_height;
		const float * p1 = fim1->m_planes[d];
		const float * p2 = fim2->m_planes[d];
		for(int i=0;i<size;i++)
		{
			plane[i] = w1 * p1[i] + w2 * p2[i] + C;
		}
	}
}

// fim1 * fim2 + B
void FloatImage::FillProduct(
		const FloatImage * fim1,const FloatImage * fim2,float B)
{
	for(int d=0;d<m_depth;d++)
	{
		int size = m_info.m_width*m_info.m_height;
		float * plane = m_planes[d];
		const float * p1 = fim1->m_planes[d];
		const float * p2 = fim2->m_planes[d];
		for(int i=0;i<size;i++)
		{
			plane[i] = p1[i] * p2[i] + B;
		}
	}
}

#define SHIFT_RIGHT_ROUND_UP( val, shift )	( (val) + (1<<(shift)) - 1 ) >> (shift)

FloatImagePtr FloatImage::CreateDownFiltered(const Filter & Filter) const
{
	//DeGammaCorrect();	
	// meh let him do what he wants :
	//ASSERT( m_info.m_gammaCorrected == false );
	ASSERT( ! Filter.IsOdd() );

	int levels = Filter.GetLevels();
	ASSERT( levels >= 1 );
	int offset = 1<< (levels-1);

	TextureInfo tempInfo = m_info;
	tempInfo.m_width  = SHIFT_RIGHT_ROUND_UP( m_info.m_width  , levels );
	tempInfo.m_height = m_info.m_height;

	FloatImagePtr tempImage( new FloatImage(tempInfo,m_depth) );

	// @@@@ : eEdge_Shared not supported yet
	ASSERT( m_info.m_eEdgeMode != TextureInfo::eEdge_Shared );

	// first fill tempImage by gFiltering in X only
	int d;
	for(d=0;d<m_depth;d++)
	{
		for(int y=0;y<tempInfo.m_height;y++)
		{
			float * dest = tempImage->m_planes[d] + y * tempInfo.m_width;
			FloatRow srcRow = GetRow(d,y);

			for(int x=0;x<tempInfo.m_width;x++)
			{
				int srcX = x << levels;
				//dest[x] = srcRow.Apply(Filter,srcX);
				dest[x] = srcRow.ApplyCentered(Filter,srcX+offset);
			}
		}
	}

	TextureInfo newInfo = m_info;
	newInfo.m_width  = SHIFT_RIGHT_ROUND_UP( m_info.m_width  , levels );
	newInfo.m_height = SHIFT_RIGHT_ROUND_UP( m_info.m_height , levels );

	FloatImagePtr newImage( new FloatImage(newInfo,m_depth) );

	for(d=0;d<m_depth;d++)
	{
		for(int x=0;x<newInfo.m_width;x++)
		{
			float * destCol = newImage->m_planes[d] + x;
			FloatRow srcCol = tempImage->GetColumn(d,x);

			for(int y=0;y<newInfo.m_height;y++)
			{
				int srcY = y << levels;
				//destCol[y * newInfo.m_width] = srcCol.Apply(Filter,srcY);
				destCol[y * newInfo.m_width] = srcCol.ApplyCentered(Filter,srcY+offset);
			}
		}
	}

	tempImage = NULL;

	if ( m_info.m_usage == TextureInfo::eUsage_Normalmap )
		newImage->Normalize();

	return newImage;
}

FloatImagePtr FloatImage::CreateHalvedBoxFilter() const
{
	//DeGammaCorrect();	
	//ASSERT( m_info.m_gammaCorrected == false );

	TextureInfo newInfo = m_info;
	newInfo.m_width  = m_info.m_width  >> 1;
	newInfo.m_height = m_info.m_height >> 1;

	FloatImagePtr newImage( new FloatImage(newInfo,m_depth) );

	for(int d=0;d<m_depth;d++)
	{
		for(int y=0;y<newInfo.m_height;y++)
		{
			int srcY = y << 1;

			float * destRow = newImage->m_planes[d] + y * newInfo.m_width;

			for(int x=0;x<newInfo.m_width;x++)
			{
				int srcX = x << 1;
				
				destRow[x] =
					(this->GetSample(d,srcX,srcY)+
					this->GetSample(d,srcX,srcY+1)+
					this->GetSample(d,srcX+1,srcY)+
					this->GetSample(d,srcX+1,srcY+1))*0.25f;
			}
		}
	}

	if ( m_info.m_usage == TextureInfo::eUsage_Normalmap )
		newImage->Normalize();

	return newImage;
}

FloatImagePtr FloatImage::CreateHalvedPointSampled() const
{
	//DeGammaCorrect();	
	//ASSERT( m_info.m_gammaCorrected == false );

	TextureInfo newInfo = m_info;
	newInfo.m_width  = m_info.m_width  >> 1;
	newInfo.m_height = m_info.m_height >> 1;

	FloatImagePtr newImage( new FloatImage(newInfo,m_depth) );

	for(int d=0;d<m_depth;d++)
	{
		for(int y=0;y<newInfo.m_height;y++)
		{
			int srcY = y << 1;

			float * destRow = newImage->m_planes[d] + y * newInfo.m_width;

			for(int x=0;x<newInfo.m_width;x++)
			{
				int srcX = x << 1;
				
				destRow[x] = this->GetSample(d,srcX,srcY);
			}
		}
	}

	if ( m_info.m_usage == TextureInfo::eUsage_Normalmap )
		newImage->Normalize();

	return newImage;
}

FloatImagePtr FloatImage::CreateSameSizeFiltered(const Filter & Filter) const
{
	//DeGammaCorrect();	
	//ASSERT( m_info.m_gammaCorrected == false );	
	ASSERT( Filter.IsOdd() );

	FloatImagePtr tempImage( new FloatImage(m_info,m_depth) );

	// @@@@ : eEdge_Shared not supported yet
	ASSERT( m_info.m_eEdgeMode != TextureInfo::eEdge_Shared );

	// the Filter Apply will start the Filter center at the given pixel
	//	I want the Filter centered on the pixel
	// offset for Apply() :
	//const int offset = - ((Filter.GetCenterWidth()+1)/2 - 1);
	// simpler offset for ApplyBase :
	const int offset = - (Filter.GetWidth()/2);

	// first fill tempImage by gFiltering in X only
	int d;
	for(d=0;d<m_depth;d++)
	{
		for(int y=0;y<m_info.m_height;y++)
		{
			float * dest = tempImage->m_planes[d] + y * m_info.m_width;
			FloatRow srcRow = GetRow(d,y);

			for(int x=0;x<m_info.m_width;x++)
			{
				dest[x] = srcRow.ApplyBase(Filter,x+offset);
			}
		}
	}

	FloatImagePtr newImage( new FloatImage(m_info,m_depth) );

	for(d=0;d<m_depth;d++)
	{
		for(int x=0;x<m_info.m_width;x++)
		{
			float * destCol = newImage->m_planes[d] + x;
			FloatRow srcCol = tempImage->GetColumn(d,x);

			for(int y=0;y<m_info.m_height;y++)
			{
				destCol[y * m_info.m_width] = srcCol.ApplyBase(Filter,y+offset);
			}
		}
	}

	tempImage = NULL;

	return newImage;
}

FloatImagePtr FloatImage::CreateSameSizeFilteredPlanes(const Filter * planeFilters) const
{
	//DeGammaCorrect();	
	//ASSERT( m_info.m_gammaCorrected == false );	

	FloatImagePtr tempImage = FloatImage::CreateEmpty(m_info,m_depth);

	// @@@@ : eEdge_Shared not supported yet
	ASSERT( m_info.m_eEdgeMode != TextureInfo::eEdge_Shared );

	// first fill tempImage by gFiltering in X only
	int d;
	for(d=0;d<m_depth;d++)
	{
		// the Filter Apply will start the Filter center at the given pixel
		//	I want the Filter centered on the pixel
		const Filter & filter = planeFilters[d];
		ASSERT( filter.IsOdd() );
		//const int offset = - ((Filter.GetCenterWidth()+1)/2 - 1);
	
		for(int y=0;y<m_info.m_height;y++)
		{
			float * dest = tempImage->m_planes[d] + y * m_info.m_width;
			FloatRow srcRow = GetRow(d,y);

			for(int x=0;x<m_info.m_width;x++)
			{
				//dest[x] = srcRow.Apply(Filter,x+offset);
				dest[x] = srcRow.ApplyCentered(filter,x);
			}
		}
	}

	FloatImagePtr newImage = FloatImage::CreateEmpty(m_info,m_depth);

	for(d=0;d<m_depth;d++)
	{
		const Filter & Filter = planeFilters[d];
		ASSERT( Filter.IsOdd() );
		//const int offset = - ((Filter.GetCenterWidth()+1)/2 - 1);
		
		for(int x=0;x<m_info.m_width;x++)
		{
			float * destCol = newImage->m_planes[d] + x;
			FloatRow srcCol = tempImage->GetColumn(d,x);

			for(int y=0;y<m_info.m_height;y++)
			{
				//destCol[y * m_info.m_width] = srcCol.Apply(Filter,y+offset);
				destCol[y * m_info.m_width] = srcCol.ApplyCentered(Filter,y);
			}
		}
	}

	tempImage = NULL;

	return newImage;
}


FloatImagePtr FloatImage::CreateDoubledBoxFilter() const
{
	TextureInfo newInfo = m_info;
	newInfo.m_width  = m_info.m_width*2;
	newInfo.m_height = m_info.m_height*2;

	FloatImagePtr newImage( new FloatImage(newInfo,m_depth) );

	for(int d=0;d<m_depth;d++)
	{
		for(int y=0;y<newInfo.m_height;y++)
		{
			for(int x=0;x<newInfo.m_width;x++)
			{
				newImage->Sample(d,x,y) = GetSample(d,x>>1,y>>1);
			}
		}
	}

	return newImage;
}

FloatImagePtr FloatImage::CreateNormalMapFromHeightMap() const
{
	ASSERT( m_info.m_usage == TextureInfo::eUsage_Heightmap );

	//DeGammaCorrect();	
	ASSERT( m_info.m_gammaCorrected == false );

	FloatImagePtr newImage( new FloatImage(m_info,3) );

	newImage->MutableInfo().m_usage = TextureInfo::eUsage_Normalmap;

	float altitude = GetInfo().m_heightMapAltitude;
	ASSERT( altitude > 0.f );

	for(int y=0;y<m_info.m_height;y++)
	{
		for(int x=0;x<m_info.m_width;x++)
		{
			// "Sobel filter" for local image gradient

			// uses wrap mode of source image
			const float N = GetGreyAroundEdge( x, y+1 );
			const float S = GetGreyAroundEdge( x, y-1 );
			const float E = GetGreyAroundEdge( x+1, y );
			const float W = GetGreyAroundEdge( x-1, y );
			const float NE = GetGreyAroundEdge( x+1, y+1 );
			const float NW = GetGreyAroundEdge( x-1, y+1 );
			const float SE = GetGreyAroundEdge( x+1, y-1 );
			const float SW = GetGreyAroundEdge( x-1, y-1 );

			/*
			// Sobel :
			// [1,2,1] taps 
			float fE = (E + E + NE + SE) * 0.25f;
			float fW = (W + W + NW + SW) * 0.25f;
			float fN = (N + N + NW + NE) * 0.25f;
			float fS = (S + S + SW + SE) * 0.25f;
			/**/

			// Scharr : 10,3,3 :
			float fE = (10.f/16.f)* E + (NE + SE) * (3.f/16.f);
			float fW = (10.f/16.f)* W + (NW + SW) * (3.f/16.f);
			float fN = (10.f/16.f)* N + (NW + NE) * (3.f/16.f);
			float fS = (10.f/16.f)* S + (SW + SE) * (3.f/16.f);
			
			// factor of 0.5 here is because taps are centered two pixels apart :
			//	dx = (sample(+1) - sample(-1))/2
			float dx = (fE - fW) * 0.5f;
			float dy = (fN - fS) * 0.5f;

			Vec3 normal( dx, dy, altitude );
			normal.NormalizeFast();
			
			const int i = y * GetInfo().m_width + x;

			newImage->Sample(0,i) = normal.x * 0.5f + 0.5f;
			newImage->Sample(1,i) = normal.y * 0.5f + 0.5f;
			newImage->Sample(2,i) = normal.z * 0.5f + 0.5f;

		}
	}

	return newImage;
}

FloatImagePtr FloatImage::CreateRandomDownSampled() const
{
	TextureInfo newInfo = m_info;
	newInfo.m_width  = SHIFT_RIGHT_ROUND_UP( m_info.m_width  , 1 );
	newInfo.m_height = SHIFT_RIGHT_ROUND_UP( m_info.m_height , 1 );

	FloatImagePtr newImage( new FloatImage(newInfo,m_depth) );

	for(int y=0;y<newInfo.m_height;y++)
	{
		for(int x=0;x<newInfo.m_width;x++)
		{
			// add 0.5f because we want to center our sample at (x*2) + 0.5f , not x*2
			//const int px = ftoi( gRand::GetGaussianFloat() * 2.f + 0.5f );
			//const int py = ftoi( gRand::GetGaussianFloat() * 2.f + 0.5f );

			const int ox = irandranged(-1,1) + irandranged(0,1);
			const int oy = irandranged(-1,1) + irandranged(0,1);
			
			// ox and oy go from -1 to 2 , with an average of 0.5 and a higher chance of 0 and 1

			const int px = x * 2 + ox;
			const int py = y * 2 + oy;

			const int pi = IndexAroundEdge(px,py);
			const int i = x + y * newInfo.m_width;

			for(int d=0;d<m_depth;d++)
			{
				const float p = m_planes[d][pi];
				newImage->m_planes[d][i] = p;
			}
		}
	}

	return newImage;
}

//-----------------------------------------------------------------

/*static*/ FloatImagePtr FloatImage::CreateFromFile(const char * const fileName)
{
	FloatImagePtr ret;
	
	if ( strisame(extensionpart(fileName),"fim") )
	{
		ret = FloatImage::LoadFim(fileName);		
	}
	else
	{
		BmpImagePtr im = BmpImage::Create(fileName);

		if ( im == NULL )
			return FloatImagePtr(NULL);

		im->Depalettize();

		ret = CreateFromBmp(im);
	}
	
	if ( ret != NULL )
	{
		ret->m_info.m_fileName = fileName;
	}
		
	return ret;
}

/*static*/ FloatImagePtr FloatImage::CreateFromBmp(BmpImagePtr pSource)
{
	if ( pSource->IsPalettized() )
	{
		// depal without mutating source :
		//	(this is a stupid extra malloc and memcpy but whatever)
		pSource = BmpImage::CreateCopy(pSource);
		pSource->Depalettize();
	}

	const BmpImage & source = pSource.GetRef();

	TextureInfo info;
	source.GetTextureInfo(&info);

	int planes;

	if ( source.IsGrey8() )
		planes = 1;
	else if ( info.m_alpha != TextureInfo::eAlpha_None )
		planes = 4;
	else 
		planes = 3;

	FloatImagePtr ret( new FloatImage(info,planes) );

	for(int y=0;y<info.m_height;y++)
	{
		for(int x=0;x<info.m_width;x++)
		{
			ColorDW c = source.GetColor(x,y);
			ColorF f(c);

			int i = y * info.m_width + x;

			switch(planes)
			{
			case 1:
				ret->m_planes[0][i] = f.GetR();
				break;
			case 2:
				FAIL("2 planes not supported!!");
				break;
			case 3:
				ret->m_planes[0][i] = f.GetR();
				ret->m_planes[1][i] = f.GetG();
				ret->m_planes[2][i] = f.GetB();
				break;
			case 4:
				ret->m_planes[0][i] = f.GetR();
				ret->m_planes[1][i] = f.GetG();
				ret->m_planes[2][i] = f.GetB();
				ret->m_planes[3][i] = f.GetA();
				break;
			default:
				FAIL("more than 5 planes not supported!");
				break;
			}
		}
	}

	return ret;
}

BmpImagePtr FloatImage::CopyToBmp() const
{
	ASSERT( m_info.m_gammaCorrected );

	BmpImagePtr to = BmpImage::Create();

	int w = m_info.m_width;
	int h = m_info.m_height;
	
	if( GetDepth() == 1 )
	{
		to->Allocate8Bit(w,h);
		for(int y=0;y<h;y++)
		{
			uint8 * ptr = to->m_pData + y * to->m_pitch;
			for(int x=0;x<w;x++)
			{
				float f = GetSample(0,x,y);
				int i = ColorFTOI(f);
				*ptr++ = ClampTo8(i);
			}
		}
	}
	else if ( GetDepth() >= 4 )
	{	
		to->Allocate32Bit(w,h);
		for(int y=0;y<h;y++)
		{
			uint8 * ptr = to->m_pData + y * to->m_pitch;
			for(int x=0;x<w;x++)
			{
				ColorF cf = GetPixel(x + y*w);
				ColorDW dw(ColorDW::eFromFloatSafe,cf);
				ptr[0] = (uint8) dw.GetB();
				ptr[1] = (uint8) dw.GetG();
				ptr[2] = (uint8) dw.GetR();
				ptr[3] = (uint8) dw.GetA();
				ptr += 4;
			}
		}
	}
	else
	{	
		to->Allocate24Bit(w,h);
		for(int y=0;y<h;y++)
		{
			uint8 * ptr = to->m_pData + y * to->m_pitch;
			for(int x=0;x<w;x++)
			{
				ColorF cf = GetPixel(x + y*w);
				ColorDW dw(ColorDW::eFromFloatSafe,cf);
				ptr[0] = (uint8) dw.GetB();
				ptr[1] = (uint8) dw.GetG();
				ptr[2] = (uint8) dw.GetR();
				ptr += 3;
			}
		}
	}
	
	return to;
}

void FloatImage::ConvertRGBtoYUV()
{
	if ( GetDepth() < 3 )
		return;
		
	int w = m_info.m_width;
	int h = m_info.m_height;
	int num = w*h;
	for(int i=0;i<num;i++)
	{
		ColorF cf = GetPixel(i);
		ColorF out = MakeYUVFromRGB(cf);
		SetPixel(i,out);
	}
}

void FloatImage::ConvertYUVtoRGB()
{
	if ( GetDepth() < 3 )
		return;
		
	int w = m_info.m_width;
	int h = m_info.m_height;
	int num = w*h;
	for(int i=0;i<num;i++)
	{
		ColorF cf = GetPixel(i);
		ColorF out = MakeRGBFromYUV(cf);
		SetPixel(i,out);
	}
}

// util :
double FloatImage::ComputeVariance(ColorF * pAverage) const
{
	const FloatImage * fim = this;
	int w = fim->GetInfo().m_width;
	int h = fim->GetInfo().m_height;
	int depth = fim->GetDepth();
	int count = w*h;
	
	if ( pAverage )
	{
		pAverage->Set(0,0,0,0);
	}
		
	double totalVariance = 0;
	
	for(int p=0;p<depth;p++)
	{
		double sum = 0;	
		double sumSqr = 0;	
	
		const float * plane = fim->GetPlane(p);
		for(int i=0;i<count;i++)
		{
			sum += plane[i];
			sumSqr += plane[i]*plane[i];
		}
		
		double avg = sum/count;
		double planeVariance = (sumSqr/count) - avg*avg;
		totalVariance += planeVariance;
		
		if ( pAverage && p < 4 )
		{
			pAverage->MutableVec4()[p] = (float) (sum/count);
		}
	}
	
	return totalVariance;
}

//-----------------------------------------------------------------

/*static*/ FloatImagePtr FloatImage::LoadFim(const char * fileName)
{
	FileRCPtr file = FileRC::Create(fileName,"rb");
	if ( file == NULL )
		return FloatImagePtr(NULL);
		
	FloatImagePtr ret = FloatImage::LoadFim(file);	
	return ret;
}

bool FloatImage::SaveFim(const char * name) const
{	
	FileRCPtr file = FileRC::Create(name,"wb");
	if ( file == NULL )
	{
		lprintf("Failed to write : %s\n",name);
		return false;
	}
	
	SaveFim(file);
	
	return true;
}
	
	
static const uint32 floatImage_Magic = 'flIm';
static const uint32 floatImage_Version = 1;

FloatImagePtr FloatImage::LoadFim(const FileRCPtr & stream)
{
	uint32 magic;
	stream->IO(magic);
	if ( magic != floatImage_Magic )
	{
		lprintf("Failed to get floatImage magic number!");
		return FloatImagePtr(NULL);
	}

	uint32 version;
	stream->IO(version);
	if ( version != 1 )
	{
		lprintf("Bad floatImage version number!");
		return FloatImagePtr(NULL);
	}

	TextureInfo info;
	info.IO(stream.GetRef());

	int depth;
	stream->IO(depth);

	FloatImagePtr ret = CreateEmpty(info,depth);
	if ( ret == NULL )
		return ret;

	// read planes :
	
	int plane_bytes = sizeof(float) * info.m_width * info.m_height;
	
	for(int p=0;p<depth;p++)
	{
		float * plane = ret->Plane(p);
		stream->Read(plane,plane_bytes);
	}

	return ret;
}

void FloatImage::SaveFim(const FileRCPtr & stream) const
{
	uint32 magic = floatImage_Magic;
	stream->IO(magic);
	uint32 version = floatImage_Version;
	stream->IO(version);

	const_cast<TextureInfo &>(GetInfo()).IO(stream.GetRef());

	int depth = GetDepth();
	stream->IO(depth);

	// write planes :
	
	int plane_bytes = sizeof(float) * GetInfo().m_width * GetInfo().m_height;

	for(int p=0;p<depth;p++)
	{
		const float * plane = GetPlane(p);
		stream->Write(plane,plane_bytes);
	}
}

bool FloatImagePtr6::Load(const FileRCPtr & stream)
{
	for(int i=0;i<6;i++)
	{
		data[i] = FloatImage::LoadFim(stream);
		if ( data[i] == NULL )
			return false;
	}
	return true;
}

void FloatImagePtr6::Save(const FileRCPtr & stream) const
{
	for(int i=0;i<6;i++)
	{
		data[i]->SaveFim(stream);
	}
}

bool FloatImage::Clamp_Gamma_SaveByName(const char * fileName)
{
	ClampUnit();
	ReGammaCorrect();
	return SaveByName(fileName);
}
	
bool FloatImage::SaveByName(const char * fileName) const
{
	FloatImagePtrC fim(this);
	
	if ( strisame(extensionpart(fileName),"fim") )
	{
		if ( ! fim->SaveFim(fileName) )
		{
			lprintf("Failed to write FIM : %s\n",fileName);
			return false;
		}
	}
	else
	{
		BmpImagePtr bmp = fim->CopyToBmp();
		
		if ( ! WriteByName(bmp,fileName) )
		{
			lprintf("Failed to write BMP : %s\n",fileName);
			return false;
		}
	}
	
	return true;
}

double FloatImageMSE(const FloatImage * im1,const FloatImage * im2)
{
	double mse = 0;
	
	int w = MIN(im1->m_info.m_width ,im2->m_info.m_width );
	int h = MIN(im1->m_info.m_height,im2->m_info.m_height);
	int d = MIN(im1->m_depth,im2->m_depth);
	
	for(int y=0;y<h;y++)
	{
		for(int x=0;x<w;x++)
		{
			for(int p=0;p<d;p++)
			{
				float s1 = im1->GetSample(p,x,y);
				float s2 = im2->GetSample(p,x,y);
			
				mse += fsquare(s1 - s2);
			}
		}
	}
	mse /= (w * h);
	return mse;
}
	
	
//-----------------------------------------------------------------

// optimization for gamma of 2.0 :
inline float degamma_pow2(const float f)
{
	return f*f;
}
inline float regamma_pow2(const float f)
{
	return sqrtf(f);
}

void FloatImage_DeGammaCorrect_Pow2(FloatImage * fim)
{
	if ( ! fim->m_info.m_gammaCorrected )
		return;

	for(int d=0;d<fim->m_depth;d++)
	{
		float * plane = fim->m_planes[d];
		int size = fim->m_info.m_width*fim->m_info.m_height;
		for(int i=0;i<size;i++)
		{
			ASSERT( plane[i] >= 0.f && plane[i] <= 1.f );
			plane[i] = degamma_pow2( plane[i] );
		}
	}

	fim->m_info.m_gammaCorrected = false;
}

void FloatImage_ReGammaCorrect_Pow2(FloatImage * fim)
{
	if ( fim->m_info.m_gammaCorrected )
		return;

	// do NOT ReGamma for a heightmap or normal map !
	if ( fim->m_info.m_usage == TextureInfo::eUsage_Heightmap ||
		 fim->m_info.m_usage == TextureInfo::eUsage_Normalmap )
		return;

	for(int d=0;d<fim->m_depth;d++)
	{
		float * plane = fim->m_planes[d];
		int size = fim->m_info.m_width*fim->m_info.m_height;
		for(int i=0;i<size;i++)
		{
			ASSERT( plane[i] >= 0.f && plane[i] <= 1.f );
			plane[i] = regamma_pow2( plane[i] );
		}
	}

	fim->m_info.m_gammaCorrected = true;
}

void FloatImage_DeGammaCorrect_SRGB(FloatImage * fim)
{
	if ( ! fim->m_info.m_gammaCorrected )
		return;

	for(int d=0;d<fim->m_depth;d++)
	{
		float * plane = fim->m_planes[d];
		int size = fim->m_info.m_width*fim->m_info.m_height;
		for(int i=0;i<size;i++)
		{
			//ASSERT( plane[i] >= 0.f && plane[i] <= 1.f );
			plane[i] = SRGB_To_Linear( plane[i] );
		}
	}

	fim->m_info.m_gammaCorrected = false;
}

void FloatImage_ReGammaCorrect_SRGB(FloatImage * fim)
{
	if ( fim->m_info.m_gammaCorrected )
		return;

	// do NOT ReGamma for a heightmap or normal map !
	if ( fim->m_info.m_usage == TextureInfo::eUsage_Heightmap ||
		 fim->m_info.m_usage == TextureInfo::eUsage_Normalmap )
		return;
		
	for(int d=0;d<fim->m_depth;d++)
	{
		float * plane = fim->m_planes[d];
		int size = fim->m_info.m_width*fim->m_info.m_height;
		for(int i=0;i<size;i++)
		{
			//ASSERT( plane[i] >= 0.f && plane[i] <= 1.f );
			plane[i] = Linear_To_SRGB( plane[i] );
		}
	}

	fim->m_info.m_gammaCorrected = true;
}

void FloatImage_ApplyColorMatrix(FloatImage * fim,const Mat3 & mat)
{
	int w = fim->m_info.m_width;
	int h = fim->m_info.m_height;

	for(int y=0;y<h;y++)
	{
		for(int x=0;x<w;x++)
		{
			int i = x + y * w;
			ColorF color = fim->GetPixel(i);
			Vec3 transformed = mat * color.AsVec3();
			color.MutableVec3() = transformed;
			fim->SetPixel(i,color);
		}
	}
}

/**

using a 1-level "down-filter" gives you half-pel offsetting

**/
FloatImagePtr FloatImage_CreateHalfPelFilteredH(const FloatImage *src,const Filter & filter)
{
	//DeGammaCorrect();	
	// meh let him do what he wants :
	//ASSERT( m_info.m_gammaCorrected == false );
	ASSERT( ! filter.IsOdd() );
	ASSERT( filter.GetLevels() == 1 );

	TextureInfo tempInfo = src->GetInfo();
	int depth = src->GetDepth();
	
	FloatImagePtr tempImage( new FloatImage(tempInfo,depth) );

	//ASSERT( m_info.m_eEdgeMode != TextureInfo::eEdge_Shared );

	// first fill tempImage by gFiltering in X only
	for(int d=0;d<depth;d++)
	{
		for(int y=0;y<tempInfo.m_height;y++)
		{
			float * dest = tempImage->m_planes[d] + y * tempInfo.m_width;
			FloatRow srcRow = src->GetRow(d,y);

			for(int x=0;x<tempInfo.m_width;x++)
			{
				//dest[x] = srcRow.ApplyAtCenter(filter,x);
				dest[x] = srcRow.ApplyCentered(filter,x);
			}
		}
	}

	return tempImage;
}

FloatImagePtr FloatImage_CreateHalfPelFilteredV(const FloatImage *src,const Filter & filter)
{
	FloatImagePtr ptr(const_cast<FloatImage *>(src));
	FloatImagePtr trans = FloatImage::CreateTranspose(ptr);
	trans = FloatImage_CreateHalfPelFilteredH(trans.GetPtr(),filter);
	return FloatImage::CreateTranspose(trans);
}

FloatImagePtr CreateProduct(const FloatImage * im1,const FloatImage * im2,float A )
{
	FloatImagePtr ret = FloatImage::CreateEmpty(im1->GetInfo(),im1->GetDepth());
	ret->FillProduct(im1,im2,A);
	return ret;
}

FloatImagePtr CreateProduct(FloatImagePtr im1,FloatImagePtr im2,float A )
{
	return CreateProduct(im1.GetPtr(),im2.GetPtr(),A);
}

FloatImagePtr CreateSelectOnePlane(FloatImagePtr im1,int p)
{
	FloatImagePtr ret = FloatImage::CreateEmpty(im1->GetInfo(),1);
	ret->CopyPlane(0,im1.GetPtr(),p);
	return ret;
}

FloatImagePtr CreateLinearCombo(float w1,FloatImagePtr im1,float w2,FloatImagePtr im2,float C)
{
	FloatImagePtr ret = FloatImage::CreateEmpty(im1->GetInfo(),im1->GetDepth());
	ret->FillLinearCombo(w1,im1.GetPtr(),w2,im2.GetPtr(),C);
	return ret;
}

cb::FloatImagePtr CreatePlaneSum(const cb::FloatImage * fim)
{
	cb::FloatImagePtr ret = cb::FloatImage::CreateEmpty(fim->GetInfo(),1);
	int w = fim->GetInfo().m_width;
	int h = fim->GetInfo().m_height;
	int d = fim->GetDepth();
	for LOOP(y,h)
	{
		for LOOP(x,w)
		{
			double sum = 0;
			for LOOP(p,d)
			{
				sum += fim->GetSample(p,x,y);
			}
			ret->Sample(0,x,y) = (float) sum;
		}
	}
	return ret;
}

cb::FloatImagePtr CreatePlaneSumSqr(const cb::FloatImage * fim)
{
	cb::FloatImagePtr ret = cb::FloatImage::CreateEmpty(fim->GetInfo(),1);
	int w = fim->GetInfo().m_width;
	int h = fim->GetInfo().m_height;
	int d = fim->GetDepth();
	for LOOP(y,h)
	{
		for LOOP(x,w)
		{
			double sum = 0;
			for LOOP(p,d)
			{
				sum += fsquare( fim->GetSample(p,x,y) );
			}
			ret->Sample(0,x,y) = (float) sum;
		}
	}
	return ret;
}

cb::FloatImagePtr CreatePlaneSumSqrSqrt(const cb::FloatImage * fim)
{
	cb::FloatImagePtr ret = cb::FloatImage::CreateEmpty(fim->GetInfo(),1);
	int w = fim->GetInfo().m_width;
	int h = fim->GetInfo().m_height;
	int d = fim->GetDepth();
	for LOOP(y,h)
	{
		for LOOP(x,w)
		{
			double sum = 0;
			for LOOP(p,d)
			{
				sum += fsquare( fim->GetSample(p,x,y) );
			}
			ret->Sample(0,x,y) = (float) sqrt( sum );
		}
	}
	return ret;
}


END_CB


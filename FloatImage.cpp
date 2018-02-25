#include "FloatImage.h"
#include "cblib/Vec3.h"
#include "cblib/Vec2.h"
#include "cblib/FloatUtil.h"
#include "cblib/Rand.h"
#include "cblib/BmpImage.h"
#include "cblib/File.h"
#include <string.h>

START_CB


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


// src high end points are not inclusive, eg. [0,0,w,h] is the whole thing
FloatImagePtr FloatImage::CreateBilinearResize(const FloatImagePtr & src,const RectI & rect,int toW,int toH)
{
	TextureInfo dstInfo(src->GetInfo());
	dstInfo.m_width = toW;
	dstInfo.m_height = toH;
	FloatImagePtr dst( new FloatImage(dstInfo,src->GetDepth()) );
	
	int d = src->GetDepth();	
	for(int p=0;p<d;p++)
	{
		int w = dst->m_info.m_width;
		int h = dst->m_info.m_height;

		for(int y=0;y<h;y++)
		{
			float dstV = (y + 0.5f)/h;
			float srcV = (rect.LoY() + dstV * rect.Height())/(float)src->GetInfo().m_height;
						
			for(int x=0;x<w;x++)
			{
				float dstU = (x + 0.5f)/w;
				float srcU = (rect.LoX() + dstU * rect.Width())/(float)src->GetInfo().m_width;
			
				dst->m_planes[p][x + y*w] = src->SampleBilinearFiltered(p,Vec2(srcU,srcV));
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
	FloatRow row = GetRow(0,0);
	int ix = row.Index(x);

	FloatRow col = GetColumn(0,0);
	int iy = col.Index(y);
	
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
		return (color.GetR() + color.GetG() + color.GetB()) / 3.f;
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

	ColorF bilerp = LL * ((1-fx)*(1-fy)) + LH * ((1-fx)*fy) + HL*(fx*(1-fy)) + HH*(fx*fy);

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

	float bilerp = LL * ((1-fx)*(1-fy)) + LH * ((1-fx)*fy) + HL*(fx*(1-fy)) + HH*(fx*fy);

	return bilerp;
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

// optimization for gamma of 2.0 :
inline float degamma(const float f)
{
	return f*f;
}
inline float regamma(const float f)
{
	return sqrtf(f);
}

void FloatImage::DeGammaCorrect()
{
	if ( ! m_info.m_gammaCorrected )
		return;

	for(int d=0;d<m_depth;d++)
	{
		float * plane = m_planes[d];
		int size = m_info.m_width*m_info.m_height;
		for(int i=0;i<size;i++)
		{
			ASSERT( plane[i] >= 0.f && plane[i] <= 1.f );
			plane[i] = degamma( plane[i] );
		}
	}

	m_info.m_gammaCorrected = false;
}

void FloatImage::ReGammaCorrect()
{
	if ( m_info.m_gammaCorrected )
		return;

	// do NOT ReGamma for a heightmap or normal map !
	if ( m_info.m_usage == TextureInfo::eUsage_Heightmap ||
		 m_info.m_usage == TextureInfo::eUsage_Normalmap )
		return;

	for(int d=0;d<m_depth;d++)
	{
		float * plane = m_planes[d];
		int size = m_info.m_width*m_info.m_height;
		for(int i=0;i<size;i++)
		{
			ASSERT( plane[i] >= 0.f && plane[i] <= 1.f );
			plane[i] = regamma( plane[i] );
		}
	}

	m_info.m_gammaCorrected = true;
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

#define SHIFT_RIGHT_ROUND_UP( val, shift )	( (val) + (1<<(shift)) - 1 ) >> (shift)

FloatImagePtr FloatImage::CreateDownFiltered(const Filter & Filter) const
{
	//DeGammaCorrect();	
	ASSERT( m_info.m_gammaCorrected == false );
	ASSERT( ! Filter.IsOdd() );

	TextureInfo tempInfo = m_info;
	tempInfo.m_width  = SHIFT_RIGHT_ROUND_UP( m_info.m_width  , Filter.GetLevels() );
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
				int srcX = x << Filter.GetLevels();
				dest[x] = srcRow.ApplyAndClamp(Filter,srcX);
			}
		}
	}

	TextureInfo newInfo = m_info;
	newInfo.m_width  = SHIFT_RIGHT_ROUND_UP( m_info.m_width  , Filter.GetLevels() );
	newInfo.m_height = SHIFT_RIGHT_ROUND_UP( m_info.m_height , Filter.GetLevels() );

	FloatImagePtr newImage( new FloatImage(newInfo,m_depth) );

	for(d=0;d<m_depth;d++)
	{
		for(int x=0;x<newInfo.m_width;x++)
		{
			float * destCol = newImage->m_planes[d] + x;
			FloatRow srcCol = tempImage->GetColumn(d,x);

			for(int y=0;y<newInfo.m_height;y++)
			{
				int srcY = y << Filter.GetLevels();
				destCol[y * newInfo.m_width] = srcCol.ApplyAndClamp(Filter,srcY);
			}
		}
	}

	tempImage = NULL;

	if ( m_info.m_usage == TextureInfo::eUsage_Normalmap )
		newImage->Normalize();

	return newImage;
}

FloatImagePtr FloatImage::CreateSameSizeFiltered(const Filter & Filter) const
{
	//DeGammaCorrect();	
	ASSERT( m_info.m_gammaCorrected == false );	
	ASSERT( Filter.IsOdd() );

	FloatImagePtr tempImage( new FloatImage(m_info,m_depth) );

	// @@@@ : eEdge_Shared not supported yet
	ASSERT( m_info.m_eEdgeMode != TextureInfo::eEdge_Shared );

	// the Filter Apply will start the Filter center at the given pixel
	//	I want the Filter centered on the pixel
	const int offset = - ((Filter.GetCenterWidth()+1)/2 - 1);

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
				dest[x] = srcRow.ApplyAndClamp(Filter,x+offset);
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
				destCol[y * m_info.m_width] = srcCol.ApplyAndClamp(Filter,y+offset);
			}
		}
	}

	tempImage = NULL;

	return newImage;
}

FloatImagePtr FloatImage::CreateDoubledFiltered(const Filter & filter) const
{
	//DeGammaCorrect();	
	ASSERT( m_info.m_gammaCorrected == false );
	ASSERT( filter.IsOdd() );

	// @@@@ : eEdge_Shared not supported yet
	ASSERT( m_info.m_eEdgeMode != TextureInfo::eEdge_Shared );

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

	return newImage->CreateSameSizeFiltered(filter);
	//return newImage;
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

			float dx = ((2.f * W + NW + SW) - (2.f * E + NE + SE)) * 0.25f;
			float dy = ((2.f * S + SW + SE) - (2.f * N + NE + NW)) * 0.25f;

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
	BmpImagePtr im = BmpImage::Create(fileName);

	if ( im == NULL )
		return FloatImagePtr(NULL);

	FloatImagePtr ret = CreateFromBmp(im.GetRef());

	if ( ret != NULL )
	{
		ret->m_info.m_fileName = fileName;
	}
		
	return ret;
}

/*static*/ FloatImagePtr FloatImage::CreateFromBmp(const BmpImage & source)
{
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
			ubyte * ptr = to->m_pData + y * to->m_pitch;
			for(int x=0;x<w;x++)
			{
				float f = GetSample(0,x,y);
				int i = ColorFTOI(f);
				*ptr++ = Clamp(i,0,255);
			}
		}
	}
	else
	{	
		to->Allocate24Bit(w,h);
		for(int y=0;y<h;y++)
		{
			ubyte * ptr = to->m_pData + y * to->m_pitch;
			for(int x=0;x<w;x++)
			{
				ColorF cf = GetPixel(x + y*w);
				ColorDW dw(ColorDW::eFromFloatSafe,cf);
				ptr[0] = dw.GetB();
				ptr[1] = dw.GetG();
				ptr[2] = dw.GetR();
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
		SetPixel(i,cf);
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
		SetPixel(i,cf);
	}
}
	
//-----------------------------------------------------------------

END_CB

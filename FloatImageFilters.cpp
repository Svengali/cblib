#include "FloatImage.h"
#include "BmpImage.h"
#include "FloatImageFilters.h"
#include "ColorConversions.h"
#include "vec2u.h"
#include "vec3u.h"
#include "vector_s.h"
#include "MatNM.h"
#include "stl_basics.h"
#include "Log.h"
#include <algorithm>

START_CB

float GetMedian5x5(const FloatImage * fim,int x,int y,int p)
{
	int w = fim->GetInfo().m_width;
	DURING_ASSERT( int h = fim->GetInfo().m_height );
	
	ASSERT( x >= 2 && x+2 < w );
	ASSERT( y >= 2 && y+2 < h );
	ASSERT( p >= 0 && p < fim->GetDepth() );

	const float * fromP = fim->GetPlane(p) + y * w + x;
			
	// 5x5 neighborhood
	float vals[24];
	vals[0] = fromP[ -w-w-2];
	vals[1] = fromP[ -w-w-1];
	vals[2] = fromP[ -w-w  ];
	vals[3] = fromP[ -w-w+1];
	vals[4] = fromP[ -w-w+2];
	
	vals[5] = fromP[   -w-2];
	vals[6] = fromP[   -w-1];
	vals[7] = fromP[   -w  ];
	vals[8] = fromP[   -w+1];
	vals[9] = fromP[   -w+2];
	
	vals[10] = fromP[     -2];
	vals[11] = fromP[     -1];
	vals[12] = fromP[      0];	
	vals[13] = fromP[     +1];
	vals[14] = fromP[     +2];
	
	vals[15] = fromP[    w-2];
	vals[16] = fromP[    w-1];
	vals[17] = fromP[    w  ];
	vals[18] = fromP[    w+1];
	vals[19] = fromP[    w+2];
	
	vals[20] = fromP[  w+w-2];
	vals[21] = fromP[  w+w-1];
	vals[22] = fromP[  w+w  ];
	vals[23] = fromP[  w+w+1];
	vals[24] = fromP[  w+w+2];
				
	// @@ obviously I could be a lot faster than actually doing a sort
	std::sort(vals,vals+25);
	
	return vals[12];
}

const ColorF GetMedian5x5Color(const FloatImage * fim,int x,int y)
{
	ColorF ret;
	ret.m_r = GetMedian5x5(fim,x,y,0);
	ret.m_g = GetMedian5x5(fim,x,y,1);
	ret.m_b = GetMedian5x5(fim,x,y,2);
	ret.m_a = 1;
	return ret;
}

// note : CreateFiltered_Median5x5 is a NOP on the 2-pixel border
//	where the 5x5 window would go off edge
FloatImagePtr CreateFiltered_Median5x5(FloatImagePtr fim)
{
	FloatImagePtr ret = FloatImage::CreateEmpty(fim->GetInfo(),fim->GetDepth());
	
	int w = fim->GetInfo().m_width;
	int h = fim->GetInfo().m_height;
	
	for(int y=0;y<h;y++)
	{
		for(int x=0;x<w;x++)
		{
			int i = x + y*w;
			
			if ( x >= 2 && y >= 2 && x+2 < w && y+2 < h )
			{
				ret->SetPixel(i, GetMedian5x5Color(fim.GetPtr(),x,y) );
			}
			else
			{		
				ret->SetPixel(i, fim->GetPixel(i) );
			}
		}
	}
	
	return ret;
}


/**

GetBilateralFilter:

get a semi-bilateral filter of the local neighborhood :

"Bilateral" = weight falloff by distance and also by color distance

**/	
const ColorF GetBilateralFilterColor(const FloatImage * from,int x,int y, const Filter & spatialFilter, const ColorF & basePixel, float invSigmaSqr)
{
	// Bilateral filter :
	
	ASSERT( spatialFilter.IsOdd() );
	//ASSERT( spatialFilter.GetCenterWidth() == 1 );
		
	double colorSum[3] = { 0 };
	double weightSum = 0;
	
	//const float distanceWeights[5] = { 0.3f, 0.75f, 1.f, 0.75f, 0.3f };
	//const float * pDistanceWeights = distanceWeights+2;
	
	int radius = (spatialFilter.GetWidth() - 1)/2;
	
	for(int dy=-radius;dy <= radius;dy++)
	{
		for(int dx=-radius;dx <= radius;dx++)
		{
			//if ( dx == 0 && dy == 0 ) continue;
		
			//double distWeight = pDistanceWeights[dx] * pDistanceWeights[dy];
			float distWeight = spatialFilter.GetWeight(dx+radius) * spatialFilter.GetWeight(dy+radius);
		
			ColorF curPixel = from->GetPixelAroundEdge(x+dx,y+dy);
			
			float pixelDsqr = ColorF::DistanceSqr(curPixel,basePixel);
			
			float weight = expf( - pixelDsqr * invSigmaSqr ) * distWeight;
			
			// I hit some denorm 
			if ( weight < FLT_MIN )
				continue;
			
			weightSum += weight;
			colorSum[0] += curPixel.m_r * weight;
			colorSum[1] += curPixel.m_g * weight;
			colorSum[2] += curPixel.m_b * weight;
		}
	}
	
	ASSERT( weightSum != 0.0 );
	
	colorSum[0] *= (1.0/weightSum);
	colorSum[1] *= (1.0/weightSum);
	colorSum[2] *= (1.0/weightSum);
	
	return ColorF((float)colorSum[0],(float)colorSum[1],(float)colorSum[2]);;	
}

const float GetBilateralFilterSample(const FloatImage * from,int p,int x,int y, const Filter & spatialFilter, const float baseSample, float invSigmaSqr)
{
	// Bilateral filter :
	
	ASSERT( spatialFilter.IsOdd() );
	//ASSERT( spatialFilter.GetCenterWidth() == 1 );
	const int radius = (spatialFilter.GetWidth() - 1)/2;
	
	double sum = 0;
	double weightSum = 0;
		
	for(int dy=-radius;dy <= radius;dy++)
	{
		for(int dx=-radius;dx <= radius;dx++)
		{
			//if ( dx == 0 && dy == 0 ) continue;
		
			//double distWeight = pDistanceWeights[dx] * pDistanceWeights[dy];
			float distWeight = spatialFilter.GetWeight(dx+radius) * spatialFilter.GetWeight(dy+radius);
		
			//float curSample = from->GetSampleMirrored(p,x+dx,y+dy);
			float curSample = from->GetSampleClamped(p,x+dx,y+dy);
			
			float pixelDsqr = cb::fsquare( curSample - baseSample );
			
			float weight = expf( - pixelDsqr * invSigmaSqr ) * distWeight;
			
			// I hit some denorm problemss
			//if ( weight < FLT_MIN )
			//	continue;
			
			weightSum += weight;
			sum += curSample * weight;
		}
	}
	
	ASSERT( weightSum != 0.0 );

	return (float)(sum / weightSum);	
}

FloatImagePtr CreateFiltered_Bilateral_Joint(FloatImagePtr fim,const Filter & spatialFilter,const float pixelDistanceScale)
{
	if ( pixelDistanceScale <= 0.f ) return fim;

	FloatImagePtr ret = FloatImage::CreateEmpty(fim->GetInfo(),fim->GetDepth());
		
	// pixelDistanceScale is [0,256] , scale into [0,1] for fim standard
	const float invColorSqrScale = 1.f / fsquare( pixelDistanceScale / 256.f );
	
	int w = fim->GetInfo().m_width;
	int h = fim->GetInfo().m_height;
	
	for(int y=0;y<h;y++)
	{
		for(int x=0;x<w;x++)
		{
			int i = x + y*w;
		
			ColorF basePixel = fim->GetPixel(x,y);
			
			ret->SetPixel(i, GetBilateralFilterColor(fim.GetPtr(),x,y,spatialFilter,basePixel,invColorSqrScale) );
		}
	}
	
	return ret;
}

FloatImagePtr CreateFiltered_Bilateral_Independent(FloatImagePtr fim,const Filter & spatialFilter,const float sampleDistanceScale)
{
	if ( sampleDistanceScale <= 0.f ) return fim;

	FloatImagePtr ret = FloatImage::CreateEmpty(fim->GetInfo(),fim->GetDepth());
		
	// pixelDistanceScale is [0,256] , scale into [0,1] for fim standard
	const float invColorSqrScale = 1.f / fsquare( sampleDistanceScale / 256.f );
	
	int w = fim->GetInfo().m_width;
	int h = fim->GetInfo().m_height;
	
	for(int p=0;p<fim->GetDepth();p++)
	{
		for(int y=0;y<h;y++)
		{
			for(int x=0;x<w;x++)
			{
				int i = x + y*w;
			
				float basePixel = fim->GetSample(p,i);
				
				ret->Sample(p,i) = GetBilateralFilterSample(fim.GetPtr(),p,x,y,spatialFilter,basePixel,invColorSqrScale);
			}
		}
	}
	
	return ret;
}

// Parallelness should return 1 at delta_angle=0,PI,TWOPI and 0 at delta_angle=PI/2,3PI/2,etc.
static inline float Parallelness( float delta_angle )
{
	// @@@ ???? what to use here ?
	
	// original paper uses abs of cos :
	//return fabsf( cosf( delta_angle ) );
	// newer paper uses cos^2 :
	//return fsquare( cosf( delta_angle ) );
	
	// just linear mod :
	delta_angle /= PIf;
	
	while ( delta_angle > 1.f ) delta_angle -= 1.f;
	while ( delta_angle < 0.f ) delta_angle += 1.f;
	if ( delta_angle >= 0.5f ) delta_angle = 1.f - delta_angle;
	ASSERT( delta_angle >= 0.f && delta_angle <= 0.5f);
	float t = 1.f - delta_angle/0.5f;
	// @@ ?? smoothstep ?
	return t;
}

// matrix should be filtw X filtw
void MakeBilateralFilterSample_DirectionalInvSigmaSqr(float * pMatrix,int filtW, float theta0, float sigma_theta0, float sigma_perp )
{
	const int radius = (filtW - 1)/2;
	
	for(int dy=-radius;dy <= radius;dy++)
	{
		for(int dx=-radius;dx <= radius;dx++)
		{
			int mi = ( dx + radius ) + (dy + radius ) * filtW;
			if ( dx == 0 && dy == 0 )
			{
				pMatrix[mi] = 0.f;
			}
			else
			{
				float theta = atan2f((float)dy,(float)dx); // @@ should be a precomputed table
				
				float ct = Parallelness( theta - theta0 );
				
				float sigma = flerp(sigma_perp,sigma_theta0,ct); 
		
				pMatrix[mi] = 0.5f / fsquare(sigma);
			}
		}
	}
}

const float GetBilateralFilterSample_Directional(const FloatImage * from,int p,int x,int y, const Filter & spatialFilter, const float baseSample, 
													const float * pInvSigmaSqrMatrix)
{
	// Bilateral filter :
	
	ASSERT( spatialFilter.IsOdd() );
	//ASSERT( spatialFilter.GetCenterWidth() == 1 );
	const int filtW = spatialFilter.GetWidth();
	const int radius = (filtW - 1)/2;
	
	double sum = 0;
	double weightSum = 0;
		
	for(int dy=-radius;dy <= radius;dy++)
	{
		for(int dx=-radius;dx <= radius;dx++)
		{
			int mi = ( dx + radius ) + (dy + radius ) * filtW;
			
			//double distWeight = pDistanceWeights[dx] * pDistanceWeights[dy];
			double distWeight = spatialFilter.GetWeight(dx+radius) * spatialFilter.GetWeight(dy+radius);
		
			//double curSample = from->GetSampleMirrored(p,x+dx,y+dy);
			double curSample = from->GetSampleClamped(p,x+dx,y+dy);
			
			double dsqr = cb::fsquare( curSample - baseSample );
			
			double exponent = dsqr * pInvSigmaSqrMatrix[mi];
			
			if ( exponent > 40 ) continue;
			
			double weight = exp( - exponent ) * distWeight;
						
			weightSum += weight;
			sum += curSample * weight;
		}
	}
	
	ASSERT( weightSum != 0.0 );

	return (float)(sum / weightSum);	
}

//=========================================================

inline int PlaneIndex(int w,int x,int y) { return w*y + x; }

float GetLaplacian1d( const float * plane, int x, int y, int w )
{
	float MM = plane[ PlaneIndex(w,x-1,y-1) ];
	float MC = plane[ PlaneIndex(w,x-1,y  ) ];
	float MP = plane[ PlaneIndex(w,x-1,y+1) ];
	
	float CM = plane[ PlaneIndex(w,x  ,y-1) ];
	float CC = plane[ PlaneIndex(w,x  ,y  ) ];
	float CP = plane[ PlaneIndex(w,x  ,y+1) ];
	
	float PM = plane[ PlaneIndex(w,x+1,y-1) ];
	float PC = plane[ PlaneIndex(w,x+1,y  ) ];
	float PP = plane[ PlaneIndex(w,x+1,y+1) ];
	
	// weight of direct neighbors should be 1.7 X weight of diagonal neighbors
	// 5 and 3 is pretty close
	//float pred = ((MC + PC + CP + CM) * 3.f + (MM + MP + PM + PP))/16.f;
	float pred = ((MC + PC + CP + CM) * 5.f + (MM + MP + PM + PP)*3.f)/32.f;
	float lap = CC - pred;
		
	return lap;
}

Vec2 GetLaplacianAndVariance1d( const float * plane, int x, int y, int w )
{
	float MM = plane[ PlaneIndex(w,x-1,y-1) ];
	float MC = plane[ PlaneIndex(w,x-1,y  ) ];
	float MP = plane[ PlaneIndex(w,x-1,y+1) ];
	
	float CM = plane[ PlaneIndex(w,x  ,y-1) ];
	float CC = plane[ PlaneIndex(w,x  ,y  ) ];
	float CP = plane[ PlaneIndex(w,x  ,y+1) ];
	
	float PM = plane[ PlaneIndex(w,x+1,y-1) ];
	float PC = plane[ PlaneIndex(w,x+1,y  ) ];
	float PP = plane[ PlaneIndex(w,x+1,y+1) ];
	
	// weight of direct neighbors should be 1.7 X weight of diagonal neighbors
	// 5 and 3 is pretty close
	//float pred = ((MC + PC + CP + CM) * 3.f + (MM + MP + PM + PP))/16.f;
	float pred = ((MC + PC + CP + CM) * 5.f + (MM + MP + PM + PP)*3.f)/32.f;
	float lap = CC - pred;
	
	const float VW = 0.7071068f;
	float variance = (
		fsquare( CC - pred) +
		fsquare( MC - pred)*VW +
		fsquare( CM - pred)*VW +
		fsquare( CP - pred)*VW +
		fsquare( PC - pred)*VW +
		fsquare( MM - pred)*VW*VW +
		fsquare( MP - pred)*VW*VW +
		fsquare( PM - pred)*VW*VW +
		fsquare( PP - pred)*VW*VW )*(1.f/(VW*4 + VW*VW*4 + 1))*(9.f/8.f);
	
	return cb::Vec2(lap,variance);
}

//cb::Vec2 
float GetLaplacian3d( const cb::FloatImage * fim, int x, int y )
{
	cb::Vec3 MM = fim->GetPixel(x-1,y-1).AsVec3();
	cb::Vec3 MC = fim->GetPixel(x-1,y  ).AsVec3();
	cb::Vec3 MP = fim->GetPixel(x-1,y+1).AsVec3();
	
	cb::Vec3 CM = fim->GetPixel(x  ,y-1).AsVec3();
	cb::Vec3 CC = fim->GetPixel(x  ,y  ).AsVec3();
	cb::Vec3 CP = fim->GetPixel(x  ,y+1).AsVec3();
	
	cb::Vec3 PM = fim->GetPixel(x+1,y-1).AsVec3();
	cb::Vec3 PC = fim->GetPixel(x+1,y  ).AsVec3();
	cb::Vec3 PP = fim->GetPixel(x+1,y+1).AsVec3();
	
	cb::Vec3 pred = ((MC + PC + CP + CM) * 5.f + (MM + MP + PM + PP)*3.f)/32.f;
	float lap = cb::Distance( CC , pred );
	
	/*
	float variance = (
		cb::Distance( MM , CC) +
		cb::Distance( MC , CC) +
		cb::Distance( MP , CC) +
		cb::Distance( CM , CC) +
		cb::Distance( CP , CC) +
		cb::Distance( PM , CC) +
		cb::Distance( PC , CC) +
		cb::Distance( PP , CC) )/8.f;
		
	return cb::Vec2(lap,variance);
	/**/
	
	return lap;
}

cb::Vec2 GetGradient_Scharr( const float * plane, int x, int y, int w )
{
	float MM = plane[ PlaneIndex(w,x-1,y-1) ];
	float MC = plane[ PlaneIndex(w,x-1,y  ) ];
	float MP = plane[ PlaneIndex(w,x-1,y+1) ];
	
	float CM = plane[ PlaneIndex(w,x  ,y-1) ];
//	float CC = plane[ PlaneIndex(w,x  ,y  ) ];
	float CP = plane[ PlaneIndex(w,x  ,y+1) ];
	
	float PM = plane[ PlaneIndex(w,x+1,y-1) ];
	float PC = plane[ PlaneIndex(w,x+1,y  ) ];
	float PP = plane[ PlaneIndex(w,x+1,y+1) ];
	
	//Scharr :
	float DX = (3.f/16.f) * (PM + PP - MM - MP) + (10.f/16.f) * (PC - MC);
	float DY = (3.f/16.f) * (MP + PP - MM - PM) + (10.f/16.f) * (CP - CM);

	// note : extra *0.5 because the taps are 2 pixels apart :
	return cb::Vec2(DX*0.5f,DY*0.5f);
}

cb::Vec2 GetGradient_Sobel( const float * plane, int x, int y, int w )
{
	float MM = plane[ PlaneIndex(w,x-1,y-1) ];
	float MC = plane[ PlaneIndex(w,x-1,y  ) ];
	float MP = plane[ PlaneIndex(w,x-1,y+1) ];
	
	float CM = plane[ PlaneIndex(w,x  ,y-1) ];
//	float CC = plane[ PlaneIndex(w,x  ,y  ) ];
	float CP = plane[ PlaneIndex(w,x  ,y+1) ];
	
	float PM = plane[ PlaneIndex(w,x+1,y-1) ];
	float PC = plane[ PlaneIndex(w,x+1,y  ) ];
	float PP = plane[ PlaneIndex(w,x+1,y+1) ];
	
	// Sobel :
	float DX = (PM + PP - MM - MP)*(1.f/4.f) + (2.f/4.f) * (PC - MC);
	float DY = (MP + PP - MM - PM)*(1.f/4.f) + (2.f/4.f) * (CP - CM);
	
	// note : extra *0.5 because the taps are 2 pixels apart :
	return cb::Vec2(DX*0.5f,DY*0.5f);
}

//=========================================================

// MakeGradientMap : returns 2 channel with Grad XY in channels
FloatImagePtr MakeGradientMap_Scharr(const FloatImage * fim, int p)
{
	int w = fim->GetInfo().m_width;
	int h = fim->GetInfo().m_height;
	const float * plane = fim->GetPlane(p);
	
	cb::FloatImagePtr ret = FloatImage::CreateEmpty(fim->GetInfo(),2);
	ret->FillZero();
	
	for(int y=1;y<h-1;y++)
	{
		for(int x=1;x<w-1;x++)
		{
			const Vec2 G = GetGradient_Scharr(plane,x,y,w);
			ret->Sample(0,x,y) = G.x;
			ret->Sample(1,x,y) = G.y;
		}
	}
	
	return ret;
}
FloatImagePtr MakeGradientMap_Sobel(const FloatImage * fim, int p)
{
	int w = fim->GetInfo().m_width;
	int h = fim->GetInfo().m_height;
	const float * plane = fim->GetPlane(p);
	
	cb::FloatImagePtr ret = FloatImage::CreateEmpty(fim->GetInfo(),2);
	ret->FillZero();
	
	for(int y=1;y<h-1;y++)
	{
		for(int x=1;x<w-1;x++)
		{
			const Vec2 G = GetGradient_Sobel(plane,x,y,w);
			ret->Sample(0,x,y) = G.x;
			ret->Sample(1,x,y) = G.y;
		}
	}
	
	return ret;
}

FloatImagePtr MakeGradientMap_BoxDown(const FloatImage * fim, int p)
{
	TextureInfo retInfo = fim->GetInfo();
	retInfo.m_width  /= 2;
	retInfo.m_height /= 2;
	
	cb::FloatImagePtr ret = FloatImage::CreateEmpty(retInfo,2);

	for(int y=0;y<retInfo.m_height;y++)
	{
		for(int x=0;x<retInfo.m_width;x++)
		{
			float NW = fim->GetSample(p, 2*x  , 2*y );
			float NE = fim->GetSample(p, 2*x+1, 2*y );
			float SW = fim->GetSample(p, 2*x  , 2*y+1 );
			float SE = fim->GetSample(p, 2*x+1, 2*y+1 );

			/**
			
			trying to match the scale of normal centered [-1,0,1] or Sobel type taps
			the Sobel is two pixels apart in space
			so dx = sample(1) - sample(-1) / 2
			my taps here are only 0.5 apart
			so dx = (NE - NW) / 0.5
			
			**/		
			
			// *0.5 to average the two samples and /0.5 for sample spacing :
			float dx = ((NE - NW) + (SE - SW));
			float dy = ((SE - NE) + (SW - NW));
			
			ret->Sample(0,x,y) = dx;
			ret->Sample(1,x,y) = dy;
		}
	}
	
	return ret;
}

cb::BmpImagePtr MakeVisualEdgeMap(
	cb::FloatImage * fim1,
	int p,
	float edgeThreshLo, float edgeThreshHi)
{

	/*
	Vec3 lo(FLT_MAX,FLT_MAX,FLT_MAX),hi(-FLT_MAX,-FLT_MAX,-FLT_MAX);
	for(int r=0;r<256;r++)
	{
		for(int g=0;g<256;g++)
		{
			for(int b=0;b<256;b++)
			{
				ColorDW cdw(r,g,b);

				//ColorDW l = ColorTransformYCoCgWrap(cdw);
				//ColorDW out = ColorTransformYCoCgWrapInverse(l);

				ColorF cf(cdw);
				Vec3 lab = SRGB_to_LAB(cf.AsVec3());
				lab = LAB_to_LCH(lab);
				
				lo.SetMin(lab);
				hi.SetMax(lab);

				lab = LCH_to_LAB(lab);
				Vec3 inv = LAB_to_SRGB(lab);
				
				ColorDW out(ColorDW::eFromFloatSafe,ColorF(inv));
				
				ASSERT_RELEASE( cdw == out );
			}
		}
	}
	
	lo.Log();
	hi.Log();
	/**/

	int w = fim1->GetInfo().m_width;
	int h = fim1->GetInfo().m_height;
	const float * plane = fim1->GetPlane(p);
	
	BmpImagePtr bmp = BmpImage::CreateEmpty24b(w,h);
	bmp->FillZero();
	
	edgeThreshLo /= 256.f;
	edgeThreshHi /= 256.f;
	ASSERT( edgeThreshLo >= 0.f );
	ASSERT( edgeThreshHi > edgeThreshLo );
	
	for(int y=1;y<h-1;y++)
	{
		for(int x=1;x<w-1;x++)
		{
			const Vec2 G = GetGradient_Scharr(plane,x,y,w);
			float Dsqr = G.LengthSqr();
			
			float t = (sqrtf(Dsqr) - edgeThreshLo)/(edgeThreshHi - edgeThreshLo);
			if ( t <= 0.f )
				continue;
			
			/*	
			int strength = 256;
			if ( t < 1.f )
			{
				//t = cb::fHermiteLerpParameter(t);
				strength = cb::froundint( t * 256.f );
			}
			/**/
			
			// t is the magnitude of the edge in [0,1]
			t = MIN(t,1.f);
			
			float angle = atan2f(G.y,G.x);

			// turn the angle of the edge into a hue :
			float unitHue = (angle + PIf)/TWO_PIf;
			
			// normal "HSV" is not very brightness preserving
			//  so you will see apparent strength variations due to angle variations
			// try to do better ...
			
			/*
			cb::ColorDW c;
			int intHue = cb::froundint( 6 * 256.f * unitHue );
			c.SetFromHueFast(intHue);
			
			//c *= strength;
			c.Set( 
				(c.GetR()*strength)>>8,
				(c.GetG()*strength)>>8,
				(c.GetB()*strength)>>8 );
			/**/
			
			//*
			ColorF cf;
			cf.SetFromHue(unitHue);
			cf *= t; // scale RGB magnitude by t
			cf = MakeYUVFromRGB(cf);
			cf.m_r = t; // force Y to t
			cf = MakeRGBFromYUV(cf);
			/**/
			
			/*
			// try LCH color space :
			Vec3 lch;
			lch.x = t;
			lch.y = t; // C ?
			lch.z = unitHue;
			
			Vec3 srgb = LAB_to_SRGB( LCH_to_LAB(lch) );
			srgb = Linear_To_SRGB(srgb);
			
			ColorF cf(srgb);
			/**/
			
			//ColorF cf(t,t,t);
			//cf.MutableVec3() = Linear_To_SRGB(cf.AsVec3());
			
			ColorDW c;
			c.SetFloatSafe(cf);
					
			bmp->SetColor(x,y, c);
		}
	}
	
	return bmp;
}


FloatImagePtr CreateLocalVarianceMap(FloatImagePtr fim,const Filter & spatialFilter)
{
	FloatImagePtr fimsq = CreateProduct(fim,fim);

	FloatImagePtr fim_filt = fim->CreateSameSizeFiltered(spatialFilter);
	FloatImagePtr fimsq_filt = fimsq->CreateSameSizeFiltered(spatialFilter);
	
	int width  = fim->GetInfo().m_width;
	int height = fim->GetInfo().m_height;

	int depth = fim->GetDepth();
	FloatImagePtr ret = FloatImage::CreateEmpty(fim->GetInfo(),depth);
	
	for(int d=0;d<depth;d++)
	{
		const float * plane_x = fim_filt->GetPlane(d);
		const float * plane_xx = fimsq_filt->GetPlane(d);
		float * plane_ret = ret->Plane(d);
			
		for LOOP(y,height)
		{
			for LOOP(x,width)
			{
				int i = x + y * width;
			
				double avg = plane_x[i];
				double avgSqr = plane_xx[i];
				
				double var = avgSqr - avg*avg;

				plane_ret[i] = MAX((float)var,0.f);		
			}
		}
	}
	
	return ret;
}


FloatImagePtr MakeFiltered_3x3(const FloatImage * fim, const float * taps3x3 )
{
	int w = fim->GetInfo().m_width;
	int h = fim->GetInfo().m_height;
	int d = fim->GetDepth();
	
	cb::FloatImagePtr ret = FloatImage::CreateEmpty(fim->GetInfo(),d);
	
	for LOOP(p,d)
	{
		for LOOP(y,h)
		{
			for LOOP(x,w)
			{
				double out = 0;
				
				out += taps3x3[0] * fim->GetSampleMirrored(p,x-1,y-1);
				out += taps3x3[1] * fim->GetSampleMirrored(p,x  ,y-1);
				out += taps3x3[2] * fim->GetSampleMirrored(p,x+1,y-1);
				
				out += taps3x3[3] * fim->GetSampleMirrored(p,x-1,y  );
				out += taps3x3[4] * fim->GetSampleMirrored(p,x  ,y  );
				out += taps3x3[5] * fim->GetSampleMirrored(p,x+1,y  );
				
				out += taps3x3[6] * fim->GetSampleMirrored(p,x-1,y+1);
				out += taps3x3[7] * fim->GetSampleMirrored(p,x  ,y+1);
				out += taps3x3[8] * fim->GetSampleMirrored(p,x+1,y+1);
				
				ret->Sample(p,x,y) = (float) out;
			}
		}
	}
		
	return ret;
}

FloatImagePtr MakeFiltered_5x5(const FloatImage * fim, const float * taps5x5 )
{
	int w = fim->GetInfo().m_width;
	int h = fim->GetInfo().m_height;
	int d = fim->GetDepth();
	
	cb::FloatImagePtr ret = FloatImage::CreateEmpty(fim->GetInfo(),d);
	
	for LOOP(p,d)
	{
		for LOOP(y,h)
		{
			for LOOP(x,w)
			{
				double out = 0;

				int di = 0;
				for (int dy=-2;dy<=2;dy++)
					for (int dx=-2;dx<=2;dx++)
						out += taps5x5[di++] * fim->GetSampleMirrored(p,x+dx,y+dy);
				
				ASSERT( di == 25 );
				
				ret->Sample(p,x,y) = (float) out;
			}
		}
	}
		
	return ret;
}

// Kroon3x3 is normalized so half sum is 0.5
const float c_Kroon3x3_DY[9] = { -17/190.f, -61/190.f, -17/190.f, 0,0,0, 17/190.f, 61/190.f, 17/190.f };

// half sum = 0.552500
const float c_Kroon5x5_DY[25] = { 
	-0.0007f, -0.0052f, -0.0370f, -0.0052f, -0.0007f,
	-0.0037f, -0.1187f, -0.2589f, -0.1187f, -0.0037f,
	0.f,0.f,0.f,0.f,0.f,
	0.0037f, 0.1187f, 0.2589f, 0.1187f, 0.0037f,
	0.0007f, 0.0052f, 0.0370f, 0.0052f, 0.0007f
};

void SimpleFilter_Normalize(float * taps, int dim, float desired_sum /*= 1.f*/)
{
	int count = dim*dim;
	double sum = sumfloat(taps,taps+count);
	float scale = (float)(desired_sum / sum);
	for LOOP(i,count)
		taps[i] *= scale;
}

void SimpleFilter_Transpose(float * to, const float * from, int dim)
{
	for LOOP(y,dim)
	{
		for LOOP(x,dim)
		{
			to[ x + y * dim ] = from[ y + x * dim ];
		}
	}
}

FloatImagePtr MakeGradientMap_Kroon3x3(const FloatImage * fim, int p)
{
	float c_Kroon3x3_DX[3*3];
	SimpleFilter_Transpose(c_Kroon3x3_DX,c_Kroon3x3_DY,3);
	
	FloatImagePtr fimDX = MakeFiltered_3x3(fim, c_Kroon3x3_DX);
	FloatImagePtr fimDY = MakeFiltered_3x3(fim, c_Kroon3x3_DY);
	
	cb::FloatImagePtr ret = FloatImage::CreateEmpty(fim->GetInfo(),2);
	
	ret->CopyPlane(0, fimDX.GetPtr(), p);
	ret->CopyPlane(1, fimDY.GetPtr(), p);
	
	return ret;
}

FloatImagePtr MakeGradientMap_Kroon5x5(const FloatImage * fim, int p)
{
	float c_Kroon5x5_DX[5*5];
	SimpleFilter_Transpose(c_Kroon5x5_DX,c_Kroon5x5_DY,5);
	
	// half sum = 0.552500
	//lprintf("half sum = %a\n",sumfloat((const float *)c_Kroon5x5_DY,c_Kroon5x5_DY+10));
	
	FloatImagePtr fimDX = MakeFiltered_5x5(fim, c_Kroon5x5_DX);
	FloatImagePtr fimDY = MakeFiltered_5x5(fim, c_Kroon5x5_DY);
	
	cb::FloatImagePtr ret = FloatImage::CreateEmpty(fim->GetInfo(),2);
	
	ret->CopyPlane(0, fimDX.GetPtr(), p);
	ret->CopyPlane(1, fimDY.GetPtr(), p);
	
	return ret;
}


FloatImagePtr CreateFilteredResizeH(FloatImagePtr src, int newW,
					float filterCenterWidth,
					const FilterGenerator & filter)
{
	int srcW = src->GetInfo().m_width;
	int srcH = src->GetInfo().m_height;
	int depth = src->GetDepth();

	//if ( newW == srcW ) return src;
	
	TextureInfo newInfo = src->GetInfo();
	newInfo.m_width = newW;
	FloatImagePtr ret = FloatImage::CreateEmpty(newInfo,depth);
	
	float filterFullWidth = filterCenterWidth * filter.baseWidth;
	
	for LOOP(d,depth)
	{
		for LOOP(y,srcH)
		{
			//FloatRow row = src->GetRow(d,y);
			float * pRow = & src->Sample(d,0,y);

			// iterate x in the *target*
			for LOOP(x,newW)
			{
				// apply filter on the *source* :
				float srcCenterX = (x + 0.5f) * srcW / newW;
				
				double out = ApplyFilter(pRow,srcW,srcCenterX,filterCenterWidth,filterFullWidth,filter.pulser,filter.windower);
			
				ret->Sample(d,x,y) = (float) out;
			}
		}
	}

	return ret;	
}

FloatImagePtr CreateResizeFractionalFilterH(FloatImagePtr src, int newW,
					const FilterGenerator & filter,
					float maxCenterWidth)
{
	if ( newW <= src->GetWidth() )
	{
		float filterCenterWidth = (float) src->GetWidth() / newW;
		return CreateFilteredResizeH(src,newW,filterCenterWidth,filter);
	}
	else
	{
		//return CreateExpandH_Additive(src,newW,pulser,windower,widthMultiplier);
		//float filterCenterWidth = 1.f; // I believe this is right, and maximally sharp
		//float filterCenterWidth = 1.5f; // this is pretty sharp and also not pixelly
		//	no need to go past 1.5
		
		float filterCenterWidth = (float) newW / src->GetWidth();
		filterCenterWidth = MIN(filterCenterWidth,maxCenterWidth);
		ASSERT( filterCenterWidth >= 1.f );
		
		return CreateFilteredResizeH(src,newW,filterCenterWidth,filter);
	}
}
					
FloatImagePtr CreateResizeFractionalFilter(FloatImagePtr src, int newW, int newH,
					const FilterGenerator & filter,
					float maxCenterWidth)
{
	FloatImagePtr fim = src;
	
	// @@ same size filter is not actually a nop
	//	should I pretend it is?
	//if ( newW != fim->GetWidth() )
	{
		fim = CreateResizeFractionalFilterH(fim,newW,filter,maxCenterWidth);
	}
	
	// @@ same size filter is not actually a nop
	//	should I pretend it is?
	//if ( newH != fim->GetHeight() )
	{
		// to do columns :
		// transpose, do row filter, transpose back
	
		fim = FloatImage::CreateTranspose(fim);
		
		fim = CreateResizeFractionalFilterH(fim,newH,filter,maxCenterWidth);

		fim = FloatImage::CreateTranspose(fim);
	}
	
	return fim;
}

/*

CreateDoubledFilterGenerator
makes the same output as CreateResizeFractionalFilter
but much faster

CreateIntegerExpandFilterGenerator does the same thing but is more general

/**/
FloatImagePtr CreateDoubledFilterGenerator(FloatImagePtr fim,const FilterGenerator & generator,
											float centerWidth ,
											bool doH, bool doV)
{
	float fwf = generator.baseWidth * centerWidth;
	int hw = ftoi_ceil(fwf/2);
	int fw = hw*2;
	
	Filter  left(Filter::eEmpty, 1,hw);
	Filter right(Filter::eEmpty, 1,hw);
	
	ASSERT( left.GetWidth() == fw );
	
	for(int x=-hw;x<hw;x++)
	{
		right.m_data[x+hw] = (float) SampleFilterGenerator(x+0.25,generator,centerWidth);
	}
	right.Normalize();

	//lprintfCFloatArray(right.m_data,fw,"right",80,8,6);

	for(int i=0;i<fw;i++)
	{
		left.m_data[i] = right.m_data[fw-i-1];
	}
	
	//lprintfCFloatArray(left.m_data,fw,"left",80,8,6);

	int depth = fim->GetDepth();
	const int offset = - hw + 1;

	if ( doH )
	{	
		const TextureInfo srcInfo = fim->GetInfo();
		
		TextureInfo tempInfo = srcInfo;
		tempInfo.m_width *= 2;
		FloatImagePtr tempImage( new FloatImage(tempInfo,depth) );

		// first fill tempImage by filtering in X only
		for(int d=0;d<depth;d++)
		{
			for(int y=0;y<srcInfo.m_height;y++)
			{
				float * dest = tempImage->m_planes[d] + y * tempInfo.m_width;
				FloatRow srcRow = fim->GetRow(d,y);

				for(int x=0;x<srcInfo.m_width;x++)
				{
					dest[2*x+0] = srcRow.ApplyBase(right,x+offset-1);
					dest[2*x+1] = srcRow.ApplyBase(left ,x+offset);
				}
			}
		}
		
		fim = tempImage;
	}
	
	if ( doV )
	{
		const TextureInfo srcInfo = fim->GetInfo();
		TextureInfo newInfo = srcInfo;
		newInfo.m_height *= 2;
		FloatImagePtr newImage( new FloatImage(newInfo,depth) );
		
		int destStride = newInfo.m_width;

		// first fill newImage by filtering in X only
		for(int d=0;d<depth;d++)
		{
			for(int x=0;x<srcInfo.m_width;x++)
			{
				float * dest = newImage->m_planes[d] + x;
				FloatRow tempRow = fim->GetColumn(d,x);

				for(int y=0;y<srcInfo.m_height;y++)
				{
					dest[(2*y+0)*destStride] = tempRow.ApplyBase(right,y+offset-1);
					dest[(2*y+1)*destStride] = tempRow.ApplyBase(left ,y+offset);
				}
			}
		}
		
		fim = newImage;
	}
	
	return fim;
}


FloatImagePtr FloatImage_CreateResized_Old(const FloatImagePtr & in_src,int toW,int toH)
{
	FloatImagePtr src = in_src;
	
	//src->ConvertRGBtoYUV(); // ??
	//src->DeGammaCorrect();

	// do double & half steps with a filter
	while( toW >= src->GetInfo().m_width*2 )
	{
		//OddMitchellFilterT<0> upfilter; // less blurry
		//OddMitchellFilter1 upfilter; // a bit too blurry
		//OddMitchellFilter2 upfilter; // way over-blurring
		//OddSincFilter upfilter;
		OddLanczosFilter upfilter;
		
		FloatImagePtr doubled = FloatImage_CreateDoubled_BoxThenSameSizeFiltered(src,upfilter);
		src = doubled;
	}
	while( toW <= src->GetInfo().m_width/2 )
	{
		BoxFilter downfilter(1);
		FloatImagePtr halved = src->CreateDownFiltered(downfilter);
		src = halved;
	}
	
	FloatImagePtr ret;
	
	if ( src->GetInfo().m_width == toW &&
		 src->GetInfo().m_height == toH )
	{
		// if it was an exact double, we're done
		ret = src;
	}
	else
	{
		// do fractional part
		// then just bilerp to finish
		//	this is kind of retarded but w/e
		ret = FloatImage::CreateBilinearResize(src,toW,toH);
	}
	
	//ret->ReGammaCorrect();
	//ret->ConvertYUVtoRGB(); // ??
	
	return ret;
}

FloatImagePtr FloatImage_CreateDoubled_BoxThenSameSizeFiltered(const FloatImagePtr & src,const Filter & filter)
{
	// box upsample, then in-place odd-filter :
	//	this is okay, but actually makes the image blurrier than necessary
	//	essentially the filter has been convolved with an extra box filter

	FloatImagePtr newImage = src->CreateDoubledBoxFilter();

	return newImage->CreateSameSizeFiltered(filter);
}

//===========================================================================

// Shrink/Expand by an integer ratio :
//  this should produce the same output as CreateResizeFractionalFilter
//	but it's much faster because it uses simple filtering
	
FloatImagePtr CreateIntegerExpandFilterGeneratorH(FloatImagePtr fim,
											int expansion,
											const FilterGenerator & generator,
											float centerWidth )
{
	if ( expansion <= 1 ) return fim;

	// integer expansion can be done by precomputing a Filter for each
	//	destination offset
	// this should be just like CreateResizeFractionalFilter
	//	except the fractional part of the offset repeats, so we precompute them all
	// and make iscrete filters

	float fwf = generator.baseWidth * centerWidth;
	int hw = ftoi_ceil(fwf/2);
	
	// can't use a regular vector because Filter can't be moved :
	vector_s<Filter,32> filters;
	ASSERT_RELEASE( expansion <= filters.capacity() );
	filters.resize(expansion);
	
	double tapStep = 1.0/expansion;
	double firstStep = -0.5 + 0.5*tapStep;
	
	for(int i=0;i<expansion;i++)
	{
		filters[i].Init(true,1,hw,Filter::pulse_unity,Filter::window_unity);
	
		ASSERT( filters[i].m_width == hw*2+1);
	
		double offset = firstStep+tapStep*i;
		offset = - offset;
		for(int x=-hw;x<=hw;x++)
		{
			filters[i].m_data[x+hw] = (float) SampleFilterGenerator(x+offset,generator,centerWidth);
		}
		filters[i].Normalize();
	
		// this is okay because we use applycentered :
		filters[i].CutZeroTails();
		
		//lprintf("%d : ",i);
		//filters[i].LogWeights();
	}
	

	int depth = fim->GetDepth();
	//const int offset = - hw; // + 1;

	const TextureInfo srcInfo = fim->GetInfo();
	
	TextureInfo tempInfo = srcInfo;
	tempInfo.m_width *= expansion;
	FloatImagePtr tempImage( new FloatImage(tempInfo,depth) );

	// first fill tempImage by filtering in X only
	for(int d=0;d<depth;d++)
	{
		for(int y=0;y<srcInfo.m_height;y++)
		{
			float * dest = tempImage->m_planes[d] + y * tempInfo.m_width;
			FloatRow srcRow = fim->GetRow(d,y);

			for(int x=0;x<srcInfo.m_width;x++)
			{
				for(int i=0;i<expansion;i++)
				{
					dest[expansion*x+i] = srcRow.ApplyCentered(filters[i],x);
				}
			}
		}
	}
	
	return tempImage;
}

FloatImagePtr CreateIntegerExpandFilterGenerator(FloatImagePtr fim,
											int expansion,
											const FilterGenerator & generator,
											float centerWidth )
{
	fim = CreateIntegerExpandFilterGeneratorH(fim,expansion,generator,centerWidth);
	fim = FloatImage::CreateTranspose(fim);
	fim = CreateIntegerExpandFilterGeneratorH(fim,expansion,generator,centerWidth);
	fim = FloatImage::CreateTranspose(fim);
	return fim;
}

FloatImagePtr CreateIntegerShrinkFilterGeneratorH(FloatImagePtr fim,
											int factor,
											const FilterGenerator & generator)
{
	if ( factor <= 1 ) return fim;

	// just make a single filter with center width = shrink factor

	float centerWidth = (float)factor;
	int fwf = generator.baseWidth * factor;
	int hw = (fwf/2);
	
	int filterWidth = 2*hw + (factor&1);
	//int sideWidth = (filterWidth - factor)/2;
	//ASSERT( filterWidth == sideWidth*2 + factor );
	
	Filter filter;
	filter.Init(filterWidth,centerWidth,generator.pulser,generator.windower);
	
	filter.CutZeroTails();
	
	// must compute offset after CutZeroTails :
	ASSERT( filter.m_width >= factor );
	int sideWidth = (filter.m_width - factor)/2;
	ASSERT( filter.m_width == sideWidth*2 + factor );
	
	// info :
	//filter.LogWeights();	
	
	int depth = fim->GetDepth();
	const TextureInfo srcInfo = fim->GetInfo();
	
	TextureInfo tempInfo = srcInfo;
	tempInfo.m_width /= factor;
	FloatImagePtr tempImage( new FloatImage(tempInfo,depth) );

	// first fill tempImage by filtering in X only
	for(int d=0;d<depth;d++)
	{
		for(int y=0;y<srcInfo.m_height;y++)
		{
			float * dest = tempImage->m_planes[d] + y * tempInfo.m_width;
			FloatRow srcRow = fim->GetRow(d,y);

			// iterate on dest x : 
			for(int x=0;x<tempInfo.m_width;x++)
			{
				// apply the beginning of the "center" at x*factor
				dest[x] = srcRow.ApplyBase(filter,x*factor - sideWidth);
			}
		}
	}
	
	return tempImage;
}

FloatImagePtr CreateIntegerShrinkFilterGenerator(FloatImagePtr fim,
											int expansion,
											const FilterGenerator & generator)
{
	fim = CreateIntegerShrinkFilterGeneratorH(fim,expansion,generator);
	fim = FloatImage::CreateTranspose(fim);
	fim = CreateIntegerShrinkFilterGeneratorH(fim,expansion,generator);
	fim = FloatImage::CreateTranspose(fim);
	return fim;
}

FloatImagePtr CreateResizeFractionalFilter_Standard(FloatImagePtr src, int newW, int newH)
{
	return CreateResizeFractionalFilter(src,newW,newH,FLOATFILTER_STANDARD_RESIZEFILTER,FLOATFILTER_STANDARD_MAXCENTERWIDTH);
}

FloatImagePtr CreateDoubledFilterGenerator_Standard(FloatImagePtr src,
											bool doH, bool doV)
{
	return CreateDoubledFilterGenerator(src,FLOATFILTER_STANDARD_RESIZEFILTER,FLOATFILTER_STANDARD_MAXCENTERWIDTH,doH,doV);
}

#define ANALYSIS_DIM 9 // limits width in a way that's probably good
//#define ANALYSIS_DIM 11 // a bit slow with my shitty inverse code

// make a discrete analysis filter which will let you interpolate with any synthesis filter :

void MakeAnalysisForSynthesis(Filter * pAnalysis,const Filter & synthesis)
{
	int synth_halfWidth = synthesis.m_width/2;
	
	MatN<ANALYSIS_DIM> synthMat;

	for LOOP(i,ANALYSIS_DIM)
	{
		for LOOP(j,ANALYSIS_DIM)
		{
			int d = abs(i-j);
			// wrap edge condition provides nicest symmetry :
			if ( d > ANALYSIS_DIM/2) d = ANALYSIS_DIM - d;
			
			int f = d + synth_halfWidth;
			
			if ( f >= 0 && f < synthesis.m_width )
				synthMat.Element(i,j) = synthesis.m_data[f];
			else
				synthMat.Element(i,j) = 0.f;
		}
	}

	//LogMatN(synthMat);

	#if 0

	MatN<ANALYSIS_DIM> synthInv;
	synthMat.GetInverse(&synthInv);

	//LogMatN(synthInv);

	/*
	MatN<ANALYSIS_DIM> prod;
	prod.SetProduct(synthMat,synthInv);

	LogMatN(prod);
	/**/

	float * synthInvRow = synthInv.MutableRow(ANALYSIS_DIM/2).GetData();
	
	#else
	
	VecN<ANALYSIS_DIM> synthInv;
	
	synthMat.GetInverseOneRow(ANALYSIS_DIM/2,&synthInv);

	float * synthInvRow = synthInv.GetData();
		
	#endif
	
	//lprintfCFloatArray(synthInvRow,ANALYSIS_DIM,"synthInvRow",ANALYSIS_DIM,8,5);

	Filter analysis(Filter::eEmpty,Filter::eOdd,0,ANALYSIS_DIM);
	ASSERT( analysis.m_width == ANALYSIS_DIM );
	memcpy(analysis.m_data,synthInvRow,sizeof(float)*ANALYSIS_DIM);
	
	pAnalysis->Swap(analysis);
}

void MakeAnalysisForSynthesisGenerator(Filter * pAnalysis,const FilterGenerator & synthesisFG)
{
	Filter synthesis(Filter::eOdd,0,synthesisFG);
	MakeAnalysisForSynthesis(pAnalysis,synthesis);
}
											
//=================================================================

END_CB

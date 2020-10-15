#pragma once

#include "FloatImage.h"
#include "Filter.h"
#include "BmpImage.h"
#include "vec2.h"
#include "vec3.h"

START_CB

// note : GetBilateralFilterColor internally does a /256 on pixelDistanceScale so that
//	pixelDistanceScale is in [0,256] even though fim is in [0,1]

const ColorF GetBilateralFilterColor(const FloatImage * from,int x,int y, const Filter & spatialFilter, const ColorF & basePixel, float invSigmaSqr);
const float  GetBilateralFilterSample(const FloatImage * from,int p,int x,int y, const Filter & spatialFilter, const float baseSample, float invSigmaSqr);

// note : CreateFiltered_Median5x5 is a NOP on the 2-pixel border
//	where the 5x5 window would go off edge
const ColorF GetMedian5x5Color(const FloatImage * fim,int x,int y);
float GetMedian5x5(const FloatImage * fim,int x,int y,int p);

FloatImagePtr CreateFiltered_Median5x5(FloatImagePtr fim);

// pixelDistanceScale is a full *color* difference , in [0,256]
FloatImagePtr CreateFiltered_Bilateral_Joint(FloatImagePtr fim,const Filter & spatialFilter,const float pixelDistanceScale);
// sampleDistanceScale is a *sample* differene , in [0,256]
FloatImagePtr CreateFiltered_Bilateral_Independent(FloatImagePtr fim,const Filter & spatialFilter,const float sampleDistanceScale);



// sigma here is [0,1] not [0,256] like the others
void MakeBilateralFilterSample_DirectionalInvSigmaSqr(float * pMatrix,int filtW, float theta0, float sigma_theta0, float sigma_perp );
const float GetBilateralFilterSample_Directional(const FloatImage * from,int p,int x,int y, const Filter & spatialFilter, const float baseSample, 
													const float * pInvSigmaSqrMatrix);
													

// warning - these do no clamp at edge, you must ensure you don't cross edge :
cb::Vec2 GetGradient_Sobel(  const float * plane, int x, int y, int w );
cb::Vec2 GetGradient_Scharr( const float * plane, int x, int y, int w );

// signed laplacian = delta from 1-ring pred :
float GetLaplacian1d( const float * plane, int x, int y, int w );
float GetLaplacian3d( const cb::FloatImage * fim, int x, int y );

// ret.x = laplacian, ret.y = local variance
Vec2 GetLaplacianAndVariance1d( const float * plane, int x, int y, int w );

// MakeGradientMap : returns 2 channel with Grad XY in channels
//	 use CreatePlaneSumSqrSqrt to get gradient magnitude
FloatImagePtr MakeGradientMap_Scharr(const FloatImage * fim, int p);
FloatImagePtr MakeGradientMap_Sobel( const FloatImage * fim, int p);
FloatImagePtr MakeGradientMap_BoxDown(const FloatImage * fim, int p); // ret is half size of fim

// edge thresholds are in [0,256]
cb::BmpImagePtr MakeVisualEdgeMap(cb::FloatImage * fim1,int p, float edgeThreshLo, float edgeThreshHi);

// CreateLocalVarianceMap : works on all planes independently
//	eg. returns plane R = variance in R channel
//	use CreatePlaneSum to make a total variance
FloatImagePtr CreateLocalVarianceMap(FloatImagePtr fim,const Filter & spatialFilter);
	
FloatImagePtr MakeFiltered_3x3(const FloatImage * fim, const float * taps3x3 );
FloatImagePtr MakeFiltered_5x5(const FloatImage * fim, const float * taps5x5 );

void SimpleFilter_Transpose(float * to, const float * from, int dim);
void SimpleFilter_Normalize(float * taps, int dim, float desired_sum = 1.f);

FloatImagePtr MakeGradientMap_Kroon3x3(const FloatImage * fim, int p);
FloatImagePtr MakeGradientMap_Kroon5x5(const FloatImage * fim, int p);

// this seems good :
// centerWidth around 1.15 seems ideal
// centerWidth should be >= 1.0 and <= 1.5
#define FLOATFILTER_STANDARD_MAXCENTERWIDTH	(1.15f)
#define FLOATFILTER_STANDARD_RESIZEFILTER	c_filterGenerator_mitchell1

// arbitrary ratio resizes :
//	VERY SLOW ! calls function pointers in the generator
FloatImagePtr CreateFilteredResizeH(FloatImagePtr src, int newW,
					float filterCenterWidth,
					const FilterGenerator & filter);
					
FloatImagePtr CreateResizeFractionalFilterH(FloatImagePtr src, int newW,
					const FilterGenerator & filter,
					float maxCenterWidth);
					
FloatImagePtr CreateResizeFractionalFilter(FloatImagePtr src, int newW, int newH,
					const FilterGenerator & filter,
					float maxCenterWidth);

FloatImagePtr CreateResizeFractionalFilter_Standard(FloatImagePtr src, int newW, int newH);

// good resizes by integer ratios ; either up or down :
//	reasonable fast
FloatImagePtr CreateIntegerShrinkFilterGenerator(FloatImagePtr fim,
											int expansion,
											const FilterGenerator & generator);
FloatImagePtr CreateIntegerShrinkFilterGeneratorH(FloatImagePtr fim,
											int factor,
											const FilterGenerator & generator);
FloatImagePtr CreateIntegerExpandFilterGenerator(FloatImagePtr fim,
											int expansion,
											const FilterGenerator & generator,
											float centerWidth );
FloatImagePtr CreateIntegerExpandFilterGeneratorH(FloatImagePtr fim,
											int expansion,
											const FilterGenerator & generator,
											float centerWidth );


/*

CreateDoubledFilterGenerator
makes the same output as CreateResizeFractionalFilter
but much faster

/**/
FloatImagePtr CreateDoubledFilterGenerator(FloatImagePtr src,const FilterGenerator & generator,
											float centerWidth,
											bool doH=true, bool doV=true);

FloatImagePtr CreateDoubledFilterGenerator_Standard(FloatImagePtr src,
											bool doH=true, bool doV=true);

// the old FloatImage::CreateResized
FloatImagePtr FloatImage_CreateResized_Old(const FloatImagePtr & in_src,int toW,int toH);

FloatImagePtr FloatImage_CreateDoubled_BoxThenSameSizeFiltered(const FloatImagePtr & src,const Filter & filter);

// make a discrete analysis filter which will let you interpolate with any synthesis filter :
void MakeAnalysisForSynthesis(Filter * pAnalysis,const Filter & synthesis);
void MakeAnalysisForSynthesisGenerator(Filter * pAnalysis,const FilterGenerator & synthesisFG);
											
END_CB

#pragma once

#include "Color.h"
#include "FloatImage.h"
#include "Vec3.h"
#include "Vec3i.h"

START_CB

ColorDW ColorTransformLocoWrap(const ColorDW & fm);
ColorDW ColorTransformLocoWrapInverse(const ColorDW & fm);

// standard "YUV" (Y Cb Cr) (rec601)
//  puts Y in "m_r" - that's channel 0 in a fim
const ColorF MakeYUVFromRGB(const ColorF & fm);
const ColorF MakeRGBFromYUV(const ColorF & fm);

const ColorF MakeYCoCgFromRGB(const ColorF & fm);
const ColorF MakeRGBFromYCoCg(const ColorF & fm);

//=====================================================

class Mat3;
extern const Mat3 c_SRGB_to_XYZ;
extern const Mat3 c_XYZ_to_SRGB;

// NOTEZ : SRGB used in XYZ is *degamma'ed* SRGB (linear light taken from SRGB)
//	RGB used in YCbCr formulas should be in gamma space (that is, gamma-encoded)

const Vec3 XYZ_to_LAB(const Vec3 & XYZ);
const Vec3 LAB_to_XYZ(const Vec3 & LAB);

const Vec3 SRGB_to_XYZ(const Vec3 & XYZ);
const Vec3 XYZ_to_SRGB(const Vec3 & XYZ);

const Vec3 LCH_to_LAB(const Vec3 & LCH);
const Vec3 LAB_to_LCH(const Vec3 & LAB);

// range of LCH is all [0,1]
//  C = chromaticity
//  to get saturation you can use :
//  S = C/L or   S^2 = C^2 / (L^2 + C^2)

inline const Vec3 SRGB_to_LAB(const Vec3 & rgb) { return XYZ_to_LAB( SRGB_to_XYZ( rgb ) ); }
inline const Vec3 LAB_to_SRGB(const Vec3 & lab) { return XYZ_to_SRGB( LAB_to_XYZ( lab ) ); }

//======================================================

// SRGB Gamma correction :
//	linear section at low values
//	then a pow ramp of 2.4
// approximates a gamma curve of 2.2
//

// encoded in [0,1]
inline float SRGB_To_Linear(float encoded)
{
	ASSERT( encoded >= 0.f );
	if ( encoded <= 0.04045f )
	{
		return encoded / 12.92f;
	}
	else
	{
		return powf( (encoded + 0.055f)/1.055f , 2.4f );
	}
}

// linear in [0,1]
inline float Linear_To_SRGB(float linear)
{
	ASSERT( linear >= 0.f );
	if ( linear <= 0.0031308 )
	{
		return 12.92f * linear;
	}
	else
	{
		return 1.055f * powf( linear , 1.f/2.4f ) - 0.055f;
	}
}

inline const Vec3 SRGB_To_Linear(const Vec3 & xyz)
{
	return Vec3( 
		SRGB_To_Linear(xyz.x),
		SRGB_To_Linear(xyz.y),
		SRGB_To_Linear(xyz.z) );
}

inline const Vec3 Linear_To_SRGB(const Vec3 & xyz)
{
	return Vec3( 
		Linear_To_SRGB(xyz.x),
		Linear_To_SRGB(xyz.y),
		Linear_To_SRGB(xyz.z) );
}

//======================================================

void FloatImage_LAB_to_XYZ(FloatImage * fim);
void FloatImage_XYZ_to_LAB(FloatImage * fim);

void FloatImage_SRGB_to_XYZ(FloatImage * fim);
void FloatImage_XYZ_to_SRGB(FloatImage * fim);

//======================================================

END_CB

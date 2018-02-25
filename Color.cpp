#include "Base.h"
#include "Color.h"

START_CB

//------------------------------------------------------------------------------
//! class statics :

const ColorDW ColorDW::white   ( 0xFFFFFFFF );
const ColorDW ColorDW::black   ( 0xFF000000 );
const ColorDW ColorDW::grey    ( 0xFF808080 );

const ColorDW ColorDW::red     ( 0xFFFF0000 );
const ColorDW ColorDW::green   ( 0xFF00FF00 );
const ColorDW ColorDW::blue    ( 0xFF0000FF );
const ColorDW ColorDW::yellow  ( 0xFFFFFF00 );
const ColorDW ColorDW::turqoise( 0xFF00FFFF );
const ColorDW ColorDW::purple  ( 0xFFFF00FF );

const ColorDW ColorDW::debug( (uint32)ColorDW::DEBUG_COLOR );

const ColorF ColorF::white(	1.f,1.f,1.f);
const ColorF ColorF::black(	0.f,0.f,0.f);

const ColorF ColorF::red(		1.f,0.f,0.f);
const ColorF ColorF::green(	0.f,1.f,0.f);
const ColorF ColorF::blue(	0.f,0.f,1.f);
const ColorF ColorF::debug(	1.f,0.f,1.f);
const ColorF ColorF::yellow(	1.f,1.f,0.f);
const ColorF ColorF::turqoise(0.f,1.f,1.f);
const ColorF ColorF::purple(	1.f,0.f,1.f);

const ColorF ColorF::fltmax(FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX);

//------------------------------------------------------------------------------

void ColorDW::SetAverage(const ColorDW & fm,const ColorDW & to)
{
	const int r = (fm.GetR() + to.GetR() + 1)/2;
	const int g = (fm.GetG() + to.GetG() + 1)/2;
	const int b = (fm.GetB() + to.GetB() + 1)/2;
	const int a = (fm.GetA() + to.GetA() + 1)/2;
	Set(r,g,b,a);
}

void ColorDW::SetLerp(const ColorDW & fm,const ColorDW & to,const float t)
{
	ColorF cf1; cf1 = fm;
	ColorF cf2; cf2 = to;
	ColorF cf = t * cf2 + (1-t) * cf1;
	SetFloatSafe(cf);
}

void ColorDW::SetFromHueFast(const int hue) // Hue = 0 to 256*6 periodic
{
	switch( (hue % (256*6))>>8 )
	{
		case 0: Set(255,				(hue & 0xFF),		0);					return;
		case 1: Set(255-(hue & 0xFF),	255,				0);					return;
		case 2: Set(0,					255,				(hue & 0xFF));		return;
		case 3: Set(0,					255-(hue & 0xFF),	255);				return;
		case 4: Set((hue & 0xFF),		0,					255);				return;
		case 5: Set(255,				0,					255-(hue & 0xFF));	return;
	}
}

//! SetFromHue is a convenient debug colorizing tool
void ColorF::SetFromHue(const float unitHue) // Hue = 0 to 1.f periodic
{
	const float hue = unitHue * 6.f;
	const int hueInteger = ftoi(hue);
	const float hueFrac = hue - float(hueInteger);
	ASSERT( fiszerotoone(hueFrac) );
	switch( hueInteger % 6 )
	{
		case 0: Set(1.f,			hueFrac,		0);				return;
		case 1: Set(1.f-hueFrac,	1.f,			0);				return;
		case 2: Set(0,				1.f,			hueFrac);		return;
		case 3: Set(0,				1.f-hueFrac,	1.f);			return;
		case 4: Set(hueFrac,		0,				1.f);			return;
		case 5: Set(1.f,			0,				1.f-hueFrac);	return;
	}
}

//------------------------------------------------------------------------------

/*****
	supposedly better YUV :

    RGB -> YCbCr (with Rec 601-1 specs)         | YCbCr (with Rec 601-1 specs) -> RGB
    Y=  0.2989*Red+0.5867*Green+0.1144*Blue     | Red=  Y+0.0000*Cb+1.4022*Cr
    Cb=-0.1687*Red-0.3312*Green+0.5000*Blue     | Green=Y-0.3456*Cb-0.7145*Cr
    Cr= 0.5000*Red-0.4183*Green-0.0816*Blue     | Blue= Y+1.7710*Cb+0.0000*Cr
*****/

const ColorF MakeYUVFromRGB(const ColorF & fm)
{
	return ColorF(
		  0.2989f*fm.m_r + 0.5867f*fm.m_g + 0.1144f*fm.m_b,
		 -0.1687f*fm.m_r - 0.3312f*fm.m_g + 0.5000f*fm.m_b + 0.5f,
		  0.5000f*fm.m_r - 0.4183f*fm.m_g - 0.0816f*fm.m_b + 0.5f,
		  fm.m_a );
}

const ColorF MakeRGBFromYUV(const ColorF & fm)
{
	const float Y = fm.m_r;
	const float U = fm.m_g - 0.5f;
	const float V = fm.m_b - 0.5f;
	return ColorF(
			Y	+ 1.4022f*V,
			Y   - 0.3456f*U	- 0.7145f*V,
			Y   + 1.7710f*U,
			fm.m_a );   
}

/**

Color transform notes :

JPEG-LS (LOCO-I) uses :

{RGB} -> { G , (R-G), (B-G) }

J2K (aka RCT aka CREW) uses :

{RGB} ->
	Y = (R + 2G + B)/4
	U = R-G
	V = B-G
	
	G = Y - (U+V)/4;
	R = U + G
	B = V + G

	(the divides are floored)

YCoCg is :
(similar to "RCT" but decorrelates better)

lossy :

	Y = (R + 2G + B)/4
	Co= (R-B)/2
	Cg= (2G - R - B)/4

	G = Y + Cg
	R = Y - Cg + Co
	B = Y - Cg - Co

lossless :
	(Co and Cg are effectively scaled up by 2)

	Co = R-B
	t = B + (Co/2)
	Cg = G-t
	Y = t + (Cg/2)

	s = Y - (Cg/2)
	G = Cg + s
	B = s - (Co/2)
	R = B + Co

**/

// {RGB} -> { G , (R-G), (B-G) }
ColorDW ColorTransformLocoWrap(const ColorDW & fm)
{
	int G = fm.GetG();
	int R = fm.GetR() - G + 128;
	int B = fm.GetB() - G + 128;
	
	return ColorDW( (R&0xFF), G, (B&0xFF), fm.GetA() );
}

ColorDW ColorTransformLocoWrapInverse(const ColorDW & fm)
{
	int G = fm.GetG();
	int R = fm.GetR() + G - 128;
	int B = fm.GetB() + G - 128;
	
	return ColorDW( (R&0xFF), G, (B&0xFF), fm.GetA() );
}

const ColorF MakeYCoCgFromRGB(const ColorF & fm)
{	
	return ColorF(
		(fm.m_r + fm.m_g + fm.m_g + fm.m_b)*0.25f,
		(fm.m_r - fm.m_b)*0.5f + 0.5f,
		(fm.m_g + fm.m_g - fm.m_r - fm.m_b)*0.25f + 0.5f,
		fm.m_a );
}

const ColorF MakeRGBFromYCoCg(const ColorF & fm)
{	
	const float Y = fm.m_r;
	const float Co = fm.m_g - 0.5f;
	const float Cg = fm.m_b - 0.5f;
	return ColorF(
			Y - Cg + Co,
			Y + Cg,
			Y - Cg - Co,
			fm.m_a );   
}

// test the color converters here :

/*
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
			ColorF yuv = MakeYCoCgFromRGB(cf);
			ColorF inv = MakeRGBFromYCoCg(yuv);
			
			ColorDW out(ColorDW::eFromFloatSafe,inv);
			
			ASSERT_RELEASE( cdw == out );
		}
	}
}
*/
	
//------------------------------------------------------------------------------

#if 0

#include "ColorD3D.h"

static void Color_Test()
{
	ColorF cf1(1,0,0);
	ColorF cf2(0,1,0,.5);

	ColorF cf3 = cf1 + 0.5f * cf2;
	cf3 *= 2;
	cf3 -= cf1;
	cf3 *= cf2;
	cf2 *= ColorF::red;

	ColorDW cdw(0);
	cdw.SetFloatSafe(cf3);

	ColorF cf4(cdw);
	cf4 = cdw;

	D3DCOLOR dc;
	dc = ColorAsD3D(cdw);
	
	D3DCOLORVALUE dcv;
	dcv = ColorAsD3D(cf3);

	cf3 = ColorFromD3D(dcv);
}

#endif

//------------------------------------------------------------------------------

END_CB


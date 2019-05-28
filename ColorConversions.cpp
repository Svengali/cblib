#include "ColorConversions.h"
#include "Mat3.h"
#include "Mat3Util.h"
#include "FloatImage.h"
#include "MathFunctors.h"
#include "LogUtil.h"

START_CB

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


// D65 SRGB
const Mat3 c_SRGB_to_XYZ(
 0.4124564f,  0.3575761f,  0.1804375f,
 0.2126729f,  0.7151522f,  0.0721750f,
 0.0193339f,  0.1191920f,  0.9503041f
);

const Mat3 c_XYZ_to_SRGB(
   3.24045f,-1.53714f,-0.49853f,
  -0.96927f, 1.87601f, 0.04156f,
   0.05564f,-0.20403f, 1.05723f
);


inline float LABf( float x )
{
	// @@ ?? clamp necessary ?
	//x = fclampunit(x);
	if ( x < (216.0f/24389) )
	{
		const float k = (24389/27.0f);
		return ( k * x + 16.0f ) * (1.f / 116.0f);
	}
	else
	{
		return pow( x, 1.0f/3.0f );
	}
}

inline float iLABf( float f )
{
	if ( f < 0.2068966f )
	{
		// f = ( k * x + 16.0 ) / 116.0;
		// 116 * f - 16 = k * x;
		// x = (116 * f - 16)/k;
		const float k = (24389/27.0f);
		return (116.0f * f - 16.0f) * (1.0f/k);
	}
	else
	{
		//return pow( f , 3.0 );
		return f*f*f;
	}
}

// range of my LAB output is :
// {0.000,-0.862,-1.079} - {1.000,0.982,0.945}
const Vec3 XYZ_to_LAB(const Vec3 & XYZ)
{
	//D65 	0.95047 	1.00000 	1.08883
	float fx = LABf( XYZ.x / 0.95047f );
	float fy = LABf( XYZ.y / 1.0f );
	float fz = LABf( XYZ.z / 1.08883f );
	
	float L = 116.f * fy - 16;
	float a = 500.f * ( fx - fy );
	float b = 200.f * ( fy - fz );

	// L is [0,100]	
	// put it in [0,1] for ColorF

	return Vec3(L/100.f,a/100.f,b/100.f);
}

const Vec3 LAB_to_XYZ(const Vec3 & LAB)
{
	float L = LAB.x * 100.f;
	float a = LAB.y * 100.f;
	float b = LAB.z * 100.f;

	float fy = (L + 16) / 116.f;
	float fx = fy + a/500.f;
	float fz = fy - b/200.f;
	
	Vec3 XYZ;
	XYZ.x = iLABf(fx) * 0.95047f;
	XYZ.y = iLABf(fy) * 1.0f;
	XYZ.z = iLABf(fz) * 1.08883f;
	
	return XYZ;
}

const Vec3 XYZ_to_SRGB(const Vec3 & XYZ)
{
	return c_XYZ_to_SRGB * XYZ;
}

const Vec3 SRGB_to_XYZ(const Vec3 & XYZ)
{
	return c_SRGB_to_XYZ * XYZ;
}

// this makes the range of LCH be [0,1]
static const float c_LCH_Cmax = 1.33802f;

const Vec3 LAB_to_LCH(const Vec3 & LAB)
{
	Vec3 lch;
	lch.x = LAB.x;
	lch.y = sqrtf(LAB.y*LAB.y + LAB.z*LAB.z) * (1.f / c_LCH_Cmax);
	lch.z = (atan2f(LAB.z,LAB.y) + PIf)/TWO_PIf; // in [0,1]
	
	return lch;
}

const Vec3 LCH_to_LAB(const Vec3 & LCH)
{
	Vec3 LAB;
	LAB.x = LCH.x;
	float angle = LCH.z * TWO_PIf - PIf;
	float len = LCH.y * c_LCH_Cmax;
	LAB.y = len * cosf(angle);
	LAB.z = len * sinf(angle);
	
	return LAB;
}

MAKE_FUNCTOR(const Vec3,LAB_to_XYZ);
MAKE_FUNCTOR(const Vec3,XYZ_to_LAB);

template <typename t_functor>
void FloatImage_ApplyColorFunctorVec3(FloatImage * fim,t_functor functor)
{
	int w = fim->m_info.m_width;
	int h = fim->m_info.m_height;

	for(int y=0;y<h;y++)
	{
		for(int x=0;x<w;x++)
		{
			int i = x + y * w;
			ColorF color = fim->GetPixel(i);
			color.MutableVec3() = functor( color.AsVec3() );
			fim->SetPixel(i, color );
		}
	}
}

void FloatImage_XYZ_to_LAB(FloatImage * fim)
{
	FloatImage_ApplyColorFunctorVec3(fim,XYZ_to_LAB_functor());
}

void FloatImage_LAB_to_XYZ(FloatImage * fim)
{
	FloatImage_ApplyColorFunctorVec3(fim,LAB_to_XYZ_functor());
}

void FloatImage_SRGB_to_XYZ(FloatImage * fim)
{
	FloatImage_ApplyColorMatrix(fim,c_SRGB_to_XYZ);
}

void FloatImage_XYZ_to_SRGB(FloatImage * fim)
{
	/*
	Mat3 XYZ_to_SRGB;
	GetInverse(c_SRGB_to_XYZ,&XYZ_to_SRGB);
	
	lprintfCFloatArray((float *)&XYZ_to_SRGB,9,"c_XYZ_to_SRGB",3,8,5);
	/**/
	
	FloatImage_ApplyColorMatrix(fim,c_XYZ_to_SRGB);
}

END_CB



#if 0

/*

lossy YCoCg - no range expansion
drops bits in Co & Cg

*/

//*

struct ColorFunctorI_RGB_to_YCoCg
{
	const rrColor4I operator() (const rrColor4I & from)
	{
		rrColor4I ret;
		
		/*
		S32 y  = ( from.g + from.g + from.r + from.b ) / 4;
		S32 Co = ( from.r - from.b )/2;
		S32 Cg = ( from.g + from.g - from.r - from.b )/4;
		*/
		// this wins by a lot :
		S32 y  = ( from.g + from.g + from.r + from.b + 2) / 4;
		S32 Co = ( from.r - from.b )/2;
		S32 Cg = ( from.g + from.g - from.r - from.b )/4;
		
		ret.g = y;
		ret.r = Cg + 128;
		ret.b = Co + 128;
		ret.a = from.a;
			
		return ret;
	}
};

struct ColorFunctorI_YCoCg_to_RGB
{
	const rrColor4I operator() (const rrColor4I & from)
	{
		rrColor4I ret;
		
		S32 y  = from.g;
		S32 Cg = from.r - 128;
		S32 Co = from.b - 128;
		
		ret.g = y + Cg;
		ret.r = y - Cg + Co;
		ret.b = y - Cg - Co;
		ret.a = from.a;
			
		return ret;
	}
};

// worse

struct ColorFunctorI_RGB_to_YCoCgLossless
{
	const rrColor4I operator() (const rrColor4I & from)
	{
		rrColor4I ret;
		
		int Co = from.r - from.b;
		int t = from.b + (Co/2);
		int Cg = from.g - t;
		int y = t + (Cg/2);
	   
		ret.g = y;
		ret.r = Co + 128;
		ret.b = Cg + 128;
		ret.a = from.a;
			
		return ret;
	}
};

struct ColorFunctorI_YCoCgLossless_to_RGB
{
	const rrColor4I operator() (const rrColor4I & from)
	{
		rrColor4I ret;
		
		S32 y  = from.g;
		S32 Co = from.r - 128;
		S32 Cg = from.b - 128;
		
		int s = y - (Cg/2);
		ret.g = Cg + s;
		ret.b = s - (Co/2);
		ret.r = ret.b + Co;
		ret.a = from.a;
			
		return ret;
	}
};

struct ColorFunctorF_RGB_to_YCoCg
{
	const rrColor4F operator() (const rrColor4F & from)
	{
		rrColor4F ret;
		
		F32 y  = ( from.g + from.g + from.r + from.b ) / 4.f;
		F32 Co = ( from.r - from.b )/2.f;
		F32 Cg = ( from.g + from.g - from.r - from.b )/4.f;
   
		ret.g = y;
		ret.r = Cg + 128.f;
		ret.b = Co + 128.f;
		ret.a = from.a;
			
		return ret;
	}
};

struct ColorFunctorF_YCoCg_to_RGB
{
	const rrColor4F operator() (const rrColor4F & from)
	{
		rrColor4F ret;
		
		F32 y  = from.g;
		F32 Cg = from.r - 128.f;
		F32 Co = from.b - 128.f;
		
		ret.g = y + Cg;
		ret.r = y - Cg + Co;
		ret.b = y - Cg - Co;
		ret.a = from.a;
			
		return ret;
	}
};

struct ColorFunctorF_RGB_to_YCrCb601
{
	const rrColor4F operator() (const rrColor4F & from)
	{
		rrColor4F ret;
		
		float R = from.r, G = from.g, B = from.b;
		
		float Y  =  0.29900f * R + 0.58700f * G + 0.11400f * B;
		float Cb = -0.16874f * R - 0.33126f * G + 0.50000f * B  + 128.f;
		float Cr =  0.50000f * R - 0.41869f * G - 0.08131f * B  + 128.f;
 
		ret.r = Cr;
		ret.g = Y;
		ret.b = Cb;
		ret.a = from.a;
			
		return ret;
	}
};

struct ColorFunctorF_YCrCb601_to_RGB
{
	const rrColor4F operator() (const rrColor4F & from)
	{
		rrColor4F ret;
		
		F32 y  = from.g;
		F32 Cr = from.r - 128.f;
		F32 Cb = from.b - 128.f;
		
		ret.r = y + 1.40200f * Cr;
		ret.g = y - 0.34414f * Cb - 0.71414f * Cr;
		ret.b = y + 1.77200f * Cb;
		ret.a = from.a;
			
		return ret;
	}
};

#endif

/*

rec601 :

Y 0 = 0.299 ? R0 + 0.587 ? G0 + 0.114 ? B0
Cb = ?0.169 ? R0 ? 0.331 ? G0 + 0.500 ? B0
Cr = 0.500 ? R0 ? 0.419 ? G0 ? 0.081 ? B0

R0 = Y 0 + 0.000 ? U0 + 1.403 ? V 0
G0 = Y 0 ? 0.344 ? U0 ? 0.714 ? V 0
B0 = Y 0 + 1.773 ? U0 + 0.000 ? V 0

rec709 :

Y  = 0.2215 ? R + 0.7154 ? G + 0.0721 ? B
Cb =?0.1145 ? R0? 0.3855 ? G0+ 0.5000 ? B0
Cr = 0.5016 ? R0? 0.4556 ? G0? 0.0459 ? B0
R0= Y 0+ 0.0000 ? Cb + 1.5701 ? Cr
G0= Y 0? 0.1870 ? Cb ? 0.4664 ? Cr
B0= Y 0? 1.8556 ? Cb + 0.0000 ? Cr

=============================

n any case, CCIR 601 defines the relationship between YCrCb and RGB values:

Y = 0.299R + 0.587G + 0.114B
U'= (B-Y)*0.565
V'= (R-Y)*0.713

where Ey, R, G and B are in the range [0,1] and Ecr and Ecb are in the range [-0.5,0.5] (Equations corrected per input from Stephan Bourgeois and Gregory Smith below)

with reciprocal versions:

    R = Y + 1.403V'

    G = Y - 0.344U' - 0.714V'

    B = Y + 1.770U'


JPEG :

R = Y + 1.402 (Cr-128)
G = Y - 0.34414 (Cb-128) - 0.71414 (Cr-128)
B = Y + 1.772 (Cb-128)

===========================================

 * YCbCr is defined per CCIR 601-1, except that Cb and Cr are
 * normalized to the range 0..MAXJSAMPLE rather than -0.5 .. 0.5.
 * The conversion equations to be implemented are therefore
 *	Y  =  0.29900 * R + 0.58700 * G + 0.11400 * B
 *	Cb = -0.16874 * R - 0.33126 * G + 0.50000 * B  + CENTERJSAMPLE
 *	Cr =  0.50000 * R - 0.41869 * G - 0.08131 * B  + CENTERJSAMPLE
 
 * YCbCr is defined per CCIR 601-1, except that Cb and Cr are
 * normalized to the range 0..MAXJSAMPLE rather than -0.5 .. 0.5.
 * The conversion equations to be implemented are therefore
 *	R = Y                + 1.40200 * Cr
 *	G = Y - 0.34414 * Cb - 0.71414 * Cr
 *	B = Y + 1.77200 * Cb
 * where Cb and Cr represent the incoming values less CENTERJSAMPLE.

========================================

BT709 :

    Y'= 0.2215*R' + 0.7154*G' + 0.0721*B'
    Cb=-0.1145*R' - 0.3855*G' + 0.5000*B'
    Cr= 0.5016*R' - 0.4556*G' - 0.0459*B'

    R'= Y' + 0.0000*Cb + 1.5701*Cr
    G'= Y' - 0.1870*Cb - 0.4664*Cr
    B'= Y' - 1.8556*Cb + 0.0000*Cr


*/

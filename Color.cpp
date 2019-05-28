#include "Base.h"
#include "Color.h"

START_CB

//------------------------------------------------------------------------------
//! class statics :

const ColorDW ColorDW::white(	0xFFFFFFFF);
const ColorDW ColorDW::black(	0xFF000000);
const ColorDW ColorDW::grey    ( 0xFF808080 );

const ColorDW ColorDW::red(		0xFFFF0000);
const ColorDW ColorDW::green(	0xFF00FF00);
const ColorDW ColorDW::blue(	0xFF0000FF);
const ColorDW ColorDW::yellow(	0xFFFFFF00);
const ColorDW ColorDW::turqoise( 0xFF00FFFF );
const ColorDW ColorDW::purple  ( 0xFFFF00FF );

const ColorDW ColorDW::debug( (uint32)ColorDW::DEBUG_COLOR );

const ColorF ColorF::white(		1.f,1.f,1.f);
const ColorF ColorF::black(		0.f,0.f,0.f);

const ColorF ColorF::red(		1.f,0.f,0.f);
const ColorF ColorF::green(		0.f,1.f,0.f);
const ColorF ColorF::blue(		0.f,0.f,1.f);
const ColorF ColorF::debug(		1.f,0.f,1.f);
const ColorF ColorF::yellow(	1.f,1.f,0.f);
const ColorF ColorF::turqoise(	0.f,1.f,1.f);
const ColorF ColorF::purple(	1.f,0.f,1.f);

const ColorF ColorF::fltmax(FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX);

//------------------------------------------------------------------------------

void ColorDW::SetDebugColor(const int index)
{
	//SetFromHueFast(index * 281);
	ColorF cf;
	cf.SetFromHue( index * 0.18366f );
	float scale = (float(index % 7) + 7.f)/13.f;
	cf *= scale;
	SetFloatUnsafe(cf);
}

void ColorDW::SetSumClamped(const ColorDW & fm,const ColorDW & to)
{
	const int r = MIN( fm.GetR() + to.GetR() ,255 );
	const int g = MIN( fm.GetG() + to.GetG() ,255 );
	const int b = MIN( fm.GetB() + to.GetB() ,255 );
	const int a = MIN( fm.GetA() + to.GetA() ,255 );
	Set(r,g,b,a);
}

void ColorDW::SetAverage(const ColorDW & fm,const ColorDW & to)
{
	const int r = (fm.GetR() + to.GetR() + 1)/2;
	const int g = (fm.GetG() + to.GetG() + 1)/2;
	const int b = (fm.GetB() + to.GetB() + 1)/2;
	const int a = (fm.GetA() + to.GetA() + 1)/2;
	Set(r,g,b,a);
}

void ColorDW::SetLerpF(const ColorDW & fm,const ColorDW & to,const float t)
{
	ColorF cf1; cf1 = fm;
	ColorF cf2; cf2 = to;
	ColorF cf = t * cf2 + (1-t) * cf1;
	SetFloatSafe(cf);
}

void ColorDW::SetLerpI(const ColorDW & fm,const ColorDW & to,const int i256)
{
	int r = fm.GetR(); r += ((to.GetR() - r)*i256)>>8;
	int g = fm.GetG(); g += ((to.GetG() - g)*i256)>>8;
	int b = fm.GetB(); b += ((to.GetB() - b)*i256)>>8;
	int a = fm.GetA(); a += ((to.GetA() - a)*i256)>>8;
	Set(r,g,b,a);
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
void ColorF::SetFromHue(float unitHue) // Hue = 0 to 1.f periodic
{
	//unitHue = fmodf(unitHue,1.f);
	const float hue = unitHue * 6.f;
	const int hueInteger = ftoi(hue);
	const float hueFrac = hue - float(hueInteger);
	ASSERT( fiszerotoone(hueFrac) );
	switch( (hueInteger + 60000) % 6 )
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


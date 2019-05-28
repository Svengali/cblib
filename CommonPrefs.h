#pragma once

#include "Prefs.h"
#include "Win32Util.h"

START_CB

// some standard PrefIO's for simple composite types :
class Vec2;
class Vec3;
class AxialBox;
class Sphere;
class ColorDW;
class ColorF;
class RectI;
class RectF;

void PrefIO(PrefBlock & block,Vec3 & what);
void PrefIO(PrefBlock & block,Vec2 & what);
void PrefIO(PrefBlock & block,AxialBox & what);
void PrefIO(PrefBlock & block,Sphere & what);
void PrefIO(PrefBlock & block,ColorDW & what);
void PrefIO(PrefBlock & block,ColorF & what);
void PrefIO(PrefBlock & block,RectI & what);
void PrefIO(PrefBlock & block,RectF & what);

//===============================================================================

// PrefIO's for some Windows types :

void PrefIO(PrefBlock & block,POINT & me);
void PrefIO(PrefBlock & block,RECT & me);
void PrefIO(PrefBlock & block,COLORREF & me);
void PrefIO(PrefBlock & block,RGBQUAD & me);

/*

// Nah, don't do this, better to use PrefIO so I can write rgba and not worry about confusing byte order
// still useful as an example of how to get one-line IO for custom types

// a COLORREF in hex looks like :
// MyInfoColor:0005C8FF
// MyInfoColor:AABBGGRR
//	that's A,B,G,R

template <>
inline void ReadFromText<COLORREF>(COLORREF * pValue, const char * const text)
{
	ReadFromTextULHex((uint32 *)pValue,text);
}

template <>
inline void WriteToText<COLORREF>(const COLORREF & val, String * pInto)
{
	WriteToTextULHex((uint32)val,pInto);
}

template <>
inline void ReadFromText<RGBQUAD>(RGBQUAD * pValue, const char * const text)
{
	ReadFromTextULHex((uint32 *)pValue,text);
}

template <>
inline void WriteToText<RGBQUAD>(const RGBQUAD & val, String * pInto)
{
	WriteToTextULHex(*((uint32 *)&val),pInto);
}
*/

END_CB

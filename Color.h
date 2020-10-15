#pragma once

#include "Util.h"
#include "FloatUtil.h"
#include "Vec3.h"
#include "Vec4.h"

START_CB

// forward declarations of the classes defined here :

class ColorDW; // DWORD color; matches "D3DCOLOR" bitwise (also same as RGBQUAD , but NOT same as COLORREF)
class ColorF;	// Float color; matches "D3DCOLORVALUE" bitwise

/*

\todo Make a separate header which has
	ColorDW <-> D3DCOLOR casters
and ColorF <-> D3DCOLORVALUE casters

NOTE ON COLOR CONVERSIONS :

int colors are in [0,255]

Int pel "x" is considered to represent the span {x-0.5,x+0.5}/255

float colors are in [0,1]

int 0 -> float 0
int 255 -> float 1

Note that this way is nice in that it keeps the maximums the same,
 but it's bad in that it wastes color space.  That is, there's only 1/255
 precision instead of 1/256 which is totally possible.

Another way of seeing it is that the int range [0,255] is actually being
mapped to the float range { -0.5/255 , 255.5/255 } , that is, {0,1} is not
the full range.

One nice thing about this though is that it gives you some slop in your float->int.
As long as the float is in { -0.5/255 , 255.5/255 } you don't need to clamp.

*/

inline int		ColorFTOI(float f);
inline float	ColorITOF(int i);
inline int		Clamp255( int i);


//}{=================================================================================

/*! ColorDW
   integer color, for storage and d3d
   a,r,g,b, normalized to 0->255
	(can NOT go out of that range a-ok)
	matches "D3DCOLOR" bitwise
	
	In bytes ColorDW is :
		uint8	b,g,r,a;
		
	union ColorDWBytes
	{
		uint32		dw;
		struct
		{
			uint8	b,g,r,a;
		};
	};
*/
class ColorDW
{
private:
	
	//! byte mapping like D3D :
	enum _Constants
	{
		 ASHIFT = 24,
		 RSHIFT = 16,
		 GSHIFT = 8,
		 BSHIFT = 0,
		 AMASK = 0xFF<<ASHIFT,
		 RMASK = 0xFF<<RSHIFT,
		 GMASK = 0xFF<<GSHIFT,
		 BMASK = 0xFF<<BSHIFT,
		 DEBUG_COLOR = 0xFFFF00FF // hot pink
	};

public:
	// dmoore: make the debug color enum equal to the color of DEBUG_COLOR
	enum EDebugColor { eDebugColor = DEBUG_COLOR };

	inline  ColorDW()
	{
		// load with invalid data in debug mode
		// there is no "invalid", so use the "debug color"
		#ifdef _DEBUG
		m_color = (uint32)DEBUG_COLOR;
		#endif
	}

	// default copy constructor and assignment and such are all fine
	//inline ~ColorDW() { }

	//! important explicit on constructor from a dword
	explicit inline  ColorDW(const uint32 c) : m_color(c) { }

	//! default alpha is *opaque* (255)
	explicit inline  ColorDW(const int R,const int G,const int B,const int A = 0xFF) { Set(R,G,B,A); }

	//! Set to the selected debugcolor
	ColorDW(const EDebugColor, int d) { SetDebugColor(d); }
	
	enum EFromFloatUnsafe { eFromFloatUnsafe };
	enum EFromFloatSafe { eFromFloatSafe };

	ColorDW(const EFromFloatUnsafe, const ColorF & f) { SetFloatUnsafe(f); }
	ColorDW(const EFromFloatSafe, const ColorF & f) { SetFloatSafe(f); }
	
	//! no consistency conditions on a raw dword to validate !
	bool IsValid() const { return true; }

	////////////////////////////////////////////////
	// basic RGB Get/Set :

	//! get the R,G,B,A components; in the 0->255 range
	int GetR() const { return (m_color & RMASK) >> RSHIFT; }
	int GetG() const { return (m_color & GMASK) >> GSHIFT; }
	int GetB() const { return (m_color & BMASK) >> BSHIFT; }
	int GetA() const { return (m_color & AMASK) >> ASHIFT; }

	//! Set the colors one by one, must be in 0->255 (ASSERTs if not)
	void SetR(const int R) { ASSERT( R == Clamp255(R) ); m_color = (m_color & ~RMASK) | (R << RSHIFT); }
	void SetG(const int G) { ASSERT( G == Clamp255(G) ); m_color = (m_color & ~GMASK) | (G << GSHIFT); }
	void SetB(const int B) { ASSERT( B == Clamp255(B) ); m_color = (m_color & ~BMASK) | (B << BSHIFT); }
	void SetA(const int A) { ASSERT( A == Clamp255(A) ); m_color = (m_color & ~AMASK) | (A << ASHIFT); }

	//! set all components (more efficient than setting one by one)
	//! default alpha is *opaque* (255)
	void Set(const int R,const int G,const int B,const int A = 0xFF);
	
	/*! SetFloat : set the color from floats in the 0->1 range
		scales up and converts from ints
		must be in 0->1 unless you use the Safe() function, which will Clamp
	*/
	void SetFloatUnsafe(const float R,const float G,const float B,const float A);
	void SetFloatUnsafe(const float R,const float G,const float B);

	void SetFloatSafe(const float R,const float G,const float B,const float A = 1.f);

	void SetFloatUnsafe(const ColorF & cf);
	void SetFloatSafe(const ColorF & cf);

	//! SetFromHueFast is a convenient debug colorizing tool
	void SetFromHueFast(const int hue); // Hue = 0 to 256*6 periodic

	//! SetDebugColor should make a nicely different color for every index, with
	//! the colors of consecutive indices being very different
	void SetDebugColor(const int index);
	
	void SetSumClamped(const ColorDW & fm,const ColorDW & to);
	void SetAverage(const ColorDW & fm,const ColorDW & to);
	void SetLerpF(const ColorDW & fm,const ColorDW & to,const float t);
	void SetLerpI(const ColorDW & fm,const ColorDW & to,const int i256); // i is in [0,256]

	////////////////////////////////////////////////

	static uint32 DistanceSqr(const ColorDW & fm,const ColorDW & to);
	static uint32 DistanceSqrRGB(const ColorDW & fm,const ColorDW & to);
	
	////////////////////////////////////////////////
	//! static colors for your convenience;
	//! 	access like ColorDW::red
	//! all are opaque in alpha

	static const ColorDW white;
	static const ColorDW black;
	static const ColorDW grey;
	static const ColorDW red;
	static const ColorDW green;
	static const ColorDW blue;
	static const ColorDW yellow;
	static const ColorDW turqoise;
	static const ColorDW purple;
	static const ColorDW debug; // "debug" color for showing un-filled colors, textures, etc.

	////////////////////////////////////////////////

	//! raw dword access
	//! for interfacing to D3D, really
	uint32 GetDW() const		{ return m_color; }
	void SetDW(const uint32 dw) { m_color = dw; }

	// no operator= from dword, and constructor is explicit
	//void operator = (const uint32 dw)

	bool operator==(const ColorDW& comp) const { return m_color == comp.m_color; }
	bool operator!=(const ColorDW& comp) const { return !operator==(comp); }
	
private:

	uint32	m_color;

}; // end class ColorDW

//}{=================================================================================

/* ColorF
   floating-point color for mathematics
   r,g,b,a normalized to 0->1
	(but can go out of that range)
	matches "D3DCOLORVALUE" bitwise
*/
class ColorF
{
public:

	//! default constructor invalidates data
	inline  ColorF()
	{
		finvalidatedbg(m_r);
		finvalidatedbg(m_g);
		finvalidatedbg(m_b);
		finvalidatedbg(m_a);
	}
	
	// default copy constructor and assignment and such are all fine
	//inline ~ColorF() { }
	
	//! load order is (R,G,B,A)
	//! default alpha is *opaque* (1.f)
	explicit inline  ColorF(const float ir,const float ig,const float ib,const float ia = 1.f) :
								m_r(ir),m_g(ig),m_b(ib),m_a(ia)
	{
		ASSERT(IsValid());
	}

	explicit inline  ColorF(const Vec3 & rgb,const float ia = 1.f) :
								m_r(rgb.x),m_g(rgb.y),m_b(rgb.z),m_a(ia)
	{
		ASSERT(IsValid());
	}
	
	//! explicit constructor from a dword color
	explicit inline  ColorF(const ColorDW & dw)
	{
		Set(dw);
		ASSERT(IsValid());
	}

	bool IsValid() const
	{
		ASSERT_LOW( fisvalid(m_r) && fisvalid(m_g) && fisvalid(m_b) && fisvalid(m_a) );
		return true;
	}

	bool IsZeroToOne() const
	{
		return fiszerotoone(m_r) && fiszerotoone(m_g) && fiszerotoone(m_b) && fiszerotoone(m_a);
	}
	
	////////////////////////////////////////////////
	//! static colors for your convenience;
	//! 	access like ColorF::red
	//! all are opaque in alpha

	static const ColorF white;
	static const ColorF black;
	static const ColorF red;
	static const ColorF green;
	static const ColorF blue;
	static const ColorF yellow;
	static const ColorF turqoise;
	static const ColorF purple;
	static const ColorF debug; // "debug" color for showing un-filled colors, textures, etc.
	static const ColorF fltmax;

	/////////////////////////////////////////////
	//! basic GetRGB/SetRGB stuff

	float GetR() const			{ ASSERT_LOW(IsValid()); return m_r; }
	float GetG() const			{ ASSERT_LOW(IsValid()); return m_g; }
	float GetB() const			{ ASSERT_LOW(IsValid()); return m_b; }
	float GetA() const			{ ASSERT_LOW(IsValid()); return m_a; }
	void  SetR(const float v)	{ ASSERT_LOW(IsValid()); m_r = v; ASSERT_LOW(IsValid()); }
	void  SetG(const float v)	{ ASSERT_LOW(IsValid()); m_g = v; ASSERT_LOW(IsValid()); }
	void  SetB(const float v)	{ ASSERT_LOW(IsValid()); m_b = v; ASSERT_LOW(IsValid()); }
	void  SetA(const float v)	{ ASSERT_LOW(IsValid()); m_a = v; ASSERT_LOW(IsValid()); }

	//! default alpha is *opaque* (1.f)
	void Set(const float ir,const float ig,const float ib,const float ia = 1.f);

	//! clamped [0, 1] set.  default alpha is *opaque* (1.f)
	void SetSafe(const float ir,const float ig,const float ib,const float ia = 1.f);

	//! ColorF <- ColorDW loader
	void Set(const ColorDW & cdw);

	//! SetFromHue is a convenient debug colorizing tool
	void SetFromHue(const float unitHue); // Hue = 0 to 1.f periodic

	void SetLerp(const ColorF & a,const ColorF & b,const float t);

	void SetScaled(const ColorF & color,const float scale);

	void ScaleRGB(const float scale);

	static float DistanceSqr(const ColorF & fm,const ColorF & to);
	static float DistanceSqrRGB(const ColorF & fm,const ColorF & to);

	/////////////////////////////////////////////

	void Clamp(const float Lo = 0.f, const float Hi = 1.f);

	//! operator= from DW to match constructor from DW
	void operator = (const ColorDW & cdw)
	{
		Set(cdw);
	}

	/////////////////////////////////////////////
	//! some mutating math operators

	void operator *= (const float f);
	void operator *= (const ColorF & v);
	void operator += (const ColorF & v);	
	void operator -= (const ColorF & v);

	/////////////////////////////////////////////
	//! conversion from/to normals, as you would do in a normal map :

	const Vec3 GetNormal() const;
	void SetFromNormal(const Vec3 & vec,const float a = 1.f);

	const Vec3 & AsVec3() const;
	const Vec4 & AsVec4() const;
	Vec3 & MutableVec3();
	Vec4 & MutableVec4();

	/////////////////////////////////////////////

public :
	// no point in hiding this data with private
	/////////////////////////////////////////////
	// data :

	float m_r,m_g,m_b,m_a;

}; // end class ColorF

//}{=======================================================================

/*! DW <-> F converters :
	DW -> F is fast
	F  -> DW is slow
*/
inline void ColorDW::SetFloatUnsafe(const ColorF & cf)
{
	SetFloatUnsafe(cf.GetR(),cf.GetG(),cf.GetB(),cf.GetA());
}
inline void ColorDW::SetFloatSafe(const ColorF & cf)
{
	SetFloatSafe(cf.GetR(),cf.GetG(),cf.GetB(),cf.GetA());
}

inline void ColorDW::Set(const int R,const int G,const int B,const int A /* = 0xFF*/)
{
	ASSERT_LOW( R == Clamp255(R) );
	ASSERT_LOW( G == Clamp255(G) );
	ASSERT_LOW( B == Clamp255(B) );
	ASSERT_LOW( A == Clamp255(A) );
	m_color = (R<<RSHIFT) | (G<<GSHIFT) | (B<<BSHIFT) | (A<<ASHIFT);
}

inline void ColorDW::SetFloatUnsafe(const float R,const float G,const float B)
{
	Set( ColorFTOI(R), ColorFTOI(G), ColorFTOI(B) );
}

inline void ColorDW::SetFloatUnsafe(const float R,const float G,const float B,const float A)
{
	Set( ColorFTOI(R), ColorFTOI(G), ColorFTOI(B), ColorFTOI(A) );
}

inline void ColorDW::SetFloatSafe(const float R,const float G,const float B,const float A /*= 1.f*/)
{
	const int r = Clamp255( ColorFTOI(R) );
	const int g = Clamp255( ColorFTOI(G) );
	const int b = Clamp255( ColorFTOI(B) );
	const int a = Clamp255( ColorFTOI(A) );
	Set(r,g,b,a);
}

/*static*/ inline uint32 ColorDW::DistanceSqr(const ColorDW & fm,const ColorDW & to)
{
	// @@ could/should do a fast multi-bit version of this

	uint32 d =
		 (fm.GetR() - to.GetR())*(fm.GetR() - to.GetR());
	d += (fm.GetG() - to.GetG())*(fm.GetG() - to.GetG());
	d += (fm.GetB() - to.GetB())*(fm.GetB() - to.GetB());
	d += (fm.GetA() - to.GetA())*(fm.GetA() - to.GetA());
	return d;
}

/*static*/ inline uint32 ColorDW::DistanceSqrRGB(const ColorDW & fm,const ColorDW & to)
{
	// @@ could/should do a fast multi-bit version of this

	uint32 d =
		 (fm.GetR() - to.GetR())*(fm.GetR() - to.GetR());
	d += (fm.GetG() - to.GetG())*(fm.GetG() - to.GetG());
	d += (fm.GetB() - to.GetB())*(fm.GetB() - to.GetB());
	return d;
}

/*static*/ inline float ColorF::DistanceSqr(const ColorF & fm,const ColorF & to)
{
	float d =
		 (fm.GetR() - to.GetR())*(fm.GetR() - to.GetR());
	d += (fm.GetG() - to.GetG())*(fm.GetG() - to.GetG());
	d += (fm.GetB() - to.GetB())*(fm.GetB() - to.GetB());
	d += (fm.GetA() - to.GetA())*(fm.GetA() - to.GetA());
	return d;
}

/*static*/ inline float ColorF::DistanceSqrRGB(const ColorF & fm,const ColorF & to)
{
	float d =
		 (fm.GetR() - to.GetR())*(fm.GetR() - to.GetR());
	d += (fm.GetG() - to.GetG())*(fm.GetG() - to.GetG());
	d += (fm.GetB() - to.GetB())*(fm.GetB() - to.GetB());
	return d;
}

inline void ColorF::Set(const ColorDW & cdw)
{
	Set(	
		ColorITOF(cdw.GetR()),
		ColorITOF(cdw.GetG()),
		ColorITOF(cdw.GetB()),
		ColorITOF(cdw.GetA())
		);
}

inline void ColorF::Clamp(const float Lo /*= 0.f*/, const float Hi /*= 1.f*/)
{
	SetR( fclamp( GetR(), Lo, Hi) );
	SetG( fclamp( GetG(), Lo, Hi) );
	SetB( fclamp( GetB(), Lo, Hi) );
	SetA( fclamp( GetA(), Lo, Hi) );
}

inline void ColorF::Set(const float ir,const float ig,const float ib,const float ia /*= 1.f*/)
{
	m_r = ir;
	m_g = ig;
	m_b = ib;
	m_a = ia;
	ASSERT(IsValid());
}

inline void ColorF::SetSafe(const float ir,const float ig,const float ib,const float ia /*= 1.f*/)
{
	m_r = cb::Clamp(ir, 0.0f, 1.0f);
	m_g = cb::Clamp(ig, 0.0f, 1.0f);
	m_b = cb::Clamp(ib, 0.0f, 1.0f);
	m_a = cb::Clamp(ia, 0.0f, 1.0f);
	ASSERT(IsValid());
}

inline void ColorF::SetLerp(const ColorF & a,const ColorF & b,const float t)
{
	m_r = flerp(a.m_r,b.m_r,t);
	m_g = flerp(a.m_g,b.m_g,t);
	m_b = flerp(a.m_b,b.m_b,t);
	m_a = flerp(a.m_a,b.m_a,t);
}

inline void ColorF::SetScaled(const ColorF & a,const float scale)
{
	m_r = a.m_r * scale;
	m_g = a.m_g * scale;
	m_b = a.m_b * scale;
	m_a = a.m_a * scale;
}

inline void ColorF::ScaleRGB(const float scale)
{
	// preserves alpha
	m_r *= scale;
	m_g *= scale;
	m_b *= scale;
}

/////////////////////////////////////////////
//! conversion from/to normals, as you would do in a normal map :

inline const Vec3 & ColorF::AsVec3() const
{
	return *((const Vec3 *)&m_r);
}

inline const Vec4 & ColorF::AsVec4() const
{
	return *((const Vec4 *)&m_r);
}

inline Vec3 & ColorF::MutableVec3()
{
	return *((Vec3 *)&m_r);
}

inline Vec4 & ColorF::MutableVec4()
{
	return *((Vec4 *)&m_r);
}

inline const Vec3 ColorF::GetNormal() const
{
	return Vec3( 
		m_r * 2.f - 1.f,
		m_g * 2.f - 1.f,
		m_b * 2.f - 1.f );
}

inline void ColorF::SetFromNormal(const Vec3 & vec,const float a)
{
	m_r = vec.x * 0.5f + 0.5f;
	m_g = vec.y * 0.5f + 0.5f;
	m_b = vec.z * 0.5f + 0.5f;
	m_a = a;
}

//}{=======================================================================
// some math operators for ColorF ; acts like a 4-vector

inline void ColorF::operator *= (const float f)
{
	ASSERT(IsValid());
	m_r *= f;
	m_g *= f;
	m_b *= f;
	m_a *= f;
	ASSERT(IsValid());
}

inline void ColorF::operator *= (const ColorF & v)
{
	ASSERT(IsValid() && v.IsValid());
	m_r *= v.m_r;
	m_g *= v.m_g;
	m_b *= v.m_b;
	m_a *= v.m_a;
	ASSERT(IsValid());
}

inline void ColorF::operator += (const ColorF & v)
{
	ASSERT(IsValid() && v.IsValid());
	m_r += v.m_r;
	m_g += v.m_g;
	m_b += v.m_b;
	m_a += v.m_a;
	ASSERT(IsValid());
}

inline void ColorF::operator -= (const ColorF & v)
{
	ASSERT(IsValid() && v.IsValid());
	m_r -= v.m_r;
	m_g -= v.m_g;
	m_b -= v.m_b;
	m_a -= v.m_a;
	ASSERT(IsValid());
}

inline const ColorF operator * (const ColorF & u,const ColorF & v) // product
{
	ASSERT(u.IsValid() && v.IsValid());
	return ColorF( u.m_r * v.m_r , u.m_g * v.m_g , u.m_b * v.m_b , u.m_a * v.m_a );
}

inline const ColorF operator * (const float f,const ColorF & v) // scale
{
	ASSERT( fisvalid(f) && v.IsValid());
	return ColorF( f * v.m_r , f * v.m_g , f * v.m_b , f * v.m_a );
}

inline const ColorF operator * (const ColorF & v,const float f) // scale
{
	ASSERT( fisvalid(f) && v.IsValid());
	return ColorF( f * v.m_r , f * v.m_g , f * v.m_b , f * v.m_a );
}

inline const ColorF operator + (const ColorF & u,const ColorF & v) // add
{
	ASSERT(u.IsValid() && v.IsValid());
	return ColorF( u.m_r + v.m_r , u.m_g + v.m_g , u.m_b + v.m_b , u.m_a + v.m_a );
}

inline const ColorF operator - (const ColorF & u,const ColorF & v) // subtract
{
	ASSERT(u.IsValid() && v.IsValid());
	return ColorF( u.m_r - v.m_r , u.m_g - v.m_g , u.m_b - v.m_b , u.m_a - v.m_a );
}

//}{=======================================================================

/*
// more pure quantizer way :
// int value [i] represents the bucket (i,i+1)/256

inline int		ColorFTOI(float f)
{
	return ftoi(f * 256.f);
}

inline float	ColorITOF(int i)
{
	return (i + 0.5f) * (1.f/256.f);
}
*/

// way that maps 0<->0 and 255<->1.0
//  btw this also maps 0.5 -> 128

inline int		ColorFTOI(float f)
{
	//return ftoi(f * 255.f + 0.5f);
	return ftoi_round(f * 255.f);
}

inline float	ColorITOF(int i)
{
	return i * (1.f/255.f);
}

inline int		Clamp255( int i)
{
	return Clamp(i,0,255);
}

//}{=======================================================================

END_CB

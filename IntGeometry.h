#pragma once
#ifndef GALAXY_INTGEOMETRY_H
#define GALAXY_INTGEOMETRY_H

/*

IntGeometry provides some simple helpers for doing geometry
with integers.  Most of the real work is in Vec2i and Vec3i

*/

#include "Base.h"
#include "Vec3i.h"
#include "Vec2i.h"

START_CB

class Vec2;
class Vec3;
class AxialBox;
class RectF;

//=============================================================================

struct Facei
{
	Facei() { }
	Facei(const Vec3i & _a,const Vec3i & _b,const Vec3i & _c) : a(_a), b(_b), c(_c)
	{
		// make sure it's not degenerate :
		ASSERT( ! Colinear(a,b,c) );
	}

	enum ESide
	{
		eFront=0,
		eBack =1,
		eIntersecting
	};

	ESide Side(const Vec3i & p) const
	{
		const int64 v = Volume(a,b,c,p);
		if ( v == 0 ) return eIntersecting;
		else if ( v > 0 ) return eFront;
		else return eBack;
	}

	void Flip()
	{
		Swap(a,b);
	}

	// this is NOT accurate !!
	double Distance(const Vec3i & p) const;

	Vec3i	a;
	Vec3i	b;
	Vec3i	c;
};

//---------------------------------------------------------------------------

struct Edgei
{
	Edgei() { }
	Edgei(const Vec3i & _a,const Vec3i & _b) : a(_a), b(_b)
	{
		// make sure it's not degenerate :
		ASSERT( a != b );
	}

	Vec3i a;
	Vec3i b;

	bool operator == (const Edgei & other) const { return a == other.a && b == other.b; }
	bool operator != (const Edgei & other) const { return ! (*this == other); }

	static bool EqualFlipped(const Edgei &u,const Edgei &v) { return u.a == v.b && u.b == v.a; }

	void Flip() { Swap(a,b); }
};

//=============================================================================

namespace IntGeometry
{
	//-----------------------------------------
	
	extern const int c_coordinateMax;

	//-----------------------------------------
//};

	void Quantize(   Vec3i * pTo, const Vec3 * pFrom, const int numVerts, const AxialBox & ab );
	void Dequantize( Vec3 * pTo, const Vec3i * pFrom, const int numVerts, const AxialBox & ab );

	void Quantize(   Vec2i * pTo, const Vec2 * pFrom, const int numVerts, const RectF & r );
	void Dequantize( Vec2 * pTo, const Vec2i * pFrom, const int numVerts, const RectF & r );

	const Vec3i Quantize(  const Vec3 & from, const AxialBox & ab );
	const Vec3  Dequantize(const Vec3i & from, const AxialBox & ab );

	const Vec2i Quantize(  const Vec2 & from, const RectF & ab );
	const Vec2  Dequantize(const Vec2i & from, const RectF & ab );

	//-----------------------------------------
};

END_CB

//=============================================================================

#endif // GALAXY_INTGEOMETRY_H

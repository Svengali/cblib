#include "IntGeometry.h"
#include "cblib/AxialBox.h"
#include "cblib/Vec3.h"
#include "cblib/Vec2.h"
#include "cblib/Rect.h"
#include <limits.h>
#include <float.h>

START_CB

// int verts get 20 bits, since I need to hold *6* verts cubed in 64 bits
// note that it's also important that INT_VEC_BITS is less than the number of
//	bits in the mantissa of a float

#define INT_VEC_BITS	(20)
#define INT_VEC_MAX		(1<<INT_VEC_BITS)

COMPILER_ASSERT( INT_VEC_BITS <= FLT_MANT_DIG );
COMPILER_ASSERT( ((uint64)INT_VEC_MAX*INT_VEC_MAX*INT_VEC_MAX*6) <= _UI64_MAX );

const int IntGeometry::c_coordinateMax = INT_VEC_MAX;

//=============================================================================

double Facei::Distance(const Vec3i & p) const
{
	// @@ is there a better way to do this ?
	//	perhaps I should go ahead and generate the normal and do the
	//	ordinary plane-math thing ?

	const Vec3i e1 = b - a;
	const Vec3i e2 = c - a;

	const double nx = double(e1.y) * double(e2.z) - double(e1.z) * double(e2.y);
	const double ny = double(e1.z) * double(e2.x) - double(e1.x) * double(e2.z);
	const double nz = double(e1.x) * double(e2.y) - double(e1.y) * double(e2.x);

	const int64 vol = Volume(a,b,c,p);

	const double normal_length_square = nx*nx + ny*ny + nz*nz;
	const double normal_length = sqrt( normal_length_square );

	const double distance = double( vol ) / normal_length;

	/*
	const int64 vol = Volume(a,b,c,p);
	const double asqr = AreaSqrD(a,b,c);
	const double volume = double(vol);
	const double area = sqrt( double(asqr) );
	const double distance = volume / area;
	*/

	return distance;
}

//=============================================================================

/**********

Quantizer :

range 0-2

[0.....1.....2]
    X     X

two buckets; restore to midpoints

So round down in quantize; add 0.5 in restore

***********/

const Vec3i Quantize(  const Vec3 & from, const AxialBox & ab )
{
	ASSERT( ab.Contains(from) );
	const Vec3 unit = ab.ScaleVectorIntoBox(from);

	Vec3i to(
		ftoi( unit.x * INT_VEC_MAX ),
		ftoi( unit.y * INT_VEC_MAX ),
		ftoi( unit.z * INT_VEC_MAX ) );
	to.x = Clamp(to.x,0,INT_VEC_MAX-1);
	to.y = Clamp(to.y,0,INT_VEC_MAX-1);
	to.z = Clamp(to.z,0,INT_VEC_MAX-1);
	return to;
}
const Vec3  Dequantize(const Vec3i & from, const AxialBox & ab )
{
	ASSERT( from.x == Clamp(from.x,0,INT_VEC_MAX-1) );
	ASSERT( from.y == Clamp(from.y,0,INT_VEC_MAX-1) );
	ASSERT( from.z == Clamp(from.z,0,INT_VEC_MAX-1) );
	
	const Vec3 unit(
		(from.x + 0.5f) * (1.f / INT_VEC_MAX),
		(from.y + 0.5f) * (1.f / INT_VEC_MAX),
		(from.z + 0.5f) * (1.f / INT_VEC_MAX)
	);

	return ab.ScaleVectorFromBox(unit);
}

const Vec2i Quantize(  const Vec2 & from, const RectF & ab )
{
	ASSERT( ab.ContainsV(from) );
	const Vec2 unit = ab.ScaleVectorIntoRect(from);

	Vec2i to(
		ftoi( unit.x * INT_VEC_MAX ),
		ftoi( unit.y * INT_VEC_MAX ) );
	to.x = Clamp(to.x,0,INT_VEC_MAX-1);
	to.y = Clamp(to.y,0,INT_VEC_MAX-1);
	return to;
}
const Vec2  Dequantize(const Vec2i & from, const RectF & ab )
{
	ASSERT( from.x == Clamp(from.x,0,INT_VEC_MAX-1) );
	ASSERT( from.y == Clamp(from.y,0,INT_VEC_MAX-1) );
	
	const Vec2 unit(
		(from.x + 0.5f) * (1.f / INT_VEC_MAX),
		(from.y + 0.5f) * (1.f / INT_VEC_MAX)
	);

	return ab.ScaleVectorFromRect(unit);
}

void Quantize(   Vec3i * pTo, const Vec3 * pFrom, const int numVerts, const AxialBox & ab )
{
	for(int i=0;i<numVerts;i++)
	{
		pTo[i] = Quantize( pFrom[i], ab );
	}
}

void Dequantize( Vec3 * pTo, const Vec3i * pFrom, const int numVerts, const AxialBox & ab )
{
	for(int i=0;i<numVerts;i++)
	{
		pTo[i] = Dequantize( pFrom[i], ab );
	}
}

void Quantize(   Vec2i * pTo, const Vec2 * pFrom, const int numVerts, const RectF & r )
{
	for(int i=0;i<numVerts;i++)
	{
		pTo[i] = Quantize( pFrom[i], r );
	}
}

void Dequantize( Vec2 * pTo, const Vec2i * pFrom, const int numVerts, const RectF & r )
{
	for(int i=0;i<numVerts;i++)
	{
		pTo[i] = Dequantize( pFrom[i], r );
	}
}

END_CB
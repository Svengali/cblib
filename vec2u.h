#pragma once

#include "cblib/Vec2.h"
#include "cblib/Vec3.h"

//! would normally be called Vec2Util , but abbreviated due to very common use
//! Vec2U contains the Vec2 <-> Vec3 conversions (lifting and projection)
START_CB

//{========================================================================================
// PROTOS 

	float DistanceSqr(const Vec2 & a,const Vec2 &b);
	float Distance(const Vec2 & a,const Vec2 &b);
	
	//! little IsShort utility
	bool IsShort(const Vec2 & v, const float len = EPSILON);
	bool IsShort(const Vec2 & v1,const Vec2 & v2, const float len = EPSILON);
	
	//----------------------------------------------------------------------------------

	//! length-based clampers :
	//!	(NOTEZ : Clamping to Min Len is dangerous/undefined if the
	//	original is very short !!!!)
	void ClampToMaxLen(Vec2 * pVec,const float maxLen);
	void ClampToMinLen(Vec2 * pVec,const float minLen);
	void ClampLength(Vec2 * pVec,const float minLen,const float maxLen);
	
	//! returns 0-1 ; which component has the largest abs value
	int GetLargestComponent(const Vec2 & v);
	int GetSmallestComponent(const Vec2 & v);
	
	//-----------------------------------------

	//! Cross in 2d is a scalar.
	//!	this is well-defined as the hodge-dual of the wedge-products :
	//!	result = *( u /\ v)
	//!	this is also the determinant of the Mat3 made from u and v as rows
	float Cross(const Vec2 & u,const Vec2 & v); // cross

	//! TriangleCross
	//!	with return positive if the winding is clockwise !
	float TriangleCrossCW(const Vec2 & a,const Vec2 & b,const Vec2 & c);

	inline float TriangleAreaCW(const Vec2 & a,const Vec2 & b,const Vec2 & c)
	{
		return TriangleCrossCW(a,b,c) * 0.5f;
	}

	inline float TriangleCrossCCW(const Vec2 & a,const Vec2 & b,const Vec2 & c)
	{
		return - TriangleCrossCW(a,b,c);
	}

	inline float TriangleAreaCCW(const Vec2 & a,const Vec2 & b,const Vec2 & c)
	{
		return TriangleCrossCW(a,b,c) * (-0.5f);
	}

	//-----------------------------------------

	//! *pv = a random normal, with a correct circular distribution
	void SetRandomNormal(Vec2 * pv);
	const Vec2 MakeRandomNormal();

	//----------------------------------------------------------------------------------
	// the "Make" functions all create a Vec2 by value
	//	the __forceinline and the const on all of these returns is quite important

	//! make the Vec3 by taking the Vec2 and putting a Zero in Z
	const Vec3 MakeLiftZeroZ(const Vec2 & v);

	//! make a Vec2 from the XY part of a Vec3
	const Vec2 MakeProjectionXY(const Vec3 & v);
	//! make a Vec2 by projecting a Vec3 onto two axes
	const Vec2 MakeProjection(const Vec3 & v,const Vec3 & x_axis, const Vec3 & y_axis);
	
	//! Make the Perp of a Vec2 in either rotation direction
	const Vec2 MakePerpCW(const Vec2 & v);
	const Vec2 MakePerpCCW(const Vec2 & v);

	//! return {1,0} rotated around "Z" by "radians"
	const Vec2 MakeRotation(const float radians);

	//! rotate "v" around "Z" by "radians"
	const Vec2 MakeRotated(const Vec2 & v,const float radians);

	//! Math operators; see Vec3U
	const Vec2 MakeAverage(const Vec2 &a,const Vec2 &b);
	const Vec2 MakeWeightedSum(const float ca,const Vec2 &a,const float cb,const Vec2 &b);
	const Vec2 MakeWeightedSum(const Vec2 &a,const float cb,const Vec2 &b);
	const Vec2 MakeComponentwiseScaled(const Vec2 &a,const Vec2 &b);
	const Vec2 MakeScaled(const Vec2 &a,const float ca);
	const Vec2 MakeAbs(const Vec2 &a);
	const Vec2 MakeMin(const Vec2 & a,const Vec2 & b);
	const Vec2 MakeMax(const Vec2 & a,const Vec2 & b);
	const Vec2 MakeClamped(const Vec2 & v,const Vec2 & lo,const Vec2 & hi);
	const Vec2 MakeLerp(const Vec2 &v1,const Vec2 &v2,const float t);
	const Vec2 MakeNormalizedFast(const Vec2 & v);
	const Vec2 MakeNormalizedSafe(const Vec2 & v, const Vec2 & fallback = Vec2::unitX);

	const Vec2 MakeNormalFast(const Vec2 & fm,const Vec2 &to);
	const Vec2 MakeNormalSafe(const Vec2 & fm,const Vec2 &to,const Vec2 & fallback = Vec2::unitX);
	
	//! Returns a random point around center that is in the AABB square bounding the circle given by the radius
	const Vec2 MakeRandomInBox( const Vec2 &center, const float radius );
	
	//! Same as above except you can specify the radius for x and y seperately
	const Vec2 MakeRandomInBox( const Vec2 &center, const Vec2 &halfSize );


	const bool SetAngularRotated(Vec2 * pLerp,const Vec2 & normal1,const Vec2 & normal2,const float radians);

	//! GetAngleBetweenNormals is expensive!
	//	GetAngleBetweenNormals is >= 0 and <= PI
	const float GetAngleBetweenNormals(const Vec2 &a,const Vec2 & b);
	const float GetSignOfAngleBetweenNormals(const Vec2 &a,const Vec2 & b);  // +clockwise, -counter

	// Vec2 version of FloatUtil DampedDrive
	const Vec2 LerpedDriveVec(const Vec2 & val,const Vec2 & towards,const float speed,const float time_step);
	const Vec2 LerpedDriveComponents(const Vec2 & val,const Vec2 & towards,const float speed,const float time_step);
	
//}{========================================================================================
// INLINE FUNCTIONS

	inline float DistanceSqr(const Vec2 & a,const Vec2 &b)
	{
		ASSERT( a.IsValid() && b.IsValid() );
		return fsquare(a.x - b.x) + fsquare(a.y - b.y);
	}

	inline float Distance(const Vec2 & a,const Vec2 &b)
	{
		return sqrtf( DistanceSqr(a,b) );
	}
	
	__forceinline bool IsShort(const Vec2 & v, const float len /*= EPSILON*/)
	{
		return v.LengthSqr() <= fsquare(len);
	}
	__forceinline bool IsShort(const Vec2 & v1,const Vec2 & v2, const float len /*= EPSILON*/)
	{
		return DistanceSqr(v1,v2) <= fsquare(len);
	}
	
	//-----------------------------------------

	// Cross in 2d is a scalar.
	//	this is well-defined as the hodge-dual of the wedge-products :
	//	result = *( u /\ v)
	//	this is also the determinant of the Mat3 made from u and v as rows
	inline float Cross(const Vec2 & u,const Vec2 & v) // cross
	{
		ASSERT( u.IsValid() && v.IsValid() );
		return ( u.x * v.y - u.y * v.x );
	}

	// TriangleCross
	//	with return positive if the winding is clockwise !
	inline float TriangleCrossCW(const Vec2 & a,const Vec2 & b,const Vec2 & c)
	{
		ASSERT( a.IsValid() && b.IsValid() && c.IsValid() );
		//const Vec2 e1 = b - a;
		//const Vec2 e2 = c - a;
		//const float cross = Cross(e2,e1);
		const float cross = (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
		return cross;
	}

	//--------------------------------------------------------------------------------
	
	__forceinline void ClampLength(Vec2 * pVec,const float minLen,const float maxLen)
	{
		float len = pVec->Length();
		if ( len < minLen )
		{
			// can be problematic :
			if ( len == 0.f )
				return;
			*pVec *= (minLen / len);
		}
		else if ( len > maxLen )
		{
			*pVec *= (maxLen / len);
		}
	}
	__forceinline void ClampToMaxLen(Vec2 * pVec,const float maxLen)
	{
		float len = pVec->Length();
		if ( len > maxLen )
		{
			*pVec *= (maxLen / len);
		}
	}
	
	__forceinline void ClampToMinLen(Vec2 * pVec,const float minLen)
	{
		float len = pVec->Length();
		if ( len < minLen )
		{
			// can be problematic :
			if ( len == 0.f )
				return;
			*pVec *= (minLen / len);
		}
	}
	
	//--------------------------------------------------------------------------------
	// the "Make" functions all create a Vec2 by value
	//	the __forceinline and the const on all of these returns is quite important

	__forceinline const Vec3 MakeLiftZeroZ(const Vec2 & v)
	{
		ASSERT( v.IsValid() );
		return Vec3(v.x,v.y,0.f);
	}

	__forceinline const Vec2 MakeProjectionXY(const Vec3 & v)
	{
		ASSERT( v.IsValid() );
		return Vec2(v.x,v.y);
	}

	__forceinline const Vec2 MakeProjection(const Vec3 & v,const Vec3 & x_axis, const Vec3 & y_axis)
	{
		ASSERT( v.IsValid() && x_axis.IsValid() && y_axis.IsValid() );
		return Vec2( v * x_axis , v * y_axis);
	}

	__forceinline const Vec2 MakePerpCW(const Vec2 & v)
	{
		ASSERT( v.IsValid() );
		return Vec2( v.y, -v.x);
	}

	__forceinline const Vec2 MakePerpCCW(const Vec2 & v)
	{
		ASSERT( v.IsValid() );
		return Vec2(-v.y, v.x);
	}

	__forceinline const Vec2 MakeAverage(const Vec2 &a,const Vec2 &b)
	{
		ASSERT( a.IsValid() && b.IsValid() );
		return Vec2(
			(a.x + b.x)*0.5f,
			(a.y + b.y)*0.5f
		);
	}
	__forceinline const Vec2 MakeWeightedSum(const float ca,const Vec2 &a,const float cb,const Vec2 &b)
	{
		ASSERT( a.IsValid() && b.IsValid() );
		return Vec2(
			ca * a.x + cb * b.x,
			ca * a.y + cb * b.y
		);
	}
	__forceinline const Vec2 MakeWeightedSum(const Vec2 &a,const float cb,const Vec2 &b)
	{
		ASSERT( a.IsValid() && b.IsValid() );
		return Vec2(
			a.x + cb * b.x,
			a.y + cb * b.y
		);
	}
	__forceinline const Vec2 MakeComponentwiseScaled(const Vec2 &a,const Vec2 &b)
	{
		ASSERT( a.IsValid() && b.IsValid() );
		return Vec2(
			b.x * a.x,
			b.y * a.y
		);
	}
	__forceinline const Vec2 MakeScaled(const Vec2 &a,const float ca)
	{
		ASSERT( a.IsValid() );
		return Vec2(
			ca * a.x,
			ca * a.y
		);
	}
	__forceinline const Vec2 MakeAbs(const Vec2 &a)
	{
		ASSERT( a.IsValid() );
		return Vec2(
			fabsf(a.x),
			fabsf(a.y)
		);
	}
	__forceinline const Vec2 MakeMin(const Vec2 & a,const Vec2 & b)
	{
		ASSERT( a.IsValid() && b.IsValid() );
		return Vec2(
			Min(a.x,b.x),
			Min(a.y,b.y)
		);
	}
	__forceinline const Vec2 MakeMax(const Vec2 & a,const Vec2 & b)
	{
		ASSERT( a.IsValid() && b.IsValid() );
		return Vec2(
			Max(a.x,b.x),
			Max(a.y,b.y)
		);
	}
	__forceinline const Vec2 MakeClamped(const Vec2 & v,const Vec2 & lo,const Vec2 & hi)
	{
		ASSERT( v.IsValid() && lo.IsValid() && hi.IsValid() );
		return Vec2(
			Clamp(v.x,lo.x,hi.x),
			Clamp(v.y,lo.y,hi.y)
		);
	}
	__forceinline const Vec2 MakeLerp(const Vec2 &v1,const Vec2 &v2,const float t)
	{
		ASSERT( v1.IsValid() && v2.IsValid() );
		return Vec2(
			v1.x + t * (v2.x - v1.x),
			v1.y + t * (v2.y - v1.y)
		);
	}

	__forceinline const Vec2 MakeNormalizedFast(const Vec2 & v)
	{
		return Vec2(Vec2::eNormalize,v);
	}

	__forceinline const Vec2 MakeNormalizedSafe(const Vec2 & v, const Vec2 & fallback)
	{
		//v passed as an actual non-const parameter to get a copy we can normalize. 
		Vec2 ret(v);
		ret.NormalizeSafe(fallback);
		return ret;
	}
	
	__forceinline const Vec2 MakeNormalFast(const Vec2 & fm,const Vec2 &to)
	{
		return Vec2(Vec2::eNormalize, to.x-fm.x,to.y-fm.x);
	}
	__forceinline const Vec2 MakeNormalSafe(const Vec2 & fm,const Vec2 &to,const Vec2 & fallback)
	{
		Vec2 ret(to-fm);
		ret.NormalizeSafe(fallback);
		return ret;
	}

	__forceinline const Vec2 LerpedDriveComponents(const Vec2 & from,const Vec2 & to,const float speed,const float time_step)
	{
		return Vec2(
			LerpedDrive(from.x,to.x,speed,time_step),
			LerpedDrive(from.y,to.y,speed,time_step));
	}

	__forceinline const Vec2 LerpedDriveVec(const Vec2 & from,const Vec2 & to,const float speed,const float time_step)
	{
		const float step = speed * time_step;        
		ASSERT( step > 0.f );
		const Vec2 delta = to - from;
		const float deltaLen = delta.Length();
        
		if ( deltaLen <= step )
        {
			return to;
        }
        else
        {
            const Vec2 stepV = delta * (step / deltaLen);
			return from + stepV;
        }
	}

//}========================================================================================
	
	//----------------------------------------------------------------------------------
END_CB

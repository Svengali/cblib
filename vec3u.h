/*!
  \file
  \brief 3-Space Vector Utilities
*/

#pragma once

#include "cblib/Vec3.h"
#include "cblib/Util.h"

START_CB

// would normally be called Vec3Util , but abbreviated due to very common use
//namespace Vec3U
//{

//{========================================================================================
// PROTOS 

	float DistanceSqr(const Vec3 & a,const Vec3 &b);
	float Distance(const Vec3 & a,const Vec3 &b);

	float DistanceSqrXY(const Vec3 & a,const Vec3 &b);
	float DistanceXY(const Vec3 & a,const Vec3 &b);
	
	//! little IsShort utility
	bool IsShort(const Vec3 & v, const float len = EPSILON);
	bool IsShort(const Vec3 & v1,const Vec3 & v2, const float len = EPSILON);
	
	float HorizontalSum(const Vec3 & v);
	
	//----------------------------------------------------------------------------------

	//! length-based clampers :
	//!	(NOTEZ : Clamping to Min Len is dangerous/undefined if the
	//	original is very short !!!!)
	void ClampToMaxLen(Vec3 * pVec,const float maxLen);
	void ClampToMinLen(Vec3 * pVec,const float minLen);
	void ClampLength(Vec3 * pVec,const float minLen,const float maxLen);
	
	//! project v to the part of it perp to "normal"
	const Vec3 ProjectPerp(const Vec3 & v,const Vec3 & normal);
	
	//! get two semi-random vectors perp to this one
	//!	useful for making frames from a look-at vector
	//! fills out v1 and v2
	//	Note that these give you consistent winding :
	//		( v * (v1 x v2) ) > 0
	//	(= 1.0 for the normal case)
	void GetTwoPerp(const Vec3 & v,Vec3 * pv1,Vec3 * pv2);
	void GetTwoPerpNormals(const Vec3 & v,Vec3 * pv1,Vec3 * pv2);

	//! returns 0-2 ; which component has the largest abs value
	int GetLargestComponent(const Vec3 & v);
	int GetSmallestComponent(const Vec3 & v);
	
	//! returns v[GetLargestComponent(v)]
	float GetLargestComponentValue(const Vec3 & v);
	
	//! point in Z-cylinder
	//!	 base is at the bottom-center of the cylinder
	bool PointInCylinder(const Vec3 & point, const Vec3 & base, const float radius, const float height );
	
	//! *pv = the cross of the edges of the triangle a,b,c
	//!	its magnitude is twice the area of the triangle
	// normal points out from *counter-clockwise* winding of the triangle
	void SetTriangleCross(Vec3 * pv,const Vec3 &a,const Vec3 &b,const Vec3 &c);

	float TriangleArea(const Vec3 &a,const Vec3 &b,const Vec3 &c);

	//! *pv = the normal of the triangle a,b,c
	//!	returns the length (twice the area of the triangle)
	float SetTriangleNormal(Vec3 * pv,const Vec3 &a,const Vec3 &b,const Vec3 &c);

	//! return Epsilon_ijk A_i B_j C_k = (a * (b X c))
	float TripleProduct(const Vec3 &a,const Vec3 &b,const Vec3 &c);

	//! *pv = a random normal, with a correct spherical distribution
	void SetRandomNormal(Vec3 * pv);

	void SetRandomInUnitCube(Vec3 * pv);

	//! Get the two polar coordinates that define a normal (asserts on isnormalized)
	//!	(WARNING : by "theta" and "phi" names are the opposite of what some people use)
	//!	 Theta in [-pi,pi] , Phi in [-pi/2,pi/2]
	void GetPolarFromNormal(const Vec3 & normal,float * pTheta, float * pPhi);
	void SetNormalFromPolar(Vec3 * pNormal,const float theta, const float phi);

	//! Does an angular lerp of two normals (using quats, btw, actually a slerp)
	//!	 SLOW , but nice
	void SetAngularLerp(Vec3 * pSlerp,const Vec3 & normal1,const Vec3 & normal2,const float t);

	//! SetPolarLerp is like SetAngularLerp but uses Z-up to prevent
	//!	 pitching of the normals
	void SetPolarLerp(Vec3 * pSlerp,const Vec3 & normal1,const Vec3 & normal2,const float t);

	//! SetAngularRotated is like SetAngularLerp, but it rotates a max "radians"
	//!	 in the rotation sweep
	//!	returns whether the end was reached
	bool SetAngularRotated(Vec3 * pSlerp,const Vec3 & normal1,const Vec3 & normal2,const float radians);

	//! GetAngleBetweenNormals is expensive!
	//	is in [0,PI]
	float GetAngleBetweenNormals(const Vec3 &a,const Vec3 & b);

	//! get the cotangent of [v0,apex,v1]
	// cotangent goes from infinity (zero angle) to -infinity (180)
	// cotangent(90) = 0
	// cotangent(60) = sqrt(1/3)
	float GetCotangent(const Vec3 & apex, const Vec3 & v0, const Vec3 & v1);

	//! get the angle of [v0,apex,v1]
	//	angle in [0,pi]
	float GetAngle(const Vec3 & apex, const Vec3 & v0, const Vec3 & v1);

	//! returns distanceSqr and time in [0,1]
	void ComputeClosestPointToEdge(const Vec3 & point,const Vec3 &fm,const Vec3 &to,
							float * pDistSqr, float * pTime);

	//! returns time in [0,1], the point is MakeLerp(edgeFm,edgeTo,time);
	void ComputeClosestOnEdgeToLine(const Vec3 &edgeFm,const Vec3 &edgeTo,
							const Vec3 & lineBase,const Vec3 & lineNormal,
							float * pTime);

	//! Rat-Tri intersection code
	bool RayTriangleIntersect(const Vec3 & vert0,const Vec3 & vert1,const Vec3 & vert2, 
									const Vec3 & orig,const Vec3 & dir,
									float * pB0,float * pB1,float *pT);

	//----------------------------------------------------------------------------------
	//! the "Make" functions all create a Vec3 by value
	//!	the __forceinline and the const on all of these returns is quite important

	const Vec3 MakeAverage(const Vec3 &a,const Vec3 &b);
	const Vec3 MakeWeightedSum(const float ca,const Vec3 &a,const float cb,const Vec3 &b);
	const Vec3 MakeWeightedSum(const Vec3 &a,const float cb,const Vec3 &b);
	const Vec3 MakeComponentwiseScaled(const Vec3 &a,const Vec3 &b);
	const Vec3 MakeScaled(const Vec3 &a,const float ca);
	const Vec3 MakeAbs(const Vec3 &a);
	const Vec3 MakeMin(const Vec3 & a,const Vec3 & b);
	const Vec3 MakeMax(const Vec3 & a,const Vec3 & b);
	const Vec3 MakeClamped(const Vec3 & v,const Vec3 & lo,const Vec3 & hi);
	const Vec3 MakeCross(const Vec3 & u,const Vec3 & v);
	const Vec3 MakeLerp(const Vec3 &v1,const Vec3 &v2,const float t);	
	const Vec3 MakeInverse(const Vec3 &a);
	//const Vec3 MakeProjectionXY(const Vec3 & v);
	//const Vec3 MakeProjectionXZ(const Vec3 & v);
	//const Vec3 MakeProjectionYZ(const Vec3 & v);
	const Vec3 MakeNormalizedFast(const Vec3 & v);
	const Vec3 MakeNormalizedSafe(const Vec3 & v, const Vec3 & fallback = Vec3::unitZ);
	const Vec3 MakeRandomInSphere(const float radius);
	const Vec3 MakeRandomInSphereFast(const float radius);

	const Vec3 MakeNormalFast(const Vec3 & fm,const Vec3 &to);
	const Vec3 MakeNormalSafe(const Vec3 & fm,const Vec3 &to,const Vec3 & fallback = Vec3::unitZ);
	
	//! Returns a random point around center that is in the AABB cube bounding the sphere given by the radius
	const Vec3 MakeRandomInBox( const Vec3 &center, const float radius );
	
	//! Same as above except you can specify the radius for x y and z seperately
	const Vec3 MakeRandomInBox( const Vec3 &center, const Vec3 &halfSize );

	const Vec3 MakeUnitPreservingZFast( const float x, const float y, const float z ); 
	const Vec3 MakeUnitPreservingZSafe( const float x, const float y, const float z, const Vec3& fallback = Vec3::unitZ ); 

	const Vec3 LerpedDriveVec(const Vec3 & from,const Vec3 & to,const float speed,const float time_step);
	const Vec3 LerpedDriveComponents(const Vec3 & from,const Vec3 & to,const float speed,const float time_step);

//}{========================================================================================
// INLINE FUNCTIONS

	inline float DistanceSqr(const Vec3 & a,const Vec3 &b)
	{
		ASSERT( a.IsValid() && b.IsValid() );
		return fsquare(a.x - b.x) + fsquare(a.y - b.y) + fsquare(a.z - b.z);
	}

	inline float Distance(const Vec3 & a,const Vec3 &b)
	{
		return sqrtf( DistanceSqr(a,b) );
	}

	inline float DistanceSqrXY(const Vec3 & a,const Vec3 &b)
	{
		ASSERT( a.IsValid() && b.IsValid() );
		return fsquare(a.x - b.x) + fsquare(a.y - b.y);
	}

	inline float DistanceXY(const Vec3 & a,const Vec3 &b)
	{
		return sqrtf( DistanceSqrXY(a,b) );
	}
	
	inline float HorizontalSum(const Vec3 & v)
	{
		return v.x + v.y + v.z;
	}

	//----------------------------------------------------------------------------------

	inline void GetTwoPerpNormals(const Vec3 & v,Vec3 * pv1,Vec3 * pv2)
	{
		// GetTwoPerp will do plenty of ASSERTing for us
		// pv1 and pv2 are (any) two normals perpendicular to v
		GetTwoPerp(v,pv1,pv2);
		pv1->NormalizeFast();
		pv2->NormalizeFast();
	}
	
	inline float TriangleArea(const Vec3 &a,const Vec3 &b,const Vec3 &c)
	{
		Vec3 n;
		SetTriangleCross(&n,a,b,c);
		return n.Length() * 0.5f;
	}

	// normal points out from *counter-clockwise* winding of the triangle
	inline void SetTriangleCross(Vec3 * pv,const Vec3 &a,const Vec3 &b,const Vec3 &c)
	{
		// *pv = the cross of the edges of the triangle a,b,c
		//	its magnitude is twice the area of the triangle
		ASSERT(pv);
		ASSERT( a.IsValid() && b.IsValid() && c.IsValid() );

		const Vec3 e1 = b - a;
		const Vec3 e2 = c - a;
		pv->SetCross(e1,e2);
		ASSERT(pv->IsValid());
	}

	// normal points out from *counter-clockwise* winding of the triangle
	inline float SetTriangleNormal(Vec3 * pv,const Vec3 &a,const Vec3 &b,const Vec3 &c)
	{
		// *pv = the normal of the triangle a,b,c
		//	returns the length (twice the area of the triangle)
		SetTriangleCross(pv,a,b,c);
		return pv->NormalizeSafe();
	}

	inline float TripleProduct(const Vec3 &a,const Vec3 &b,const Vec3 &c)
	{
		// return Epsilon_ijk A_i B_j C_k
		ASSERT( a.IsValid() && b.IsValid() && c.IsValid() );
		const float t =
			a.x * (b.y * c.z - b.z * c.y) +
			a.y * (b.z * c.x - b.x * c.z) +
			a.z * (b.x * c.y - b.y * c.x);
		return t;
	}

	//----------------------------------------------------------------------------------
	// the "Make" functions all create a Vec3 by value
	//	the __forceinline and the const on all of these returns is quite important

	__forceinline const Vec3 MakeAverage(const Vec3 &a,const Vec3 &b)
	{
		ASSERT( a.IsValid() && b.IsValid() );
		return Vec3(
			(a.x + b.x)*0.5f,
			(a.y + b.y)*0.5f,
			(a.z + b.z)*0.5f
		);
	}
	__forceinline const Vec3 MakeWeightedSum(const float ca,const Vec3 &a,const float cb,const Vec3 &b)
	{
		ASSERT( a.IsValid() && b.IsValid() );
		return Vec3(
			ca * a.x + cb * b.x,
			ca * a.y + cb * b.y,
			ca * a.z + cb * b.z
		);
	}
	__forceinline const Vec3 MakeWeightedSum(const Vec3 &a,const float cb,const Vec3 &b)
	{
		ASSERT( a.IsValid() && b.IsValid() );
		return Vec3(
			a.x + cb * b.x,
			a.y + cb * b.y,
			a.z + cb * b.z
		);
	}
	__forceinline const Vec3 MakeComponentwiseScaled(const Vec3 &a,const Vec3 &b)
	{
		ASSERT( a.IsValid() && b.IsValid() );
		return Vec3(
			b.x * a.x,
			b.y * a.y,
			b.z * a.z
		);
	}
	__forceinline const Vec3 MakeScaled(const Vec3 &a,const float ca)
	{
		ASSERT( a.IsValid() );
		return Vec3(
			ca * a.x,
			ca * a.y,
			ca * a.z
		);
	}
	__forceinline const Vec3 MakeAbs(const Vec3 &a)
	{
		ASSERT( a.IsValid() );
		return Vec3(
			fabsf(a.x),
			fabsf(a.y),
			fabsf(a.z)
		);
	}
	__forceinline const Vec3 MakeInverse(const Vec3 &a)
	{
		ASSERT( a.IsValid() );
		return Vec3(
			-(a.x),
			-(a.y),
			-(a.z)
		);
	}
	__forceinline const Vec3 MakeMin(const Vec3 & a,const Vec3 & b)
	{
		ASSERT( a.IsValid() && b.IsValid() );
		return Vec3(
			MIN(a.x,b.x),
			MIN(a.y,b.y),
			MIN(a.z,b.z)
		);
	}
	__forceinline const Vec3 MakeMax(const Vec3 & a,const Vec3 & b)
	{
		ASSERT( a.IsValid() && b.IsValid() );
		return Vec3(
			MAX(a.x,b.x),
			MAX(a.y,b.y),
			MAX(a.z,b.z)
		);
	}
	__forceinline const Vec3 MakeClamped(const Vec3 & v,const Vec3 & lo,const Vec3 & hi)
	{
		ASSERT( v.IsValid() && lo.IsValid() && hi.IsValid() );
		return Vec3(
			Clamp(v.x,lo.x,hi.x),
			Clamp(v.y,lo.y,hi.y),
			Clamp(v.z,lo.z,hi.z)
		);
	}
	__forceinline const Vec3 MakeCross(const Vec3 & u,const Vec3 & v)
	{
		ASSERT( u.IsValid() && v.IsValid() );
		return Vec3( 
			u.y * v.z - u.z * v.y,
			u.z * v.x - u.x * v.z,
			u.x * v.y - u.y * v.x );
	}
	__forceinline const Vec3 MakeLerp(const Vec3 &v1,const Vec3 &v2,const float t)
	{
		ASSERT( v1.IsValid() && v2.IsValid() );
		float	t1 = 1.0f - t;
		return Vec3(
			v1.x * t1 + v2.x * t,
			v1.y * t1 + v2.y * t,
			v1.z * t1 + v2.z * t
		);
	}

	/*
	__forceinline const Vec3 MakeProjectionXY(const Vec3 & v)
	{
		ASSERT( v.IsValid() );
		return Vec3(v.x,v.y,0.f);
	}

	__forceinline const Vec3 MakeProjectionXZ(const Vec3 & v)
	{
		ASSERT( v.IsValid() );
		return Vec3(v.x,0.f,v.z);
	}
	
	__forceinline const Vec3 MakeProjectionYZ(const Vec3 & v)
	{
		ASSERT( v.IsValid() );
		return Vec3(0.f,v.y,v.z);
	}
	*/
	
	__forceinline const Vec3 MakeNormalizedFast(const Vec3 & v)
	{
		return Vec3(Vec3::eNormalize,v);
	}

	__forceinline const Vec3 MakeNormalizedSafe(const Vec3 & v, const Vec3 & fallback)
	{
		//v passed as an actual non-const parameter to get a copy we can normalize. 
		Vec3 ret(v);
		ret.NormalizeSafe(fallback);
		return ret;
	}
	
	__forceinline const Vec3 MakeNormalFast(const Vec3 & fm,const Vec3 &to)
	{
		return Vec3(Vec3::eNormalize, to.x-fm.x,to.y-fm.x,to.z-fm.z);
	}
	__forceinline const Vec3 MakeNormalSafe(const Vec3 & fm,const Vec3 &to,const Vec3 & fallback)
	{
		Vec3 ret(to-fm);
		ret.NormalizeSafe(fallback);
		return ret;
	}
	
	__forceinline void ClampLength(Vec3 * pVec,const float minLen,const float maxLen)
	{
		const float lenSqr = pVec->LengthSqr();
		if ( lenSqr < fsquare(minLen) )
		{
			// can be problematic :
			if ( lenSqr == 0.f )
				return;
			*pVec *= (minLen / sqrtf(lenSqr));
		}
		else if ( lenSqr > fsquare(maxLen) )
		{
			*pVec *= (maxLen / sqrtf(lenSqr));
		}
	}
	__forceinline void ClampToMaxLen(Vec3 * pVec,const float maxLen)
	{
		const float lenSqr = pVec->LengthSqr();
		if ( lenSqr > fsquare(maxLen) )
		{
			*pVec *= (maxLen / sqrtf(lenSqr));
		}
	}
	
	__forceinline void ClampToMinLen(Vec3 * pVec,const float minLen)
	{
		const float lenSqr = pVec->LengthSqr();
		if ( lenSqr < fsquare(minLen) )
		{
			// can be problematic :
			if ( lenSqr == 0.f )
				return;
			*pVec *= (minLen / sqrtf(lenSqr));
		}
	}

	//! Use to compose a hybrid unit vector out of three components, which usually comes from 
	//! two normalized vectors. XY from one vector, Z from the other. The idea is to maintain the 
	//! XY relationship, while absolutely preserving the Z value. 
	//! Note, there are some rough cases here, that cannot lead to a unit vector. 
	//! These cases are: 
	//! 1. abs(z) > 1
	//! 2. x==0 and y==0
	//! This is a pretty expensive and tricky call to use. It's not something to be done lightly. 
	//! 
	__forceinline const Vec3 MakeUnitPreservingZFast( const float x, const float y, const float z )
	{
		ASSERT( fiszerotoone(fabsf(z)) ); //You're gonna get a Vec3 with NaN's in it if this assert fails. 
		const float denom = fsquare(x) + fsquare(y);
		ASSERT( denom > 0.f );
		const float a = sqrtf((1.f - fsquare(z)) / denom);
		return Vec3( a * x, a * y, z );
	}

	__forceinline const Vec3 MakeUnitPreservingZSafe( const float x, const float y, const float z, const Vec3& fallback /*=Vec3::unitX*/ )
	{
		const bool xyZero = fiszero(x) && fiszero(y);
		if( xyZero && fisone(fabsf(z)) )
		{
			return Vec3(x, y, z);
		}
		else if( xyZero || fabsf(z) > 1.f )
		{
			return fallback;
		}
		return MakeUnitPreservingZFast( x, y, z );
	}
	//}========================================================================================

	__forceinline const Vec3 LerpedDriveComponents(const Vec3 & from,const Vec3 & to,const float speed,const float time_step)
	{
		return Vec3(
			LerpedDrive(from.x,to.x,speed,time_step),
			LerpedDrive(from.y,to.y,speed,time_step),
			LerpedDrive(from.z,to.z,speed,time_step));
	}
	
	// !! CB : LerpedDriveComponents is generally what you want, use it !!
	__forceinline const Vec3 LerpedDriveVec(const Vec3 & from,const Vec3 & to,const float speed,const float time_step)
	{
		const float step = speed * time_step;        
		ASSERT( step > 0.f );
		const Vec3 delta = to - from;
		const float deltaLen = delta.Length();
        
		if ( deltaLen <= step )
        {
			return to;
        }
        else
        {
            const Vec3 stepV = delta * (step / deltaLen);
			return from + stepV;
        }
	}	
	
	__forceinline bool IsShort(const Vec3 & v, const float len /*= EPSILON*/)
	{
		return v.LengthSqr() <= fsquare(len);
	}
	__forceinline bool IsShort(const Vec3 & v1,const Vec3 & v2, const float len /*= EPSILON*/)
	{
		return DistanceSqr(v1,v2) <= fsquare(len);
	}

	__forceinline const Vec3 ProjectPerp(const Vec3 & v,const Vec3 & normal)
	{
		ASSERT( normal.IsNormalized() );
		float dot = v * normal;
		return (v - dot * normal);
	}

	void ApplyDampedDrive(
					Vec3 * pValue,
					Vec3 * pVelocity,
					const Vec3 & toValue,
					const float frequency,
					const float dt );
					
	void ApplyDampedDriveClamping(
					Vec3 * pValue,
					Vec3 * pVelocity,
					const Vec3 & toValue,
					const float frequency,
					const float minVelocity,
					const float maxVelocity,
					const float minStep,
					const float dt );
//};

END_CB

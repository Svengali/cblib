#include "cblib/Vec3U.h"
#include "cblib/Rand.h"
#include "cblib/QuatUtil.h"

START_CB

//----------------------------------------------------------------------------------

//! Does an angular lerp of two normals (using quats, btw, actually a slerp)
//!	 SLOW , but nice
void SetAngularLerp(Vec3 * pSlerp,const Vec3 & normal1,const Vec3 & normal2,const float t)
{
	ASSERT( normal1.IsNormalized() && normal2.IsNormalized() );

	// make the full rotation :
	Quat qrotation;
	SetRotationArc(&qrotation,normal1,normal2);

	Vec3 axis;
	float angle;
	qrotation.GetAxisAngleMod2Pi(&axis,&angle);

	// ramp it up :
	Quat qpartial;
	qpartial.SetFromAxisAngle(axis, angle * t );

	*pSlerp = qpartial.Rotate( normal1 );
}

float GetAngleBetweenNormals(const Vec3 &a,const Vec3 & b)
{
	ASSERT( a.IsNormalized() && b.IsNormalized() );
	const float dot = a * b;
	const float angle = acosf_safe(dot);
	ASSERT( fisinrange(angle,0.f,PIf) );
	return angle;
}

bool SetAngularRotated(Vec3 * pSlerp,const Vec3 & normal1,const Vec3 & normal2,const float radians)
{
	ASSERT( normal1.IsNormalized() && normal2.IsNormalized() );

	// make the full rotation :
	Quat qrotation;
	SetRotationArc(&qrotation,normal1,normal2);

	Vec3 axis;
	float angle;
	qrotation.GetAxisAngleMod2Pi(&axis,&angle);

	ASSERT( angle >= 0.f && angle <= CBPI );

	if ( angle <= radians )
	{
		*pSlerp = normal2;
		return true;
	}
	else
	{
		// ramp it up :
		Quat qpartial;
		qpartial.SetFromAxisAngle(axis, radians );

		*pSlerp = qpartial.Rotate( normal1 );
		
		return false;
	}
}

void GetTwoPerp(const Vec3 & v,Vec3 *pv1,Vec3 *pv2)
{
	ASSERT(pv1 && pv2);
	ASSERT( v.IsValid() );
	ASSERT(!fiszero(v.Length()));
	const Vec3 vabs = MakeAbs(v);

	if ( vabs.x < vabs.y && vabs.x < vabs.z )
	{
		*pv1 = Vec3::unitX;
	} 
	else if ( vabs.y < vabs.z )
	{
		*pv1 = Vec3::unitY;
	} 
	else
	{
		*pv1 = Vec3::unitZ;
	}
	
	pv2->SetCross(v,*pv1);
	pv1->SetCross(*pv2,v);

	ASSERT(!fiszero(pv1->Length()));
	ASSERT(!fiszero(pv2->Length()));

	// v1 = v2 X v = - (v x v2)
	//thus v1 * (v x v2) = -1.0 (if they're all normalized)
	// so v * (v1 x v2) = 1.0

	/*
	// make the winding consistent :
	// it already is !!
	float triple = TripleProduct(v,*pv1,*pv2);
	if ( triple < 0.f )
	{
		Util::Swap(*pv1,*pv2);
	}
	*/
}

void SetRandomNormal(Vec3 * pv)
{
	ASSERT(pv);
	do
	{
		pv->x = frand(-1.f,1.f);
		pv->y = frand(-1.f,1.f);
		pv->z = frand(-1.f,1.f);
	} while( pv->LengthSqr() > 1.f );
	pv->NormalizeSafe();
}


int GetLargestComponent(const Vec3 & v)
{
	ASSERT( v.IsValid() );
	const Vec3 a = MakeAbs(v);

	if ( a.x > a.y && a.x > a.z )
		return 0;
	else if ( a.y > a.z )
		return 1;
	else
		return 2;
}

int GetSmallestComponent(const Vec3 & v)
{
	ASSERT( v.IsValid() );
	const Vec3 a = MakeAbs(v);

	if ( a.x < a.y && a.x < a.z )
		return 0;
	else if ( a.y < a.z )
		return 1;
	else
		return 2;
}

float GetLargestComponentValue(const Vec3 & v)
{
	return v[ GetLargestComponent(v) ];
}

void GetPolarFromNormal(const Vec3 & normal,float * pTheta, float * pPhi)
{
	// z = 1, phi = pi/2, z = -1, phi = -pi/2 ; z = sin(phi) phi = asin(z);
	ASSERT( normal.IsNormalized() );
	ASSERT( pTheta && pPhi );

	const float safeZ = Clamp(normal.z,-1.f,1.f);
	*pPhi = asinf( safeZ );

	*pTheta = atan2f( normal.y , normal.x );

	#ifdef DO_ASSERTS
	{
	Vec3 test;
	SetNormalFromPolar(&test,*pTheta,*pPhi);
	ASSERT( Vec3::Equals(test,normal,1e-3f) );
	}
	#endif
}

void SetNormalFromPolar(Vec3 * pNormal,const float theta, const float phi)
{
	ASSERT( pNormal );

	pNormal->z = sinf(phi);
	const float cosPhi = cosf(phi);

	pNormal->x = cosf(theta) * cosPhi;
	pNormal->y = sinf(theta) * cosPhi;

	ASSERT( pNormal->IsNormalized() );
	// do NOT verify with GetPolarFromNormal, since it uses us !
	//	and anyway, there's the nasty trig multi-coverage problem so you
	//	can't just say theta == *pTheta
}


/** Generate a random vector of length less than the given radius.
    Vectors generated here have a uniform distribution within the
    volume of the sphere w/ radius (centered at origin). */
const Vec3 MakeRandomInSphere(const float radius)
{
	// @@ there's some alternate way to do this using the magic of
	// calculus.

	for (;;)
	{
		// Make random vectors within the unit cube; use the first one
		// that's within the unit sphere.
		Vec3 v(frandunit() * 2 - 1, frandunit() * 2 - 1, frandunit() * 2 - 1);
		if (v.LengthSqr() <= 1.0f)
		{
			return v * radius;
		}
	}
}

void ApplyDampedDrive(
					Vec3 * pValue,
					Vec3 * pVelocity,
					const Vec3 & toValue,
					const float frequency,
					const float dt )
{
	// @@@@ : does expf three times instead of the one needed !!
	ApplyDampedDrive(&(pValue->x),&(pVelocity->x),toValue.x,frequency,dt);
	ApplyDampedDrive(&(pValue->y),&(pVelocity->y),toValue.y,frequency,dt);
	ApplyDampedDrive(&(pValue->z),&(pVelocity->z),toValue.z,frequency,dt);
}

void ApplyDampedDriveClamping(
					Vec3 * pValue,
					Vec3 * pVelocity,
					const Vec3 & toValue,
					const float frequency,
					const float minVelocity,
					const float maxVelocity,
					const float minStep,
					const float dt )

{
	// @@@@ : does expf three times instead of the one needed !!
	ApplyDampedDriveClamping(&(pValue->x),&(pVelocity->x),toValue.x,frequency,minVelocity,maxVelocity,minStep,dt);
	ApplyDampedDriveClamping(&(pValue->y),&(pVelocity->y),toValue.y,frequency,minVelocity,maxVelocity,minStep,dt);
	ApplyDampedDriveClamping(&(pValue->z),&(pVelocity->z),toValue.z,frequency,minVelocity,maxVelocity,minStep,dt);
}
	
bool PointInCylinder(const Vec3 & point, const Vec3 & base, const float radius, const float height )
{
	if ( point.z < base.z || point.z > (base.z + height) )
		return false;
		
	float dXYSqr = DistanceSqrXY(point,base); 
	return ( dXYSqr < fsquare(radius) );
}
	
//----------------------------------------------------------------------------------

// cotangent goes from infinity (zero angle) to -infinity (180)
// cotangent(90) = 0
// cotangent(60) = sqrt(1/3)
float GetCotangent(const Vec3 & apex, const Vec3 & v0, const Vec3 & v1)
{
	float e0Sqr = DistanceSqr(apex,v0);
	float e1Sqr = DistanceSqr(apex,v1);
	float dSqr = DistanceSqr(v0,v1);
	
	float area = TriangleArea(apex,v0,v1);
	if ( area == 0.f )
		return FLT_MAX;

	float cotangentNumerator = e0Sqr + e1Sqr - dSqr;
	float cotangent = cotangentNumerator / (4.f * area);

	return cotangent;
}

float GetAngle(const Vec3 & apex, const Vec3 & v0, const Vec3 & v1)
{
	const Vec3 a = v0 - apex;
	const Vec3 b = v1 - apex;

	// I want the angle between "a" and "b"

	float cos_angle = (a * b) / sqrtf(a.LengthSqr() * b.LengthSqr());

	float angle = acosf_safe(cos_angle);

	ASSERT( angle >= 0.f && angle <= CBPI );

	return angle;
}

void ComputeClosestPointToEdge(const Vec3 & point,const Vec3 &fm,const Vec3 &to,
							float * pDistSqr, float * pTime)
{
	const Vec3 fmToPoint(point - fm);

	const float hypotenuseSqr = fmToPoint.LengthSqr();

	const Vec3 edge(to - fm);
	const float length = edge.Length();
	if ( length < EPSILON )
	{
		*pDistSqr = hypotenuseSqr;
		*pTime = 0.5f;
		return;
	}

	const float alongSeg = (fmToPoint * edge) / length;

	const float perpDistSqr = hypotenuseSqr - fsquare(alongSeg);

	if ( alongSeg <= 0.f )
	{
		*pDistSqr = perpDistSqr + fsquare(alongSeg);
		*pTime = 0.f;
		return;
	}
	else if ( alongSeg >= length )
	{
		*pDistSqr = perpDistSqr + fsquare(alongSeg - length);
		*pTime = 1.f;
		return;		
	}
	else
	{
		*pDistSqr = perpDistSqr;
		*pTime = alongSeg / length;
		return;		
	}
}

//! returns time in [0,1], the point is MakeLerp(edgeFm,edgeTo,time);
void ComputeClosestOnEdgeToLine(const Vec3 &edgeFm,const Vec3 &edgeTo,
							const Vec3 & lineBase,const Vec3 & lineNormal,
							float * pTime)
{
	ASSERT( pTime != NULL );
	ASSERT( lineNormal.IsNormalized() );

	const Vec3 segDirection( edgeTo - edgeFm );

    const float lineDotSeg = lineNormal * segDirection;
    const float segDirLenSqr = segDirection.LengthSqr();
    const float determinant = fabsf(segDirLenSqr - lineDotSeg*lineDotSeg);

	static const float c_minDet = 1e-6f;
    if ( determinant >= c_minDet )
    {
		const Vec3 delta( lineBase - edgeFm );
	
		const float deltaLine = delta * lineNormal;
		const float deltaSeg  = delta * segDirection;
		const float t = deltaSeg - lineDotSeg*deltaLine;

		// this is just : treat the segment like a line,
		// find the closest point, then clamp it onto the segment
		// you can't do this for a seg-seg closest point, but you can for seg-line

		*pTime = fclampunit( t / determinant );
    }
    else
    {
        // lines are parallel (or segment is degenerate)
        // what point on segment is closest !?
	    *pTime = 0.5f;
    }
}
							
//----------------------------------------------------------------------------------

bool RayTriangleIntersect(const Vec3 & vert0,const Vec3 & vert1,const Vec3 & vert2, 
									const Vec3 & orig,const Vec3 & dir,
									float * pB0,float * pB1,float *pT)
{
	/* find vectors for two edges sharing vert0 */
	Vec3 edge1 = vert1 - vert0;
	Vec3 edge2 = vert2 - vert0;

	/* begin calculating determinant - also used to calculate U parameter */
	Vec3 pvec;
	pvec.SetCross(dir, edge2);

	/* if determinant is near zero, ray lies in plane of triangle */
	float det = edge1 * pvec;

	float u,v;
	Vec3 qvec;

	static const float c_minDet = 1e-6f;

	if (det > c_minDet)
	{
		/* calculate distance from vert0 to ray origin */
		Vec3 tvec = orig - vert0;

		/* calculate U parameter and test bounds */
		u = tvec * pvec;
		if (u < 0.0 || u > det)
			return false;

		/* prepare to test V parameter */
		qvec.SetCross(tvec, edge1);

		/* calculate V parameter and test bounds */
		v = dir * qvec;
		if (v < 0.0 || u + v > det)
			return false;
	}
	else if(det < -c_minDet)
	{
		/* calculate distance from vert0 to ray origin */
		Vec3 tvec = orig - vert0;

		/* calculate U parameter and test bounds */
		u = tvec * pvec;
		if (u > 0.0 || u < det)
			return false;

		/* prepare to test V parameter */
		qvec.SetCross(tvec, edge1);

		/* calculate V parameter and test bounds */
		v = dir * qvec;
		if (v > 0.0 || u + v < det)
			return false;
	}
	else
	{
		return false;  /* ray is parallel to the plane of the triangle */
	}

	float inv_det = 1.f / det;

	/* calculate t, ray intersects triangle */
	*pT = (edge2 * qvec) * inv_det;
	u *= inv_det;
	v *= inv_det;
	*pB0 = 1.f - (u+v);
	*pB1 = u;

	return true;
}

//! Returns a random point around center that is in the AABB cube bounding the sphere given by the radius
const Vec3 MakeRandomInBox( const Vec3 &center, const float radius )
{
	const float xVal = frandunit() * radius * 2 - radius;
	const float yVal = frandunit() * radius * 2 - radius;
	const float zVal = frandunit() * radius * 2 - radius;

	return Vec3( center.x + xVal, center.y + yVal, center.z + zVal );
}


const Vec3 MakeRandomInBox( const Vec3 &center, const Vec3 &halfSize )
{
	const float xVal = frandunit() * halfSize.x * 2 - halfSize.x;
	const float yVal = frandunit() * halfSize.y * 2 - halfSize.y;
	const float zVal = frandunit() * halfSize.z * 2 - halfSize.z;

	return Vec3( center.x + xVal, center.y + yVal, center.z + zVal );
}

END_CB

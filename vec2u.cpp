#include "cblib/Vec2U.h"
#include "cblib/Rand.h"

START_CB

//----------------------------------------------------------------------------------
void SetRandomNormal(Vec2 * pv)
{
	/*
	ASSERT(pv);
	do
	{
		pv->x = gRand::GetRangedFloat(-1.f,1.f);
		pv->y = gRand::GetRangedFloat(-1.f,1.f);
	} while( pv->LengthSqr() > 1.f );
	pv->NormalizeSafe();
	*/

	ASSERT(pv);
	const float theta = frandranged(0.f,TWO_PIf);
	*pv = MakeRotation(theta);
}

const Vec2 MakeRandomNormal()
{
	const float theta = frandranged(0.f,TWO_PIf);
	Vec2 v;
	v = MakeRotation(theta);
	return v;
}


int GetLargestComponent(const Vec2 & v)
{
	const Vec2 a = MakeAbs(v);

	if ( a.x > a.y )
		return 0;
	else
		return 1;
}

int GetSmallestComponent(const Vec2 & v)
{
	const Vec2 a = MakeAbs(v);

	if ( a.x < a.y )
		return 0;
	else
		return 1;
}

const Vec2 MakeRotation(const float radians)
{
	ASSERT( fisvalid(radians) );

	Vec2 retVal;
	fsincos(radians, &retVal.y, &retVal.x);
	return retVal;
}

const Vec2 MakeRotated(const Vec2 & v,const float radians)
{
	ASSERT( fisvalid(radians) );

	// dmoore: tiny bit of opt here, no sense in using 2 trancendentals when
	//  one slightly larger one will do...
	float c, s;
	fsincos(radians, &s, &c);

	/*

	rotation matrix around Z is :

	pm->RowX().Set( cosa,-sina,    0 );
	pm->RowY().Set( sina, cosa,    0 );
	pm->RowZ().Set(    0,    0,    1 );
	*/

	return Vec2( 
			v.x * c - v.y * s ,
			v.x * s + v.y * c );
}

const bool SetAngularRotated(Vec2 * pLerp,const Vec2 & normal1,const Vec2 & normal2,const float radians)
{
	ASSERT( normal1.IsNormalized() && normal2.IsNormalized() );
	ASSERT( radians >= 0.f );

	//rotate normal1 toward normal2
	DURING_ASSERT( const float angle = GetAngleBetweenNormals( normal1, normal2 ) );
	ASSERT( angle <= PI );
	const float dot = normal1 * normal2;
	const float cosAngle = cosf(radians);
	//if( angle <= radians )
	if ( dot >= cosAngle )
	{
		*pLerp = normal2;
		return true;
	}
	else
	{
		//Need to find out which way to rotate. But that's pretty easy in 2D. Maybe 
		//even easer than this:
		const Vec2 normal2Cross(normal2.y, -normal2.x);
		if( normal2Cross * normal1 > 0.f ) //This way is shorter. 
		{
			ASSERT( Vec2::Equals( normal2 , MakeRotated( normal1, angle ) , EPSILON_NORMALS*2) );
			*pLerp = MakeRotated( normal1, radians );
		}
		else	//No, this way is shorter. 
		{
			ASSERT( Vec2::Equals( normal2 , MakeRotated( normal1, -angle ) , EPSILON_NORMALS*2) );
			*pLerp = MakeRotated( normal1, -radians );
		}
		return false;
	}
}

const float GetAngleBetweenNormals(const Vec2 &a,const Vec2 & b)
{
	ASSERT( a.IsNormalized() && b.IsNormalized() );
	const float dot = a * b;
	const float angle = acosf_safe(dot);
	ASSERT( fisinrange(angle,0.f,PIf) );
	return angle;	
}

//! Assumes angle from a to b. 
//! positive if angle is counterclockwise, negative if clockwise. 
const float GetSignOfAngleBetweenNormals(const Vec2 &a,const Vec2 & b)
{
	const Vec2 bPerp = MakePerpCW(b);
	const float cross = a *bPerp;
	return cross >= 0.f ? 1.f : -1.f;
}



//! Returns a random point around center that is in the AABB cube bounding the sphere given by the radius
const Vec2 MakeRandomInBox( const Vec2 &center, const float radius )
{
	const float xVal = frandunit() * radius * 2 - radius;
	const float yVal = frandunit() * radius * 2 - radius;

	return Vec2( center.x + xVal, center.y + yVal );
}


const Vec2 MakeRandomInBox( const Vec2 &center, const Vec2 &halfSize )
{
	const float xVal = frandunit() * halfSize.x * 2 - halfSize.x;
	const float yVal = frandunit() * halfSize.y * 2 - halfSize.y;

	return Vec2( center.x + xVal, center.y + yVal );
}
//----------------------------------------------------------------------------------

END_CB

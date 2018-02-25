#include "cblib/Vec2U.h"
#include "cblib/Rand.h"

START_CB

//----------------------------------------------------------------------------------

const Vec2 MakeRandomNormal()
{
	const float theta = frand(0.f,TWO_PIf);
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

	float c, s;
	fsincos(radians, &s, &c);

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
	ASSERT( angle <= CBPI );
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
			ASSERT( Vec2::Equals( normal2 , MakeRotated( normal1, angle ) , EPSILON*2) );
			*pLerp = MakeRotated( normal1, radians );
		}
		else	//No, this way is shorter. 
		{
			ASSERT( Vec2::Equals( normal2 , MakeRotated( normal1, -angle ) , EPSILON*2) );
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


//----------------------------------------------------------------------------------

END_CB

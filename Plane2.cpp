#include "cblib/Base.h"
#include "Plane2.h"
#include "Mat2.h"

START_CB

//=============================================================================================

bool Plane2::IsValid() const
{
	ASSERT( fisvalid(m_offset) );
	ASSERT( m_normal.IsValid() );
	ASSERT( m_normal.IsNormalized() );
	return true;
}

/*! define a Plane2 with three points on the Plane2
 returns the area normally, zero on failure !!
 facing of Plane2 defined by counter-clockwise (??) orientation of the triangle
	defined by these three points
 can fail if the points are on top of eachother
*/
/*
float Plane2::SetFromThreePoints(const Vec2 & point1,const Vec2 & point2,const Vec2 & point3)
{
	Vec2U::SetTriangleCross(&m_normal,point1,point2,point3);
	const float areaSqr = m_normal.LengthSqr();

	if ( areaSqr < EPSILON )
	{
		m_normal = Vec2::zero;
		m_offset = 0;

		//ASSERT(!IsValid());

		return 0.f;
	}
	else
	{
		const float area = sqrtf(areaSqr);
		m_normal *= (1.f/area);
		m_offset = - (m_normal * point1);

		ASSERT(IsValid());

		return area;
	}
}
*/

void Plane2::Rotate(const Mat2 & mat)
{
	ASSERT(IsValid());
	ASSERT( mat.IsOrthonormal() );

	// rotate the normal
	m_normal = mat.Rotate(m_normal);

	ASSERT(IsValid());
}


//=============================================================================================

bool Plane2::GetRayIntersectionTime(const Vec2 &point, const Vec2 &dir, float *pResult) const
{
	ASSERT( dir.IsNormalized() );
	float denom = m_normal * dir;	// dot product

	if(denom == 0.f)
		return false;

	*pResult = -DistanceToPoint(point) / denom;
	return true;
}

bool Plane2::GetRayIntersectionPoint(const Vec2 &point, const Vec2 &dir, Vec2 *pResult) const
{
	float t;
	if(GetRayIntersectionTime(point, dir, &t))
	{
		if(t >= 0.f)
		{
			*pResult = point + dir * t;
			return true;
		}
	}
	return false;
}

//=============================================================================================

END_CB

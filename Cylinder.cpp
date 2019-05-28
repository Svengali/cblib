#include "cblib/Cylinder.h"
#include "cblib/Vec2U.h"
#include "cblib/Vec3U.h"

START_CB

bool Cylinder::IsValid() const
{
	ASSERT( m_base.IsValid() );
	ASSERT( m_radius > 0.f );
	ASSERT( m_height > 0.f );
	return true;
}

bool Cylinder::Contains(const Vec3 & point) const
{
	// see also gPointInCylinder
	if ( point.z < GetLoZ() || point.z > GetHiZ() )
		return false;
		
	float dXYSqr = DistanceSqrXY(point,m_base); 
	return ( dXYSqr < fsquare(m_radius) );
}
	
void Cylinder::ClampInside(Vec3 * pTo, const Vec3 & p) const
{
	pTo->z = fclamp( p.z, GetLoZ(), GetHiZ() );
	
	float dXYSqr = DistanceSqrXY(p,m_base); 
	if ( dXYSqr < fsquare(m_radius) )
	{
		pTo->x = p.x;
		pTo->y = p.y;
	}
	else
	{
		// clamp to base
		Vec2 d = MakeProjectionXY(p) - MakeProjectionXY(m_base);
		d *= m_radius / sqrtf(dXYSqr);
		pTo->x = m_base.x + d.x;
		pTo->y = m_base.y + d.y;
	}
}
	
float Cylinder::GetDistanceSqr(const Vec3 & point) const
{
	Vec3 inside;
	ClampInside(&inside,point);
	return DistanceSqr(inside,point);
}

void Cylinder::ScaleFromCenter(const float s)
{
	const Vec3 center = GetCenter();
	m_height *= s;
	m_radius *= s;
	m_base = center;
	m_base.z -= m_height * 0.5f;
}
	
/*static*/ bool Cylinder::Equals(const Cylinder &a,const Cylinder &b,const float tolerance)
{
	return Vec3::Equals(a.m_base,b.m_base,tolerance) &&
		fequal(a.m_height,b.m_height,tolerance) &&
		fequal(a.m_radius,b.m_radius,tolerance);
}
	
END_CB

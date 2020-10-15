#include "Cone.h"
#include "Frame3.h"
#include "Sphere.h"

START_CB

//---------------------------------------------------------

void Cone::SetAngle(const float angle)
{
	ASSERT( angle < HALF_PI );
	// tiny angles forbidden :
	ASSERT( angle > EPSILON );
	m_angle = angle;
	UpdateDerivedMembers();
}

void Cone::Set(
	const Vec3 & pos,
	const Vec3 & normal,
	const float angle)
{
	ASSERT( pos.IsValid() );
	ASSERT( normal.IsNormalized() );
	m_pos = pos;
	m_normal = normal;
	SetAngle(angle);
	ASSERT(IsValid());
}

void Cone::Rotate(const Mat3 & mat)
{
	ASSERT( IsValid() );
	m_normal = mat.Rotate(m_normal);
	m_pos    = mat.Rotate(m_pos);
	ASSERT( IsValid() );
}

void Cone::Transform(const Frame3 & xf)
{
	ASSERT( IsValid() );
	m_normal = xf.Rotate(m_normal);
	m_pos    = xf.Transform(m_pos);
	ASSERT( IsValid() );
}

bool Cone::IsValid() const
{
	// base members :
	ASSERT( m_normal.IsValid() );
	ASSERT( m_pos.IsValid() );
	ASSERT( fisvalid(m_angle) );
	ASSERT( m_angle < HALF_PI );
	ASSERT( m_angle > EPSILON ); // tiny angles forbidden
	// derived members :
	ASSERT( fisvalid(m_cos) );
	ASSERT( fisvalid(m_tan) );
	return true;
}

void Cone::UpdateDerivedMembers()
{
	ASSERT( m_normal.IsValid() );
	ASSERT( m_pos.IsValid() );
	ASSERT( fisvalid(m_angle) );
	ASSERT( m_angle < HALF_PI );
	// tiny angles forbidden :
	ASSERT( m_angle > EPSILON );

	m_cos = cosf(m_angle);
	m_tan = tanf(m_angle);

	//	m_negNormalOverSin = m_normal * (-1.f / sinf(m_angle));

	ASSERT( IsValid() );
}

//---------------------------------------------------------

bool Cone::Contains(const Vec3 & v) const
{
	ASSERT( IsValid() && v.IsValid() );
	const Vec3 delta = v - m_pos;
	const float projection = delta * m_normal;
	if ( projection <= 0.f )
		return false;
	const float radius = projection * m_tan;
	const float perpDistSqr = delta.LengthSqr() - fsquare(projection);
	ASSERT( perpDistSqr >= 0.f );

	return ( perpDistSqr <= fsquare(radius) );
}

// Distance needs a sqrt
//  the result can be negative, indicating inclusion
float Cone::Distance(const Vec3 & v) const
{
	ASSERT( IsValid() && v.IsValid() );
	const Vec3 delta = v - m_pos;
	const float projection = delta * m_normal;
	if ( projection <= 0.f )
	{
		// behind the apex; return distance to apex
		return delta.Length();
	}
	const float radius = projection * m_tan;
	const float perpDistSqr = delta.LengthSqr() - fsquare(projection);
	
	//ASSERT( perpDistSqr >= 0.f );
	ASSERT( perpDistSqr >= -EPSILON );
	if ( perpDistSqr <= 0.f ) // can slip epsilon
	{
		return - radius * m_cos;
	}
	
	const float distAway = (sqrtf(perpDistSqr) - radius) * m_cos;

	return distAway;
}
 
// Cull needs a sqrt
Cull::EResult Cone::Cull(const Sphere & s) const
{
	ASSERT( IsValid() && s.IsValid() );
	// assembly checked 9-4; all inlined nicely

	const float distanceToCenter = Distance(s.GetCenter());
	if ( distanceToCenter >= s.GetRadius() )
		return Cull::eOut;
	else if ( distanceToCenter <= - s.GetRadius() )
		return Cull::eIn;
	else
		return Cull::eCrossing;
}

/*******************************
IntersectsRough : Sphere-Cone test
avoid the sqrt that Cull does

The approximation is that some Spheres which are just behind the
apex will called intersecting when they aren't.  The magnitude of
the approximation is related to how narrow the cone is.  At theta = 90,
it's perfectly accurate, at theta = 0, it's completely wrong.

This is not usefull as a ::Cull routine because it can't return
all-in vs. partially-in information without doing a lot more work.

********************************/
/*
bool Cone::IntersectsRough(const Sphere & s) const
{
	// move back the Apex to treat the cone as being larger :
	const Vec3 posModified = Vec3::MakeWeightedSum(m_pos, s.GetRadius(), m_negNormalOverSin);

	// now do "Contains" with posModified on the Sphere center : :
	const Vec3 delta = s.GetCenter() - posModified;
	const float projection = delta * m_normal;
	if ( projection <= 0.f )
		return false;
	const float radius = projection * m_tan;
	const float perpDistSqr = delta.LengthSqr() - fsquare(projection);

	return ( perpDistSqr <= fsquare(radius) );
}
*/

/*****

	with 4 Spheres (eg. no cache misses) :

	cone.IntersectsRough : 59.5 clocks
	cone.Cull : 68.0 clocks
	Frustum.Cull : 77.5 clocks
	
	with 16k Spheres (eg. cache misses) :

	cone.IntersectsRough : 103.7 clocks
	cone.Cull : 107.9 clocks
	Frustum.Cull : 130.0 clocks

	Conclusion : IntersectsRough is pointless !!

*****/

//---------------------------------------------------------

END_CB

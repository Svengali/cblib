#pragma once

#include "Vec3.h"
#include "Plane.h"

START_CB

class Sphere;
class Mat3;
class Frame3;

/*!
	Cone primitive; and infinite, un-capped code.  Defined by:
		apex position
		normal direction
		angle of aperture

	a Cone can be used efficienty for culling against Spheres
	it's somewhat useful as a rough view cull
	it's also useful as a spot-light cull, or an LOS cull

	Cone API is somewhat above that of the other geometric primitives
		(like Sphere).
	Cone API is similar to that of Frustum
*/

class Cone
{
public:
//---------------------------------------------------------
	
	//! default constructor invalidates
	Cone()
	{
		finvalidatedbg(m_angle);
		finvalidatedbg(m_cos);
		finvalidatedbg(m_tan);
	}

	Cone(const Vec3 & pos,const Vec3 & normal,const float angle) :
		m_pos(pos) , m_normal(normal)
	{
		ASSERT( m_pos.IsValid() );
		ASSERT( m_normal.IsNormalized() );
		SetAngle(angle);
		ASSERT( IsValid() );
	}

	bool IsValid() const;

	//! default operator =, ==, destructor, and copy constructor

	//---------------------------------------------------------
	//! accessors :

	const Vec3 & GetPos() const	{ ASSERT(IsValid()); return m_pos; }
	const Vec3 & GetNormal() const	{ ASSERT(IsValid()); return m_normal; }

	float GetAngle() const		{ ASSERT(IsValid()); return m_angle; }
	float GetAngleCos() const	{ ASSERT(IsValid()); return m_cos; }
	float GetAngleTan() const	{ ASSERT(IsValid()); return m_tan; }

	//---------------------------------------------------------
	//! mutators :

	void Set(const Vec3 & pos,const Vec3 & normal,const float angle);

	void SetAngle(const float angle);

	void SetPos(const Vec3 & pos)
	{
		ASSERT( pos.IsValid() );
		ASSERT(IsValid());
		m_pos = pos;
		ASSERT(IsValid());
	}

	void SetNormal(const Vec3 & normal)
	{
		ASSERT( normal.IsNormalized() );
		ASSERT(IsValid());
		m_normal = normal;
		ASSERT(IsValid());
	}

	void Translate(const Vec3 & delta) { ASSERT(IsValid()); m_pos += delta; ASSERT(IsValid()); }
	void Rotate(const Mat3 & mat);
	void Transform(const Frame3 & xf);

	//---------------------------------------------------------

	//! Cone Contains point test
	bool Contains(const Vec3 & v) const;

	//! this Distance is signed - negative is inside
	//!	(needs a sqrt)
	float Distance(const Vec3 & v) const;

	//! Cull Sphere needs a sqrt
	//!	about 60 clocks without cache misses, 100 with cache misses
	Cull::EResult Cull(const Sphere & s) const;

	Cull::EResult Cull(const Vec3 & v) const
	{
		return Contains(v) ? Cull::eIn : Cull::eOut;
	}

	//---------------------------------------------------------
private:

	void UpdateDerivedMembers();

	//! definition :
	Vec3		m_normal;
	Vec3		m_pos;
	float		m_angle;

	//! derived, kept in sync via UpdateDerivedMembers :
	float		m_cos;
	float		m_tan;

	// Cone = 36 bytes
//---------------------------------------------------------
};

END_CB

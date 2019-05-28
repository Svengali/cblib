#pragma once

#include "cblib/Vec3.h"

START_CB

/**

Cylinder is always aligned with Z !!

it has a radius in XY

m_base is a Vec3 at the Low Z

the Cylinder extends m_height above that

see also gPointInCylinder

**/

class Sphere;

class Cylinder
{
public:

	//! default constructor sets no data members
	Cylinder()
	{
		finvalidatedbg(m_height);
		finvalidatedbg(m_radius);
	}
	
	Cylinder(const Vec3 & base, const float height, const float radius) :
		m_base(base), m_height(height), m_radius(radius)
	{
		ASSERT( IsValid() );
	}
	
	enum EConstructCenter { eCenter };
	Cylinder(EConstructCenter e,const Vec3 & center, const float height, const float radius) :
		m_base(center), m_height(height), m_radius(radius)
	{
		m_base.z -= height * 0.5f;
		ASSERT( IsValid() );
	}
	
	static bool Equals(const Cylinder &a,const Cylinder &b,const float tolerance = EPSILON);
	
	bool IsValid() const;
	
	bool Contains(const Vec3 & point) const;
	
	void ClampInside(Vec3 * pTo, const Vec3 & p) const;
	
	float GetDistanceSqr(const Vec3 & point) const;
	
	const Vec3 & GetBase() const	{ return m_base; }
	float GetHeight() const			{ return m_height; }
	float GetRadius() const			{ return m_radius; }
	
	void SetBase(const Vec3 & b)  { m_base = b; }
	void SetHeight(const float f) { m_height = f; }
	void SetRadius(const float f) { m_radius = f; }
	
	float & MutableRadius()			{ return m_radius; }
	float & MutableHeight()			{ return m_height; }
	
	void Translate(const Vec3 &v)	{ m_base += v; }
	
	void ScaleFromCenter(const float s);
	
	float GetLoZ() const { return m_base.z; }
	float GetHiZ() const { return m_base.z + m_height; }
	
	const Vec3 GetCenter() const { return Vec3(m_base.x,m_base.y,m_base.z + m_height*0.5f); }
	
private:

	// 20 bytes
	Vec3	m_base;
	float	m_height;
	float	m_radius;
	
};

END_CB

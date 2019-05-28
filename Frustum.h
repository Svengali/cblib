#pragma once

#include "cblib/Vec3.h"
#include "cblib/Plane.h"

START_CB

/*!
	a Frustum is just a bunch of Planes which can act together
	to perform a Cull

	Frustum building and manipulation not provided at the moment

	points on the BACK side of the Planes are out,
	so the Planes point to the inside
*/

class Frame3;
class Sphere;
class AxialBox;
class OrientedBox;

class Frustum
{
public:
	//! hard limit on number of Planes; could increase, or turn this
	//!	into an allocation if we ever need Frustum for something more
	//!	generic than just view Culling
	enum { c_MaxPlanes = 8 };

	Frustum() { m_count = 0; }
	
	//! default operator =, ==, and copy-con are fine

	bool IsValid() const;

	//----------------------------------------------------------

	void SetNumPlanes(const int n)
	{
		ASSERT( n >= 0 && n <= c_MaxPlanes );
		m_count = n;
		// not necessarily valid now
	}
	void SetPlane(const int i,const Plane & Plane)
	{
		ASSERT( i >= 0 && i < m_count );
		m_planes[i] = Plane;
		// not necessarily valid now
	}
	const Plane & GetPlane(const int i) const
	{
		ASSERT(IsValid());
		ASSERT( i >= 0 && i < m_count );
		return m_planes[i];
	}
	int GetNumPlanes() const
	{
		ASSERT(IsValid());
		return m_count;
	}

	void Transform(const Frame3 & xf);

	//----------------------------------------------------------

	//! Cull of a point can only be In or Out
	Cull::EResult Cull(const Vec3 & point) const;

	//! trinary Culls of volume primitives :
	Cull::EResult Cull(const Sphere & sphere) const;
	Cull::EResult Cull(const AxialBox & box) const;
	Cull::EResult Cull(const OrientedBox & box) const;

	//
	// trianary culls returning crossing plane flags
	//
	Cull::EResult Cull(const Sphere & sphere, const uint32 testPlanes, uint32 * const pResultPlanes) const;
	Cull::EResult Cull(const AxialBox & box, const uint32 testPlanes, uint32 * const pResultPlanes) const;
	Cull::EResult Cull(const OrientedBox & box, const uint32 testPlanes, uint32 * const pResultPlanes) const;

	//----------------------------------------------------------

	uint32 ComputeOutFlags(const Vec3 & point,uint32 checkMask = 0xFF) const;
	
private:
	//! Frustum = 100 bytes (with 6 Planes)

	template <class t_Volume>
	Cull::EResult CullGeneric(const t_Volume & vol) const;

	template <class t_Volume>
	Cull::EResult CullGeneric(const t_Volume & vol, const uint32 testPlanes, uint32 * const pResultPlanes) const;

	Plane	m_planes[c_MaxPlanes];
	int		m_count;
	
	// CullGeneric is cool, but requires VC7
	/*
	template <class t_Volume>
	Cull::EResult CullGeneric(const t_Volume & vol) const;
	*/

}; // end class Frustum

END_CB

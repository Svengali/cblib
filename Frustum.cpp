#include "Frustum.h"
#include "Sphere.h"
#include "AxialBox.h"
#include "OrientedBox.h"

START_CB

//----------------------------------------------------------

bool Frustum::IsValid() const
{
	ASSERT( m_count >= 0 && m_count <= c_MaxPlanes );
	
	for(int i=0;i<m_count;i++)
	{
		ASSERT( m_planes[i].IsValid() );
	}

	return true;
}

void Frustum::Transform(const Frame3 & xf)
{
	ASSERT(IsValid());
	for(int i=0;i<m_count;i++)
	{
		m_planes[i].Transform(xf);
	}
	ASSERT(IsValid());
}

//----------------------------------------------------------

//! Cull of a point can only be In or Out
Cull::EResult Frustum::Cull(const Vec3 & point) const
{
	ASSERT(IsValid() && point.IsValid());
	for(int i=0;i<m_count;i++)
	{
		if ( m_planes[i].PointSideGE(point) == Plane::eBack )
			return Cull::eOut;
	}

	return Cull::eIn;
}

uint32 Frustum::ComputeOutFlags(const Vec3 & point,uint32 checkMask) const
{
	uint32 ret = 0;
	
	checkMask &= (1<<m_count)-1;
	
	uint32 mask = 1;
	int i = 0;
	while ( checkMask )
	{
		if ( checkMask & mask )
		{
			ret |= ( m_planes[i].PointSideGE(point) == Plane::eBack ) ? mask : 0;

			checkMask -= mask;
		}

		mask <<= 1;
		i++;
	}

	return ret;
}


//! CullGeneric is pretty nifty, but requires VC7, it seems
//!	not sure why it doesn't work with VC6 !?
template <class t_Volume>
Cull::EResult Frustum::CullGeneric(const t_Volume & vol) const
{
	ASSERT(IsValid() && vol.IsValid());
	
	//! \todo need to check the assembly again
	// assembly checked 9-4; all inlined nicely

	Cull::EResult eRes = Cull::eIn;

	for(int i=0;i<m_count;i++)
	{
		const Plane::ESide eSide = vol.PlaneSide(m_planes[i]);
		
		if ( eSide == Plane::eBack )
			return Cull::eOut;
		else if ( eSide != Plane::eFront )
			eRes = Cull::eCrossing;
	}

	return eRes;
}

//! CullGeneric is pretty nifty, but requires VC7, it seems
//!	not sure why it doesn't work with VC6 !?
//! version allowing user to specify clip planes. 
template <class t_Volume>
Cull::EResult Frustum::CullGeneric(const t_Volume & vol, const uint32 testPlanes, uint32 * const pResultPlanes) const
{
	ASSERT(m_count <= 8); // plane flags only a byte!
	ASSERT(IsValid() && vol.IsValid());
	
	//! \todo need to check the assembly again
	// assembly checked 9-4; all inlined nicely

	Cull::EResult eRes = Cull::eIn;

	uint32 planes=0;
	for(int i=0;i<m_count;i++)
	{
		// check this plane?
		if((0x01 & testPlanes>>i)==0x01) 
		{
			const Plane::ESide eSide = vol.PlaneSide(m_planes[i]);
		
			if ( eSide == Plane::eBack )
			{
				// assume no crossing
				if(pResultPlanes!=NULL)
				{
					*pResultPlanes=0;
				}
	
				return Cull::eOut;
			}
			else if ( eSide != Plane::eFront )
			{
				eRes = Cull::eCrossing;
				planes |= (1<<i);
			}
		}
	}

	// flag crossing planes
	if(pResultPlanes!=NULL)
	{
		*pResultPlanes = planes;
	}

	return eRes;
}





Cull::EResult Frustum::Cull(const Sphere & sphere) const
{
	return CullGeneric(sphere);
}

Cull::EResult Frustum::Cull(const AxialBox & box) const
{
	return CullGeneric(box);
}

Cull::EResult Frustum::Cull(const OrientedBox & box) const
{
	return CullGeneric(box);
}

//
// trianary culls returning crossing plane flags
//
Cull::EResult Frustum::Cull(const Sphere & sphere, const uint32 testPlanes, uint32 * const pResultPlanes) const
{
	return CullGeneric(sphere, testPlanes, pResultPlanes);
}

Cull::EResult Frustum::Cull(const AxialBox & box, const uint32 testPlanes, uint32 * const pResultPlanes) const
{
	return CullGeneric(box, testPlanes, pResultPlanes);
}

Cull::EResult Frustum::Cull(const OrientedBox & box, const uint32 testPlanes, uint32 * const pResultPlanes) const
{
	return CullGeneric(box, testPlanes, pResultPlanes);
}


//----------------------------------------------------------

END_CB

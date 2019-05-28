#pragma once

#include "Vec3.h"
#include "Plane.h"
#include "Vec3U.h"

START_CB

class Segment;
class Frame3Scaled;

/*!

	AxialBox is a "Volume Primitive" just like Sphere, OrientedBox, etc.
	see more notes in Sphere.h

	AxialBox is the fastest and easiest to build, but slow to cull and
	not great at fitting data.

*/
//{=====================================================================================

class AxialBox
{
public:

	AxialBox() { } // min & max invalidated in debug

	AxialBox(const Vec3 &vmin,const Vec3 &vmax) : m_min(vmin), m_max(vmax)
	{
		// NOT necessarilly valid
		//ASSERT( IsValid() );
	}

	explicit AxialBox(const Vec3 & point) : m_min(point), m_max(point)
	{
	}
	
	AxialBox(const Vec3 & center,const float radius) :
		m_min(center - Vec3(radius,radius,radius)),
		m_max(center + Vec3(radius,radius,radius))
	{
		// NOT necessarilly valid
		//ASSERT( IsValid() );
	}
	
	AxialBox(const float lo,const float hi) : m_min(lo,lo,lo), m_max(hi,hi,hi)
	{
		// NOT necessarilly valid
		//ASSERT( IsValid() );
	}

	// default operator = and operator == and copy constructor are fine

	static const AxialBox unitBox; // from -.5 to .5

	/////////////////////////////////////////////////////

	bool IsValid() const;

	const Vec3 & GetMin() const		{ return m_min; }
	const Vec3 & GetMax() const		{ return m_max; }

	Vec3&	MutableMin()			{ return m_min; }
	Vec3&	MutableMax()			{ return m_max; }

	void SetMin(const Vec3 &v)		{ m_min = v; }
	void SetMax(const Vec3 &v)		{ m_max = v; }

	float GetSizeX() const			{ ASSERT(IsValid()); return m_max.x - m_min.x; }
	float GetSizeY() const			{ ASSERT(IsValid()); return m_max.y - m_min.y; }
	float GetSizeZ() const			{ ASSERT(IsValid()); return m_max.z - m_min.z; }

	float GetSize(const int i) const{ ASSERT(IsValid()); return m_max[i] - m_min[i]; }

	// Center/Extent interface.
	const Vec3	GetExtent() const { return (m_max - m_min) * 0.5; }

	/////////////////////////////////////////////////////

	void Set(const Vec3 &lo,const Vec3 &hi);
	void SetToPoint(const Vec3 &v);
	void SetToSphere(const Vec3 &v,const float radius);
	
	void Translate(const Vec3 &v);

	void Scale(const Vec3 & s);
	void Scale(const float s);

	//! Rotate will always *expand* the box !
	void Rotate(const Mat3 & xf);

	void Transform(const Frame3 & xf);
	void Transform(const Frame3Scaled & xf);

	/*!  SetIntersection
		makes a bbox from the intersection of two others
		returns bool to indicate whether the boxes actually intersected 

		if the return is false, then the intersection was the empty set,
		and the resultant rect is INVALID !  You must clear it or not use
		it again.  I intentionally do NOT set it to (0,0) or anything valid
		like that so that I can be sure that you don't just keep using it.
		AP: the resultant rect is not touched if there is no intersection. 
	*/
	bool SetIntersection(const AxialBox & ab)
	{
		return SetIntersection(*this,ab);
	}

	bool SetIntersection(const AxialBox & ab1,const AxialBox & ab2);

	void SetEnclosing(const AxialBox & ab1,const AxialBox & ab2);
	void SetEnclosing(const Vec3 & v1,const Vec3 & v2);
	void SetEnclosing(const Segment& seg);

	void ExtendToPoint(const Vec3 &v);

	void ExtendToBox(const AxialBox & ab);

	void Expand(const float f);
	void Expand(const Vec3 &size);

	//! Make any dimensions smaller than minSize at least that big
	//! helps some algorithms that can't handle boxes with degenerate dimensions
	void PadNullDimensions(const float minSize = EPSILON);

	//! Extends smaller dimensions to match the largest
	void GrowToCube();

	/////////////////////////////////////////////////////
	//! Two different LSS-ABB tests

	bool IntersectVolume_Woo(const Segment & seg) const;
	bool IntersectVolume(const Segment & seg) const;

	//! Surface collide test.
	bool IntersectSurface(const Segment & seg, SegmentResults * pResults) const;

	/////////////////////////////////////////////////////
	
	//! returns the point closest to "v" which is in the box
	//! returns "v" if v is inside the box
	const Vec3 GetClampInside(const Vec3 & v) const;

	//! distance from a point to the box; 0.f for inside
	float DistanceSqr(const Vec3 & v) const;

	/////////////////////////////////////////////////////

	//! does this box Contain other things?
	bool Contains(const Vec3 & v) const;
	bool Contains(const AxialBox & ab) const;

	bool ContainsXY(const Vec3 & v) const;
	bool ContainsXY(const AxialBox & ab) const;

	bool ContainsSoft(const Vec3 & v,const float tolerance = EPSILON) const;
	bool ContainsSoft(const AxialBox & ab,const float tolerance = EPSILON) const;

	bool Intersects(const AxialBox & ab) const;

	/////////////////////////////////////////////////////

	//! the diagonal from min to max
	const Vec3 GetDiagonal() const;

	const Vec3 GetCenter() const;
	float GetRadiusSqr() const;

	//! If you cast a line through the center of the box, this tells
	//!	you how much of that line is inside the box; any convex volume
	//! can be fully described with a radius(direction) function.
	float GetDiameterInDirection(const Vec3 & dir) const;
	float GetRadiusInDirection(const Vec3 & dir) const;

	/*! GetCorner(corner)
		corner in [0,7]
		it's three bit flags
		1 = x , 2 = y, 4 = z
	*/
	const Vec3 GetCorner(const int corner) const;

	//! face in [0,6) , fills four verts in pVerts
	//  CounterClockwise points *out* of the box
	void GetFace(const int face,Vec3 * pVerts) const;

	float GetVolume() const;
	float GetSurfaceArea() const;
	
	/////////////////////////////////////////////////////

	//! v should be in the box
	//!	makes inBox in 0->1, a range in "box space"
	const Vec3 ScaleVectorIntoBox(const Vec3 & v) const;
	//! inBox should be in 0->1
	//!	makes v in the box's min,max range
	const Vec3 ScaleVectorFromBox(const Vec3 & inBox) const;
	
	/////////////////////////////////////////////////////
	//! PlaneDist and PlaneSide ; Plane-Box tests
	//!	have an identical preamble, just return slightly
	//!	different things

	float PlaneDist(const Plane & plane) const;
	Plane::ESide PlaneSide(const Plane & plane) const;

	/////////////////////////////////////////////////////
	//! DifferenceDistSqr : compare two boxes,
	//!	return a difference in units of distance^2
	float DifferenceDistSqr(const AxialBox &ab ) const;

	/////////////////////////////////////////////////////

	void	Log() const; //!< writes xyz to Log; does NOT add a \n !

	// IO as bytes

private:

	Vec3 m_min;
	Vec3 m_max;

	// ? representation : ?
	// min+max is best for construction
	// center + halfExtents
	//	is much better for collision detection,

}; // end class AxialBox

//}{=====================================================================================
// Functions !

inline void AxialBox::SetToPoint(const Vec3 &v)
{
	// NOT necessarilly valid now
	m_min = v;
	m_max = v;
	ASSERT(IsValid());
}

inline void AxialBox::SetToSphere(const Vec3 &v,const float radius)
{
	// NOT necessarilly valid now
	m_min = v - Vec3(radius,radius,radius);
	m_max = v + Vec3(radius,radius,radius);
	ASSERT(IsValid());
}

inline void AxialBox::Set(const Vec3 &lo,const Vec3 &hi)
{
	m_min = lo;
	m_max = hi;
	// NOT necessarilly valid now
}

inline void AxialBox::Expand(const float f)
{
	ASSERT( IsValid() );
	ASSERT( fisvalid(f) );
	m_max += Vec3(f,f,f);
	m_min -= Vec3(f,f,f);
	ASSERT( IsValid() );	
}

inline void AxialBox::Expand(const Vec3 &size)
{
	ASSERT( IsValid() );
	m_max += size;
	m_min -= size;
	ASSERT( IsValid() );	
}

inline void AxialBox::ExtendToPoint(const Vec3 &v)
{
	ASSERT( IsValid() );
	m_min.SetMin(v);
	m_max.SetMax(v);
	ASSERT( Contains(v) );
}

inline void AxialBox::ExtendToBox(const AxialBox & ab)
{
	// same as SetEnclosing(*this,ab);
	ASSERT( IsValid() );
	m_min.SetMin(ab.m_min);
	m_max.SetMax(ab.m_max);
	ASSERT( Contains(ab) );
}

inline void AxialBox::Translate(const Vec3 &v)
{
	ASSERT( IsValid() );
	m_min += v;
	m_max += v;
	ASSERT( IsValid() );
}

inline void AxialBox::Scale(const Vec3 & s)
{
	ASSERT( IsValid() );
	m_min.ComponentwiseScale(s);
	m_max.ComponentwiseScale(s);
	ASSERT( IsValid() );
}

inline void AxialBox::Scale(const float s)
{
	ASSERT( IsValid() );
	m_min *= s;
	m_max *= s;
	ASSERT( IsValid() );
}

inline void AxialBox::SetEnclosing(const Vec3 & v1,const Vec3 & v2)
{
	ASSERT( v1.IsValid() && v2.IsValid() );
	m_min = MakeMin(v1,v2);
	m_max = MakeMax(v1,v2);
	ASSERT( IsValid() );
}

inline void AxialBox::SetEnclosing(const AxialBox & ab1,const AxialBox & ab2)
{
	ASSERT( ab1.IsValid() && ab2.IsValid() );
	m_min = MakeMin(ab1.m_min,ab2.m_min);
	m_max = MakeMax(ab1.m_max,ab2.m_max);
	ASSERT( IsValid() );
}

inline bool AxialBox::SetIntersection(const AxialBox & ab1,const AxialBox & ab2)
{
	ASSERT( ab1.IsValid() && ab2.IsValid() );
	m_min = MakeMax(ab1.m_min,ab2.m_min);
	m_max = MakeMin(ab1.m_max,ab2.m_max);

	if ( m_max.x < m_min.x ||
		 m_max.y < m_min.y ||
		 m_max.z < m_min.z )
	{
		// !! not valid !!
		return false;
	}

	ASSERT( IsValid() );
	return true;
}

inline const Vec3 AxialBox::GetClampInside(const Vec3 & v) const
{
	ASSERT( IsValid() && v.IsValid() );
	return MakeClamped(v,m_min,m_max);
}

inline float AxialBox::DistanceSqr(const Vec3 & v) const
{
	ASSERT( IsValid() && v.IsValid() );
	const Vec3 t = GetClampInside(v);
	return cb::DistanceSqr(t,v);
}

inline bool AxialBox::Intersects(const AxialBox & ab) const
{
	ASSERT( IsValid() && ab.IsValid() );

	return	( ab.m_min.x <= m_max.x && ab.m_max.x >= m_min.x ) &&
			( ab.m_min.y <= m_max.y && ab.m_max.y >= m_min.y ) &&
			( ab.m_min.z <= m_max.z && ab.m_max.z >= m_min.z );
}

inline bool AxialBox::Contains(const Vec3 & v) const
{
	ASSERT( IsValid() && v.IsValid() );

	return	( v.x >= m_min.x && v.x <= m_max.x ) &&
			( v.y >= m_min.y && v.y <= m_max.y ) &&
			( v.z >= m_min.z && v.z <= m_max.z );
}

inline bool AxialBox::Contains(const AxialBox & ab) const
{
	ASSERT( IsValid() && ab.IsValid() );

	return	( ab.m_min.x >= m_min.x && ab.m_max.x <= m_max.x ) &&
			( ab.m_min.y >= m_min.y && ab.m_max.y <= m_max.y ) &&
			( ab.m_min.z >= m_min.z && ab.m_max.z <= m_max.z );
}

inline bool AxialBox::ContainsXY(const Vec3 & v) const
{
	ASSERT( IsValid() && v.IsValid() );

	return	( v.x >= m_min.x && v.x <= m_max.x ) &&
			( v.y >= m_min.y && v.y <= m_max.y );
}

inline bool AxialBox::ContainsXY(const AxialBox & ab) const
{
	ASSERT( IsValid() && ab.IsValid() );

	return	( ab.m_min.x >= m_min.x && ab.m_max.x <= m_max.x ) &&
			( ab.m_min.y >= m_min.y && ab.m_max.y <= m_max.y );
}

inline bool AxialBox::ContainsSoft(const Vec3 & v, const float tolerance) const
{
	ASSERT( IsValid() && v.IsValid() );

	return	( v.x >= m_min.x-tolerance && v.x <= m_max.x+tolerance ) &&
			( v.y >= m_min.y-tolerance && v.y <= m_max.y+tolerance ) &&
			( v.z >= m_min.z-tolerance && v.z <= m_max.z+tolerance );
}

inline bool AxialBox::ContainsSoft(const AxialBox & ab, const float tolerance) const
{
	ASSERT( IsValid() && ab.IsValid() );

	return	( ab.m_min.x >= m_min.x-tolerance && ab.m_max.x <= m_max.x+tolerance ) &&
			( ab.m_min.y >= m_min.y-tolerance && ab.m_max.y <= m_max.y+tolerance ) &&
			( ab.m_min.z >= m_min.z-tolerance && ab.m_max.z <= m_max.z+tolerance );
}

inline const Vec3 AxialBox::GetDiagonal() const
{
	ASSERT( IsValid() );

	return m_max - m_min;
}

inline const Vec3 AxialBox::GetCenter() const
{
	ASSERT( IsValid() );

	return MakeAverage(m_min,m_max);
}

inline float AxialBox::GetRadiusSqr() const
{
	ASSERT( IsValid() );
	return cb::DistanceSqr(m_min,m_max) * 0.25f;
}

inline float AxialBox::GetDiameterInDirection(const Vec3 & dir) const
{
	ASSERT( IsValid() );

	const float diameter = 
		 fabsf(dir.x * (m_max.x - m_min.x)) +
		 fabsf(dir.y * (m_max.y - m_min.y)) +
		 fabsf(dir.z * (m_max.z - m_min.z));

	return diameter;
}

inline float AxialBox::GetRadiusInDirection(const Vec3 & dir) const
{
	return GetDiameterInDirection(dir) * 0.5f;
}

inline float AxialBox::GetVolume() const
{
	ASSERT( IsValid() );
	return (m_max.x - m_min.x) * (m_max.y - m_min.y) * (m_max.z - m_min.z);
}

inline float AxialBox::GetSurfaceArea() const
{
	ASSERT(IsValid());

	float	sx = GetSizeX();
	float	sy = GetSizeY();
	float	sz = GetSizeZ();

	return (sx * sy + sy * sz + sz * sx) * 2;
}

inline float AxialBox::DifferenceDistSqr(const AxialBox &ab ) const
{
	ASSERT( IsValid() && ab.IsValid() );
	const float dSqr = cb::DistanceSqr( m_min , ab.m_min ) +
				 cb::DistanceSqr( m_max , ab.m_max );
	return dSqr;
}

//}=====================================================================================

END_CB

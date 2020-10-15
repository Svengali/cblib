/*!
  \file
  \brief OrientedBox : oriented bounding box
		a rotation
		a center
		3 radii
	(or: an AxialBox and a rotation)

	Commonly referred to as an OBB
	
	OrientedBox is a "Volume Primitive" just like Sphere, AxialBox, etc.
	see more notes in VolumeUtil.h

	It is the most accurate and most expensive of the basic volume primitives

	API nearly identical to AxialBox
*/
#pragma once

#include "Base.h"
#include "Vec3.h"
#include "Mat3.h"
#include "Plane.h"

START_CB

class Segment;
class OrientedBox;
//class gOBBCompact;

//{===================================================================================

class OrientedBox
{
public:

	//-----------------------------------------------------
	// Constructors :

	OrientedBox()			{ /* invalidated */ }

	// NOTEZ : the Mat3 in the OrientedBox is a set of 3 axes in world-space
	//	the axes are the *rows*.  This is NOT the Transformation matrix of an object.
	//	which has the axes as *columns*, so if you want to make the OBB to match an
	//	object, you should use the *TRANSPOSE* of the object's transform matrix.
	OrientedBox(const Mat3 & mat,const Vec3 & center,const Vec3 & radii) :
		m_axes(mat), m_center(center), m_radii(radii)
	{ }

	// this version takes the "Box To World" Matrix - eg. not transposed
	//	(it does a transpose, so it's not as fast)
	enum EBoxToWorldMatrix { eBoxToWorldMatrix };
	OrientedBox(EBoxToWorldMatrix e,const Mat3 & mat,const Vec3 & center,const Vec3 & radii);
	
	enum EUnitBox { eUnitBox };
	//! the UnitBox is from -0.5 to 0.5
	//! it has a volume of 1.0
	explicit OrientedBox(const EUnitBox e) :
		m_axes(Mat3::eIdentity), m_radii(0.5f,0.5f,0.5f), m_center(0.f,0.f,0.f)
	{
		// do NOT use Vec3::zero here ; cannot use other statics
		//	in initialization of statics
	}


	/** Convenience constructor: make an OrientedBox, from an AxialBox
        and an object transformation matrix. **/
	// CB - Nein! Use VolumeUtil for cross-type functions.
        
	// default copy constructor + operator= + destructer all good

	bool IsValid() const;

	static const OrientedBox unitBox;

	//-----------------------------------------------------

	//! Matrix is the world-to-box Transform
	//! its rows are the world-space axes of the box
	//! it must be orthonormal !!
	const Mat3 & GetMatrix() const		{ ASSERT(IsValid()); return m_axes; }
	void SetMatrix(const Mat3 & mat)		{ ASSERT( mat.IsOrthonormal() ); m_axes = mat; }

	const Vec3 & GetCenter() const			{ ASSERT(IsValid()); return m_center; }
	void SetCenter(const Vec3 & v )			{ m_center = v; }

	const Vec3 & GetRadii() const			{ ASSERT(IsValid()); return m_radii; }
	void SetRadii(const Vec3 & v )			{ m_radii = v; }
	
	void SetMatrixToIdentity()				{ m_axes.SetIdentity(); }

	//! BoxToWorld is the transpose of what the OBB uses,
	//!	 but occasionally it's convenient for the client to work that way
	void SetBoxToWorldMatrix(const Mat3 & mat);

	void Translate(const Vec3 & v);
	void Scale(const Vec3 & s);
	void Rotate(const Mat3 & m);

	//-----------------------------------------------------

	//! takes a world-space vector and returns a box-space vector
	//!	 m_axes and m_center together work just like a Frame3
	const Vec3 TransformWorldToBox(const Vec3 & v) const;

	//! does the opposite of TransformWorldToBox
	const Vec3 TransformBoxToWorld(const Vec3 & v) const;

	//-----------------------------------------------------

	/*! GetCorner(corner)
		corner in [0,7]
		it's three bit flags
		1 = x , 2 = y, 4 = z
	*/
	const Vec3 GetCorner(const int corner) const;

	float GetVolume() const;

	float GetSurfaceArea() const;

	float GetRadiusSqr() const;

	//! If you cast a line through the center of the box, this tells
	//!	you how much of that line is inside the box; any convex volume
	//! can be fully described with a radius(direction) function.
	float GetRadiusInDirection(const Vec3 & dir) const;

	//! which = 1,2,3, and negatives (-1,-2,-3)
	//!	makes a plane for a face, pointing out
	//!	the implementation requires these specific value assignments
	enum EBoxFace
	{
		eFaceXpos=1,
		eFaceYpos=2,
		eFaceZpos=3,
		eFaceXneg=-1,
		eFaceYneg=-2,
		eFaceZneg=-3,
	};
	void GetPlane(Plane * pPlane,const EBoxFace which) const;

	//-----------------------------------------------------

	//! returns the point closest to "v" which is in the box
	//! returns "v" if v is inside the box	
	const Vec3 GetClampInside(const Vec3 & v) const;

	//! distance from a point to the box; 0.f for inside
	float DistanceSqr(const Vec3 & v) const;

	//! does this box Contain other things?
	bool Contains(const Vec3 & v) const;
	bool Contains(const Vec3 & v,const float tolerance = CB_EPSILON) const;

	//! ExtendToPoint just increases radii
	void ExtendToPoint(const Vec3 & v);

	//-----------------------------------------------------
	//! PlaneDist and PlaneSide
	//!	have an identical preamble, just return slightly
	//!	different things

	float PlaneDist(const Plane & plane) const;
	Plane::ESide PlaneSide(const Plane & plane) const;

	//-----------------------------------------------------
	//! OBB-LSS intersection test

	bool IntersectVolume(const Segment & seg) const;

	//-----------------------------------------------------
	
private:

	/*! data :
		60 bytes

		the rows of m_axes are the basis of the box (in world space)
		so m_axes is the world-to-box Frame3
	
		in "box space" the box is an ABB centered around the origin

		m_axes should be orthonormal for a valid box

		you can make this data more compact by multiplying the axes by the radii,
		but that makes the algorithms a lot more complex, and adds some divides

		{ m_axes + m_center } really work as a Frame3 , but it's
			easier conceptually to treat the separately
	*/

	Mat3		m_axes;
	Vec3		m_center;
	Vec3		m_radii;

	//friend class gOBBCompact;
};

//}{===================================================================================
//
// Functions :
//

inline void OrientedBox::Translate(const Vec3 & v)
{
	ASSERT(IsValid());
	m_center += v;
}

inline void OrientedBox::Scale(const Vec3 & s)
{
	ASSERT(IsValid());
	m_radii.ComponentwiseScale(s);
}

//! takes a world-space vector and returns a box-space vector
//!	 m_axes and m_center together work just like a Frame3
inline const Vec3 OrientedBox::TransformWorldToBox(const Vec3 & v) const
{
	ASSERT(IsValid());
	return m_axes.Rotate( v - m_center );
}

//! does the opposite of TransformWorldToBox
inline const Vec3 OrientedBox::TransformBoxToWorld(const Vec3 & v) const
{
	ASSERT(IsValid());
	return m_axes.RotateByTranspose( v ) + m_center;
}

inline float OrientedBox::GetVolume() const
{
	ASSERT(IsValid());
	return m_radii.x * m_radii.y * m_radii.z * 8.f; 
}

inline float OrientedBox::GetSurfaceArea() const
{
	ASSERT(IsValid());
	return ( m_radii.x * m_radii.y + m_radii.x * m_radii.z + m_radii.y * m_radii.z ) * 8.f; 
}

inline float OrientedBox::GetRadiusSqr() const
{
	ASSERT(IsValid());
	return m_radii.LengthSqr();
}

inline float OrientedBox::GetRadiusInDirection(const Vec3 & dir) const
{
	ASSERT( IsValid() && dir.IsValid() );

	const float radius = 
		fabsf((dir * m_axes.GetRowX()) * m_radii.x) +
		fabsf((dir * m_axes.GetRowY()) * m_radii.y) +
		fabsf((dir * m_axes.GetRowZ()) * m_radii.z);

	ASSERT( radius >= 0.f );

	return radius;
}

//}{===================================================================================

//namespace OrientedBoxUtil
//{

	bool Intersects(const OrientedBox & ob1,const OrientedBox & ob2);

	bool IntersectsRough_FaceNormals(const OrientedBox & ob1,const OrientedBox & ob2);
	bool IntersectsRough_SphereLike (const OrientedBox & ob1,const OrientedBox & ob2);
	// if the Rough tests returns false, then they definitely don't
	//	intersect; if it returns true, they still might be separate
//};

//}===================================================================================

END_CB

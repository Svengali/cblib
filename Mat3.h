/*!
  \file
  \brief Mat3 is a 3x3 Mat3

	The member vectors (m_x,m_y,m_z) are currently the rows, but that's
	just an implementation detail.  You should always talk to the Row/Column
	API's , which will have the same meaning if the members are switched
	to be columns.
*/

#pragma once

#include "Base.h"
#include "Vec3.h"

START_CB

//}{=======================================================================

class Mat3
{
public:
	enum EConstructorIdentity	{ eIdentity };
	enum EConstructorZero		{ eZero };
	enum EConstructorRows		{ eRows };
	enum EConstructorCols		{ eCols };

	//----------------------------------------------
	//  (see note in Mat3.cpp about static construction warning)

	__forceinline  Mat3()
	{
		// just let Vec3 constructors do their work
	}

	// important : do NOT declare copy constructors which are the default !
	//	VC needs you to not declare them for it to do full return-value optimization!
	// default operator = is fine

	explicit __forceinline Mat3(const EConstructorRows e,const Vec3 & rx,const Vec3 & ry,const Vec3 & rz)
		: m_x(rx) , m_y(ry) , m_z(rz)
	{
	}

	explicit __forceinline Mat3(const EConstructorCols e,const Vec3 & cx,const Vec3 & cy,const Vec3 & cz)
		: m_x(cx.x,cy.x,cz.x) , m_y(cx.y,cy.y,cz.y) , m_z(cx.z,cy.z,cz.z)
	{
	}

	//! use like this : Mat3(Mat3::eIdentity);
	//!	(quite intentionally not using Vec3::unitX, etc. here)
	explicit __forceinline Mat3(const EConstructorIdentity e) 
		: m_x(1.f,0.f,0.f) , m_y(0.f,1.f,0.f) , m_z(0.f,0.f,1.f)
	{
	}
	
	explicit __forceinline Mat3(const EConstructorZero e) 
		: m_x(0.f,0.f,0.f) , m_y(0.f,0.f,0.f) , m_z(0.f,0.f,0.f)
	{
	}

	explicit __forceinline Mat3(
		const float xx,const float xy,const float xz,
		const float yx,const float yy,const float yz,
		const float zx,const float zy,const float zz)
		: m_x(xx,xy,xz) , m_y(yx,yy,yz) , m_z(zx,zy,zz)
	{
	}
	
	// just IO as bytes
	//----------------------------------------------

	static const Mat3 identity;
	static const Mat3 zero;

	void SetIdentity()
	{
		m_x.Set(1,0,0);
		m_y.Set(0,1,0);
		m_z.Set(0,0,1);
		ASSERT(IsValid());
	}

	//! fuzzy equality test
	static bool Equals(const Mat3 &a,const Mat3 &b,const float tolerance = EPSILON_NORMALS);

	//----------------------------------------------
	//! Row and Column accessors :

	//! @@ \todo : too many functions in here doing the same thing!

	const Vec3 & GetRowX() const	{ ASSERT(IsValid()); return m_x; }
	const Vec3 & GetRowY() const	{ ASSERT(IsValid()); return m_y; }
	const Vec3 & GetRowZ() const	{ ASSERT(IsValid()); return m_z; }

	Vec3 & RowX()					{ return m_x; }
	Vec3 & RowY()					{ return m_y; }
	Vec3 & RowZ()					{ return m_z; }

	void SetRowX(const Vec3 &v)		{ ASSERT(v.IsValid()); m_x = v; }
	void SetRowY(const Vec3 &v)		{ ASSERT(v.IsValid()); m_y = v; }
	void SetRowZ(const Vec3 &v)		{ ASSERT(v.IsValid()); m_z = v; }

	void SetRowX(const float x,const float y,const float z) { m_x.Set(x,y,z); }
	void SetRowY(const float x,const float y,const float z) { m_y.Set(x,y,z); }
	void SetRowZ(const float x,const float y,const float z) { m_z.Set(x,y,z); }

	const Vec3 GetColumnX() const { ASSERT(IsValid()); return Vec3( m_x.x,m_y.x,m_z.x ); }
	const Vec3 GetColumnY() const { ASSERT(IsValid()); return Vec3( m_x.y,m_y.y,m_z.y ); }
	const Vec3 GetColumnZ() const { ASSERT(IsValid()); return Vec3( m_x.z,m_y.z,m_z.z ); }

	void SetColumnX(const Vec3 &v)	{ ASSERT(v.IsValid()); m_x.x = v.x; m_y.x = v.y; m_z.x = v.z; }
	void SetColumnY(const Vec3 &v)	{ ASSERT(v.IsValid()); m_x.y = v.x; m_y.y = v.y; m_z.y = v.z; }
	void SetColumnZ(const Vec3 &v)	{ ASSERT(v.IsValid()); m_x.z = v.x; m_y.z = v.y; m_z.z = v.z; }

	const Vec3 & GetRow(const int i) const
	{
		ASSERT( i >= 0 && i < 3 );
		return *( (&m_x) + i );
	}
	Vec3 & Row(const int i)
	{
		ASSERT( i >= 0 && i < 3 );
		return *( (&m_x) + i );
	}

	float GetElement(const int i,const int j) const
	{
		ASSERT( i >= 0 && i < 3 );
		ASSERT( j >= 0 && j < 3 );
		return GetRow(i)[j];
	}
	float & Element(const int i,const int j)
	{
		ASSERT( i >= 0 && i < 3 );
		ASSERT( j >= 0 && j < 3 );
		return Row(i)[j];
	}

	//! array style access :
	const Vec3 & operator [](const int i) const
	{
		return GetRow(i);
	}
	Vec3 & operator [](const int i)
	{
		return Row(i);
	}

	//----------------------------------------------
	
	//! rotate fm by this to produce to
	//!	if fm is (x == 1), you just get the "x" column out, etc.
	const Vec3 Rotate(const Vec3 & fm) const;

	//! rotate fm by (transpose of this) to produce return value
	//!	if fm is (x == 1), you just get the "x" row out, etc.
	const Vec3 RotateByTranspose(const Vec3 & fm) const;

	//----------------------------------------------
	// "Is" queries

	float GetTrace() const
	{
		ASSERT(IsValid());
		return m_x.x + m_y.y + m_z.z;
	}

	float GetDeterminant() const;

	bool IsValid() const;

	bool IsIdentity(const float tolerance = EPSILON_NORMALS) const;
	
	//! IsOrthonormal means "IsRotation"
	bool IsOrthonormal(const float tolerance = EPSILON_NORMALS) const;

	//! IsOrthogonal = IsRotation + Scale (no shear)
	bool IsOrthogonal(const float tolerance = EPSILON_NORMALS) const;

	bool IsUniformScale(const float tolerance = EPSILON_NORMALS) const;
	float GetUniformScaleSqr() const;
	float GetUniformScale() const;

	//----------------------------------------------
	// modifying the Mat3 :

	void SetProduct(const Mat3 &m1,const Mat3 &m2);
		// SetMultiple : this = m1 * m2
		// m1 can be this , m2 cannot !

	void LeftMultiply(const Mat3 &m);
	void RightMultiply(const Mat3 &m);

	void Add(const Mat3 &m);
	void Subtract(const Mat3 &m);

	void SetAverage(const Mat3 &v1,const Mat3 &v2);

	void SetLerp(const Mat3 &v1,const Mat3 &v2,const float t);

	void Scale(const float f);

	//----------------------------------------------

private:

	// the rows :
	Vec3 m_x,m_y,m_z;
};

//}{=======================================================================

//! rotate fm by this to produce to
//!	if fm is (x == 1), you just get the "x" column out, etc.
inline const Vec3 Mat3::Rotate(const Vec3 & fm) const
{
	ASSERT( IsValid() && fm.IsValid() );
	
	/*
	
	Ghastly !!
	VisualC 6 just can't optimize a function calling a function!
	I end up with Vec3::operator * function calls all over my
		optimized code!
	VC7 does this very well, though

	*/

	// return Vec3( m_x * fm, m_y * fm, m_z * fm );
	return Vec3(
			m_x.x * fm.x + m_x.y * fm.y + m_x.z * fm.z,
			m_y.x * fm.x + m_y.y * fm.y + m_y.z * fm.z,
			m_z.x * fm.x + m_z.y * fm.y + m_z.z * fm.z );
}

//! rotate fm by (transpose of this) to produce return value
//!	if fm is (x == 1), you just get the "x" row out, etc.
inline const Vec3 Mat3::RotateByTranspose(const Vec3 & fm) const
{
	// return fm.x * m_x + fm.y * m_y + fm.z * m_z
	ASSERT( IsValid() && fm.IsValid() );
	return Vec3(
			m_x.x * fm.x + m_y.x * fm.y + m_z.x * fm.z,
			m_x.y * fm.x + m_y.y * fm.y + m_z.y * fm.z,
			m_x.z * fm.x + m_y.z * fm.y + m_z.z * fm.z );
}

inline bool Mat3::IsIdentity(const float tolerance /*= EPSILON*/) const
{
	ASSERT(IsValid());
	return
		Vec3::Equals(m_x,Vec3::unitX,tolerance) &&
		Vec3::Equals(m_y,Vec3::unitY,tolerance) &&
		Vec3::Equals(m_z,Vec3::unitZ,tolerance);
}

inline bool Mat3::IsOrthogonal(const float tolerance /*= EPSILON*/) const
{
	// IsOrthogonal is weaker than IsOrthonormal
	ASSERT(IsValid());
	return
		fiszero(m_x * m_y,tolerance) &&
		fiszero(m_y * m_z,tolerance) &&
		fiszero(m_x * m_z,tolerance);
}

inline void Mat3::LeftMultiply(const Mat3 &m)
{
	ASSERT( m.IsValid() && IsValid() );
	const Mat3 t(*this);
	SetProduct(m,t);
}

inline void Mat3::RightMultiply(const Mat3 &m)
{
	ASSERT( m.IsValid() && IsValid() );
	SetProduct(*this,m);
}

inline void Mat3::Add(const Mat3 &m)
{
	ASSERT( m.IsValid() && IsValid() );
	m_x += m.m_x;
	m_y += m.m_y;
	m_z += m.m_z;
	ASSERT(IsValid());
}

inline void Mat3::Subtract(const Mat3 &m)
{
	ASSERT( m.IsValid() && IsValid() );
	m_x -= m.m_x;
	m_y -= m.m_y;
	m_z -= m.m_z;
	ASSERT(IsValid());
}

inline void Mat3::Scale(const float f)
{
	ASSERT( IsValid() );
	m_x *= f;
	m_y *= f;
	m_z *= f;
	ASSERT(IsValid());
}


//! Matrix * Vector math operator; returns the rotated vector
__forceinline const Vec3 operator * (const Mat3 & m,const Vec3 & v)
{
	return m.Rotate(v);
}

inline float Mat3::GetUniformScaleSqr() const
{
	ASSERT( IsUniformScale() );
	return m_x.LengthSqr();
}

inline float Mat3::GetUniformScale() const
{
	ASSERT( IsUniformScale() );
	return m_x.Length();
}

//}{=======================================================================

END_CB

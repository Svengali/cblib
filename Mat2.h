#pragma once

#include "cblib/Base.h"
#include "cblib/Vec2.h"

START_CB

class Mat2
{
public:
	enum EConstructorIdentity	{ eIdentity };
	enum EConstructorZero		{ eZero };
	enum EConstructorRows		{ eRows };
	enum EConstructorCols		{ eCols };

	//----------------------------------------------
	//  (see note in Mat2.cpp about static construction warning)

	__forceinline  Mat2()
	{
		// just let Vec2 constructors do their work
	}

	// important : do NOT declare copy constructors which are the default !
	//	VC needs you to not declare them for it to do full return-value optimization!
	// default operator = is fine

	explicit __forceinline Mat2(const EConstructorRows,const Vec2 & rx,const Vec2 & ry)
		: m_x(rx) , m_y(ry)
	{
	}

	explicit __forceinline Mat2(const EConstructorCols,const Vec2 & cx,const Vec2 & cy)
		: m_x(cx.x,cy.x) , m_y(cx.y,cy.y)
	{
	}

	//! use like this : Mat2(Mat2::eIdentity);
	//!	(quite intentionally not using Vec2::unitX, etc. here)
	explicit __forceinline Mat2(const EConstructorIdentity) 
		: m_x(1.f,0.f) , m_y(0.f,1.f)
	{
	}
	
	explicit __forceinline Mat2(const EConstructorZero) 
		: m_x(0.f,0.f) , m_y(0.f,0.f)
	{
	}

	// just IO as bytes
	//----------------------------------------------

	Vec2 & operator[] (const int r)
	{
		ASSERT( r >= 0 && r < 2 );
		return ((Vec2 *)&m_x)[r];
	}
	const Vec2 & operator[] (const int r) const
	{
		ASSERT( r >= 0 && r < 2 );
		return ((Vec2 *)&m_x)[r];
	}

	const Vec2 & GetRowX() const	{ ASSERT(IsValid()); return m_x; }
	const Vec2 & GetRowY() const	{ ASSERT(IsValid()); return m_y; }
	Vec2 & RowX()					{ return m_x; }
	Vec2 & RowY()					{ return m_y; }
	const Vec2 GetColumnX() const { ASSERT(IsValid()); return Vec2( m_x.x,m_y.x ); }
	const Vec2 GetColumnY() const { ASSERT(IsValid()); return Vec2( m_x.y,m_y.y ); }
	void SetColumnX(const Vec2 &v)	{ ASSERT(v.IsValid()); m_x.x = v.x; m_y.x = v.y; }
	void SetColumnY(const Vec2 &v)	{ ASSERT(v.IsValid()); m_x.y = v.x; m_y.y = v.y; }

	const Vec2 & GetRow(const int i) const
	{
		ASSERT( i >= 0 && i < 2 );
		return *( (&m_x) + i );
	}
	Vec2 & Row(const int i)
	{
		ASSERT( i >= 0 && i < 2 );
		return *( (&m_x) + i );
	}
	float GetElement(const int i,const int j) const
	{
		ASSERT( i >= 0 && i < 2 );
		ASSERT( j >= 0 && j < 2 );
		return GetRow(i)[j];
	}
	float & Element(const int i,const int j)
	{
		ASSERT( i >= 0 && i < 2 );
		ASSERT( j >= 0 && j < 2 );
		return Row(i)[j];
	}

	//----------------------------------------------

	void SetOuterProduct(const Vec2 & u,const Vec2 & v)
	{
		m_x.x = u.x * v.x;
		m_x.y = u.x * v.y;
		m_y.x = u.y * v.x;
		m_y.y = u.y * v.y;
	}

	const Vec2 Rotate(const Vec2 & v) const
	{
		return Vec2( m_x * v , m_y * v );
	}
	const Vec2 RotateByTranspose(const Vec2 & v) const
	{
		//return m_x * v.x + m_y * v.y;
		return Vec2( m_x.x * v.x + m_y.x * v.y,
					  m_x.y * v.x + m_y.y * v.x );
	}

	void SetIdentity()
	{
		m_x.Set(1,0);
		m_y.Set(0,1);
	}
	void SetZero()
	{
		m_x.Set(0,0);
		m_y.Set(0,0);
	}

	void Add(const Mat2 & m)
	{
		m_x += m.m_x;
		m_y += m.m_y;
	}

	float GetTrace() const
	{
		ASSERT(IsValid());
		return m_x.x + m_y.y;
	}

	float GetDeterminant() const
	{
		return m_x.x * m_y.y - m_x.y * m_y.x;
	}

	bool IsValid() const
	{
		ASSERT( m_x.IsValid() );
		ASSERT( m_y.IsValid() );
		return true;
	}

	bool IsOrthonormal(const float tolerance = EPSILON) const
	{
		return fisone( GetDeterminant(), tolerance );	
	}

	//! fuzzy equality test
	static bool Equals(const Mat2 &a,const Mat2 &b,const float tolerance = EPSILON);

	void SetProduct(const Mat2 &m1,const Mat2 &m2)
		// SetMultiple : this = m1 * m2
		// m1 and m2 cannot be "this"
	{
		ASSERT( &m1 != this && &m2 != this );
		m_x.x = m1.m_x * m2.GetColumnX();
		m_x.y = m1.m_x * m2.GetColumnY();
		m_y.x = m1.m_y * m2.GetColumnX();
		m_y.y = m1.m_y * m2.GetColumnY();
	}

private:
	// rows :
	Vec2	m_x;
	Vec2	m_y;
};

//! Matrix * Vector math operator; returns the rotated vector
__forceinline const Vec2 operator * (const Mat2 & m,const Vec2 & v)
{
	return m.Rotate(v);
}

namespace Mat2U
{
	float GetInverse(const Mat2 & fm,Mat2 * pTo);
};

END_CB

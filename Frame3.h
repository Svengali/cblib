/*!
  \file
  \brief Frame3 is a Mat3 with a Translation
*/

#pragma once

#include "cblib/Mat3.h"

START_CB

//}{=======================================================================

/*!

a Frame3 is a Mat3 + a translation

it's equivalent to a 3x4 Mat3

In typical use, the Mat3 should be orthonormal, representing just a Rotation,
but this class does not enforce that constraint (you must check it with
IsOrthonormal).

tulrich: Frame3 used to privately inherit from Mat3, which simplified
passing through the Mat3::Get/SetRowX/Y/Z stuff, but also made it hard
to wrap with CVAR.  So I changed it to the more conventional approach
of using a Mat3 member.

*/

class Frame3
{
public:

	enum EConstructorIdentity	{ eIdentity };
	enum EConstructorZero		{ eZero };
	enum EConstructorRows		{ eRows };
	enum EConstructorCols		{ eCols };

	//----------------------------------------------
	//  (see note in Mat3.cpp about static construction warning)

	__forceinline  Frame3()
	{
		// just let Vec3 constructors do their work
	}

	// important : do NOT declare copy constructors which are the default !
	//	VC needs you to not declare them for it to do full return-value optimization!
	// default operator = is fine

	explicit __forceinline Frame3(const Mat3 & m,const Vec3 & t)
		: m_mat(m) , m_t(t)
	{
	}

	explicit __forceinline Frame3(const Vec3 & t)
		: m_mat(Mat3::eIdentity) , m_t(t)
	{
	}
	
	explicit __forceinline Frame3(const EConstructorRows,const Vec3 & rx,const Vec3 & ry,const Vec3 & rz)
		: m_mat(m_mat.eRows,rx,ry,rz) , m_t(0.f,0.f,0.f)
	{
	}

	explicit __forceinline Frame3(const EConstructorRows,const Vec3 & rx,const Vec3 & ry,const Vec3 & rz,const Vec3 & t)
		: m_mat(Mat3::eRows,rx,ry,rz) , m_t(t)
	{
	}

	explicit __forceinline Frame3(const EConstructorCols,const Vec3 & rx,const Vec3 & ry,const Vec3 & rz)
		: m_mat(Mat3::eCols,rx,ry,rz) , m_t(0.f,0.f,0.f)
	{
	}

	explicit __forceinline Frame3(const EConstructorCols,const Vec3 & rx,const Vec3 & ry,const Vec3 & rz,const Vec3 & t)
		: m_mat(Mat3::eCols,rx,ry,rz) , m_t(t)
	{
	}

	//! use like this : Frame3(Frame3::eIdentity);
	//!	(quite intentionally not using Vec3::unitX, etc. here)
	explicit __forceinline Frame3(const EConstructorIdentity) 
		: m_mat(Mat3::eIdentity) , m_t(0.f,0.f,0.f)
	{
	}
	
	explicit __forceinline Frame3(const EConstructorZero) 
		: m_mat(Mat3::eZero) , m_t(0.f,0.f,0.f)
	{
	}

	//----------------------------------------------

	static const Frame3 identity;
	static const Frame3 zero;

	void SetIdentity();

	static bool Equals(const Frame3 &a,const Frame3 &b,const float tolerance = EPSILON);

	//----------------------------------------------
	//!  Mat3 accessors

	void SetMatrix(const Mat3 & m);
	const Mat3 & GetMatrix() const;
	Mat3 & MutableMatrix();

	//----------------------------------------------
	//!  Translation accessors

	const Vec3 & GetTranslation() const		{ ASSERT(m_t.IsValid()); return m_t; }
	void SetTranslation(const Vec3 & v)		{ ASSERT(v.IsValid()); m_t = v; }
	Vec3 & MutableTranslation()				{ /*ASSERT(m_t.IsValid());*/ return m_t; }

	//----------------------------------------------
	//! Frame3 a vector :
	//!		Rotate then Translate

	const Vec3	Rotate(const Vec3& fm) const { return m_mat.Rotate(fm); }
	const Vec3	RotateByTranspose(const Vec3& fm) const { return m_mat.RotateByTranspose(fm); }

	//! Rotate and Translate "fm" to make result
	const Vec3 Transform(const Vec3 & fm) const;

	//! Rotate and Translate "fm" to make result
	//!	If the matrix is orthonormal, then this willl undo what Transform() does
	const Vec3 TransformTranspose(const Vec3 & fm) const;

	//----------------------------------------------
	//! Mat3 stuff ; identity rotation
	//!	 with translation support
	
//	using Mat3::GetTrace;
//	using Mat3::GetDeterminant;
//	using Mat3::IsOrthonormal;
//	using Mat3::IsOrthogonal;

	bool IsOrthonormal() const { return m_mat.IsOrthonormal(); }

	bool IsValid() const;
	bool IsIdentity(const float tolerance = EPSILON) const;

	//----------------------------------------------
	//! Building the Transform

	//! SetMultiple : this = m1 * m2
	//! m1 can be this , m2 cannot !
	void SetProduct(const Frame3 &m1,const Frame3 &m2);
	void LeftMultiply(const Frame3 &m);
	void RightMultiply(const Frame3 &m);

	//! Mat3 + Frame3 combiners :
	void SetProduct(const Mat3 &m1,const Frame3 &m2);
	void SetProduct(const Frame3 &m1,const Mat3 &m2);

	void LeftMultiply(const Mat3 &m);
	void RightMultiply(const Mat3 &m);

	// I intentionally don't provide operator += and such
	//	so that the caller is aware he's doing a complex operation.
	//	in fact, adding and subtracting matrices is a really weird thing to do.

	void Add(const Frame3 &m);
	void Subtract(const Frame3 &m);

	void SetAverage(const Frame3 &v1,const Frame3 &v2);

	void SetLerp(const Frame3 &v1,const Frame3 &v2,const float t);

//	using Mat3::IsUniformScale;
//	using Mat3::GetUniformScaleSqr;
//	using Mat3::GetUniformScale;

	//----------------------------------------------
	//! Row and Column accessors :
	//!	mostly just drill up to the Mat3 parent class

	const Vec3&	GetRowX() const { return m_mat.GetRowX(); }
	const Vec3&	GetRowY() const { return m_mat.GetRowY(); }
	const Vec3&	GetRowZ() const { return m_mat.GetRowZ(); }
	const Vec3&	   RowX() 		{ return m_mat.RowX(); }
	const Vec3&	   RowY() 		{ return m_mat.RowY(); }
	const Vec3&	   RowZ() 		{ return m_mat.RowZ(); }

	void SetRowX(const Vec3& v) { m_mat.SetRowX(v); }
	void SetRowY(const Vec3& v) { m_mat.SetRowY(v); }
	void SetRowZ(const Vec3& v) { m_mat.SetRowZ(v); }

	const Vec3	GetColumnX() const { return m_mat.GetColumnX(); }
	const Vec3	GetColumnY() const { return m_mat.GetColumnY(); }
	const Vec3	GetColumnZ() const { return m_mat.GetColumnZ(); }
	void  		SetColumnX(const Vec3& v) 	   { m_mat.SetColumnX(v); }
	void  		SetColumnY(const Vec3& v) 	   { m_mat.SetColumnY(v); }
	void  		SetColumnZ(const Vec3& v) 	   { m_mat.SetColumnZ(v); }
	
	//! GetRow() and such act like they're indexing a Mat3 with
	//! 4 rows and 3 columns

	/** Needs explanation: this sounds like we're
		operator-on-rightist, but we aren't.  We are
		operator-on-leftist.  The matrix is laid out as follows:
		
		[ m_x.x  m_x.y  m_x.z  m_t.x ]
		[ m_y.x  m_y.y  m_y.z  m_t.y ]
		[ m_z.x  m_z.y  m_z.z  m_t.z ]
		[   0      0      0      1   ]

		So m_t is vertical, but m_x, m_y, m_z are horizontal.
		Confusing, yes, but that's how we do it.  When you ask for a
		"row", you're getting rows *except* if you ask for row 3, in
		which case you're getting a column!!!

		GetElement()/Element() and operator[] suffer from the same
		confusion.

	*/
	const Vec3 & GetRow(const int i) const;
	Vec3 & Row(const int i);

	float GetElement(const int i,const int j) const;
	float & Element(const int i,const int j);

	//! array style access :
	const Vec3 & operator [](const int i) const	{ return GetRow(i); }
	Vec3 & operator [](const int i)				{ return Row(i); }

	//----------------------------------------------

private:

	// variables muse be in this order for the current array-style access implementation to work
	// (row 0-2 is in the matrix & row 3 is translation)
	Mat3 m_mat;
	Vec3 m_t;
};

//}{=======================================================================

inline void Frame3::SetIdentity()
{
	m_mat.SetIdentity();
	m_t = Vec3::zero;
	ASSERT(IsValid());
}

inline void Frame3::SetMatrix(const Mat3 & m)
{
	ASSERT(m.IsValid());
	m_mat = m;
}

inline const Mat3 & Frame3::GetMatrix() const
{
	ASSERT(IsValid());
	return m_mat;
}

inline Mat3 & Frame3::MutableMatrix()
{
	// may not be valid at this point
	///ASSERT(IsValid());
	return m_mat;
}

//----------------------------------------------

//! Rotate and Translate "fm" to make result
inline const Vec3 Frame3::Transform(const Vec3 & fm) const
{
	ASSERT( IsValid() && fm.IsValid() );
	// @@ does this optimize well enough?
	return m_mat.Rotate(fm) + m_t;
}

//! Rotate and Translate "fm" to make result
//!	If the matrix is orthonormal, then this willl undo what Transform() does
inline const Vec3 Frame3::TransformTranspose(const Vec3 & fm) const
{
	return m_mat.RotateByTranspose(fm - m_t);
}

//! Transform * Vector mathematical operator product; makes a Vec3
__forceinline const Vec3 operator * (const Frame3 & xf,const Vec3 & v)
{
	return xf.Transform(v);
}

//----------------------------------------------
//! Building the Transform

inline void Frame3::SetProduct(const Frame3 &m1,const Frame3 &m2)
{
	// m1 can = this !
	ASSERT( &m2 != this );

	// make sure you set m_t before m_mat , since m1 could be "this" !!
	// m_t = m1 * m2.t + m1.t
	m_t = m1.Transform(m2.m_t);

	m_mat.SetProduct(m1.GetMatrix(),m2.GetMatrix());
}

inline void Frame3::LeftMultiply(const Frame3 &m)
{
	ASSERT( m.IsValid() && IsValid() );
	Frame3 t = *this;
	SetProduct(m,t);
}

inline void Frame3::RightMultiply(const Frame3 &m)
{
	ASSERT( m.IsValid() && IsValid() );
	//Frame3 t = *this;
	//SetProduct(t,m);
	SetProduct(*this,m);
}

//! Mat3 + Frame3 combiners :
inline void Frame3::SetProduct(const Mat3 &m1,const Frame3 &m2)
{
	m_mat.SetProduct(m1,m2.GetMatrix());

	// m_t = m1 * m2.t;
	m_t = m1.Rotate(m2.m_t);
}

inline void Frame3::SetProduct(const Frame3 &m1,const Mat3 &m2)
{
	m_mat.SetProduct(m1.GetMatrix(),m2);
	m_t = m1.m_t;
}

inline void Frame3::LeftMultiply(const Mat3 &m)
{
	ASSERT( m.IsValid() && IsValid() );
	Frame3 t = *this;
	SetProduct(m,t);
}

inline void Frame3::RightMultiply(const Mat3 &m)
{
	ASSERT( m.IsValid() && IsValid() );
	SetProduct(*this,m);
}

// I intentionally don't provide operator += and such
//	so that the caller is aware he's doing a complex operation.
//	in fact, adding and subtracting matrices is a really weird thing to do.

inline void Frame3::Add(const Frame3 &m)
{
	m_mat.Add(m.GetMatrix());
	m_t += m.m_t;
}

inline void Frame3::Subtract(const Frame3 &m)
{
	m_mat.Subtract(m.GetMatrix());
	m_t -= m.m_t;
}


//-----------------------------------------------------------
// Row accessors


	//! GetRow() and such act like they're indexing a Mat3 with
	//! 4 rows and 3 columns

inline const Vec3 & Frame3::GetRow(const int i) const
{
	ASSERT( i >= 0 && i < 4 );
	return *( (& m_mat.GetRowX()) + i );
}

inline Vec3 & Frame3::Row(const int i)
{
	ASSERT( i >= 0 && i < 4 );
	return *( (& m_mat.RowX()) + i );
}

inline float Frame3::GetElement(const int i,const int j) const
{
	ASSERT( i >= 0 && i < 4 );
	ASSERT( j >= 0 && j < 3 );
	return GetRow(i)[j];
}

inline float & Frame3::Element(const int i,const int j)
{
	ASSERT( i >= 0 && i < 4 );
	ASSERT( j >= 0 && j < 3 );
	return Row(i)[j];
}

//}{=======================================================================

END_CB

/*!
  
	Mat4 is a 4x4 matrix
	it encompasses Frame3Scaled, and more

	Mat4 is byte-wise identical to a D3D or OpenGL 4x4 matrix

	this matrix is stored "Column-Row" in memory,
	all my references to "column" and "row" are the
		mathematical definition for "Vector on the right", 
		NOT the location in memory
	this is all done to make it match D3D/OpenGL

	If you access it using my [][] function it will be [row][col] like you want
		if you cast the whole thing to a float * you will get [col][row] indexing (the transpose)

	We think of our 4x4 matrices in "projective homogenous" space, typically;
		that is, we'll eventually divide by W
	By convention, things that "transform like points" have W = 1
	Then thing that "transform like normals" have W = 0

	Row-Column X,Y,Z are the rotation, scale and shear
	The "W" column is the translation (that's [x][3])
	The "W" elements of all the columns do odd projective things, 
		they're typically 0, except for element [3][3], which is 1

	Mat4 can be cast to a D3DXMATRIX or XGMATRIX

*/

#pragma once

#include "cblib/Base.h"
#include "cblib/Vec4.h"
#include "cblib/Mat3.h"
#include "cblib/Frame3.h"
#include "cblib/Frame3Scaled.h"

START_CB

//{= decl ======================================================================

// try to use Mat4Aligned not Mat4 when possible
#define Mat4Aligned DECL_ALIGN(16) Mat4

//DECL_ALIGN(16) 
class Mat4
{
public:
	enum EConstructorIdentity	{ eIdentity };
	enum EConstructorZero		{ eZero };
	enum EConstructorRows		{ eRows };
	enum EConstructorCols		{ eCols };

	//-------------------------------------------------------------------------
	//  (see note in Mat4.cpp about static construction warning)

	__forceinline  Mat4()
	{
		// just let Vec3 constructors do their work
	}

	// important : do NOT declare copy constructors which are the default !
	//	VC needs you to not declare them for it to do full return-value optimization!
	// default operator = is fine

	explicit __forceinline Mat4(const EConstructorRows e,const Vec4 & rx,const Vec4 & ry,const Vec4 & rz,const Vec4 & rw)
		: m_x(rx.x,ry.x,rz.x,rw.x) , m_y(rx.y,ry.y,rz.y,rw.y) , m_z(rx.z,ry.z,rz.z,rw.z), m_w(rx.w,ry.w,rz.w,rw.w)
	{
	}

	explicit __forceinline Mat4(const EConstructorCols e,const Vec4 & cx,const Vec4 & cy,const Vec4 & cz,const Vec4 & cw)
		: m_x(cx) , m_y(cy) , m_z(cz), m_w(cw)
	{
	}

	//! use like this : Mat4(Mat4::eIdentity);
	//!	(quite intentionally not using Vec3::unitX, etc. here)
	explicit __forceinline Mat4(const EConstructorIdentity e) 
		: m_x(1.f,0.f,0.f,0.f) , m_y(0.f,1.f,0.f,0.f) , m_z(0.f,0.f,1.f,0.f) , m_w(0.f,0.f,0.f,1.f)
	{
	}
	
	explicit __forceinline Mat4(const EConstructorZero e) 
		: m_x(0.f,0.f,0.f,0.f) , m_y(0.f,0.f,0.f,0.f) , m_z(0.f,0.f,0.f,0.f), m_w(0.f,0.f,0.f,0.f)
	{
	}

	//constructors from Mat3, Frame3, Frame3Scaled, etc.
	explicit Mat4(const Mat3 & mat);
	explicit Mat4(const Mat3 & mat, const Vec3 & translation);
	explicit Mat4(const Mat3 & mat, const Vec3 & translation,const float scale);
	explicit Mat4(const Frame3 & frame);
	explicit Mat4(const Frame3Scaled & frame);

	// just IO as bytes
	//-------------------------------------------------------------------------

	static const Mat4 identity;
	static const Mat4 zero;

	void SetIdentity();

	//! fuzzy equality test
	static bool Equals(const Mat4 &a,const Mat4 &b,const float tolerance = EPSILON);

	bool IsValid() const;

	//! a "TRS" can be represented as a Frame3Scaled
	bool IsTRS() const;

	//-------------------------------------------------------------------------
	//! Row and Column accessors :

	const Vec4 & GetColumnX() const	{ ASSERT(IsValid()); return m_x; }
	const Vec4 & GetColumnY() const	{ ASSERT(IsValid()); return m_y; }
	const Vec4 & GetColumnZ() const	{ ASSERT(IsValid()); return m_z; }
	const Vec4 & GetColumnW() const	{ ASSERT(IsValid()); return m_w; }

	Vec4 & ColumnX()					{ return m_x; }
	Vec4 & ColumnY()					{ return m_y; }
	Vec4 & ColumnZ()					{ return m_z; }
	Vec4 & ColumnW()					{ return m_w; }

	void SetColumnX(const Vec4 &v)		{ ASSERT(v.IsValid()); m_x = v; }
	void SetColumnY(const Vec4 &v)		{ ASSERT(v.IsValid()); m_y = v; }
	void SetColumnZ(const Vec4 &v)		{ ASSERT(v.IsValid()); m_z = v; }
	void SetColumnW(const Vec4 &v)		{ ASSERT(v.IsValid()); m_w = v; }

	void SetColumnX(const float x,const float y,const float z,const float w) { m_x.Set(x,y,z,w); }
	void SetColumnY(const float x,const float y,const float z,const float w) { m_y.Set(x,y,z,w); }
	void SetColumnZ(const float x,const float y,const float z,const float w) { m_z.Set(x,y,z,w); }
	void SetColumnW(const float x,const float y,const float z,const float w) { m_w.Set(x,y,z,w); }

	const Vec4 GetRowX() const { return Vec4( m_x.x,m_y.x,m_z.x,m_w.x ); }
	const Vec4 GetRowY() const { return Vec4( m_x.y,m_y.y,m_z.y,m_w.y ); }
	const Vec4 GetRowZ() const { return Vec4( m_x.z,m_y.z,m_z.z,m_w.z ); }
	const Vec4 GetRowW() const { return Vec4( m_x.w,m_y.w,m_z.w,m_w.w ); }

	void SetRowX(const Vec4 &v)	{ ASSERT(v.IsValid()); m_x.x = v.x; m_y.x = v.y; m_z.x = v.z; m_w.x = v.w; }
	void SetRowY(const Vec4 &v)	{ ASSERT(v.IsValid()); m_x.y = v.x; m_y.y = v.y; m_z.y = v.z; m_w.y = v.w; }
	void SetRowZ(const Vec4 &v)	{ ASSERT(v.IsValid()); m_x.z = v.x; m_y.z = v.y; m_z.z = v.z; m_w.z = v.w; }
	void SetRowW(const Vec4 &v)	{ ASSERT(v.IsValid()); m_x.w = v.x; m_y.w = v.y; m_z.w = v.z; m_w.w = v.w; }

	const Vec4 & GetColumn(const int i) const
	{
		ASSERT( i >= 0 && i < 4 );
		return *( (&m_x) + i );
	}
	Vec4 & Column(const int i)
	{
		ASSERT( i >= 0 && i < 4 );
		return *( (&m_x) + i );
	}
	const Vec4 GetRow(const int i) const
	{
		ASSERT( i >= 0 && i < 4 );
		return Vec4( m_x[i],m_y[i],m_z[i],m_w[i] );
	}

	float GetElement(const int i,const int j) const
	{
		ASSERT( i >= 0 && i < 4 );
		ASSERT( j >= 0 && j < 4 );
		return ((float *)this)[ i + j*4 ];
	}
	float & Element(const int i,const int j)
	{
		ASSERT( i >= 0 && i < 4 );
		ASSERT( j >= 0 && j < 4 );
		return ((float *)this)[ i + j*4 ];
	}

	// warning : if you GetData() it's in column-row order !
	float * GetData() { return (float *)this; }
	const float * GetData() const { return (float *)this; }

	//--------------------------------------------------------------------------------

	//! rotate fm by this to produce to
	//!	if fm is (x == 1), you just get the "x" column out, etc.
	const Vec4 Multiply(const Vec4 & fm) const;

	//! rotate fm by (transpose of this) to produce return value
	//!	if fm is (x == 1), you just get the "x" row out, etc.
	const Vec4 MultiplyByTranspose(const Vec4 & fm) const;

	//! Homegeneous multiply.  Assumes that w == 1, and the w component is to be discarded after the xform
	const Vec3 Multiply(const Vec3 & fm) const;

	//--------------------------------------------------------------------------------
	
	void Set(const Mat3 & rot,const Vec3 & translation);
	void ScaleColumns(const Vec3 & scale);

	void GetMat3(Mat3 * pMat) const;
	void GetFrame3Scaled(Frame3Scaled * pXFS) const;

	void SetTranslation(const Vec3 & t);
	const Vec3 & GetTranslation() const;

	float GetUniformScale() const;

	//--------------------------------------------------------------------------------

	void SetProduct(const Mat4 &m1,const Mat4 &m2);
		// SetMultiple : this = m1 * m2
		// m1 can be this , m2 cannot !
		// this is equivalent to :
		// D3DXMatrixMultiply(this,&m2,&m1); !! note reversal !!

	void LeftMultiply(const Mat4 &m);
	void RightMultiply(const Mat4 &m);

	void Add(const Mat4 &m);
	void Subtract(const Mat4 &m);

	void Scale(const float f);

	float GetDeterminant() const;
	void  GetTranspose(Mat4 * pMat) const;
	float GetInverse(Mat4 * pMat) const; //!< returns determinant, 0.f for failure !!

	//--------------------------------------------------------------------------------

	//! set pM to |left><right|
	//! then M |right> = |left> (right^2)
	void  SetOuterProduct(const Vec4 & left,const Vec4 & right);
	float GetInnerProduct(const Vec4 & left,const Vec4 & right) const;
	float GetInnerProduct(const Vec3 & left,const Vec3 & right) const;

	//--------------------------------------------------------------------------------
	// operator[] is a PITA because I must reverse the order of accessors

	struct OperatorBracketProxy
	{
	public:
		__forceinline OperatorBracketProxy(Mat4 * const _owner,const int _first_ref) : owner(_owner) , first_ref(_first_ref)
		{
		}
		
		__forceinline float & operator [](const int second_ref) const
		{
			ASSERT( second_ref >= 0 && second_ref < 4 );
			return owner->Element(first_ref,second_ref);
		}

	private:
		Mat4 * const	owner;
		const int		first_ref;
		FORBID_ASSIGNMENT(OperatorBracketProxy);
	};

	struct OperatorBracketProxyC
	{
	public:
		__forceinline OperatorBracketProxyC(const Mat4 * const _owner,const int _first_ref) : owner(_owner) , first_ref(_first_ref)
		{
		}
		
		__forceinline float operator [](const int second_ref) const
		{
			ASSERT( second_ref >= 0 && second_ref < 4 );
			return owner->GetElement(first_ref,second_ref);
		}

	private:
		const Mat4 * const owner;
		const int			first_ref;
		FORBID_ASSIGNMENT(OperatorBracketProxyC);
	};

	//! array style access :
	const OperatorBracketProxyC operator [](const int i) const
	{
		ASSERT( i >= 0 && i < 4 );
		return OperatorBracketProxyC(this,i);
	}
	OperatorBracketProxy operator [](const int i)
	{
		ASSERT( i >= 0 && i < 4 );
		return OperatorBracketProxy(this,i);
	}

	//--------------------------------------------------------------------------------

private:

	// these are the *columns*
	Vec4	m_x;
	Vec4	m_y;
	Vec4	m_z;
	Vec4	m_w;
};

//}{= inlines ======================================================================

inline bool Mat4::IsValid() const
{
	ASSERT( m_x.IsValid() );
	ASSERT( m_y.IsValid() );
	ASSERT( m_z.IsValid() );
	ASSERT( m_w.IsValid() );
	return true;
}

inline void Mat4::SetIdentity()
{
	m_x.Set(1.f,0.f,0.f,0.f);
	m_y.Set(0.f,1.f,0.f,0.f);
	m_z.Set(0.f,0.f,1.f,0.f);
	m_w.Set(0.f,0.f,0.f,1.f);
}

inline bool Mat4::Equals(const Mat4 &a,const Mat4 &b,const float tolerance /*= EPSILON*/)
{
	return 
		Vec4::Equals(a.m_x,b.m_x,tolerance) &&
		Vec4::Equals(a.m_y,b.m_y,tolerance) &&
		Vec4::Equals(a.m_z,b.m_z,tolerance) &&
		Vec4::Equals(a.m_w,b.m_w,tolerance);
}

//--------------------------------------------------------------------------------
//! rotate fm by this to produce to
//!	if fm is (x == 1), you just get the "x" column out, etc.
inline const Vec4 Mat4::Multiply(const Vec4 & fm) const
{
	// return fm.x * m_x + fm.y * m_y + fm.z * m_z + fm.w * m_w
	return Vec4(
			m_x.x * fm.x + m_y.x * fm.y + m_z.x * fm.z + m_w.x * fm.w,
			m_x.y * fm.x + m_y.y * fm.y + m_z.y * fm.z + m_w.y * fm.w,
			m_x.z * fm.x + m_y.z * fm.y + m_z.z * fm.z + m_w.z * fm.w,
			m_x.w * fm.x + m_y.w * fm.y + m_z.w * fm.z + m_w.w * fm.w); 
}

//! rotate fm by (transpose of this) to produce return value
//!	if fm is (x == 1), you just get the "x" row out, etc.
inline const Vec4 Mat4::MultiplyByTranspose(const Vec4 & fm) const
{
	// return Vec4( m_x * fm, m_y * fm, m_z * fm, m_w * fm );
	return Vec4(
			m_x.x * fm.x + m_x.y * fm.y + m_x.z * fm.z + m_x.w * fm.w,
			m_y.x * fm.x + m_y.y * fm.y + m_y.z * fm.z + m_y.w * fm.w,
			m_z.x * fm.x + m_z.y * fm.y + m_z.z * fm.z + m_z.w * fm.w,
			m_w.x * fm.x + m_w.y * fm.y + m_w.z * fm.z + m_w.w * fm.w );
}

inline const Vec3 Mat4::Multiply(const Vec3 & fm) const
{
	// return fm.x * m_x + fm.y * m_y + fm.z * m_z + fm.w * m_w
	return Vec3(m_x.x * fm.x + m_y.x * fm.y + m_z.x * fm.z + m_w.x,
				m_x.y * fm.x + m_y.y * fm.y + m_z.y * fm.z + m_w.y,
				m_x.z * fm.x + m_y.z * fm.y + m_z.z * fm.z + m_w.z);
}

//! Matrix * Vector math operator; returns the rotated Vector
__forceinline const Vec4 operator * (const Mat4 & m,const Vec4 & v)
{
	return m.Multiply(v);
}

//--------------------------------------------------------------------------------

inline void Mat4::LeftMultiply(const Mat4 &m)
{
	ASSERT( m.IsValid() && IsValid() );
	// no copies needed with current SetProduct
	SetProduct(m,*this);
}

inline void Mat4::RightMultiply(const Mat4 &m)
{
	ASSERT( m.IsValid() && IsValid() );
	// no copies needed with current SetProduct
	SetProduct(*this,m);
}

inline void Mat4::Add(const Mat4 &m)
{
	ASSERT( m.IsValid() && IsValid() );
	m_x += m.m_x;
	m_y += m.m_y;
	m_z += m.m_z;
	m_w += m.m_w;
	ASSERT(IsValid());
}

inline void Mat4::Subtract(const Mat4 &m)
{
	ASSERT( m.IsValid() && IsValid() );
	m_x -= m.m_x;
	m_y -= m.m_y;
	m_z -= m.m_z;
	m_w -= m.m_w;
	ASSERT(IsValid());
}

inline void Mat4::Scale(const float f)
{
	ASSERT( IsValid() && fisvalid(f) );
	m_x *= f;
	m_y *= f;
	m_z *= f;
	m_w *= f;
	ASSERT(IsValid());
}

//--------------------------------------------------------------------------------

inline void Mat4::Set(const Mat3 & rot,const Vec3 & translation)
{
	ASSERT( rot.IsValid() && translation.IsValid() );
	m_x.Set( rot.GetColumnX(), 0.f );
	m_y.Set( rot.GetColumnY(), 0.f );
	m_z.Set( rot.GetColumnZ(), 0.f );
	m_w.Set( translation, 1.f );
}

inline float Mat4::GetUniformScale() const
{
	return m_x.Length();
}

inline const Vec3 & Mat4::GetTranslation() const
{
	ASSERT( m_w.GetVec3().IsValid() );
	return m_w.GetVec3(); 
}

inline void Mat4::SetTranslation(const Vec3 & t)
{
	m_w.MutableVec3() = t;
}

inline void Mat4::GetMat3(Mat3 * pMat) const
{
	pMat->SetColumnX( m_x.GetVec3() );
	pMat->SetColumnY( m_y.GetVec3() );
	pMat->SetColumnZ( m_z.GetVec3() );
}

inline void Mat4::ScaleColumns(const Vec3 & scale)
{
	ASSERT( scale.IsValid() );
	m_x *= scale.x;
	m_y *= scale.y;
	m_z *= scale.z;
}

inline Mat4::Mat4(const Mat3 & mat) : 
	m_x(mat.GetColumnX(),0.f),
	m_y(mat.GetColumnY(),0.f),
	m_z(mat.GetColumnZ(),0.f),
	m_w(0.f,0.f,0.f,1.f)
{
}
inline Mat4::Mat4(const Mat3 & mat, const Vec3 & translation) : 
	m_x(mat.GetColumnX(),0.f),
	m_y(mat.GetColumnY(),0.f),
	m_z(mat.GetColumnZ(),0.f),
	m_w(translation,1.f)
{
}
inline Mat4::Mat4(const Mat3 & mat, const Vec3 & translation,const float scale) :
	m_x(scale*mat.GetColumnX(),0.f),
	m_y(scale*mat.GetColumnY(),0.f),
	m_z(scale*mat.GetColumnZ(),0.f),
	m_w(translation,1.f)
{
}
inline Mat4::Mat4(const Frame3 & frame) : 
	m_x(frame.GetColumnX(),0.f),
	m_y(frame.GetColumnY(),0.f),
	m_z(frame.GetColumnZ(),0.f),
	m_w(frame.GetTranslation(),1.f)
{
}
inline Mat4::Mat4(const Frame3Scaled & frame) : 
	m_x(frame.GetMatrix().GetColumnX() * frame.GetScale(),0.f),
	m_y(frame.GetMatrix().GetColumnY() * frame.GetScale(),0.f),
	m_z(frame.GetMatrix().GetColumnZ() * frame.GetScale(),0.f),
	m_w(frame.GetTranslation(),1.f)
{
}

//}=======================================================================

END_CB

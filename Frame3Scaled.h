/*!
  \file
  \brief Frame3Scaled is a Frame3 with a float scale
*/

#pragma once

#include "Frame3.h"

START_CB

//}{=======================================================================

/*!

Frame3Scaled is a Frame3 with a float scale

This is a full "TRS" transform, and the order of operations is

S (Scale) first
R (Rotate) second
T (Translate) third

in matrix order, this is "TRS" , since the right-most matrix is applied first

*/

class Frame3Scaled
{
public:
	enum EConstructorIdentity	{ eIdentity };
	enum EConstructorZero		{ eZero };

	//----------------------------------------------
	//  (see note in Mat3.cpp about static construction warning)

	__forceinline  Frame3Scaled()
	{
		// just let Vec3 constructors do their work
	}

	// important : do NOT declare copy constructors which are the default !
	//	VC needs you to not declare them for it to do full return-value optimization!
	// default operator = is fine

	explicit __forceinline Frame3Scaled(const Frame3 & f,const float s = 1.f)
		: m_frame(f) , m_scale(s)
	{
	}

	explicit __forceinline Frame3Scaled(const Mat3 & rotation,const Vec3 & translation,const float scale = 1.f)
		: m_frame(rotation,translation) , m_scale(scale)
	{
	}

	explicit __forceinline Frame3Scaled(const Vec3 & translation,const float scale = 1.f)
		: m_frame(translation) , m_scale(scale)
	{
	}
	
	//! use like this : Frame3Scaled(Frame3Scaled::eIdentity);
	//!	(quite intentionally not using Vec3::unitX, etc. here)
	explicit __forceinline Frame3Scaled(const EConstructorIdentity e)
		: m_frame(Frame3::eIdentity) , m_scale(1.f)
	{
	}

	explicit __forceinline Frame3Scaled(const EConstructorZero e)
		: m_frame(Frame3::eZero) , m_scale(0.f)
	{
	}

	//----------------------------------------------

	static const Frame3Scaled identity;
	static const Frame3Scaled zero;

	void SetIdentity();

	static bool Equals(const Frame3Scaled &a,const Frame3Scaled &b,const float tolerance = EPSILON);

	//----------------------------------------------
	//!  basic accessors

	void SetFrame(const Frame3 & m);
	const Frame3 & GetFrame() const;
	Frame3 & MutableFrame();

	void SetMatrix(const Mat3 & m);
	const Mat3 & GetMatrix() const;
	Mat3 & MutableMatrix();

	void SetTranslation(const Vec3 & m);
	const Vec3 & GetTranslation() const;
	Vec3 & MutableTranslation();

	void SetScale(const float m);
	float GetScale() const;
	float & MutableScale();

	//----------------------------------------------
	//! Transform a vector :

	//! Rotate and Translate and Scale "fm" to make result
	const Vec3 Transform(const Vec3 & fm) const;

	//! Rotate and Translate and Scale "fm" to make result
	//!	If the matrix is orthonormal, then this willl undo what Transform() does
	const Vec3 TransformTranspose(const Vec3 & fm) const;

	//! Rotation alone; no translate or scale.
	const Vec3	Rotate(const Vec3& fm) const;

	//----------------------------------------------

	bool IsValid() const;
	bool IsIdentity(const float tolerance = EPSILON) const;
	bool IsOrthonormal(const float tolerance = EPSILON_NORMALS) const;
	bool IsOrthogonal(const float tolerance = EPSILON_NORMALS) const;

	//----------------------------------------------
	//! Building the Transform

	//! SetMultiple : this = m1 * m2
	//! m1 can be this , m2 cannot !
	void SetProduct(const Frame3Scaled &m1,const Frame3Scaled &m2);
	void LeftMultiply(const Frame3Scaled &m);
	void RightMultiply(const Frame3Scaled &m);

	void SetProduct(const Frame3 &m1,const Frame3Scaled &m2);
	void SetProduct(const Frame3Scaled &m1,const Frame3 &m2);
	void LeftMultiply(const Frame3 &m);
	void RightMultiply(const Frame3 &m);
	
	void SetInverse(const Frame3Scaled &m);
	void SetTranspose(const Frame3Scaled &m);

	//----------------------------------------------
	// Utility-ish stuff.

	void SetLerped(const Frame3Scaled& from, const Frame3Scaled& to, const float t);

	// Make a transform that sits at the given position, with its up
	// (i.e. z) vector aligned in the specified direction.  The other
	// axes are arbitrary.
	void SetOrientedToSurface(const Vec3& position, const Vec3& up);

private:

	Frame3 m_frame;
	float m_scale;
};

//}{=======================================================================

inline void Frame3Scaled::SetIdentity()
{
	m_frame.SetIdentity();
	m_scale = 1.f;
	ASSERT(IsValid());
}

inline void Frame3Scaled::SetFrame(const Frame3 & m)
{
	ASSERT(m.IsValid());
	m_frame = m;
}

inline const Frame3 & Frame3Scaled::GetFrame() const
{
	ASSERT(IsValid());
	return m_frame;
}

inline Frame3 & Frame3Scaled::MutableFrame()
{
	// may not be valid at this point
	///ASSERT(IsValid());
	return m_frame;
}

inline void Frame3Scaled::SetMatrix(const Mat3 & m)
{
	ASSERT(m.IsValid());
	m_frame.SetMatrix(m);
}

inline const Mat3 & Frame3Scaled::GetMatrix() const
{
	ASSERT(IsValid());
	return m_frame.GetMatrix();
}

inline Mat3 & Frame3Scaled::MutableMatrix()
{
	// may not be valid at this point
	///ASSERT(IsValid());
	return m_frame.MutableMatrix();
}

inline void Frame3Scaled::SetTranslation(const Vec3 & m)
{
	ASSERT(m.IsValid());
	m_frame.SetTranslation(m);
}

inline const Vec3 & Frame3Scaled::GetTranslation() const
{
	ASSERT(IsValid());
	return m_frame.GetTranslation();
}

inline Vec3 & Frame3Scaled::MutableTranslation()
{
	// may not be valid at this point
	///ASSERT(IsValid());
	return m_frame.MutableTranslation();
}

inline void Frame3Scaled::SetScale(const float s)
{
	ASSERT( fisvalid(s) );
	m_scale = s;
}

inline float Frame3Scaled::GetScale() const
{
	ASSERT(IsValid());
	return m_scale;
}

inline float & Frame3Scaled::MutableScale()
{
	// may not be valid at this point
	///ASSERT(IsValid());
	return m_scale;
}

//----------------------------------------------

//! Rotate and Translate "fm" to make result
inline const Vec3 Frame3Scaled::Transform(const Vec3 & fm) const
{
	ASSERT( IsValid() && fm.IsValid() );
	// @@ does this optimize well enough?
	return m_frame.Transform(fm * m_scale);
}

//! Rotate and Translate "fm" to make result
//!	If the matrix is orthonormal, then this willl undo what Transform() does
inline const Vec3 Frame3Scaled::TransformTranspose(const Vec3 & fm) const
{
	ASSERT( IsValid() && fm.IsValid() );
	ASSERT( ! fiszero(m_scale) );
	return m_frame.TransformTranspose(fm) / m_scale;
}

//! Transform * Vector mathematical operator product; makes a Vec3
__forceinline const Vec3 operator * (const Frame3Scaled & xf,const Vec3 & v)
{
	return xf.Transform(v);
}


inline const Vec3	Frame3Scaled::Rotate(const Vec3& fm) const
{
	return m_frame.GetMatrix().Rotate(fm);
}


//----------------------------------------------
//! Building the Transform

inline void Frame3Scaled::SetProduct(const Frame3Scaled &m1,const Frame3Scaled &m2)
{
	// m1 can = this !
	ASSERT( &m2 != this );

	// first set T, since S and R don't use it
	MutableTranslation() = m1 * m2.GetTranslation();

	// then S and R are just simple products
	m_scale = m1.GetScale() * m2.GetScale();

	MutableMatrix().SetProduct(m1.GetMatrix(),m2.GetMatrix());
}

inline void Frame3Scaled::LeftMultiply(const Frame3Scaled &m)
{
	ASSERT( m.IsValid() && IsValid() );
	Frame3Scaled t = *this;
	SetProduct(m,t);
}

inline void Frame3Scaled::RightMultiply(const Frame3Scaled &m)
{
	ASSERT( m.IsValid() && IsValid() );
	//Frame3Scaled t = *this;
	//SetProduct(t,m);
	SetProduct(*this,m);
}

inline void Frame3Scaled::SetProduct(const Frame3 &m1,const Frame3Scaled &m2)
{
	// m1 can = this !
	ASSERT( &m2 != this );

	// first set T, since S and R don't use it
	MutableTranslation() = m1 * m2.GetTranslation();

	// then S and R are just simple products
	m_scale = m2.GetScale();

	MutableMatrix().SetProduct(m1.GetMatrix(),m2.GetMatrix());
}

inline void Frame3Scaled::SetProduct(const Frame3Scaled &m1,const Frame3 &m2)
{
	// m1 can = this !
	ASSERT( &m2 != &GetFrame() );

	// first set T, since S and R don't use it
	MutableTranslation() = m1 * m2.GetTranslation();

	// then S and R are just simple products
	m_scale = m1.GetScale();

	MutableMatrix().SetProduct(m1.GetMatrix(),m2.GetMatrix());
}

inline void Frame3Scaled::LeftMultiply(const Frame3 &m)
{
	ASSERT( m.IsValid() && IsValid() );
	Frame3Scaled t = *this;
	SetProduct(m,t);
}

inline void Frame3Scaled::RightMultiply(const Frame3 &m)
{
	ASSERT( m.IsValid() && IsValid() );
	//Frame3Scaled t = *this;
	//SetProduct(t,m);
	SetProduct(*this,m);
}

//}{=======================================================================

END_CB

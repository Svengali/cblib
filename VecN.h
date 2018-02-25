#pragma once

#include "cblib/Base.h"
#include "cblib/FloatUtil.h"
#include "cblib/Vec2.h"
#include "cblib/Vec3.h"

START_CB

//{===========================================================================================

//! N-Space Vector
template <int t_size> class VecN
{
public:
	enum { c_size = t_size };
	typedef VecN<t_size> this_type;

	//-------------------------------------------------------------------

	__forceinline VecN()
	{
		//! do-nada constructor; 
		for(int i=0;i<c_size;i++)
			finvalidatedbg(m_data[i]);
	}
	
	// important : do NOT declare copy constructors which are the default !
	//	VC needs you to not declare them for it to do full return-value optimization!
	// default operator = is fine

	//-------------------------------------------------------------------
	// math operators on vectors

	void operator *= (const float scale);
	void operator /= (const float scale);
	void operator += (const this_type & v);
	void operator -= (const this_type & v);
	bool operator == (const this_type & v) const; //!< exact equality test
	bool operator != (const this_type & v) const; //!< exact equality test

	//! array style access :
	float   operator [](const int c) const;
	float & operator [](const int c);
	
	//! access chunks as a Vec3 :

	const Vec3 & GetVec3(const int c) const;
	Vec3 &	MutableVec3(const int c);
	const Vec2 & GetVec2(const int c) const;
	Vec2 &	MutableVec2(const int c);

	//-------------------------------------------------------------------
	
	bool IsValid() const;
	bool IsNormalized(const float tolerance = EPSILON) const;

	float LengthSqr() const;
	float Length() const;
	
	//! Normalize :
	//!	returns the length
	/*! if length is less than EPSILON, returns fallback and length of 0 */
	float Normalize(const float tolerance = EPSILON);
	
	//-------------------------------------------------------------------
	// Set mutators

	//! this = zero
	void SetZero();
	//! all elements = f
	void SetAllElements(const float f);
	//! this = abs(v) ; component-wise
	void SetAbs(const this_type &v);
	//! this = min(this,v) ; component-wise
	void SetMin(const this_type &v);
	//! this = min(this,v) ; component-wise
	void SetMax(const this_type &v);
	//! this.x *= a.x; etc.
	void ComponentwiseScale(const this_type &a);

	void SetScaled(const this_type &v,const float scale);
	void AddScaled(const this_type &v,const float scale);

	//-------------------------------------------------------------------
	
	//! fuzzy equality test
	static bool Equals(const this_type &a,const this_type &b,const float tolerance = EPSILON);

	//-------------------------------------------------------------------
	// just IO as bytes

private:
	float	m_data[c_size];

}; // end of class VecN

//}{===========================================================================================

#define T_PRE1	template <int t_size>
#define T_THIS_TYPE	VecN<t_size>
#define T_PRE2	T_THIS_TYPE

T_PRE1 inline bool T_PRE2::IsValid() const
{
	for(int i=0;i<c_size;i++)
		ASSERT( fisvalid(m_data[i]) );
	return true;
}

//-------------------------------------------------------------------
// math operators on vectors

T_PRE1 inline void T_PRE2::operator *= (const float scale)
{
	ASSERT( IsValid() );
	for(int i=0;i<c_size;i++)
		m_data[i] *= scale;
	ASSERT( IsValid() );
}

T_PRE1 inline void T_PRE2::operator /= (const float scale)
{
	ASSERT( scale != 0.f );
	ASSERT( IsValid() );
	const float inv = 1.f / scale;
	for(int i=0;i<c_size;i++)
		m_data[i] *= inv;
	ASSERT( IsValid() );
}

T_PRE1 inline void T_PRE2::operator += (const T_THIS_TYPE & v)
{
	ASSERT( IsValid() && v.IsValid() );
	for(int i=0;i<c_size;i++)
		m_data[i] += v.m_data[i];
	ASSERT( IsValid() );
}

T_PRE1 inline void T_PRE2::operator -= (const T_THIS_TYPE & v)
{
	ASSERT( IsValid() && v.IsValid() );
	for(int i=0;i<c_size;i++)
		m_data[i] -= v.m_data[i];
	ASSERT( IsValid() );
}

T_PRE1 inline bool T_PRE2::operator == (const T_THIS_TYPE & v) const
{
	ASSERT( IsValid() && v.IsValid() );
	for(int i=0;i<c_size;i++)
		if ( ! m_data[i] == v.m_data[i] )
			return false;
	return true;
}

T_PRE1 inline bool T_PRE2::operator != (const T_THIS_TYPE & v) const
{
	ASSERT( IsValid() && v.IsValid() );
	return !operator==(v);
}

//! array style access :
T_PRE1 inline float T_PRE2::operator [](const int c) const
{
	ASSERT( c >= 0 && c < c_size );
	ASSERT( IsValid() );
	return m_data[c];
}

T_PRE1 inline float & T_PRE2::operator [](const int c)
{
	ASSERT( c >= 0 && c < c_size );
	// may not be valid yet
	//ASSERT( IsValid() );
	return m_data[c];
}

T_PRE1 inline const Vec3 & T_PRE2::GetVec3(const int c) const
{
	ASSERT( c >= 0 && (c+3) <= c_size );
	ASSERT( IsValid() );
	return *((const Vec3 *)(m_data+c));
}

T_PRE1 inline Vec3 & T_PRE2::MutableVec3(const int c)
{
	ASSERT( c >= 0 && (c+3) <= c_size );
	// may not be valid yet
	//ASSERT( IsValid() );
	return *((Vec3 *)(m_data+c));
}

T_PRE1 inline const Vec2 & T_PRE2::GetVec2(const int c) const
{
	ASSERT( c >= 0 && (c+2) <= c_size );
	ASSERT( IsValid() );
	return *((const Vec2 *)(m_data+c));
}

T_PRE1 inline Vec2 & T_PRE2::MutableVec2(const int c)
{
	ASSERT( c >= 0 && (c+2) <= c_size );
	// may not be valid yet
	//ASSERT( IsValid() );
	return *((Vec2 *)(m_data+c));
}

//-------------------------------------------------------------------

T_PRE1 inline float T_PRE2::LengthSqr() const
{
	ASSERT( IsValid() );
	float ret = 0.f;
	for(int i=0;i<c_size;i++)
		ret += fsquare( m_data[i] );
	return ret;
}

T_PRE1 inline float T_PRE2::Length() const
{
	return sqrtf( LengthSqr() );
}

//! NormalizeSafe :
//!	returns the length
/*! if length is less than EPSILON, returns fallback and length of 0 */
T_PRE1 inline float T_PRE2::Normalize(const float tolerance) 
{
	const float length = Length();
	if ( length < tolerance )
	{
		SetZero();
		return 0.f;
	}
	else
	{
		(*this) *= 1.f / length;
		ASSERT(IsNormalized());
		return length;
	}
}

T_PRE1 inline bool T_PRE2::IsNormalized(const float tolerance /*= EPSILON*/) const
{
	// @@ part of the EPSILON disaster
	ASSERT( IsValid() );
	return fisone( LengthSqr(), /*fsquare*/ (tolerance) );
}

//-------------------------------------------------------------------
// Set mutators

T_PRE1 inline void T_PRE2::SetZero()
{
	for(int i=0;i<c_size;i++)
		m_data[i] = 0.f;
}
T_PRE1 inline void T_PRE2::SetAbs(const T_THIS_TYPE &v)
{
	ASSERT( v.IsValid() && IsValid() );
	for(int i=0;i<c_size;i++)
		m_data[i] = fabsf(v.m_data[i]);
	ASSERT( IsValid() );
}
T_PRE1 inline void T_PRE2::SetMin(const T_THIS_TYPE &v)
{
	ASSERT( v.IsValid() && IsValid() );
	for(int i=0;i<c_size;i++)
		m_data[i] = min(m_data[i],v.m_data[i]);
	ASSERT( IsValid() );
}
T_PRE1 inline void T_PRE2::SetMax(const T_THIS_TYPE &v)
{
	ASSERT( v.IsValid() && IsValid() );
	for(int i=0;i<c_size;i++)
		m_data[i] = max(m_data[i],v.m_data[i]);
	ASSERT( IsValid() );
}

T_PRE1 inline void T_PRE2::ComponentwiseScale(const T_THIS_TYPE &a)
{
	ASSERT( a.IsValid() && IsValid() );
	for(int i=0;i<c_size;i++)
		m_data[i] *= a.m_data[i];
	ASSERT( IsValid() );
}

T_PRE1 inline void T_PRE2::SetScaled(const T_THIS_TYPE &v,const float scale)
{
	for(int i=0;i<c_size;i++)
		m_data[i] = v.m_data[i] * scale;
	ASSERT( IsValid() );
}

T_PRE1 inline void T_PRE2::AddScaled(const T_THIS_TYPE &v,const float scale)
{
	for(int i=0;i<c_size;i++)
		m_data[i] += v.m_data[i] * scale;
	ASSERT( IsValid() );
}

//-------------------------------------------------------------------

/*static*/ T_PRE1 inline bool T_PRE2::Equals(const T_THIS_TYPE &a,const T_THIS_TYPE &b,const float tolerance /*= EPSILON*/)
{
	ASSERT( a.IsValid() && b.IsValid() );
	for(int i=0;i<c_size;i++)
		if ( ! fequal(m_data[i],v.m_data[i],tolerance) )
			return false;
	return true;
}

//}{===========================================================================================
// Math operators
//
// one of the many dangerous things about binary operators is
//	order of evaluation.  Check this out :
//		float = - vec1 * Vec2
// what does that do?  Actually, in C++, it invokes unary operator-
//	which makes a temporary vector and then does the dot (rather than
//	do the dot and then invoke operator - on a float).  ugly.  Of course
//	this is even much worse with things like :
//		vec = - Mat * Mat * vec
//	This line could be done like this :
//		vec = - ( Mat * (Mat * vec) )
//	with ideal efficiency
//	but standard C++ will evaluate like this :
//		vec = ((- Mat) * Mat) * vec
//	with way-below-best efficiency

T_PRE1 __forceinline float operator * (const T_THIS_TYPE & u,const T_THIS_TYPE & v) // dot
{
	ASSERT( u.IsValid() && v.IsValid() );
	float ret = 0.f;
	for(int i=0;i<t_size;i++)
		ret += u[i] * v[i];
	return ret;
}

T_PRE1 __forceinline const T_THIS_TYPE operator * (const float f,const T_THIS_TYPE & v) // scale
{
	ASSERT( fisvalid(f) && v.IsValid() );
	T_THIS_TYPE temp(v);
	temp *= f;
	return temp;
}

T_PRE1 __forceinline const T_THIS_TYPE operator * (const T_THIS_TYPE & v,const float f) // scale
{
	ASSERT( fisvalid(f) && v.IsValid() );
	T_THIS_TYPE temp(v);
	temp *= f;
	return temp;
}

T_PRE1 __forceinline const T_THIS_TYPE operator / (const T_THIS_TYPE & v,const float f) // scale
{
	ASSERT( fisvalid(f) && v.IsValid() );
	ASSERT( f != 0.f );
	T_THIS_TYPE temp(v);
	temp /= f;
	return temp;
}

T_PRE1 __forceinline const T_THIS_TYPE operator + (const T_THIS_TYPE & u,const T_THIS_TYPE & v) // add
{
	ASSERT( u.IsValid() && v.IsValid() );
	T_THIS_TYPE temp(u);
	temp += v;
	return temp;
}

T_PRE1 __forceinline const T_THIS_TYPE operator - (const T_THIS_TYPE & u,const T_THIS_TYPE & v) // subtract
{
	ASSERT( u.IsValid() && v.IsValid() );
	T_THIS_TYPE temp(u);
	temp -= v;
	return temp;
}

// VecNU for similarity with Vec3U
namespace VecNU
{

T_PRE1 inline float DistanceSqr(const T_THIS_TYPE & a,const T_THIS_TYPE &b)
{
	ASSERT( a.IsValid() && b.IsValid() );
	float ret = 0.f;
	for(int i=0;i<t_size;i++)
		ret += fsquare( a[i] - b[i] );
	return ret;
}

T_PRE1 inline float Distance(const T_THIS_TYPE & a,const T_THIS_TYPE &b)
{
	return sqrtf( DistanceSqr(a,b) );
}

};

#undef T_PRE1
#undef T_THIS_TYPE
#undef T_PRE2

//}===========================================================================================

END_CB

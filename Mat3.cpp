#include "Base.h"
#include "Mat3.h"
#include "Vec3U.h"

START_CB


//////////////////////////////////////////
// NOTEZ : IMPORTANT WARNING
//	the order of initialization of static class variables is
//		undefined !!
//	that means you CANNOT use things like Vec3::zero to initialize here !!

const Mat3 Mat3::identity(Mat3::eIdentity);
const Mat3 Mat3::zero(Mat3::eZero);


//--------------------------------------------------------------------------------

/*
void 
Mat3::IO(const StreamPtr& str)
{
//	m_x.IO(str);
//	m_y.IO(str);
//	m_z.IO(str);
	StreamUtil::IO_POD(str, this);
	ASSERT(IsValid());
}
*/

//--------------------------------------------------------------------------------

bool Mat3::IsValid() const
{
	ASSERT( m_x.IsValid() && m_y.IsValid() && m_z.IsValid() );
	return true;
}

bool Mat3::IsUniformScale(const float tolerance /*= EPSILON*/) const
{
	ASSERT(IsValid());
	ASSERT(IsOrthogonal(tolerance));
	const float lenX = m_x.LengthSqr();
	const float lenY = m_y.LengthSqr();
	const float lenZ = m_z.LengthSqr();
	
	return	fequal(lenX,lenY,tolerance) &&
			fequal(lenY,lenZ,tolerance) &&
			fequal(lenX,lenZ,tolerance);
}

bool Mat3::IsOrthonormal(const float tolerance /*= EPSILON*/) const
{
	ASSERT(IsValid());
 	const float det = GetDeterminant();
	return fisone(det,tolerance) && 
		IsOrthogonal(tolerance) && 
		fisone(m_x.LengthSqr(), tolerance/*tolerance*/) && 
		fisone(m_y.LengthSqr(), tolerance/*tolerance*/) &&
		fisone(m_z.LengthSqr(), tolerance/*tolerance*/);
}

float Mat3::GetDeterminant() const
{
	ASSERT( IsValid() );
	//return 
	return TripleProduct(m_x,m_y,m_z);
}

void Mat3::SetProduct(const Mat3 &m1,const Mat3 &m2)
{
	// m1 can = this !
	ASSERT( &m2 != this );
	ASSERT( m1.IsValid() && m2.IsValid() );
	
	// Mat3 multiplication :

	m_x =	m2.m_x * m1.m_x.x +
			m2.m_y * m1.m_x.y +
			m2.m_z * m1.m_x.z;

	m_y =	m2.m_x * m1.m_y.x +
			m2.m_y * m1.m_y.y +
			m2.m_z * m1.m_y.z;

	m_z =	m2.m_x * m1.m_z.x +
			m2.m_y * m1.m_z.y +
			m2.m_z * m1.m_z.z;

	ASSERT(IsValid());
}

void Mat3::SetAverage(const Mat3 &v1,const Mat3 &v2)
{
	ASSERT( v1.IsValid() && v2.IsValid() );
	m_x = MakeAverage(v1.m_x,v2.m_x);
	m_y = MakeAverage(v1.m_y,v2.m_y);
	m_z = MakeAverage(v1.m_z,v2.m_z);
	ASSERT(IsValid());
}

void Mat3::SetLerp(const Mat3 &v1,const Mat3 &v2,const float t)
{
	ASSERT( v1.IsValid() && v2.IsValid() && fisvalid(t) );
	m_x = MakeLerp(v1.m_x,v2.m_x,t);
	m_y = MakeLerp(v1.m_y,v2.m_y,t);
	m_z = MakeLerp(v1.m_z,v2.m_z,t);
	ASSERT(IsValid());
}

//! fuzzy equality test
/*static*/ bool Mat3::Equals(const Mat3 &a,const Mat3 &b,const float tolerance /*= EPSILON*/)
{
	return 
		Vec3::Equals(a.m_x,b.m_x,tolerance) && 
		Vec3::Equals(a.m_y,b.m_y,tolerance) && 
		Vec3::Equals(a.m_z,b.m_z,tolerance);
}

//--------------------------------------------------------------------------------

END_CB

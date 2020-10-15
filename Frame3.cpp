#include "Base.h"
#include "Frame3.h"
#include "Vec3U.h"

START_CB

//////////////////////////////////////////
// NOTEZ : IMPORTANT WARNING
//	the order of initialization of static class variables is
//		undefined !!
//	that means you CANNOT use things like Vec3::zero to initialize here !!

const Frame3 Frame3::identity(Frame3::eIdentity);
const Frame3 Frame3::zero(Frame3::eZero);

//--------------------------------------------------------------------------------

/*static*/ bool Frame3::Equals(const Frame3 &a,const Frame3 &b,const float tolerance /*= EPSILON*/)
{
	return	Mat3::Equals( a.GetMatrix(), b.GetMatrix(), tolerance) &&
			Vec3::Equals( a.GetTranslation(), b.GetTranslation(), tolerance );
}

//--------------------------------------------------------------------------------

bool Frame3::IsValid() const
{
	ASSERT( m_mat.IsValid() && m_t.IsValid() );
	return true;
}

bool Frame3::IsIdentity(const float tolerance /*= EPSILON*/) const
{
	return ( m_mat.IsIdentity(tolerance) && Vec3::Equals(m_t,Vec3::zero,tolerance) );
}

//--------------------------------------------------------------------------------

void Frame3::SetAverage(const Frame3 &v1,const Frame3 &v2)
{
	ASSERT( v1.IsValid() && v2.IsValid() );
	m_mat.SetAverage(v1.GetMatrix(),v2.GetMatrix());
	m_t = MakeAverage(v1.m_t,v2.m_t);
}

void Frame3::SetLerp(const Frame3 &v1,const Frame3 &v2,const float t)
{
	ASSERT( v1.IsValid() && v2.IsValid() && fisvalid(t) );
	m_mat.SetLerp(v1.GetMatrix(),v2.GetMatrix(),t);
	m_t = MakeLerp(v1.m_t,v2.m_t,t);
}

//-------------------------------------------------------------------

#pragma warning(disable : 4505) // unreferenced static removed

static void Frame3Test()
{
	Frame3 xf(Frame3::eIdentity);

	Vec3 v = xf.GetMatrix().GetRowX();

	xf.MutableMatrix().SetRowY(v);
	xf.MutableMatrix().SetRowZ(0,1,2);

}

//-------------------------------------------------------------------

END_CB

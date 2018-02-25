#include "cblib/Base.h"
#include "cblib/Frame3Scaled.h"
#include "cblib/Vec3U.h"
#include "cblib/Mat3Util.h"
#include "cblib/Frame3Util.h"

START_CB


//////////////////////////////////////////
// NOTEZ : IMPORTANT WARNING
//	the order of initialization of static class variables is
//		undefined !!
//	that means you CANNOT use things like Vec3::zero to initialize here !!

const Frame3Scaled Frame3Scaled::identity(Frame3Scaled::eIdentity);
const Frame3Scaled Frame3Scaled::zero(Frame3Scaled::eZero);

//--------------------------------------------------------------------------------

/*static*/ bool Frame3Scaled::Equals(const Frame3Scaled &a,const Frame3Scaled &b,const float tolerance /*= EPSILON*/)
{
	return	Frame3::Equals( a.GetFrame(), b.GetFrame(), tolerance) &&
			fequal( a.GetScale(), b.GetScale(), tolerance );
}

//--------------------------------------------------------------------------------

bool Frame3Scaled::IsValid() const
{
	ASSERT( m_frame.IsValid() && fisvalid(m_scale) );
	return true;
}

bool Frame3Scaled::IsIdentity(const float tolerance /*= EPSILON*/) const
{
	return ( m_frame.IsIdentity(tolerance) && fisone(m_scale,tolerance) );
}

bool Frame3Scaled::IsOrthonormal(const float tolerance/* = EPSILON*/) const
{
	return ( fisone(m_scale,tolerance) && m_frame.GetMatrix().IsOrthonormal(tolerance) );
}

bool Frame3Scaled::IsOrthogonal(const float tolerance/* = EPSILON*/) const
{
	return m_frame.GetMatrix().IsOrthogonal(tolerance);
}

//-------------------------------------------------------------------

void Frame3Scaled::SetInverse(const Frame3Scaled &m)
{
	ASSERT( &m != this );
	ASSERT( ! fiszero(m.GetScale()) );

	GetInverse( m.GetMatrix(), & MutableMatrix() );
	m_scale = 1.f/ m.GetScale();

	// I'm using my new inverted scale and matrix here :
	//	use MutableMatrix just because we're not yet totally valid
	MutableTranslation() = MutableMatrix() * ( (- m_scale) * m.GetTranslation() );

	#ifdef DO_ASSERTS
	{
		Frame3Scaled product;
		product.SetProduct(*this,m);
		ASSERT( product.IsIdentity() );
	}
	#endif // DO_ASSERTS
}

void Frame3Scaled::SetTranspose(const Frame3Scaled &m)
{
	ASSERT( &m != this );
	ASSERT( ! fiszero(m.GetScale()) );

	GetTranspose( m.GetMatrix(), & MutableMatrix() );
	m_scale = 1.f/ m.GetScale();

	// I'm using my new inverted scale and matrix here :
	//	use MutableMatrix just because we're not yet totally valid
	MutableTranslation() = MutableMatrix() * ( (- m_scale) * m.GetTranslation() );

	#ifdef DO_ASSERTS
	{
		Frame3Scaled product;
		product.SetProduct(*this,m);
		ASSERT( product.IsIdentity() );
	}
	#endif // DO_ASSERTS
}

void Frame3Scaled::SetLerped(const Frame3Scaled& from, const Frame3Scaled& to, const float t)
{
	ASSERT( fiszerotoone(t,0.f) );

	m_scale = flerp(from.GetScale(), to.GetScale(), t);

	cb::SetLerped(&m_frame,from.m_frame,to.m_frame,t);
}

//-------------------------------------------------------------------


void	Frame3Scaled::SetOrientedToSurface(const Vec3& position, const Vec3& up)
{
	SetScale(1.0f);
	SetTranslation(position);
	cb::SetOrientedToSurface(&MutableMatrix(), up);
}

//-------------------------------------------------------------------

#pragma warning(disable : 4505) // unreferenced static removed

static void Frame3ScaledTest()
{
	Frame3Scaled xf(Frame3Scaled::eIdentity);

	Vec3 v = xf.GetMatrix().GetRowX();

	xf.MutableMatrix().SetRowY(v);
	xf.MutableMatrix().SetRowZ(0,1,2);

}

END_CB

#include "cblib/Base.h"
#include "cblib/Frame3Util.h"

START_CB

//namespace Frame3Util
//{

void GetInverse(const Frame3 & m,Frame3 * pInv)
{
	GetInverse( m.GetMatrix() , &(pInv->MutableMatrix()) );

	pInv->SetTranslation( pInv->Rotate( m.GetTranslation() ) );
	pInv->MutableTranslation().Invert();
}

void GetTranspose(const Frame3 & m,Frame3 * pTrans)
{
	GetTranspose( m.GetMatrix() , &(pTrans->MutableMatrix()) );

	pTrans->SetTranslation( pTrans->Rotate( m.GetTranslation() ) );
	pTrans->MutableTranslation().Invert();
}

// make it Orthonormal
void GetOrthonormalized(const Frame3 & m,Frame3 * pOrthoNorm)
{
	GetOrthonormalized( m.GetMatrix() , &(pOrthoNorm->MutableMatrix()) );

	// and copy along the translation :
	pOrthoNorm->SetTranslation( m.GetTranslation() );
}

/** Align our z-axis with the given normal; other axes are
	arbitrary. */
void	SetOrientedToSurface(Frame3 * pFrame, const Vec3& up, const Vec3 & pos)
{
	SetOrientedToSurface( &(pFrame->MutableMatrix()), up );
	pFrame->SetTranslation(pos);
}
	
//! MakeFrameFromThreePoints makes an *orthonormal* basis from 3 points
bool MakeFrameFromThreePoints(Frame3 * pFrame,const Vec3 &a, const Vec3 &b,const Vec3 &c)
{
	Vec3 e1 = b-a;
	Vec3 e2 = c-a;
	Vec3 e3;
	e3.SetCross(e1,e2);
	e2.SetCross(e3,e1);

	if ( e1.NormalizeSafe() == 0.f )
		return false;
	if ( e2.NormalizeSafe() == 0.f )
		return false;
	if ( e3.NormalizeSafe() == 0.f )
		return false;

	pFrame->SetColumnX(e1);
	pFrame->SetColumnY(e2);
	pFrame->SetColumnZ(e3);
	
	const Vec3 center = (a+b+c) * (1.f/3.f);

	pFrame->SetTranslation(center);

	ASSERT( pFrame->IsValid() );

	return true;
}

//! MakeBarycentricFrame makes the Frame which takes a point from "barycentric space"
//!	 to world space.
//!	In particular, a coordinate of the form {b0,b1,n} is taken to the world-space
//!	 determined by the triangle points (a,b,c)
//! *this frame is NOT orthonormal* !!
//!	The inverse frame takes points from world space into "barycentric space"
bool MakeBarycentricFrame(Frame3 * pFrame,const Vec3 &a,const Vec3 &b,const Vec3 &c)
{
	Vec3 normal;
	if ( SetTriangleNormal(&normal,a,b,c) == 0.f )
		return false;

	const Vec3 e1 = a - c;
	const Vec3 e2 = b - c;

	pFrame->SetColumnX(e1);
	pFrame->SetColumnY(e2);
	pFrame->SetColumnZ(normal);
	pFrame->SetTranslation(c);

	#ifdef DO_ASSERTS
	{
		Frame3 toBary;
		GetInverse(*pFrame,&toBary);
		Vec3 b0 = toBary * a;
		Vec3 b1 = toBary * b;
		Vec3 b2 = toBary * c;
		ASSERT( Vec3::Equals(b0,Vec3::unitX,0.001f) );
		ASSERT( Vec3::Equals(b1,Vec3::unitY,0.001f) );
		ASSERT( Vec3::Equals(b2,Vec3::zero ,0.001f) );
	}
	#endif

	return true;
}

void SetLookAt(Frame3 * pFrame,const Vec3 & fm,const Vec3 & to,
			   const Vec3 & up /*= Vec3::unitZ*/)
{
	ASSERT( ! Vec3::Equals(fm,to) );
	// position is at "fm"
	pFrame->SetTranslation( fm );
	
	Vec3 viewIn = to - fm;
	if ( viewIn.NormalizeSafe(Vec3::unitX) == 0.f )
	{
		FAIL("Degenerate vector in LookAt!");
		// use the fallback unitX, sort of appropriate for our
		//	cameras with unitZ up
	}
	
	SetLookDirection(&pFrame->MutableMatrix(),viewIn,up);
}

void SetLerped(Frame3 * result, const Frame3& from, const Frame3& to, const float t)
{
	ASSERT( fiszerotoone(t,0.f) );

	Vec3	t0, t1;
	t0 = from.GetTranslation();
	t1 = to.GetTranslation();

	result->MutableTranslation().SetLerp(t0, t1, t);
	SetLerped(&result->MutableMatrix(),from.GetMatrix(),to.GetMatrix(),t);
}

void MakeFrameFromPositionAndUpDir(Frame3 *frame,  const Vec3& pos, const Vec3& up)
{
	ASSERT(up.LengthSqr() > 0);

	Vec3 zDir(up);
	zDir.NormalizeSafe();

	Vec3 yDir;
	Vec3 xDir;
	if (fabs(zDir[0]) < fabs(zDir[1]))
	{
		if (fabs(zDir[0]) < fabs(zDir[2]))
		{
			// x is the shortest
			yDir = Vec3::unitX ^ zDir;
		}
		else
		{
			// z is the shortest
			yDir = Vec3::unitZ ^ zDir;
		}
	}
	else
	{
		if (fabs(zDir[1]) < fabs(zDir[2]))
		{
			// y is the shortest
			yDir = Vec3::unitY ^ zDir;
		}
		else
		{
			// z is the shortest
			yDir = Vec3::unitZ ^ zDir;
		}
	}
	yDir.NormalizeSafe();
	xDir = zDir ^ yDir;

	frame->MutableMatrix().SetColumnX(yDir);
	frame->MutableMatrix().SetColumnY(xDir);
	frame->MutableMatrix().SetColumnZ(zDir);

	ASSERT( frame->GetMatrix().IsOrthonormal() );
	frame->SetTranslation(pos);
}

//}; // Frame3Util

END_CB

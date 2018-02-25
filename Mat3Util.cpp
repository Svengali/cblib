#include "cblib/Base.h"
#include "cblib/Mat3Util.h"
#include "cblib/Rand.h"
#include "cblib/QuatUtil.h"
//#include "cblib/Basis.h"

START_CB

//namespace Mat3Util
//{

//--------------------------------------------------------------------------------

/*! get the transpose; can NOT work on self
	Transpose is a faster way to invert if it's orthonormal
*/
void GetTranspose(const Mat3 & m,Mat3 * pTrans)
{
	ASSERT( pTrans );
	ASSERT( pTrans != &m );
	ASSERT( m.IsValid() );

	pTrans->SetColumnX(m.GetRowX());
	pTrans->SetColumnY(m.GetRowY());
	pTrans->SetColumnZ(m.GetRowZ());
	
	ASSERT( pTrans->IsValid() );
}

/*! get the inverse; can work on self
	 NOTEZ : calling this on matrices which have
		no inverse will result in a divide by zero !!
	 first check if the Determinant is != 0 if you
	 want to be vareful
*/
void GetInverse(const Mat3 & m,Mat3 * pInv)
{
	ASSERT( pInv );
	ASSERT( m.IsValid() );
	
	// if my determinant is zero, I have no inverse :
	float det = m.GetDeterminant();
	ASSERT( fabsf(det) > 0.f );

	float invDet = 1.f / det;

    // get the columns :

	const Vec3 u = m.GetColumnX();
	const Vec3 v = m.GetColumnY();
	const Vec3 w = m.GetColumnZ();

    // RowU = (V x W) / ( U * (V x W)) , and so on

	// NOTEZ : calling this on matrices which have
	//	no inverse will result in a divide by zero !!
	//	(ASSERT above on Determinant should catch that)

	const Vec3 vw = MakeCross(v , w);
	pInv->SetRowX( vw * invDet );
	
	const Vec3 uw = MakeCross(w , u);
	pInv->SetRowY( uw * invDet );

	const Vec3 uv = MakeCross(u , v);
	pInv->SetRowZ( uv * invDet );

	ASSERT( pInv->IsValid() );

    #ifdef DO_ASSERTS // make sure it really is the inverse!
    {
	Mat3 product;
	product.SetProduct(m,*pInv);
	ASSERT(product.IsIdentity(0.005f)); // within EPSILON
    }
    #endif
}

//--------------------------------------------------------------------------------

//! make it Orthonormal; can work on self
void GetOrthonormalized(const Mat3 & m,Mat3 * pOrthoNorm)
{
	ASSERT( pOrthoNorm );
	ASSERT( m.IsValid() );

	Vec3 n = m.GetColumnY();
	Vec3 uv = m.GetColumnZ();

	n.NormalizeFast();
	uv.NormalizeFast();

	// v1 = vUV - ( vN * vUV ) * vN;
	// v is uv with all the 'n' part taken off
	Vec3 v = uv - ( n * uv ) * n;
	v.NormalizeFast();
	// now v dot n == small

	Vec3 u = MakeCross( n , v );
	// u.NormalizeFast(); ??

	pOrthoNorm->SetColumnX(u);
	pOrthoNorm->SetColumnY(n);
	pOrthoNorm->SetColumnZ(v);
	
	ASSERT( pOrthoNorm->IsOrthonormal() ); // within EPSILON
}

//--------------------------------------------------------------------------------

void SetRotation(Mat3 *pm,const Vec3 & axis,const float angle)
{
	ASSERT( axis.IsNormalized() && fisvalid(angle) && pm );

	// based on quaternion maths: 

	const float halfcos = cosf(angle * 0.5f);
	const float halfsin = sinf(angle * 0.5f);
	
	const float xsin = halfsin * axis.x;
	const float ysin = halfsin * axis.y;
	const float zsin = halfsin * axis.z;
	
	pm->RowX().x = 1.0f - 2.0f * ( ysin * ysin + zsin * zsin );
	pm->RowY().y = 1.0f - 2.0f * ( xsin * xsin + zsin * zsin );
	pm->RowZ().z = 1.0f - 2.0f * ( xsin * xsin + ysin * ysin );

	pm->RowX().y = 2.0f * ( xsin * ysin - halfcos * zsin );
	pm->RowX().z = 2.0f * ( xsin * zsin + halfcos * ysin );

	pm->RowY().x = 2.0f * ( xsin * ysin + halfcos * zsin );
	pm->RowY().z = 2.0f * ( ysin * zsin - halfcos * xsin );

	pm->RowZ().x = 2.0f * ( xsin * zsin - halfcos * ysin );
	pm->RowZ().y = 2.0f * ( ysin * zsin + halfcos * xsin );

	ASSERT( pm->IsOrthonormal(EPSILON*10.f) );

	#ifdef DO_ASSERTS //{

	// pretty hard sanity check,
	//	cuz I don't trust my maths:
	const Vec3 rotatedAxis = pm->Rotate(axis);
	const float dot = rotatedAxis * axis;
	ASSERT( dot >= 1.f - 2*EPSILON ); // should not rotate axis

	#endif //}
}

void SetXRotation(Mat3 *pm,const float angle)
{
	ASSERT(pm && fisvalid(angle));
	const float cosa = cosf(angle);
	const float sina = sinf(angle);

	pm->RowX().Set(    1,    0,    0 );
	pm->RowY().Set(    0, cosa,-sina );
	pm->RowZ().Set(    0, sina, cosa );
	
	ASSERT( pm->IsOrthonormal() );
}

void SetYRotation(Mat3 *pm,const float angle)
{
	ASSERT(pm && fisvalid(angle));
	const float cosa = cosf(angle);
	const float sina = sinf(angle);

	pm->RowX().Set( cosa,    0, sina );
	pm->RowY().Set(    0,    1,    0 );
	pm->RowZ().Set(-sina,    0, cosa );
	
	ASSERT( pm->IsOrthonormal() );
}

void SetZRotation(Mat3 *pm,const float angle)
{
	ASSERT(pm && fisvalid(angle));
	const float cosa = cosf(angle);
	const float sina = sinf(angle);

	pm->RowX().Set( cosa,-sina,    0 );
	pm->RowY().Set( sina, cosa,    0 );
	pm->RowZ().Set(    0,    0,    1 );

	ASSERT( pm->IsOrthonormal() );
}

//--------------------------------------------------------------------------------

void SetRandomRotation(Mat3 * pM)
{
	ASSERT(pM);
	Vec3 axis;
	SetRandomNormal(&axis);
	//const float angle = Rand::GetAngle();
	const float angle = frandunit() * CBPI;
	SetRotation(pM,axis,angle);
}

//--------------------------------------------------------------------------------

void GetEulerAnglesXYZ(const Mat3 & mat,Vec3 * pAngles)
{
	ASSERT( mat.IsOrthonormal() );
	ASSERT( pAngles != NULL );

	pAngles->y = asinf_safe( mat[0][2] );
	if ( pAngles->y < HALF_PI )
	{
		if ( pAngles->y > - HALF_PI )
		{
			pAngles->x = atan2f( - mat[1][2] , mat[2][2] );
			pAngles->z = atan2f( - mat[0][1] , mat[0][0] );
		}
		else
		{
			// degenerate
			pAngles->x = - atan2f( mat[1][0] , mat[1][1] );
			pAngles->z = 0.f;
		}
	}
	else
	{
		// degenerate
		pAngles->x = atan2f( mat[1][0] , mat[1][1] );
		pAngles->z = 0.f;
	}

	#ifdef DO_ASSERTS
	{
	// hard core check :
		Mat3 matRemade;
		SetFromEulerAnglesXYZ(&matRemade,*pAngles);
		ASSERT( Mat3::Equals(matRemade,mat) );
	}
	#endif // DO_ASSERTS
}

void SetFromEulerAnglesXYZ(Mat3 * pMat,const Vec3 & angles)
{
	//! \todo : could optimize by filling pMat directly
	
	//mat = RotX( angles.x ) * RotY( angles.y ) * RotZ( angle.z )

	ASSERT( pMat != NULL );

	SetZRotation(pMat, angles.z );
	LeftMultiplyYRotation( pMat, angles.y );
	LeftMultiplyXRotation( pMat, angles.x );
	
	ASSERT( pMat->IsOrthonormal() );

	// can't verify with GetEulerAnglesXYZ here since it uses us
	// couldn't do it anyway, since there are many euler angles
	//	that lead to the same rotation
}


/** Returns R^-1 * I * R */
void	SetRotatedTensor(Mat3* result, const Mat3& R, const Mat3& I)
{
	ASSERT(R.IsOrthonormal());

	GetTranspose(R, result);
	result->LeftMultiply(I);
	result->LeftMultiply(R);
}

void	SetLookDirectionNoUp(Mat3* result, const Vec3& dir)
{
	ASSERT(dir.IsNormalized());
	Vec3 viewLeft,viewUp;
	GetTwoPerpNormals(dir,&viewLeft,&viewUp);
	
	result->SetColumnX(-1.f * viewLeft);
	result->SetColumnY(viewUp);
	result->SetColumnZ(-1.f * dir);
	
	// make sure the handedness is right :
	if ( result->GetDeterminant() < 0.f )
	{
		// flip column X; any row or column would do
		result->SetColumnX(viewLeft);
		
		ASSERT( result->GetDeterminant() > 0.f );
	}
}
	
void	SetLookDirection(Mat3* result, const Vec3& dir, const Vec3& up)
{
	ASSERT(dir.IsNormalized());
	ASSERT(up.IsNormalized());
	#ifndef FINAL
	if ( !dir.IsNormalized() )
	{
		FAIL("Unnormalized direction vector in LookAt!");
		return; // do nada
	}
	#endif
	
	// try to make an Up that's close to "up"
	Vec3 viewLeft = MakeCross( up, dir);
	if ( viewLeft.NormalizeSafe() == 0.f )
	{
		// "fm-to" is along Z, so just bail out !
		//! \todo @@ this will be jerky; would be better to do some
		//	kind of smoother well controlled ease-in to this degenerate case
		Vec3 viewUp;
		GetTwoPerpNormals(dir,&viewLeft,&viewUp);
		
		result->SetColumnX(-1.f * viewLeft);
		result->SetColumnY(viewUp);
		result->SetColumnZ(-1.f * dir);
		
		// make sure the handedness is right :
		if ( result->GetDeterminant() < 0.f )
		{
			// flip column X; any row or column would do
			result->SetColumnX(viewLeft);
			
			ASSERT( result->GetDeterminant() > 0.f );
		}
	}
	else
	{
		Vec3 viewUp = MakeCross( dir, viewLeft );
		
		result->SetColumnX(-1.f * viewLeft);
		result->SetColumnY(viewUp);
		result->SetColumnZ(-1.f * dir);
		ASSERT( result->GetDeterminant() > 0.f );
	}
	
	ASSERT( result->IsOrthonormal() );
}

void SetLerped(Mat3* result, const Mat3& from, const Mat3& to, const float t)
{
	ASSERT( fiszerotoone(t,0.f) );

	Quat	q0, q1;
	q0.SetFromMatrix(from);
	q1.SetFromMatrix(to);

	Quat	qnext;
	SetSlerp(&qnext, q0, q1, t);
	qnext.GetMatrix(result);
	GetOrthonormalized(*result,result);	// avoid drifting out of orthonormal
}

//=====================================================================================

static void EigenSolver_Tridiagonal(Mat3 * pMat,Vec3 * diag,Vec3 * subd);
static bool EigenSolver_QLAlgorithm(Mat3 * pMat,Vec3 * diag,Vec3 * subd);

bool EigenSolveSymmetric (const Mat3 &mat,
							Vec3 * pEigenValues,
							Mat3 * pEigenVectors)
{
	ASSERT(pEigenValues && pEigenVectors);
	// assert that it is symmetric :
	/*
	DURING_ASSERT( Mat3 matTransposed );
	DURING_ASSERT( GetTranspose(mat,&matTransposed) );
	ASSERT( Mat3::Equals(matTransposed,mat) );
	*/
	
    Vec3 subd;

	*pEigenVectors = mat;
    EigenSolver_Tridiagonal(pEigenVectors,pEigenValues,&subd);
    if ( ! EigenSolver_QLAlgorithm(pEigenVectors,pEigenValues,&subd) )
	{
		*pEigenValues = Vec3::zero;
		pEigenVectors->SetIdentity();
		return false;
	}

    // make eigenvectors form a right-handed system
    if ( pEigenVectors->GetDeterminant() < 0.f )
    {
		pEigenVectors->Row(2) *= -1.f;
    }

	return true;
}

//----------------------------------------------------------------------------
static void EigenSolver_Tridiagonal(Mat3 * pMat,Vec3 * pDiag,Vec3 * pSubd)
{
    // Householder reduction T = Q^t M Q
    //   Input:   
    //     mat, symmetric 3x3 matrix M
    //   Output:  
    //     mat, orthogonal matrix Q
    //     diag, diagonal entries of T
    //     subd, subdiagonal entries of T (T is symmetric)
	ASSERT(pMat);
	Mat3 & mat = *pMat;
	Vec3 & diag = *pDiag;
	Vec3 & subd = *pSubd;

    const float epsilon = 1e-08f;

    float a = mat.GetElement(0,0);
    float b = mat.GetElement(0,1);
    float c = mat.GetElement(0,2);
    float d = mat.GetElement(1,1);
    float e = mat.GetElement(1,2);
    float f = mat.GetElement(2,2);

    diag[0] = a;
    subd[2] = 0.f;
    if ( fabsf(c) >= epsilon )
    {
        const float ell = sqrtf(b*b+c*c);
        b /= ell;
        c /= ell;
        const float q = 2*b*e+c*(f-d);
        diag[1] = d+c*q;
        diag[2] = f-c*q;
        subd[0] = ell;
        subd[1] = e-b*q;
        mat.Element(0,0) = 1; mat.Element(0,1) = 0; mat.Element(0,2) = 0;
        mat.Element(1,0) = 0; mat.Element(1,1) = b; mat.Element(1,2) = c;
        mat.Element(2,0) = 0; mat.Element(2,1) = c; mat.Element(2,2) = -b;
    }
    else
    {
        diag[1] = d;
        diag[2] = f;
        subd[0] = b;
        subd[1] = e;
        mat.Element(0,0) = 1; mat.Element(0,1) = 0; mat.Element(0,2) = 0;
        mat.Element(1,0) = 0; mat.Element(1,1) = 1; mat.Element(1,2) = 0;
        mat.Element(2,0) = 0; mat.Element(2,1) = 0; mat.Element(2,2) = 1;
    }
}
//---------------------------------------------------------------------------
static bool EigenSolver_QLAlgorithm(Mat3 * pMat,Vec3 * pDiag,Vec3 * pSubd)
{
    // QL iteration with implicit shifting to reduce matrix from tridiagonal
    // to diagonal
	ASSERT(pMat);
	Mat3 & mat = *pMat;

	Vec3 & diag = *pDiag;
	Vec3 & subd = *pSubd;
    const int maxiter = 32;

    for (int ell = 0; ell < 3; ell++)
    {
        int iter;
        for (iter = 0; iter < maxiter; iter++)
        {
            int m;
            for (m = ell; m <= 1; m++)
            {
                float dd = fabsf(diag[m]) + fabsf(diag[m+1]);
                if ( fabsf(subd[m]) + dd == dd )
                    break;
            }
            if ( m == ell )
                break;

            float g = (diag[ell+1]-diag[ell])/(2*subd[ell]);
            float r = sqrtf(g*g+1);
            if ( g < 0 )
                g = diag[m]-diag[ell]+subd[ell]/(g-r);
            else
                g = diag[m]-diag[ell]+subd[ell]/(g+r);
            float s = 1, c = 1, p = 0;
            for (int i = m-1; i >= ell; i--)
            {
                float f = s*subd[i], b = c*subd[i];
                if ( fabsf(f) >= fabsf(g) )
                {
                    c = g/f;
                    r = sqrtf(c*c+1);
                    subd[i+1] = f*r;
                    c *= (s = 1/r);
                }
                else
                {
                    s = f/g;
                    r = sqrtf(s*s+1);
                    subd[i+1] = g*r;
                    s *= (c = 1/r);
                }
                g = diag[i+1]-p;
                r = (diag[i]-g)*s+2*b*c;
                p = s*r;
                diag[i+1] = g+p;
                g = c*r-b;

                for (int k = 0; k < 3; k++)
                {
                    f = mat[k][i+1];
                    mat[k][i+1] = s*mat[k][i]+c*f;
                    mat[k][i] = c*mat[k][i]-s*f;
                }
            }
            diag[ell] -= p;
            subd[ell] = g;
            subd[m] = 0;
        }

        if ( iter == maxiter )
            // should not get here under normal circumstances
            return false;
    }

    return true;
}

//=====================================================================================


//--------------------------------------------------------------------------------

//}; // Mat3Util

END_CB

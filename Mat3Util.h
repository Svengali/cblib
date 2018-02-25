/*!
  \file
  \brief 3-Space Mat3 Utilities
*/

#pragma once

#include "cblib/Vec3.h"
#include "cblib/Mat3.h"
#include "cblib/Vec3U.h"
#include "cblib/Util.h"

START_CB

//namespace Mat3Util
//{

	inline const Vec3 GetDiagonal(const Mat3 & m)
	{
		return Vec3(m.GetRowX().x, m.GetRowY().y, m.GetRowZ().z);
	}

	inline void SetToDiagonal(Mat3 *pm,const Vec3 & d)
	{
		ASSERT(pm);
		pm->SetRowX(d.x,0.f,0.f);
		pm->SetRowY(0.f,d.y,0.f);
		pm->SetRowZ(0.f,0.f,d.z);
		ASSERT(pm->IsValid());
	}

	//! get the inverse; can work on self
	//! use GetTranspose instead if the martix is orthonormal
	void GetInverse(const Mat3 & m,Mat3 * pInv);
		// NOTEZ : calling this on matrices which have
		//	no inverse will result in a divide by zero !!
		// first check if the Determinant is != 0 if you
		// want to be vareful
			
	//! get the inverse; can NOT work on self
	void GetTranspose(const Mat3 & m,Mat3 * pTrans);

	//! make it Orthonormal; can work on self
	void GetOrthonormalized(const Mat3 & m,Mat3 * pOrthoNorm);

	//----------------------------------------------
	// making rotation matrices :

	/**

	Rotations should be "right handed", so that a 90 degree rotation around Z
	turns unitX into unitY

	Line up your thum along the rotation axis; now your fingers curl in the
	direction of the rotation

	This is a cyclical right-handed thing in the [XYZ] pattern :

	{90 degree rotation around X} * {unit Y} = {unit Z}
	{90 degree rotation around Y} * {unit Z} = {unit X}
	{90 degree rotation around Z} * {unit X} = {unit Y}

	and negatives for anything in the anti-cyclic order, like : 

	{90 degree rotation around X} * {unit Z} = - {unit Y}

	**/

	void SetRotation(Mat3 *pm,const Vec3 & axis,const float angle);
	void SetXRotation(Mat3 *pm,const float angle);
	void SetYRotation(Mat3 *pm,const float angle);
	void SetZRotation(Mat3 *pm,const float angle);
	void SetRandomRotation(Mat3 * pM);

	/** Return R^-1 * I * R */
	void	SetRotatedTensor(Mat3* result, const Mat3& R, const Mat3& I);

	void	SetLookDirection(Mat3* result, const Vec3& dir, const Vec3& up);

	// works when dir goes nearly aligned with any concept of "up"
	//	it may have wacky spin
	void	SetLookDirectionNoUp(Mat3* result, const Vec3& dir);

	void	SetLerped(Mat3* result, const Mat3& from, const Mat3& to, const float t);

	/** Align our z-axis with the given normal; other axes are
		arbitrary. */
	inline void	SetOrientedToSurface(Mat3* pm, const Vec3& up)
	{
		ASSERT(pm && up.IsValid() && up.IsNormalized());

		Vec3	x, y;
		GetTwoPerpNormals(up, &x, &y);
		pm->SetColumnX(x);
		pm->SetColumnY(y);
		pm->SetColumnZ(up);

		ASSERT( pm->IsOrthonormal() );
	}

	inline void LeftMultiplyXRotation(Mat3 *pm,const float angle)
	{
		ASSERT(pm && fisvalid(angle));
		Mat3 temp;
		SetXRotation(&temp,angle);
		pm->LeftMultiply(temp);
	}
	inline void LeftMultiplyYRotation(Mat3 *pm,const float angle)
	{
		ASSERT(pm && fisvalid(angle));
		Mat3 temp;
		SetYRotation(&temp,angle);
		pm->LeftMultiply(temp);
	}
	inline void LeftMultiplyZRotation(Mat3 *pm,const float angle)
	{
		ASSERT(pm && fisvalid(angle));
		Mat3 temp;
		SetZRotation(&temp,angle);
		pm->LeftMultiply(temp);
	}
	inline void RightMultiplyXRotation(Mat3 *pm,const float angle)
	{
		ASSERT(pm && fisvalid(angle));
		Mat3 temp;
		SetXRotation(&temp,angle);
		pm->RightMultiply(temp);
	}
	inline void RightMultiplyYRotation(Mat3 *pm,const float angle)
	{
		ASSERT(pm && fisvalid(angle));
		Mat3 temp;
		SetYRotation(&temp,angle);
		pm->RightMultiply(temp);
	}
	inline void RightMultiplyZRotation(Mat3 *pm,const float angle)
	{
		ASSERT(pm && fisvalid(angle));
		Mat3 temp;
		SetZRotation(&temp,angle);
		pm->RightMultiply(temp);
	}

	//----------------------------------------------
	/*! Get/SetFrom Euler Angles

	There are many possible conventions here; the one currently
	implemented is the "XYZ" convention.  The matrix is made from
	3 angles like :

	mat = RotX( angles.x ) * RotY( angles.y ) * RotZ( angle.z )

	that, is the Z rotation is applied to any vector first.

	Use of Euler angles should be very rare, since it suffers from
	nasty degeneracies (Gimball lock, etc.)

	Euler angles are an over-covering of the rotation space.  That is,
	many euler angles correspond to the same rotation.  What that
	means is that if you do a SetFromEulerAnglesXYZ() and then a
	GetEulerAnglesXYZ() you may not get the same angles back out!

	*/

	void GetEulerAnglesXYZ(const Mat3 & mat,Vec3 * pAngles);
	void SetFromEulerAnglesXYZ(Mat3 * pMat,const Vec3 & angles);

	//----------------------------------------------

	bool EigenSolveSymmetric (const Mat3 &mat,
					Vec3 * pEigenValues,
					Mat3 * pEigenVectors);
	// Symmetric requires "mat" to be a symmetric matrix
	//	(as for example you get from a Covariance matrix)
	// return value indicates failure (due to a degenerate matrix)
					
//}; // Mat3Util

END_CB

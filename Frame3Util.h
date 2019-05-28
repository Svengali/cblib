/*!
  \file
  \brief Frame3Util : 3-Space Frame3 Utilities
*/

#pragma once

#include "cblib/Util.h"
#include "cblib/Vec3.h"
#include "cblib/Frame3.h"
#include "cblib/Mat3Util.h"

START_CB

//namespace Frame3Util
//{

	//! GetInverse; inverts the matrix and the translation
	//!	Can work on Self
	void GetInverse(const Frame3 & m,Frame3 * pInv);
		// NOTEZ : calling this on matrices which have
		//	no inverse will result in a divide by zero !!
		// first check if the Determinant is != 0 if you
		// want to be vareful

	//! GetTranspose; transposes the matrix and the translation
	//!	this is the full inverse if the matrix is orthonormal
	//!	Can NOT work on Self
	void GetTranspose(const Frame3 & m,Frame3 * pTrans);

	//! make it Orthonormal (only works on the matrix part)
	//!	copies the translation
	//!	Can work on Self
	void GetOrthonormalized(const Frame3 & m,Frame3 * pOrthoNorm);

	//! make a Frame from 3 points ; useful for taking points into
	//	and out of the "space" of 3 points	
	//! MakeFrameFromThreePoints makes an *orthonormal* basis from 3 points
	//	(this is NOT barycentric)
	// return true/false for failure ; fails if the 3 points are colinear or coincident
	bool MakeFrameFromThreePoints(Frame3 * pFrame,const Vec3 &a, const Vec3 &b,const Vec3 &c);

	//! MakeBarycentricFrame makes the Frame which takes a point from "barycentric space"
	//!	 to world space.
	//!	In particular, a coordinate of the form {b0,b1,n} is taken to the world-space
	//!	 determined by the triangle points (a,b,c)
	//! *this frame is NOT orthonormal* !!
	//!	The inverse frame takes points from world space into "barycentric space"
	// return true/false for failure ; fails if the 3 points are colinear or coincident
	bool MakeBarycentricFrame(Frame3 * pFrame,const Vec3 &a,const Vec3 &b,const Vec3 &c);

	void SetLookAt(Frame3 * pFrame,const Vec3 & fm,const Vec3 & to,const Vec3 & up = Vec3::unitZ);

	/** Align our z-axis with the given normal; other axes are
		arbitrary. */
	void	SetOrientedToSurface(Frame3 * pFrame, const Vec3& up, const Vec3 & pos);
	
	void SetLerped(Frame3 * result, const Frame3& from, const Frame3& to, const float t);

	//----------------------------------------------
	// making frame from 16-float array :

	void MakeFromArray(Frame3 * pm, const float *array);

	void MakeFrameFromPositionAndUpDir(Frame3 *frame,  const Vec3& pos, const Vec3& up);
	
	//----------------------------------------------
	// making rotation matrices :

	inline void SetRotation(Frame3 *pM,const Vec3 & axis,const float angle)
	{
		pM->SetTranslation(Vec3::zero);
		SetRotation(&(pM->MutableMatrix()),axis,angle);
	}
	inline void SetXRotation(Frame3 *pM,const float angle)
	{
		pM->SetTranslation(Vec3::zero);
		SetXRotation(&(pM->MutableMatrix()),angle);
	}
	inline void SetYRotation(Frame3 *pM,const float angle)
	{
		pM->SetTranslation(Vec3::zero);
		SetYRotation(&(pM->MutableMatrix()),angle);
	}
	inline void SetZRotation(Frame3 *pM,const float angle)
	{
		pM->SetTranslation(Vec3::zero);
		SetZRotation(&(pM->MutableMatrix()),angle);
	}
	
	inline void SetRandomRotation(Frame3 * pM)
	{
		pM->SetTranslation(Vec3::zero);
		SetRandomRotation(&(pM->MutableMatrix()));
	}	
	
	inline void LeftMultiplyXRotation(Frame3 * pm,const float angle)
	{
		ASSERT(pm && fisvalid(angle));
		Mat3 temp;
		SetXRotation(&temp,angle);
		pm->LeftMultiply(temp);
	}
	inline void LeftMultiplyYRotation(Frame3 * pm,const float angle)
	{
		ASSERT(pm && fisvalid(angle));
		Mat3 temp;
		SetYRotation(&temp,angle);
		pm->LeftMultiply(temp);
	}
	inline void LeftMultiplyZRotation(Frame3 * pm,const float angle)
	{
		ASSERT(pm && fisvalid(angle));
		Mat3 temp;
		SetZRotation(&temp,angle);
		pm->LeftMultiply(temp);
	}
	inline void RightMultiplyXRotation(Frame3 * pm,const float angle)
	{
		ASSERT(pm && fisvalid(angle));
		Mat3 temp;
		SetXRotation(&temp,angle);
		pm->RightMultiply(temp);
	}
	inline void RightMultiplyYRotation(Frame3 * pm,const float angle)
	{
		ASSERT(pm && fisvalid(angle));
		Mat3 temp;
		SetYRotation(&temp,angle);
		pm->RightMultiply(temp);
	}
	inline void RightMultiplyZRotation(Frame3 * pm,const float angle)
	{
		ASSERT(pm && fisvalid(angle));
		Mat3 temp;
		SetZRotation(&temp,angle);
		pm->RightMultiply(temp);
	}

	//----------------------------------------------
//}; // Frame3Util

END_CB



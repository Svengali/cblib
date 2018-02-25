/*!
  \file
  \brief QuatUtil : Quaternion manipulation utilities
*/

#pragma once

#include "cblib/Quat.h"

START_CB

//namespace QuatUtil
//{

	void SetRandom(Quat * pQ);
	
	//! Slerp : "Spherical Linear Interpolation"
	//!  about 600 clocks
	void SetSlerp(Quat * pQ,const Quat &v1,const Quat &v2,const float t);
	
	/*! Squad : just like cubic Bezier, but made of Slerps instead of Lerps
			t = 0 gives you q0
			t = 1 gives you q3
			q1 and q2 are not passed through; they pull the intermediate quats in their direction
	*/
	void SetSphericalBezier(Quat * pQ,
		const Quat & q0,const Quat & q1,const Quat & q2,const Quat & q3,
		const float t);

	/*! SetRotationArc
		makes the "simplest" quat Q
		such that normalTo = Q * normalFm
	*/
	void SetRotationArc(Quat * pQ, const Vec3 & normalFm, const Vec3 & normalTo );

	void SetRotationArcUnNormalized(Quat * pQ, const Vec3 & from, const Vec3 & to );

	//! SetPower
	//! *pQ = q ^ power
	//! Power of -1 is an inverse;
	//!	Quat(axis,theta)^power = Quat(axis,theta*power)
	void SetPower(Quat * pQ,const Quat & q,const float power);
	
//};

END_CB

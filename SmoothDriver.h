#pragma once
#ifndef SMOOTHDRIVER_H
#define SMOOTHDRIVER_H

/***************

SmoothDriver v1.0
by cbloom

two Drive functions are the basic entry points :
	take a smooth (C1) step towards a target point and velocity
	gauranteed to converge in finite time, proportional to coverge_time, subject to constraints

see writings on my site for more information

this header works on 3d values, but extension to N dimensions is trivial

1d is actually a lot simpler, but I don't provide the simple 1d case at the moment, maybe next time

*****************/

#include "Base.h"
#include "ComplexDouble.h"
#include "Vec3.h"

START_CB

//=================================================================================

// cubic curve with max accel
//	max_accel should be somewhere around (typical_distance/coverge_time^2)
void DriveCubic(Vec3 * pos,Vec3 * vel,const Vec3 & toPos,const Vec3 & toVel,
		float max_accel,float time_step);

// 1d version :
//  NOTE : do NOT use a bunch of 1d drives to do N-d vectors, that creates a weird asymmetry
void DriveCubic(float * pos,float * vel,const float toPos,const float toVel,
		float max_accel,float time_step);
		
//=================================================================================

//	PD damping should generally be between 1.0 and 1.1
//	converge_time_scale is the time it takes to get very close (not exact convergence time)

void DrivePDClamped(Vec3 * pos,Vec3 * vel,const Vec3 & toPos,const Vec3 & toVel,
			const float converge_time_scale,
			const float damping,
			const float minVelocity,
			float time_step);

// 1d version :
//  NOTE : do NOT use a bunch of 1d drives to do N-d vectors, that creates a weird asymmetry
void DrivePDClamped(float * pos,float * vel,const float toPos,const float toVel,
			const float converge_time_scale,
			const float damping,
			const float minVelocity,
			float time_step);

//=================================================================================

/**

"Intercept" variants :

the normal "Drive" functions try to make you hit the spot "toPos" with velocity "toVel"

the "Intercept" functions make you hit a point which starts at toPos and has velocity toVel
	that is, the target point is linearly moving with toVel
	you might hit toPos at time 0 , or toPos + t * toVel at some time t in the future.

---

intercept_confidence should be in [0,1]
something around 0.5 is nice

it roughly blends off how strongly we assume that the target will continue at toVel exactly


**/

void DriveCubicIntercept(Vec3 * pos,Vec3 * vel,const Vec3 & toPos,const Vec3 & toVel,
		float max_accel,float time_step,
			float intercept_confidence = 1.f);

void DriveCubicIntercept(float * pos,float * vel,const float toPos,const float toVel,
		float max_accel,float time_step,
			float intercept_confidence = 1.f);
		
//=================================================================================

void DrivePDClampedIntercept(Vec3 * pos,Vec3 * vel,const Vec3 & toPos,const Vec3 & toVel,
			const float converge_time_scale,
			const float damping,
			const float minVelocity,
			float time_step,
			float intercept_confidence = 1.f);

void DrivePDClampedIntercept(float * pos,float * vel,const float toPos,const float toVel,
			const float converge_time_scale,
			const float damping,
			const float minVelocity,
			float time_step,
			float intercept_confidence = 1.f);

//=================================================================================

END_CB

#endif // SMOOTHDRIVER_H

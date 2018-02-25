/*!
  \file
  \brief Floating pointer number utilities
*/

#pragma once

#include "cblib/Base.h"
#include "cblib/Util.h"

// CB : the fabsf is just horrendous, use ours
#ifdef _DEBUG
#ifdef _XBOX
#define DO_REPLACE_FABSF
#define DO_REPLACE_BASIC_MATH
#endif
#endif

#ifdef DO_REPLACE_FABSF
#define fabsf	dont_use_fabsf
#endif 

#ifdef DO_REPLACE_BASIC_MATH
#define sqrtf	dont_use_sqrtf
#define sinf	dont_use_sinf
#define cosf	dont_use_cosf
#define tanf	dont_use_tanf
#endif

#include <float.h> // for FLT_MAX
#include <math.h>

#ifdef DO_REPLACE_FABSF
#undef fabsf
#endif

#ifdef DO_REPLACE_BASIC_MATH
#undef sqrtf
#undef sinf
#undef cosf
#undef tanf
#endif

#pragma warning(disable : 4725) // instruction may be inaccurate on some Pentiums (it's talking about fptan)

START_CB

//-------------------------------------------------------------------

	//! You must ResetFPU at startup time!
	void ResetFPU();

	//! for debugging :
	void TurnOnFPUExceptions();

	//! constant speed drive; API like DampedDrive
	//!	"speed" is in values per second
	float LerpedDrive(const float val, const float towards, const float speed, const float time_step);

	/*
		damping is usually 1.0 (critically damped)
		frequency is usually in the range 1..16
			freq has units [1/time]
			damping is unitless
	*/
	float ComputePDAccel(const float fmValue,const float fmVelocity,
						const float toValue,const float toVelocity,
						const float frequency, const float damping,
						const float dt );
						
	void ApplyPD(	float * pValue,float * pVelocity,
						const float toValue,const float toVelocity,
						const float frequency, const float damping,
						const float dt );

	// a "Standard" PD has damping = 1.0 and toVelocity = 0.0
	// (no reason to use this - just used DampedDrive)
	void ApplyPDStandard(float * pValue,float * pVelocity,
						const float toValue,
						const float frequency,
						const float dt );
						
	// New/Fixed version of DampedDrive , stores a "velocity" side variable
	void ApplyDampedDrive(
						float * pValue,
						float * pVelocity,
						const float toValue,
						const float frequency,
						const float dt );
						
	void ApplyDampedDriveClamping(
						float * pValue,
						float * pVelocity,
						const float toValue,
						const float frequency,
						const float minVelocity,
						const float maxVelocity,
						const float minStep,
						const float dt );
						
	// ReciprocalSqrt returns 1/sqrt(f)
	float ReciprocalSqrt( const float f );
	
	// Vec4->Vec4 normalize ; to use with a Vec3, you need to put a 0 in W
	//	In & Out pointers must be aligned to 16 bytes!!  (use ALIGN16)
	//	In & Out pointers can be the same
	// Approx is accurate to 12 bits (usually good enough)
	// NR uses Newton-Raphson to give 23 bits of precision (= full floating precision
	//	Approx is about 103 clocks
	//	NR is about 113 clocks
	//  Vec3::NormalizeFast is just as fast
	//void NormalizeVec4Aligned_Approx(void * pOut,const void *pIn);
	//void NormalizeVec4Aligned_NR(void * pOut,const void *pIn);

	uint32	FloatAsInt(const float f);

//-------------------------------------------------------------------
// float consts :

#ifdef PI
#undef PI
#endif

#define CBPI	(3.141592653590)
#define TWO_PI	(6.2831853071796)
#define HALF_PI	(1.570796326795)
#define PIf		(3.141592653590f)
#define TWO_PIf	(6.2831853071796f)
#define HALF_PIf	(1.570796326795f)

#define RADS_PER_DEG (0.01745329251994329576)
#define DEGS_PER_RAD (57.2957795130823208767)
#define RADS_PER_DEGf (0.01745329251994329576f)
#define DEGS_PER_RADf (57.2957795130823208767f)

#define FLOAT_AS_INT(f)			(reinterpret_cast<const uint32 &>(f))
#define FLOAT_AS_INT_MUTABLE(f)	(reinterpret_cast<		uint32 &>(f))
#define INT_AS_FLOAT(i)			(reinterpret_cast<const float &>(i))
#define INT_AS_FLOAT_MUTABLE(i)	(reinterpret_cast<		float &>(i))

#define	DOUBLE_AS_INT(f)			(reinterpret_cast<const uint64 &>(f))
#define DOUBLE_AS_INT_MUTABLE(f)	(reinterpret_cast<		uint64 &>(f))
#define INT_AS_DOUBLE(i)			(reinterpret_cast<const double &>(i))
#define INT_AS_DOUBLE_MUTABLE(i)	(reinterpret_cast<		double &>(i))

//-------------------------------------------------------------------
//
// int-float checks from Pierre 
//
// these are great for floats in memory
// not so good for floats that are in floating registers, because they have to be stored
// (see also "fcomi")

// IEEE floats are {sign bit} {8 bits of exponent + 127} {23 bits of fraction}
//	mantissa is 24 bits, made from the fraction and leading 1 bit

// NOTE : for *positive* floats you can do > or < compares on the bytes
//	for negative floats it gets the direction wrong, and mixing positive and negative is very wrong

// forced to use macros for the moment because inline functions are so rotten :
//	fiszero_I() is about twice as fast as fiszero()

// zero in a float is (0) or (1<<31)
#define fiszero_I(f)		( (FLOAT_AS_INT(f) & 0x7FFFFFFF) == 0 )

 // negative or zero (but not positive zero, so this is not a reliable <= 0)
#define fisntpositive_I(f)	( (FLOAT_AS_INT(f) & 0x80000000) != 0 )

 // positive or zero (but not negative zero, so this is not a reliable >= 0)
#define fisntnegative_I(f)	( (FLOAT_AS_INT(f) & 0x80000000) == 0 )

#define fisequal_I(f,g)		( ( FLOAT_AS_INT(f) == FLOAT_AS_INT(g) ) || ( fiszero_I(f) && fiszero_I(g) ) )
	// two floats are equal if they're bitwise equal or if they're both zero (-0 and +0)
	//	exact float equality is pretty lame

#define fabs_I(f)			( FLOAT_AS_INT(f) & 0x7FFFFFFF ) // turn off sign bit

#define fsignbit_I(f)		( FLOAT_AS_INT(f) >> 31 )
#define fsignmask_I(f)		( ((int)FLOAT_AS_INT(f)) >> 31 ) // not unsigned so I get sign replication on right-shift

#define fsign_I(f)			( -2 * fsignbit_I(f) + 1 ) // integer -1 or +1 for positive or negative floats

#define fexponent_I(f)		((( FLOAT_AS_INT(f) >> 23 )&0xFF) - 127)
#define ffraction_I(f)		( FLOAT_AS_INT(f) & ((1<<24)-1) ) // bottom 23 bits
#define fmantissa_I(f)		( (1<<24) + ffraction_I(f) ) // put on the implicit leading 1

//-------------------------------------------------------------------

#ifdef DO_REPLACE_FABSF
// replace the compiler fabs :
inline float fabsf(const float f)
{
	// or use __asm fabs ?
	int a = fabs_I(f);
	return INT_AS_FLOAT(a);
}
#endif

#ifdef DO_REPLACE_BASIC_MATH
// replace the compiler sqrtf :
inline float sqrtf(const float f)
{
	__asm fld f
	__asm fsqrt
	__asm fstp f
	return f;
}
inline float sinf(const float f)
{
	__asm fld f
	__asm fsin
	__asm fstp f
	return f;
}
inline float cosf(const float f)
{
	__asm fld f
	__asm fcos
	__asm fstp f
	return f;
}
inline float tanf(const float f)
{
	__asm fld f
	__asm fptan
	__asm fstp st(0)	// fptan pushes tanf(f), then pushes 1. Need to pop the 1 off first
	__asm fstp f		// then pop the result into f
	return f;
}
#endif

//-------------------------------------------------------------------
// float utility functions :

#ifdef DO_HEAVY_ASSERTS //{
inline bool fisvalid(const float f)
{
	// ASSERT on the freed memory values :
	//	these *are* valid floats, but the chance of
	//	getting one on purpose is very slim
	HEAVY_ASSERT( FLOAT_AS_INT(f) != 0xCDCDCDCD );
	HEAVY_ASSERT( FLOAT_AS_INT(f) != 0xDDDDDDDD );

	// this works because NAN always returns false on compares :
	HEAVY_ASSERT(f >= -FLT_MAX && f <= FLT_MAX );

	return true;
}
#else // DO_HEAVY_ASSERTS

#define fisvalid(f)		true

#endif //}

//! must do this lame shit to make sure both values are
//	treated the same; if not, one is left in a register and
//	is actually a double
bool fequalexact(const float x,const float y);

//! 1 minus an upside down fcube. NOT x^-3!
// f@0.0=0.0, f@0.5=0.875, f@1.0=1.0
// Nice for ramping down to zero, but where most of the ramping happens right near zero.
inline float fcubeinv(const float f)
{
	return 1.f - fcube(1.f - f);
}

//! 1 minus an upside down fsquare. NOT x^-2!
// f@0=0.0, f@0.5=0.75, f@1.0=1.0
// Nice for ramping down to zero, but where most of the ramping happens right near zero.
// Less extreme than fcubeinv
inline float fsquareinv(const float f)
{
	return 1.f - fsquare(1.f - f);
}

//! flipped, shifted, and remapped x^2. 
//!f@0=0, f@0.5=1, f@1=1. Looks like a bell curve without the nice tails.
//! handy for ramping up from zero then back down to zero without using cases, when 
//! the edge smoothness is not critical. 
//! practically speaking, this is fsquareinv, shished by half and shifted to the right. 
inline float fsquarebump(const float f)
{
	//y = 1.f - fsquare(2.f * f - 1.f); //original version
	return 4.f * f * (1.f - f);	//unrolled version
}

//! lerp from "t0" to "t1" ; all values are in "modulo" space,
//!	 so, for example t1 < t0 is fine, and it's a *later* time;
inline float flerpmod(float t0,float t1,float modulo,float lerper)
{
	ASSERT( fisinrangesloppy(t0,0.f,modulo) );
	ASSERT( fisinrangesloppy(t1,0.f,modulo) );
	ASSERT( fisinrangesloppy(lerper,0.f,1.f) );
	if (t0 > t1 )
	{
		t0 -= modulo;
	}
	float t = flerp(t0,t1,lerper);
	if ( t < 0 )
	{
		t += modulo;
	}
	ASSERT( fisinrangesloppy(t,0.f,modulo) );
	return t;
}

//!Computes the distance between a and b on the ring modulo modulus. 
//!Assumes a and b are -modulus/modulus ranged. 
inline float fmoddistance(float a, float b, float modulus)
{
	ASSERT( fisinrange(a,-modulus,modulus) );
	ASSERT( fisinrange(b,-modulus,modulus) );
	ASSERT( modulus > 0.f );

	const float dist = fabsf(a-b);
	if( dist > modulus )
	{
		return 2.f * modulus - dist;
	}
	return dist;
}

//! bulletproof version tolerates all kinds of crazy input
inline float flerpmod_bulletproof(float t0,float t1,float modulo,float lerper)
{
	t0 = fmodf(t0,modulo);
	t1 = fmodf(t1,modulo);
	if (t0 > t1 )
	{
		t0 -= modulo;
	}
	float t = flerp(t0,t1,lerper);
	t = fmodf(t,modulo);
	return t;
}

inline float flerpangle(float fm, float to, float t)
{
	// make sure we take the short way around the circle
	while( fm > (to + PIf) )
	{
		fm -= TWO_PIf;
	}
	while( fm < (to - PIf) )
	{
		fm += TWO_PIf;
	}
	// result is NOT gauranteed to be in any particular range, since "t" need
	//	not be in [0,1]
	return flerp(fm,to,t);
}

//! versions to map doubles to floats
//
inline float dmakelerperclamped(const double lo,const double hi,const double val)
{
	ASSERT( hi > lo );
	return (float) Clamp( (val - lo) / (hi - lo) ,0.0,1.0);
}
inline float dmakelerpernoclamp(const double lo,const double hi,const double val)
{
	ASSERT( hi > lo );
	return (float)( (val - lo) / (hi - lo) );
}

//! remap from one range to another 
inline float fremapclamped(float fromLo, float fromHi, float toLo, float toHi, float val)
{
	return flerp(toLo,toHi, fmakelerperclamped(fromLo,fromHi,val) );
}

inline float fremapnoclamp(float fromLo, float fromHi, float toLo, float toHi, float val)
{
	return flerp(toLo,toHi, fmakelerpernoclamp(fromLo,fromHi,val) );
}

//!	remap (0->1) to (0->1) but with derivative continuity
inline float fHermiteLerpParameter(const float t)
{
	ASSERT( t >= - EPSILON && t <= 1.f + EPSILON );
	return (3.f - 2.f*t)*t*t;
}

//! use fHermiteLerpClamping if t could be out of the [0,1] range
inline float fHermiteLerpClamping(const float t)
{
	ASSERT( fisvalid(t) );
	if ( t <= 0.f ) return 0.f;
	else if ( t >= 1.f ) return 1.f;
	else return fHermiteLerpParameter(t);
}

//! even better continuity than Hermite lerp, but slower
inline float fCosLerpParameter(const float t)
{
	ASSERT( t >= - EPSILON && t <= 1.f + EPSILON );
	return 0.5f - 0.5f * cosf( t * CBPI );
}

//! use fCosLerpClamping if t could be out of the [0,1] range
inline float fCosLerpClamping(const float t)
{
	ASSERT( fisvalid(t) );
	if ( t <= 0.f ) return 0.f;
	else if ( t >= 1.f ) return 1.f;
	else return fCosLerpParameter(t);
}

inline void fsincos(const float a, float* s, float *c)
{
    *s = sinf( a );
    *c = cosf( a );


/*
	__asm
		{
			fld a;
			fsincos;
			mov eax, c;
			fstp [eax];
			mov eax, s;
			fstp [eax];
		};
        */
}


/*
QuadraticSplineLerper

makes a spline P(t) which has Bezier controls
B0 = {0,0}
B1 = "point"
B2 = {1,1}

if "point" as {0.5,0.5} it would be a straight line lerp
We solve for

P_y(P_x)

"point" must be in range so that the solution is single-valued
*/
inline float fquadraticsplinelerper(const float f, const float ptX, const float ptY )
{
	ASSERT( fisinrange(ptX,0.f,1.f) );
	ASSERT( fisinrange(ptY,0.f,1.f) );
	ASSERT( !fequal(ptX,0.5f) );	//singularity here. 
	
	float bx = ptX;
	float a = (1 - 2.f*bx);
	float A = bx*bx + a*f;
	float t = (sqrtf(A) - bx)/a;
	float y = t*t + ptY*2.f*t*(1.f-t);
	return y;
}

//! float to int is expensive
//! this depends on the FPU rounding mode
//! ASSERTs if the rounding mode is not C-rounding compatible
/*
#ifdef _XBOX
inline int ftoi(const float f)
{
	// faster ?
	__asm cvttss2si eax,f;
}
inline int dtoi(const double f)
{
	int i;
	__asm
	{
		FLD   f
		FISTP i
	}

	ASSERT( i == int(f) );

	return i;
}
#else
inline int ftoi(const float f)
{
	return int(f);
}
inline int dtoi(const double f)
{
	return int(f);
}
#endif
*/

//! int to float is easy
inline float itof(const int i)
{
	return (float)i;
}

//! fast log2 of a float; truncating version
inline int intlog2(const float f)
{
	return ((FLOAT_AS_INT(f)) >> 23) - 127;
}

inline int intlog2(const int i)
{
	const float f = float(i);
	return intlog2(f);
}


//! degrees to radians converter
inline float fdegtorad(const float d)
{
	return d * RADS_PER_DEG;
}
//! radians to degrees converter
inline float fradtodeg(const float r)
{
	return r * DEGS_PER_RAD;
}

inline void finvalidate(float & f)
{
	FLOAT_AS_INT_MUTABLE(f) = 0xFFFFFFFF;
	//ASSERT( ! fisvalid(f) );
}

// finvalidatedbg
//	use on member variables of Vector, etc.
//	makes sure that you don't touch non-validated
//	stuff in debug build
inline void finvalidatedbg(float & f)
{
	#ifdef _DEBUG
	finvalidate(f);
	#endif
}

//-------------------------------------------------------------------
// little utilities to warp the unit range 0->1 to itself

//! simple squaring warps x down
//!	result >= input	(equality at 0 and 1)
inline float fwarpunit_down(const float x)
{
	return x*x;
}

//! cubic polynomial 
//!	result <= input	(equality at 0 and 1)
inline float fwarpunit_up(const float x)
{
	const float y = (1.f - x);

	// the 1.5 in here is actually 4 * ( 2 * k - 1 )
	//	where "k" is the value of the curve at x = 0.5
	// 1.5 comes from k = 0.69 , roughly, which is near sqrt(.5)
	// 1.65688 actually gives you sqrt(.5)

	return x * x * (x + 3.f * y) + 1.5f * x * y * y;
}

//! the best quadratic approximation of sqrt(x) over
//!	the entire interval [0,1]
//!	constrained so that approx(0) = 0 and approx(1) = 1
inline float fsqrt_approx_quadratic(const float x)
{
	return x * (27.f - 13.f*x) * (1.f/14.f);
}

//-------------------------------------------------------------------

// min/max of absolute values. returns the original value
// 
inline float fmaxabs( const float a, const float b )
{
	return fabsf(a) > fabsf(b) ? a : b;
}

inline float fminabs( const float a, const float b )
{
	return fabsf(a) < fabsf(b) ? a : b;
}

//-------------------------------------------------------------------


//-------------------------------------------------------------------

/***

(use of these is transparent to the user)

#define to replace some of the standard C math functions
with versions that will ASSERT about the range of arguments

Make these #define *before* the safe functions

***/

#ifdef DO_ASSERTS //{

extern float sqrtf_asserting(const float f);
extern float asinf_asserting(const float f);
extern float acosf_asserting(const float f);
extern double sqrt_asserting(const double f);
extern double asin_asserting(const double f);
extern double acos_asserting(const double f);

/*
#define sqrtf	cb::sqrtf_asserting
#define asinf	cb::asinf_asserting
#define acosf	cb::acosf_asserting
#define sqrt	cb::sqrt_asserting
#define asin	cb::asin_asserting
#define acos	cb::acos_asserting
*/
#endif //} DO_ASSERTS

//-------------------------------------------------------------------
/***

These assert that the arguments are loosely in range, but not
perfectly.  The idea is for things like this :

v.Normalize();
angle = acosf(v.x);

So, v.x should be in (-1 to 1), but cuz of epsilons, it could be
slightly out of range.  So, typically you'll just start with the
above code.  Then, hopefully, the _asserting code will catch it
and fire.  Then you can fix it up by doing this :

v.Normalize();
angle = acosf_safe(v.x);

****/

inline float sqrtf_safe(const float f)
{
	// f should be >= 0.f , but allow a tiny negative & clamp it
	ASSERT( f >= - EPSILON );
	if ( f <= 0.f )
		return 0.f;
	else
		return (float) sqrt(f);
}

inline float asinf_safe(const float f)
{
	// f should be in [-1,1] but allow a little slip & clamp it
	ASSERT( f >= (- 1.f - EPSILON) && f <= (1.f + EPSILON) );

	const float fclamped = Clamp(f,-1.f,1.f);

	return (float) asin(fclamped);
}

inline float acosf_safe(const float f)
{
	// f should be in [-1,1] but allow a little slip & clamp it
	ASSERT( f >= (- 1.f - EPSILON) && f <= (1.f + EPSILON) );

	const float fclamped = Clamp(f,-1.f,1.f);

	return (float) acos(fclamped);
}


inline double sqrt_safe(const double f)
{
	// f should be >= 0.f , but allow a tiny negative & clamp it
	ASSERT( f >= - EPSILON );
	if ( f <= 0. )
		return 0.;
	else
		return sqrt(f);
}

inline double asin_safe(const double f)
{
	// f should be in [-1,1] but allow a little slip & clamp it
	ASSERT( f >= (- 1. - EPSILON) && f <= (1. + EPSILON) );

	const double fclamped = Clamp(f,-1.,1.);

	return asin(fclamped);
}

inline double acos_safe(const double f)
{
	// f should be in [-1,1] but allow a little slip & clamp it
	ASSERT( f >= (- 1.f - EPSILON) && f <= (1.f + EPSILON) );

	const double fclamped = Clamp(f,-1.,1.);

	return acos(fclamped);
}

END_CB

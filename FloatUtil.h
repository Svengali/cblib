/*!
  \file
  \brief Floating pointer number utilities
*/

#pragma once

#include "Base.h"
#include "Util.h"

/**

FloatUtil.h is in a lot of headers, try to keep it minimal

**/

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
#include <xmmintrin.h>

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

	// SaveDefaultFPUState : should call this at startup :
	void SaveDefaultFPUState();
	// then call Restore wherever you want to put it back :
	void RestoreDefaultFPUState();
	
	struct FPUState
	{
		unsigned int word;
	};

	void SaveFPUState(FPUState * sptr);
	void RestoreFPUState(const FPUState * sptr);

	// deprecated :
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

	//spatial_i & wave_k from 0 to (dim-1)
	double ComputeDCTCoefficient(int spatial_i,int wave_k,int dim);

//-------------------------------------------------------------------
// float consts :

// EPSILON is a "distance"
#define CB_EPSILON				(0.0005f) // .5 mm
// EPSILON_NORMALS is unitless, for use with normal vectors
#define CB_EPSILON_NORMALS		(0.002f)  // unitless ; .2 % , or 0.114 degrees

#define CB_PI		(3.141592653590)
#define CB_TWO_PI	(6.2831853071796)
#define CB_HALF_PI	(1.570796326795)
#define CB_PIf		(3.141592653590f)
#define CB_TWO_PIf	(6.2831853071796f)
#define CB_HALF_PIf	(1.570796326795f)

#define CB_RADS_PER_DEG (0.01745329251994329576)
#define CB_DEGS_PER_RAD (57.2957795130823208767)
#define CB_RADS_PER_DEGf (0.01745329251994329576f)
#define CB_DEGS_PER_RADf (57.2957795130823208767f)

#define CB_FLOAT_AS_INT(f)			(reinterpret_cast<const cb::uint32 &>(f))
#define CB_FLOAT_AS_INT_MUTABLE(f)	(reinterpret_cast<		cb::uint32 &>(f))
#define CB_INT_AS_FLOAT(i)			(reinterpret_cast<const float &>(i))
#define CB_INT_AS_FLOAT_MUTABLE(i)	(reinterpret_cast<		float &>(i))

#define	CB_DOUBLE_AS_INT(f)			(reinterpret_cast<const cb::uint64 &>(f))
#define CB_DOUBLE_AS_INT_MUTABLE(f)	(reinterpret_cast<		cb::uint64 &>(f))
#define CB_INT_AS_DOUBLE(i)			(reinterpret_cast<const double &>(i))
#define CB_INT_AS_DOUBLE_MUTABLE(i)	(reinterpret_cast<		double &>(i))

union DoubleAnd64
{
	uint64		i;
	double		d;
};

union FloatAnd32
{
	uint32		i;
	float		f;
};

//-------------------------------------------------------------------

#ifdef PI
#undef PI
#endif

#ifdef RADS_PER_DEG
#undef RADS_PER_DEG
#endif
#ifdef DEGS_PER_RAD
#undef DEGS_PER_RAD
#endif

#define EPSILON		CB_EPSILON
#define EPSILON_NORMALS	CB_EPSILON_NORMALS

#define PI			CB_PI
#define TWO_PI	CB_TWO_PI
#define HALF_PI	CB_HALF_PI
#define PIf		CB_PIf
#define TWO_PIf	CB_TWO_PIf
#define HALF_PIf	CB_HALF_PIf

#define RADS_PER_DEG CB_RADS_PER_DEG
#define DEGS_PER_RAD CB_DEGS_PER_RAD
#define RADS_PER_DEGf CB_RADS_PER_DEGf
#define DEGS_PER_RADf CB_DEGS_PER_RADf

#define FLOAT_AS_INT(f)			CB_FLOAT_AS_INT(f)		
#define FLOAT_AS_INT_MUTABLE(f)	CB_FLOAT_AS_INT_MUTABLE(f)	
#define INT_AS_FLOAT(i)			CB_INT_AS_FLOAT(i)		
#define INT_AS_FLOAT_MUTABLE(i)	CB_INT_AS_FLOAT_MUTABLE(i)

#define	DOUBLE_AS_INT(f)			CB_DOUBLE_AS_INT(f)		
#define DOUBLE_AS_INT_MUTABLE(f)	CB_DOUBLE_AS_INT_MUTABLE(f)
#define INT_AS_DOUBLE(i)			CB_INT_AS_DOUBLE(i)	
#define INT_AS_DOUBLE_MUTABLE(i)	CB_INT_AS_DOUBLE_MUTABLE(i)

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
	uint32 a = fabs_I(f);
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

//-----------------------------------------------------------
// float math helpers

//! float == test with slop
inline bool fequal(const float f1,const float f2,const float tolerance = EPSILON)
{
	const float diff = fabsf(f1 - f2);
	return diff <= tolerance;
}

inline bool fequal(const double f1,const double f2,const double tolerance = EPSILON)
{
	const double diff = fabs(f1 - f2);
	return diff <= tolerance;
}

inline bool fisint(const double f,const float tolerance = EPSILON)
{
	double rem = f - ((int)f);
	return ( rem < tolerance ) || ( rem > (1.0 - tolerance) );
}

//! return a Clamp float in the range lo to hi
inline float fclamp(const float x,const float lo,const float hi)
{
	return ( ( x < lo ) ? lo : ( x > hi ) ? hi : x );
}

//! return a Clamp float in the range 0 to 1
inline float fclampunit(const float x)
{
	return ( ( x < 0.f ) ? 0.f : ( x > 1.f ) ? 1.f : x );
}

//! return a lerped float at time "t" in the interval from lo to hi
//	t need not be in [0,1] , we can extrapolate too
inline float flerp(const float lo,const float hi,const float t)
{
	return lo + t * (hi - lo);
}

inline double flerp(const double lo,const double hi,const double t)
{
	return lo + t * (hi - lo);
}

inline float faverage(const float x,const float y)
{
	return (x + y)*0.5f;
}

//! fsign; returns 1.f or -1.f for positive or negative float
//! could do a much faster version using float_AS_INT if this is needed
inline float fsign(const float f)
{
	return (f >= 0.f) ? 1.f : -1.f;
}

// clamp f to positive
inline float fpos(const float f)
{
	if ( f < 0.f ) return 0.f;
	return f;
}

// clamp f to negative
inline float fneg(const float f)
{
	if ( f > 0.f ) return 0.f;
	return f;
}


inline float fsquare(const float f)
{
	return f*f;
}

inline float fcube(const float f)
{
	return f*f*f;
}

inline double fcube(const double f)
{
	return f*f*f;
}

inline double fsquare(const double x) { return x*x; }

inline double fsign(const double f)
{
	return (f >= 0.f) ? 1.f : -1.f;
}

//! float == 0.f test with slop
inline bool fiszero(const float f1,const float tolerance = EPSILON)
{
	return fabsf(f1) <= tolerance;
}

inline bool fiszero(const double f1,const double tolerance = EPSILON)
{
	return fabs(f1) <= tolerance;
}

//! return (f1 in [0,1]) with slop
inline bool fiszerotoone(const float f1,const float tolerance = EPSILON)
{
	return (f1 >= -tolerance && f1 <= (1.f + tolerance) );
}

inline bool fisinrange(const float f,const float lo,const float hi)
{
	return f >= lo && f <= hi;
}

inline bool fisinrangesloppy(const float f,const float lo,const float hi,const float tolerance = EPSILON)
{
	return f >= lo - tolerance && f <= hi + tolerance;
}

//! float == 1.f test with slop
inline bool fisone(const float f1,const float tolerance = EPSILON)
{
	return fabsf(f1-1.f) <= tolerance;
}

//! return a float such that flerp(lo,hi, fmakelerper(lo,hi,v) ) == v
inline float fmakelerpernoclamp(const float lo,const float hi,const float v)
{
	ASSERT( hi > lo );
	return (v - lo) / (hi - lo);
}

inline float fmakelerperclamped(const float lo,const float hi,const float v)
{
	ASSERT( hi > lo );
	return fclampunit( (v - lo) / (hi - lo) );
}

//=============================================================================================

static const double floatutil_xs_doublemagic			= (6755399441055744.0); 	    //2^52 * 1.5,  uses limited precisicion to floor
static const double floatutil_xs_doublemagicdelta		= (1.5e-8);                         //almost .5f = .5f + 1e^(number of exp bit)
static const double floatutil_xs_doublemagicroundeps	= (0.5f - floatutil_xs_doublemagicdelta);       //almost .5f = .5f - 1e^(number of exp bit)

/*

ftoi : truncates ; eg. fractions -> 0

*/
inline int ftoi(const float f)
{
	// plain old C cast is actually fast with /QIfist
	//	the only problem with that is if D3D or anyone changes the FPU settings
	return (int)f;
	
	// SSE single scalar cvtt is not as fast but is reliable :
	//return _mm_cvtt_ss2si( _mm_set_ss( f ) );
}

inline int ftoi_round(const float val)
{
	return ( val >= 0.f ) ? ftoi( val + 0.5f ) : ftoi( val - 0.5f );
}

inline int ftoi(const double f)
{
	// plain old C cast is actually fast with /QIfist
	//	the only problem with that is if D3D or anyone changes the FPU settings
	return (int)f;
	
	// SSE single scalar cvtt is not as fast but is reliable :
	//return _mm_cvtt_ss2si( _mm_set_ss( f ) );
}

inline int ftoi_round(const double val)
{
	return ( val >= 0.f ) ? ftoi( val + 0.5 ) : ftoi( val - 0.5 );
}

// @@ alias ?
#define froundint	ftoi_round

#if 0

// bleck - this gets fucked by FPU modes - DO NOT USE !! TOO DANGEROUS

/*

ftoi : banker rounding
	rounds to nearest
	0.5 goes to nearest *even*
	2.5 -> 2 , 3.5 -> 4

*/
inline int ftoi_round_banker(const float val)
{
	DoubleAnd64 dunion;
    dunion.d = val + floatutil_xs_doublemagic;
	return (int) dunion.i; // just cast to grab the bottom bits
}

inline int ftoi_round_banker(const double val)
{
	DoubleAnd64 dunion;
    dunion.d = val + floatutil_xs_doublemagic;
	return (int) dunion.i; // just cast to grab the bottom bits
}

inline int ftoi_floor(const double val)
{
    return ftoi_round_banker(val - floatutil_xs_doublemagicroundeps);
}

inline int ftoi_ceil(const double val)
{
    return ftoi_round_banker(val + floatutil_xs_doublemagicroundeps);
}

#endif

inline int ftoi_floor(const double val)
{
    return ftoi( floorf((float) val) );
}

inline int ftoi_ceil(const double val)
{
    return ftoi( ceilf((float) val) );
}

//-------------------------------------------------------------------
// utils for looking up arrays of floats :

extern float fsamplelerp_clamp(const float * array,const int size,const float t);
extern float fsamplelerp_wrap( const float * array,const int size,const float t);

// t in [0,1] interpolates from B to C ; A & D are next neighbors
extern float CubicSpline( float A, float B, float C, float D, float t );

extern float FloatTableLookupCubic( float x, const float * table, int size, bool wrap );

//-------------------------------------------------------------------
// float utility functions :

#ifdef DO_ASSERTS //{
inline bool fisvalid(const float f)
{
	// ASSERT on the freed memory values :
	//	these *are* valid floats, but the chance of
	//	getting one on purpose is very slim
	ASSERT( FLOAT_AS_INT(f) != 0xCDCDCDCD );
	ASSERT( FLOAT_AS_INT(f) != 0xDDDDDDDD );

	// this works because NAN always returns false on compares :
	ASSERT(f >= -FLT_MAX && f <= FLT_MAX );

	return true;
}
inline bool fisvalid(const double f)
{
	/*
	// ASSERT on the freed memory values :
	//	these *are* valid floats, but the chance of
	//	getting one on purpose is very slim
	ASSERT( FLOAT_AS_INT(f) != 0xCDCDCDCD );
	ASSERT( FLOAT_AS_INT(f) != 0xDDDDDDDD );
	*/

	// this works because NAN always returns false on compares :
	ASSERT(f >= -DBL_MAX && f <= DBL_MAX );

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
//!Assumes a and b are -modulus/modulus ranged or [0,2*modulus]
// for example for angles, modulus = pi
inline float fmoddistance(float a, float b, float modulus)
{
	//ASSERT( fisinrange(a,-modulus,modulus) );
	//ASSERT( fisinrange(b,-modulus,modulus) );
	ASSERT( modulus > 0.f );

	const float dist = fabsf(a-b);
	ASSERT( dist <= 2.f * modulus );
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
	return 0.5f - 0.5f * cosf( t * PIf );
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
	#ifdef CB_64
	*s = sinf(a);
	*c = cosf(a);
	#else
	__asm
		{
			fld a;
			fsincos;
			mov eax, c;
			fstp [eax];
			mov eax, s;
			fstp [eax];
		};
	#endif
}

inline void dsincos(const double a, double * s, double *c)
{
	#ifdef CB_64
	*s = sin(a);
	*c = cos(a);
	#else
	__asm
		{
			fld a;
			fsincos;
			mov eax, c;
			fstp [eax];
			mov eax, s;
			fstp [eax];
		};
	#endif
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

//! int to float is easy
inline float itof(const int i)
{
	return (float)i;
}

float log2f_approx( const float X );

#define CB_LN2      (0.6931471805599453)

inline double log2( const double x )
{
	return log(x)/CB_LN2;
}

//! fast log2 of a float; truncating version
inline int intlog2(const float f)
{
	return ((FLOAT_AS_INT(f)) >> 23) - 127;
}

inline int intlog2ceil(const float f)
{
	// note : BitScanReverse might be faster
	return ((FLOAT_AS_INT(f) + 0x7FFFFF) >> 23) - 127;
}

inline int intlog2round(const float f)
{
	// note : BitScanReverse might be faster
	// 0x257D86 = (2 - sqrt(2))*(1<<22)
	return ((FLOAT_AS_INT(f) + 0x257D86) >> 23) - 127;
}

inline int intlog2(const int i)
{
	const float f = float(i);
	return intlog2(f);
}


//! degrees to radians converter
inline float fdegtorad(const float d)
{
	return d * RADS_PER_DEGf;
}
//! radians to degrees converter
inline float fradtodeg(const float r)
{
	return r * DEGS_PER_RADf;
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

inline void dinvalidatedbg(double & f)
{
	#ifdef _DEBUG
	finvalidate(*((float *)&f));
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


inline float fsqrtf_signed(float x)
{
	if ( x < 0 ) return - sqrtf(-x);
	else return sqrtf(x);
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

#if _MSC_VER < 1700 // some problem in VC2012
#ifdef DO_ASSERTS //{

extern float sqrtf_asserting(const float f);
extern float asinf_asserting(const float f);
extern float acosf_asserting(const float f);
extern double sqrt_asserting(const double f);
extern double asin_asserting(const double f);
extern double acos_asserting(const double f);

#undef sqrtf
#define sqrtf	cb::sqrtf_asserting
#undef asinf
#define asinf	cb::asinf_asserting
#undef acosf
#define acosf	cb::acosf_asserting
#undef sqrt
#define sqrt	cb::sqrt_asserting
#undef asin
#define asin	cb::asin_asserting
#undef acos
#define acos	cb::acos_asserting

#endif //} DO_ASSERTS
#endif

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

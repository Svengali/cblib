#include "FloatUtil.h"
#include "Log.h"
//#include <stdlib.h> // for rand

START_CB

//! must do this lame shit to make sure both values are
//	treated the same; if not, one is left in a register and
//	is actually a double
bool fequalexact(const float x,const float y)
{
	return x == y;
}

/**

undef replacements so we don't call ourselves with _asserting functions

**/

#undef sqrtf
#undef asinf
#undef acosf

#undef sqrt
#undef asin
#undef acos

float sqrtf_asserting(const float f)
{
	ASSERT( fisvalid(f) );
	ASSERT( f >= 0.f );
	return sqrtf(f);
}

float asinf_asserting(const float f)
{
	ASSERT( fisvalid(f) );
	ASSERT( f >= -1.f && f <= 1.f );
	return asinf(f);
}

float acosf_asserting(const float f)
{
	ASSERT( fisvalid(f) );
	ASSERT( f >= -1.f && f <= 1.f );
	return acosf(f);
}


double sqrt_asserting(const double f)
{
	ASSERT( fisvalid((float)f) );
	ASSERT( f >= 0.0 );
	return sqrt(f);
}

double asin_asserting(const double f)
{
	ASSERT( fisvalid((float)f) );
	ASSERT( f >= -1.0 && f <= 1.0 );
	return asin(f);
}

double acos_asserting(const double f)
{
	ASSERT( fisvalid((float)f) );
	ASSERT( f >= -1.0 && f <= 1.0 );
	return acos(f);
}

//-------------------------------------------------------------------

float log2f_approx( const float X )
{
    ASSERT( X > 0.0 );
    
    FloatAnd32 fi;
    fi.f = X;
    
    // get the float as an int, subtract off exponent bias        
    float vv = (float) (fi.i - (127<<23));
    
    vv *= (1.f/8388608);

    // vv is now like a fixed point with the exponent in the int
    //  and the mantissa in the fraction
    //
    // that is actually already an approximate log2, but very inaccurate

    // get the fractional part of vv :    
    //float frac = vv - ftoi(vv);
    
    fi.i = (fi.i & 0x7FFFFF) | (127<<23);
    float frac = fi.f - 1.f;

    // use frac to apply a correction hump :    
    const float C = 0.346573583f;
        
    float approx = vv + C * frac * (1.f - frac);

//    DURING_ASSERT( double exact =  log2( X ) );
//    ASSERT( fabs(exact - approx) <= 0.01 );
    
    return approx;
}

//-------------------------------------------------------------------

/*
	damping is usually 1.0 (critically damped)
	frequency is usually in the range 1..16
		freq has units [1/time]
		damping is unitless
*/
float ComputePDAccel(const float fmValue,const float fmVelocity,
					const float toValue,const float toVelocity,
					const float frequency, const float damping,
					const float dt )
{
	ASSERT( fisvalid(fmValue) );
	ASSERT( fisvalid(fmVelocity) );
	
	const float ks = fsquare(frequency)*(36.f);
	const float kd = frequency*damping*(9.f);

	// scale factor for implicit integrator :
	//	usually just slightly less than one
	const float scale = 1.0f / ( 1.0f  + kd* dt + ks*dt*dt );

	const float ksI = ks  *  scale;
	const float kdI = ( kd + ks* dt ) * scale;

	return ksI*(toValue-fmValue) + kdI*(toVelocity-fmVelocity);
}

void ApplyPD(	float * pValue,float * pVelocity,
					const float toValue,const float toVelocity,
					const float frequency, const float damping,
					const float dt )
{
	float a = ComputePDAccel(*pValue,*pVelocity,toValue,toVelocity,frequency,damping,dt);
	
	// step value with the velocity from the end of the frame :
	*pVelocity	+= a * dt;
	*pValue		+= (*pVelocity) * dt;
}

// a "Standard" PD has damping = 1.0 and toVelocity = 0.0
void ApplyPDStandard(	float * pValue,float * pVelocity,
					const float toValue,
					const float frequency,
					const float dt )
{
	ApplyPD(pValue,pVelocity,toValue,0.f,frequency,1.f,dt);
}

void ApplyDampedDrive(
					float * pValue,
					float * pVelocity,
					const float toValue,
					const float frequency,
					const float dt )
{
	ASSERT( fisvalid(*pValue) );
	ASSERT( fisvalid(*pVelocity) );
	// exact solution :
	
	// arbitrary scaling of frequency to match what Dave Wu does in his PD stuff :
	const float omega = frequency * 3.f;
	
	// step from time "0" to time "dt"
	
	const float x0 = *pValue;
	const float v0 = *pVelocity;
	const float x1 = toValue;
	
	const float odt = omega * dt;
	
	const float expp = expf( - odt );
	
	const float xd = x0 - x1;
	
	const float endValue = x1 + ( v0*dt + xd*(1.f + odt) ) * expp;
	const float endVelocity = ( v0 - odt * (v0 + omega * xd ) ) * expp;
	
	*pValue = endValue;
	*pVelocity = endVelocity;
}

void ApplyDampedDriveClamping(
					float * pValue,
					float * pVelocity,
					const float toValue,
					const float frequency,
					const float minVelocity,
					const float maxVelocity,
					const float minStep,
					const float dt )
{
	ASSERT( fisvalid(*pValue) );
	ASSERT( fisvalid(*pVelocity) );
	// exact solution :
	
	// arbitrary scaling of frequency to match what Dave Wu does in his PD stuff :
	const float omega = frequency * 3.f;
	
	// step from time "0" to time "dt"
	
	const float x0 = *pValue;
	const float v0 = fclamp(*pVelocity, -maxVelocity,maxVelocity);
	const float x1 = toValue;
	
	const float odt = omega * dt;
	
	const float expp = expf( - odt );
	
	const float xd = x0 - x1;
	
	const float endValue = x1 + ( v0*dt + xd*(1.f + odt) ) * expp;
	const float endVelocity = ( v0 - odt * (v0 + omega * xd ) ) * expp;
	
	const float minVelocityStep = minVelocity * dt;
	
	if ( fabsf(xd) <= minStep || fabsf(xd) <= minVelocityStep )
	{
		*pValue = toValue;
		*pVelocity = fclamp(endVelocity, -maxVelocity,maxVelocity);
		if ( fabsf(*pVelocity) <= minVelocity )
		{
			*pVelocity = 0.f;
		}
	}
	else if ( fabsf(v0) <= minVelocity && fabsf(endVelocity) <= minVelocity &&
				(v0*endVelocity) >= 0.f )
	{
		// tiny velocities, do min step
		float sign = fsign(x1 - x0); 
		*pVelocity = sign * minVelocity;
		*pValue = x0 + (*pVelocity) * dt;
		// dumb assert makes sure we went in the right direction
		ASSERT( (sign * (*pValue - x0)) >= 0.f );
		// we also shouldn't ever overshoot past the end in this branch
	}
	else
	{			
		*pValue = endValue;
		*pVelocity = fclamp(endVelocity, -maxVelocity,maxVelocity);
	}
	
}

float LerpedDrive(const float from, const float to, const float speed, const float time_step)
{
	const float step = speed * time_step;        
	const float delta = to - from;
	if ( delta >= 0.f )
	{
		if ( delta <= step )
		{
			return to;
		}
		else
		{
			return from + step;
		}
	}
	else
	{
		if ( delta >= -step )
		{
			return to;
		}
		else
		{
			return from - step;
		}
	}
}

/*
void VelocityDampedLerpedDrive(float * pVal,float *pVel, float towards, float move_time_scale,float vel_speed, float dt)
{
	float dampedDesire = DampedDrive(*pVal,towards,move_time_scale,dt);
	float newVel = dampedDesire - *pVal;
	*pVel = LerpedDrive(*pVel,newVel,vel_speed,dt);
	// don't let velocity ever be bigger than what DampedDrive would do; this
	//	ensures we don't overshoot; basically it means that we drive velocity up gradually,
	//	but in the decel phase we drive it down instantly
	if ( fabsf(*pVel) > fabsf(newVel) )
	{
		*pVel = fsign(*pVel) * fabsf(newVel); 
	}
	// and step :
	float newVal = *pVal + *pVel;
	*pVal = newVal;
}
*/

/**

Each bit in the floating-point control word corresponds to a mode of the floating-point math processor. The DFLIB.F90 module file in the ...\DF98\INCLUDE folder contains the INTEGER(2) parameters defined for the control word, as shown in the following table:

Parameter Name  Value in Hex  Description
FPCW$MCW_IC  #1000  Infinity control mask
FPCW$AFFINE  #1000  Affine infinity
FPCW$PROJECTIVE #0000  Projective infinity
FPCW$MCW_PC  #0300  Precision control mask
FPCW$64  #0300  64-bit precision
FPCW$53 #0200  53-bit precision
FPCW$24  #0000  24-bit precision
FPCW$MCW_RC #0C00  Rounding control mask
FPCW$CHOP #0C00  Truncate
FPCW$UP  #0800  Round up
FPCW$DOWN  #0400  Round down
FPCW$NEAR  #0000  Round to nearest
FPCW$MCW_EM  #003F  Exception mask
FPCW$INVALID  #0001  Allow invalid numbers
FPCW$DENORMAL  #0002  Allow denormals (very small numbers)
FPCW$ZERODIVIDE  #0004  Allow divide by zero
FPCW$OVERFLOW #0008  Allow overflow
FPCW$UNDERFLOW  #0010  Allow underflow
FPCW$INEXACT  #0020  Allow inexact precision

The control word defaults are:

53-bit precision
Round to nearest (rounding mode)
The denormal, underflow, overflow, invalid, and inexact precision exceptions are disabled (do not generate an exception). To change exception handling, you can use the /fpe compiler option or the FOR_SET_FPE routine.


**/


void SaveFPUState(FPUState * sptr)
{
	// 0 mask = change none
	sptr->word  = _controlfp(0,0);
}

void RestoreFPUState(const FPUState * sptr)
{
	// -1 = set all
	_controlfp(sptr->word,(unsigned int)-1);
}

static bool s_defaultFPU_saved = false;
static FPUState s_defaultFPU = { 0 };

void RestoreDefaultFPUState()
{
	if ( ! s_defaultFPU_saved )
	{
		lprintf("WARNING : called RestoreDefaultFPUState before Save\n");
		SaveDefaultFPUState();
	}
	RestoreFPUState(&s_defaultFPU);
}

// SaveDefaultFPUState : should call this at startup :
void SaveDefaultFPUState()
{
	// call a math routine to make clib set up fpu :
	volatile double tt;
	tt = cos(1.0);
	// now save standard fpu state :
	SaveFPUState(&s_defaultFPU);
	//RestoreDefaultFPUState();
	s_defaultFPU_saved = true;
}



void TurnOnFPUExceptions()
{
	#if 0
	
	// dmoore 9-4
	//  turn on noisy fpu exceptions
	// these FE flags mean "ignore this failure"
	#define FE_INEXACT       0x20
	#define FE_DIVBYZERO     0x04
	#define FE_UNDERFLOW     0x10
	#define FE_OVERFLOW      0x08
	#define FE_INVALID       0x01

	// init is too heavy
	//__asm fninit

	uint16 control_word;
	// the FWAIT instruction causes any pending exceptions
	//	to be processed immediately
	__asm fwait
	__asm fnstcw control_word;
	//__asm and control_word, ~(FE_DIVBYZERO | FE_INVALID)
	control_word &= ~FE_DIVBYZERO;

	// INVALID will trigger an exception any time you touch a NAN ;
	//	this is too harsh for our design at the moment, since we
	//	use NAN to indicate not-yet-initialized simple data types
	control_word &= ~FE_INVALID;

	// OVERFLOW would be nuts
	control_word &= ~FE_OVERFLOW;

	__asm fldcw control_word;

	// setting the INVALID exception causes an immediate exception,
	//	so I clear it here :
	__asm fnclex

	/*
	MCW_EM (Interrupt exception mask) 0x0008001F _EM_INVALID 
	_EM_DENORMAL
	_EM_ZERODIVIDE
	_EM_OVERFLOW
	_EM_UNDERFLOW
	_EM_INEXACT
	*/

	// masking it on means ignore it :
 	//_control87( _EM_UNDERFLOW|_EM_INEXACT|_EM_DENORMAL , MCW_EM );
	
	__asm fwait
	
	/*
	// test :
	
	float stuff;
	finvalidate(stuff);
	stuff += 1.f;
	float stuff2 = stuff;
	stuff2 *= stuff;
	
	stuff = FLT_MAX;
	stuff += 1.f;
	
	__asm fwait
	*/
	
	#endif
}

/***

ResetFPU is left over from when D3D used to
fuck up your FPU mode

not sure that's the case any more

still useful just to make sure you are in the FPU
mode you think you are

***/
void ResetFPU()
{
	// see _controlfp
	//	not sure I want to do anything here
	
	// changing precision on x64 is forbidden !

	// I should have a little class helper to Push/Pop FPU state
	//	see VideoTest , it has better stuff

	// use _clearfp too

	#ifndef CB_64
	#define FE_ROUNDING		0x0C00
	#define FE_ROUNDTOZERO	0x0C00

		#define FE_PRECISION	0x0300
		#define FE_PRECISION_64	0x0300
		#define FE_PRECISION_53	0x0200

		uint16 control_word;

	__asm fwait
	__asm fnstcw control_word;

		//const uint16 rounding = control_word & FE_ROUNDING;
	// default rounding = FPCW$NEAR  #0000  Round to nearest
	// but don't assert cuz ResetFPU may be called at any time
		//const uint16 precision = control_word & FE_PRECISION;
	// default precision = 	FPCW$53 #0200  53-bit precision
		//ASSERT( precision == FE_PRECISION_53 );

	control_word &= ~FE_ROUNDING;
	control_word |=  FE_ROUNDTOZERO;

		control_word &= ~FE_PRECISION;
		control_word |=  FE_PRECISION_64;

	__asm fldcw control_word;
	__asm fwait

		//UNUSED_PARAMETER(rounding);
		//UNUSED_PARAMETER(precision);
	#endif
}

//AT_STARTUP( ResetFPU(); );

/*
extern "C"
{
	__declspec(naked)
	void __cdecl _ftol2(void)
	{
		_asm
		{
			sub esp, 4
			fstp DWORD PTR [esp]
			cvttss2si eax, [esp]
			cdq
			add esp, 4
			ret
		}
	}
}
*/

//-----------------------------------------------------------------------------
// Newton-Rapson square root iteration constants
//-----------------------------------------------------------------------------
static const float g_SqrtNRConst[2] = {0.5f, 3.0f};

#ifdef _XBOX	

//-----------------------------------------------------------------------------
// Name: ReciprocalSqrtEstimateNR_ASM
// Desc: SSE Newton-Raphson reciprocal square root estimate, accurate to 23
//       significant bits of the mantissa
//       One Newtown-Raphson Iteration:
//          f(i+1) = 0.5 * rsqrtss(f) * (3.0 - (f * rsqrtss(f) * rsqrtss(f))
//       NOTE: rsqrtss(f) * rsqrtss(f) != rcpss(f) (presision is not maintained)
//-----------------------------------------------------------------------------
float ReciprocalSqrt( const float f )
{
    float recsqrt;
    __asm rsqrtss xmm0, f             // xmm0 = rsqrtss(f)
    __asm movss xmm1, f               // xmm1 = f
    __asm mulss xmm1, xmm0            // xmm1 = f * rsqrtss(f)
    __asm movss xmm2, g_SqrtNRConst+4 // xmm2 = 3.0f
    __asm mulss xmm1, xmm0            // xmm1 = f * rsqrtss(f) * rsqrtss(f)
    __asm mulss xmm0, g_SqrtNRConst   // xmm0 = 0.5f * rsqrtss(f)
    __asm subss xmm2, xmm1            // xmm2 = 3.0f -
                                      //  f * rsqrtss(f) * rsqrtss(f)
    __asm mulss xmm0, xmm2            // xmm0 = 0.5 * rsqrtss(f)
                                      //  * (3.0 - (f * rsqrtss(f) * rsqrtss(f))
    __asm movss recsqrt, xmm0         // return xmm0
    return recsqrt;
}

#else

float ReciprocalSqrt( const float f )
{
	return 1.f / sqrtf(f);
}

#endif


/*
// SHUFFLE macro sets up the arg for shufps
//	the four slots are the four dest locations
//	the index in each slow specieis the source
// Think of the registers as wzyx
//	so SHUFFLE(0,1,2,3) is the identity
//	and SHUFFLE(0,0,0,0) broadcasts x
// NOTEZ : z & w are written from SRC, and x&y are written from DEST
#define SHUFFLE(w, z, y, x)     (((w)&3)<< 6|((x)&3)<<4|((y)&3)<< 2|((x)&3))

void NormalizeVec4Aligned_Approx(void * pOut,const void *pIn)
{
	__asm
	{
        mov     edx, pIn
        movaps  xmm1, [edx]
        movaps  xmm2, xmm1	// xmm2 = in

        mulps   xmm1, xmm2	// xmm1 = in*in

        movaps  xmm0, xmm1	// xmm0 = in*in

        shufps  xmm0, xmm0, 93h // 93 = SHUFFLE(2,1,0,3) ; wzyx -> zyxw , that's rotate left one
        addps   xmm1, xmm0

        shufps  xmm0, xmm0, 93h
        addps   xmm1, xmm0

        shufps  xmm0, xmm0, 93h
        addps   xmm1, xmm0  // xmm1 = { lensqr,lensqr,lensqr,lensqr }

		rsqrtss xmm0, xmm1  // xmm0 = { lensqr,lensqr,lensqr,1/len }

		shufps xmm0, xmm0, 0h	// broadcast .x

		mulps  xmm2, xmm0	// xmm2 *= 1/len

		mov		edx, pOut
        movaps  [edx], xmm2
	}
}

//static const float g_SqrtNRConst[2] = {0.5f, 3.0f};

void NormalizeVec4Aligned_NR(void * pOut,const void *pIn)
{
	__asm
	{
        mov     edx, pIn
        movaps  xmm1, [edx]
        movaps  xmm4, xmm1	// xmm4 = in

        mulps   xmm1, xmm4	// xmm1 = in*in

        movaps  xmm0, xmm1	// xmm0 = in*in

        shufps  xmm0, xmm0, 93h // 93 = SHUFFLE(2,1,0,3) ; wzyx -> zyxw , that's rotate left one
        addps   xmm1, xmm0

        shufps  xmm0, xmm0, 93h
        addps   xmm1, xmm0

        shufps  xmm0, xmm0, 93h
        addps   xmm1, xmm0  // xmm1 = { lensqr,lensqr,lensqr,lensqr }

		rsqrtss xmm0, xmm1  // xmm0 = { lensqr,lensqr,lensqr,1/len }

		// @@ do newton-raphson here if wanted

		//rsqrtss xmm0, f             // xmm0 = rsqrtss(f)
		//movss xmm1, f               // xmm1 = f
		
		mulss xmm1, xmm0            // xmm1 = f * rsqrtss(f)
		movss xmm2, g_SqrtNRConst+4 // xmm2 = 3.0f
		mulss xmm1, xmm0            // xmm1 = f * rsqrtss(f) * rsqrtss(f)
		mulss xmm0, g_SqrtNRConst   // xmm0 = 0.5f * rsqrtss(f)
		subss xmm2, xmm1            // xmm2 = 3.0f -
									//  f * rsqrtss(f) * rsqrtss(f)
		mulss xmm0, xmm2            // xmm0 = 0.5 * rsqrtss(f)
									//  * (3.0 - (f * rsqrtss(f) * rsqrtss(f))

		shufps xmm0, xmm0, 0h	// broadcast .x

		mulps  xmm4, xmm0	// xmm2 *= 1/len

		mov		edx, pOut
        movaps  [edx], xmm4
	}
}
*/

uint32	FloatAsInt(const float f)
{
	union
	{
		uint32	i;
		float	f;
	} u;

	u.f = f;
	return u.i;
}

// spatial_i & wave_k from 0 to (dim-1)
double ComputeDCTCoefficient(int spatial_i,int wave_k,int dim)
{
	if ( wave_k == 0 )
		return sqrt(1.0/dim);
	else
		return sqrt(2.0/dim) * cos( (PI/dim)*(spatial_i + 0.5)*wave_k );
}			

//=========================================================

float fsamplelerp_clamp(const float * array,const int size,const float t)
{
	if ( t <= 0.f )
		return array[0];
	if ( t >= (size-1) )
		return array[size-1];
	const int i = ftoi(t);
	ASSERT( i >= 0 && i < (size-1) );
	const float lerper = t - i;
	return flerp( array[i], array[i+1], lerper );	
}

float fsamplelerp_wrap(const float * array,const int size,const float x)
{
	float t = x;
	
	while ( t < 0.f )
		t += size;
	while ( t >= size )
		t -= size;

	const int i = ftoi(t);
	ASSERT( i >= 0 && i <= (size-1) );
	
	const float lerper = t - i;
	
	float lo = array[i];
	
	float hi = (i == size-1) ? array[0] : array[i+1];
	
	return flerp( lo, hi, lerper );	
}

float FloatTableLookupLerp( float t, const float * table, int size, bool wrap )
{
	// must make x >= 0 before we do the ftoi
	//	because the int must be <= the float
	if ( t < 0 )
	{
		if ( wrap ) { while( t < 0 ) t += size; }
		else return table[0]; //clamp
	}
	
	ASSERT( t >= 0 );
	
	int i = ftoi(t);
	float frac = t - i;
	
	if ( i >= size )
	{
		if ( wrap ) { while( i >= size ) i -= size; }
		else return table[size-1];
	}
	
	float lo = table[i];
	float hi;
	
	if ( i == size-1 )
	{
		if ( wrap ) hi = table[0];
		else return table[size-1];	
	}
	else
	{
		hi = table[i+1];
	}
	
	return flerp(lo,hi,frac);
}

// t in [0,1] interpolates from B to C ; A & D are next neighbors
float CubicSpline( float A, float B, float C, float D, float t )
{
	float cA = t * (( 2.f - t)*t - 1.f);
	float cB = t*t*(3.f*t - 5.f) + 2.f;
	float cC = t*((4.f - 3.f*t)*t + 1.f);
	float cD = t*t*(t-1.f);
	
	float out = cA * A + cB * B + cC * C + cD * D;
	
	return out * 0.5f;;
}

float FloatTableLookup( int i, const float * table, int size, bool wrap )
{
	if ( wrap )
	{
		while ( i < 0 ) i += size;
		while ( i >= size ) i -= size;
		return table[i];
	}
	else
	{
		i = Clamp(i,0,size-1);
		return table[i];
	}
}

float FloatTableLookupCubic( float x, const float * table, int size, bool wrap )
{
	// must make x >= 0 before we do the ftoi
	//	because the int must be <= the float
	if ( x < 0 )
	{
		if ( wrap ) { while( x < 0 ) x += size; }
		else return table[0]; //clamp
	}
	
	ASSERT( x >= 0 );
	
	int i = ftoi(x);
	float frac = x - i;
	
	float A = FloatTableLookup(i-1,table,size,wrap);
	float B = FloatTableLookup(i  ,table,size,wrap);
	float C = FloatTableLookup(i+1,table,size,wrap);
	float D = FloatTableLookup(i+2,table,size,wrap);

	return CubicSpline(A,B,C,D,frac);
}

END_CB


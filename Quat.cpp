#include "cblib/Base.h"
#include "cblib/Quat.h"
#include "cblib/Vec3U.h"
#include "cblib/Mat3Util.h"
#include "cblib/Mat3.h"

START_CB

/*********************

Quick Quat cheat sheet :

	 Q and -Q represent the same rotation
	 a Rotation is 3 continuous degrees of freedom (two angles, one magnitude), mudolo one bit
	 we see this in the quat because we have four floats,
		one of which is fully determined by normalization,
	  and the net sign of the quaternion gives a double-cover of
		the rotation group

	Q is :
		{ axis * sin(angle/2) , cos(angle/2) }

	negating m_v OR m_w flips the rotation to -angle
	(negating both gives the same rotation)

	{x,y,z,w}
	{0,0,0,1} and {0,0,0,-1} are both the identity

	{1,0,0,0} is a 180 degree rotation around X

	negative the entire quat is the same as swapping :
		{axis,angle}
	for
		{-axis, 2pi - angle}
	which is equivalent to 
		{-axis,-angle}

*************************/

// statics :
/*static*/ const Quat Quat::identity(Quat::eIdentity);

//}{------------------------------------------------------------------------

float Quat::DifferenceSqr(const Quat & q) const
{
	ASSERT( IsValid() && q.IsValid() );
	return DistanceSqr(m_v,q.m_v) + fsquare(m_w - q.m_w);
}

//}{------------------------------------------------------------------------
// Conversions to & from vectors

void Quat::SetFromAxisAngle(const Vec3 & axis, const float angle)
{
	/*
	// actually, no harm in just doing it for small angles :
	if ( fabsf(angle) < EPSILON )
	{
		SetIdentity();
	}
	else
	*/
	{
		ASSERT( axis.IsNormalized() );

		float sin,cos;
		fsincos(angle*0.5f,&sin,&cos);

		m_v = axis;
		m_v *= sin;
		m_w = cos;

		ASSERT( IsNormalized() );
	}
}

void Quat::GetAxisAngle(Vec3 * pAxis, float * pAngle) const
{
	ASSERT( pAxis && pAngle );
	ASSERT( IsNormalized() );

	// just take acos of m_w and normalize the axis
	*pAngle = GetAngle();

	*pAxis = m_v;
	pAxis->NormalizeSafe();
}

void Quat::GetAxisAngleMod2Pi(Vec3 * pAxis, float * pAngle) const
{
	ASSERT( pAxis && pAngle );
	ASSERT( IsNormalized() );

	// just take acos of m_w and normalize the axis
	*pAngle = GetAngle();

	*pAxis = m_v;
	pAxis->NormalizeSafe();

	// ? is this the same as negating the quat ?
	//	yes it is
	if ( *pAngle > PIf )
	{
		*pAngle = TWO_PIf - *pAngle;
		*pAxis *= -1.f;
	}
}

/*! a "ScaledVector" is a rotation axis, whose length is
	the angle
 the nice thing about this is that it's a minimal
	representation of a rotation, and it has no axis-flip
	degeneracy (flipped axis and flipped angle gives the
	same ScaledVector), so "Q" and "-Q" give the same ScaledVector

 "ScaledVector" seems like a pretty good way to interpolate
	 rotations, and it's even more compact than a quat !
*/

void Quat::SetFromScaledVector(const Vec3 & v) // this is "Exp"
{
	m_v = v;
	const float angle = m_v.NormalizeSafe();
	float sin,cos;
	fsincos(angle*0.5f,&sin,&cos);
	m_w = cos;
	m_v *= sin;
	ASSERT( IsNormalized() );
}

void Quat::GetScaledVector(Vec3 * pV) const // this is "Log"
{
	ASSERT( IsNormalized() );
	ASSERT( pV );

	float angle;
	GetAxisAngleMod2Pi(pV,&angle);
	*pV *= angle;
}

void Quat::SetXRotation(const float angle)
{
	float sin,cos;
	fsincos(angle*0.5f,&sin,&cos);

	m_v = Vec3(sin,0,0);
	m_w = cos;
}
void Quat::SetYRotation(const float angle)
{
	float sin,cos;
	fsincos(angle*0.5f,&sin,&cos);

	m_v = Vec3(0,sin,0);
	m_w = cos;
}
void Quat::SetZRotation(const float angle)
{
	float sin,cos;
	fsincos(angle*0.5f,&sin,&cos);

	m_v = Vec3(0,0,sin);
	m_w = cos;

	ASSERT( IsNormalized() );
}


//}{------------------------------------------------------------------------
// Rotate

/*!

 Quat::Rotate
 this is quite a bit more math an a Mat3-vector multiply
	is that correct?  This is roughly the conversion to a Mat3
	followed by a vector-Mat3 multiply !

q.Rotate : 89 clocks
q.GetMatrix : 97 clocks
m.Rotate : 44 clocks

*/

const Vec3 Quat::Rotate(const Vec3 & fm) const
{
	ASSERT( IsValid() && fm.IsValid() );

	// we're basically making the matrix and applying it to the vector
	// 18 muls

    const float xs = m_v.x * 2.f;
    const float ys = m_v.y * 2.f;
    const float zs = m_v.z * 2.f;

    const float wx = m_w * xs;
    const float wy = m_w * ys;
    const float wz = m_w * zs;

    const float xx = m_v.x * xs;
    const float yy = m_v.y * ys;
    const float zz = m_v.z * zs;
    const float xy = m_v.x * ys;
    const float xz = m_v.x * zs;
    const float yz = m_v.y * zs;

	return Vec3(
			 (1 - yy - zz) * fm.x +     (xy - wz) * fm.y +     (xz + wy) * fm.z,
				 (xy + wz) * fm.x + (1 - xx - zz) * fm.y +     (yz - wx) * fm.z,
				 (xz - wy) * fm.x +     (yz + wx) * fm.y + (1 - xx - yy) * fm.z
			);
}

const Vec3 Quat::RotateUnnormalized(const Vec3 & fm) const
{
	ASSERT( IsValid() && fm.IsValid() );

	// divide but no sqrt :
	const float scale = 2.f / LengthSqr();

    const float xs = m_v.x * scale;
    const float ys = m_v.y * scale;
    const float zs = m_v.z * scale;

    const float wx = m_w * xs;
    const float wy = m_w * ys;
    const float wz = m_w * zs;

    const float xx = m_v.x * xs;
    const float yy = m_v.y * ys;
    const float zz = m_v.z * zs;
    const float xy = m_v.x * ys;
    const float xz = m_v.x * zs;
    const float yz = m_v.y * zs;

	return Vec3(
			 (1 - yy - zz) * fm.x +     (xy - wz) * fm.y +     (xz + wy) * fm.z,
				 (xy + wz) * fm.x + (1 - xx - zz) * fm.y +     (yz - wx) * fm.z,
				 (xz - wy) * fm.x +     (yz + wx) * fm.y + (1 - xx - yy) * fm.z
			);
}

//}{------------------------------------------------------------------------
// GetMatrix

void Quat::GetMatrix(Mat3 * pInto) const
{
	ASSERT( pInto );
	ASSERT( IsValid() );
	ASSERT( IsNormalized() );

	// derive this just by writing out the
	// formula in Rotate() and factoring out the
	//	terms that take fm.* to to.*

    const float xs = m_v.x * 2.f;
    const float ys = m_v.y * 2.f;
    const float zs = m_v.z * 2.f;

    const float wx = m_w * xs;
    const float wy = m_w * ys;
    const float wz = m_w * zs;

    const float xx = m_v.x * xs;
    const float yy = m_v.y * ys;
    const float zz = m_v.z * zs;
    const float xy = m_v.x * ys;
    const float xz = m_v.x * zs;
    const float yz = m_v.y * zs;

	pInto->SetRowX( 1 - yy - zz,     xy - wz,     xz + wy );
	pInto->SetRowY(     xy + wz, 1 - xx - zz,     yz - wx );
	pInto->SetRowZ(     xz - wy,     yz + wx, 1 - xx - yy );
	
	ASSERT(pInto->IsOrthonormal());
}

void Quat::GetMatrixUnNormalized(Mat3 * pInto) const
{
	ASSERT( pInto );
	ASSERT( IsValid() );

	// derive this just by writing out the
	// formula in Rotate() and factoring out the
	//	terms that take fm.* to to.*

	const float scale = 2.f / LengthSqr();

    const float xs = m_v.x * scale;
    const float ys = m_v.y * scale;
    const float zs = m_v.z * scale;

    const float wx = m_w * xs;
    const float wy = m_w * ys;
    const float wz = m_w * zs;

    const float xx = m_v.x * xs;
    const float yy = m_v.y * ys;
    const float zz = m_v.z * zs;
    const float xy = m_v.x * ys;
    const float xz = m_v.x * zs;
    const float yz = m_v.y * zs;

	pInto->SetRowX( 1 - yy - zz,     xy - wz,     xz + wy );
	pInto->SetRowY(     xy + wz, 1 - xx - zz,     yz - wx );
	pInto->SetRowZ(     xz - wy,     yz + wx, 1 - xx - yy );

	ASSERT(pInto->IsOrthonormal());
}

//}{------------------------------------------------------------------------

/*
static void SetFromMatrixSub(Quat * pQ,const Mat3 & m,const float trace,
								int a0,int a1,int b0,int b1,int c0,int c1)
{
	ASSERT( pQ );
	ASSERT( a0 != a1 && b0 != b1 && c0 != c1 );

	const float tsqrt = sqrtf_safe(trace);

	const float w = 0.5f * tsqrt;

	const float s = 0.5f / tsqrt;

	const float i = s * ( m.GetElement(a0,a1) - m.GetElement(a1,a0) );
	const float j = s * ( m.GetElement(b0,b1) - m.GetElement(b1,b0) );
	const float k = s * ( m.GetElement(c0,c1) - m.GetElement(c1,c0) );

	pQ->SetComponents(i,j,k,-w);
	
	ASSERT( pQ->IsValid() );
	ASSERT( pQ->IsNormalized() );
}

void Quat::SetFromMatrix(const Mat3 & m)
{
	// only on orthonormal matrices :
	ASSERT( m.IsOrthonormal() );

	// basically look at the Mat3 and find the
	//	chunk of elements with the largest values
	//	so that we have the most precision,
	//	then use SetFromMatrixSub

	const Vec3 diagonal( GetDiagonal(m) );

	float trace;
	static const float kMinTrace = 0.0625f;

    trace = diagonal.x + diagonal.y + diagonal.z + 1;
    if (trace > kMinTrace)
	{
		SetFromMatrixSub(this,m,trace, 1,2,2,0,0,1);
		return;
	}

    trace = -diagonal.x - diagonal.y + diagonal.z + 1;
    if (trace > kMinTrace)
	{
		SetFromMatrixSub(this,m,trace, 2,0,2,1,0,1);
		return;
	}

    trace = -diagonal.x + diagonal.y - diagonal.z + 1;
    if (trace > kMinTrace)
	{
		SetFromMatrixSub(this,m,trace, 1,0,2,1,2,0);
		return;
	}

    trace = diagonal.x - diagonal.y - diagonal.z + 1;
    ASSERT(trace > kMinTrace);
	{
		SetFromMatrixSub(this,m,trace, 1,0,2,0,1,2);
		return;
	}
}
*/

void Quat::SetFromMatrix(const Mat3 & m)
{
	// only on orthonormal matrices :
	ASSERT( m.IsOrthonormal(EPSILON*10.f) );

	const float trace = m.GetTrace();
	if ( trace > 0.f )
	{
		const float s = sqrtf( 1.f + trace );

		m_w = s * 0.5f;

		const float scale = 0.5f / s;
		m_v.x = scale * ( m[1][2] - m[2][1] );
		m_v.y = scale * ( m[2][0] - m[0][2] );
		m_v.z = scale * ( m[0][1] - m[1][0] );
	}
	else
	{
		// find the largest trace element :
		int i = 0;
		if ( m[1][1] > m[0][0] ) i = 1;
		if ( m[2][2] > m[i][i] ) i = 2;

		static const int c_next[3] = { 1,2,0 };
		const int j = c_next[i];
		const int k = c_next[j];

		const float s = sqrtf( 1.f + m[i][i] - m[j][j] - m[k][k] );

		m_v[i] = s * 0.5f;

		const float scale = 0.5f / s;

		m_w    = scale * ( m[j][k] - m[k][j] );
		m_v[j] = scale * ( m[i][j] + m[j][i] );
		m_v[k] = scale * ( m[i][k] + m[k][i] );
	}

	// this has the wrong w convention ?
	m_w *= -1.f;

	ASSERT( IsNormalized() );
}

//}{------------------------------------------------------------------------

END_CB

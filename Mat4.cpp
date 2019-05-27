#include "cblib/Base.h"
#include "Mat4.h"

START_CB

//#pragma gPragmaMessage("need a Mat4Util eventually")

/*static*/ const Mat4 Mat4::identity(Mat4::eIdentity);
/*static*/ const Mat4 Mat4::zero(Mat4::eZero);

//=================================================================================

//@@ move this rot to Mat4Util

//! set pM to |left><right|
//! then M |right> = |left> (right^2)
void Mat4::SetOuterProduct(const Vec4 & left,const Vec4 & right)
{
	ASSERT( left.IsValid() && right.IsValid() );

	for(int r=0;r<4;r++)
	{
		for(int c=0;c<4;c++)
		{
			Element(r,c) = left[r] * right[c];
		}
	}
}

float Mat4::GetInnerProduct(const Vec4 & left,const Vec4 & right) const
{
	return left * Multiply(right);
}

float Mat4::GetInnerProduct(const Vec3 & left,const Vec3 & right) const
{
	//@@ lazy implementation
	return Vec4(left,1.f) * Multiply( Vec4(right,1.f) );
}

//! a "TRS" can be represented as a Frame3Scaled
bool Mat4::IsTRS() const
{
	if ( ! fiszero(m_x.w) || ! fiszero(m_y.w) || ! fiszero(m_z.w) )
		return false;

	float scale = GetUniformScale();
	float invScale = 1.f / scale;

	Mat3 mat;
	mat.SetColumnX( m_x.GetVec3() * invScale );
	mat.SetColumnY( m_y.GetVec3() * invScale );
	mat.SetColumnZ( m_z.GetVec3() * invScale );

	return mat.IsOrthonormal();
}

void Mat4::GetFrame3Scaled(Frame3Scaled * pXFS) const
{
	ASSERT( IsValid() );
	float scale = GetUniformScale();
	pXFS->SetTranslation( GetTranslation() );
	pXFS->SetScale( scale );
	float invScale = 1.f / scale;
	pXFS->MutableMatrix().SetColumnX( m_x.GetVec3() * invScale );
	pXFS->MutableMatrix().SetColumnY( m_y.GetVec3() * invScale );
	pXFS->MutableMatrix().SetColumnZ( m_z.GetVec3() * invScale );
	ASSERT( pXFS->GetMatrix().IsOrthonormal() );
}


//=================================================================================

//=================================================================================

#if 0 //{

// tests :

#include "Renderer/gConversionsD3D.h"
#include "cblib/Mat3Util.h"
#include "Core/Log.h"
#include <stdio.h>

#define MATSAME(m1,m2)	(memcmp(&(m1),&(m2),sizeof(Mat4))==0)
#define V4SAME(m1,m2)	(memcmp(&(m1),&(m2),sizeof(Vec4))==0)

static const Vec4 RandomUnit4()
{
	return Vec4(
		funitrand(),
		funitrand(),
		funitrand(),
		funitrand() );
}

static const Mat4 RandomMat4()
{
	Mat3 rot;
	SetRandomRotation(&rot);
	
	Vec4 v4 = RandomUnit4();
	
	return Mat4(rot,v4.GetVec3(),v4[3]);
}

void Mat4_Test()
{
	srand(0);

	for(int count=100;count--;)
	{
		{
			Mat4 id(Mat4::eIdentity);
			ASSERT( MATSAME(id,Mat4::identity) );

			Vec4 in = RandomUnit4();
			Vec4 out = id * in;
			ASSERT( V4SAME(in,out) );
		}

		{
			Mat3 rot;
			SetRandomRotation(&rot);
			Mat4 m4(rot);

			Vec3 v3;
			SetRandomNormal(&v3);
			Vec4 v4(v3,0.f);

			Vec3 v3out = rot * v3;
			Vec4 v4out = m4 * v4;

			ASSERT( Vec3::Equals(v3out, v4out.GetVec3()) );

			Vec3 trans(10.f,5.f,1.f);
	
			// check the rot/trans 4x4 multiply
			Mat4 m4t(rot,trans);
			Vec4 v4out_t0 = m4t * Vec4(v3,0.f);
			Vec4 v4out_t1 = m4t * Vec4(v3,1.f);

			ASSERT( Vec3::Equals(v3out, v4out_t0.GetVec3()) );
			ASSERT( Vec3::Equals(v3out + trans, v4out_t1.GetVec3()) );

			Frame3 f3(rot,trans);
			Vec3 v3t = f3 * v3;

			ASSERT( Vec3::Equals(v3t, v4out_t1.GetVec3()) );

			// check byte-wise compatability with D3D :
			D3DXMATRIX d3dm;
			gConversionsD3D::D3DMat(f3,&d3dm);

			ASSERT( MATSAME(d3dm,m4t) );
		}
		
		// check matrix multiples :
		{
			const Mat4 m1(RandomMat4());
			const Mat4 m2(RandomMat4());

			Mat4 m3b;
			m3b.SetProductBrute(m1,m2);

			Mat4 m3;
			m3.SetProduct(m1,m2);

			const D3DXMATRIX * pm1 = (const D3DXMATRIX *) &m1;
			const D3DXMATRIX * pm2 = (const D3DXMATRIX *) &m2;

			// this does use SSE
			D3DXMATRIX out;
			D3DXMatrixMultiply(&out,pm2,pm1); //note reversal!!

			ASSERT( Mat4::Equals(m3,m3b) );
			ASSERT( Mat4::Equals(m3,reinterpret_cast<Mat4 &>(out)) );
		}
		
		// check matrix multiples with this :
		{
			const Mat4 m1(RandomMat4());
			const Mat4 m2(RandomMat4());
			
			Mat4 m3;
			m3.SetProduct(m1,m2);

			Mat4 m3_l(m1);
			m3_l.SetProduct(m3_l,m2);
			
			Mat4 m3_r(m2);
			m3_r.SetProduct(m1,m3_r);
			
			ASSERT( Mat4::Equals(m3,m3_l) );
			ASSERT( Mat4::Equals(m3,m3_r) );
		}
		
		// test operator []
		{
			Mat4 id(Mat4::eIdentity);

			ASSERT( id[0][0] == 1.f );
			ASSERT( id[2][2] == 1.f );
			ASSERT( id[2][1] == 0.f );
			
			Vec3 trans(10.f,5.f,1.f);

			id.SetColumnW( Vec4(trans,1.f) );

			ASSERT( id[0][3] == 10.f );
			ASSERT( id[1][3] == 5.f );
			ASSERT( id[2][3] == 1.f );
		}
	}

	puts("success!");
}

#include "cblib/Timer.h"

void Mat4_Time()
{
	#define REPEAT	1000000

	#pragma gPragmaMessage("need to get these guys to align!")
	//__declspec(align(16))

	const Mat4 m1(RandomMat4());
	const Mat4 m2(RandomMat4());

	volatile Mat4 m3;

	{
	ulong addr = (ulong) &m1;
	int offset = addr & 15;
	Log::Printf("m1 aligned = %s\n", (offset == 0) ? "yes" : "no");
	}

	// 334 clocks
	{
		gTimer::AutoTimer at("SetProductBrute",gTimer::eTicks,REPEAT);

		for(int count=REPEAT;count--;)
		{
			((Mat4 *)&m3)->SetProductBrute(m1,m2);
		}	
	}

	// 175 clocks
	{
		gTimer::AutoTimer at("SetProductASM",gTimer::eTicks,REPEAT);

		for(int count=REPEAT;count--;)
		{
			((Mat4 *)&m3)->SetProductASM(m1,m2);
		}	
	}
	
	// 136 clocks
	{
		gTimer::AutoTimer at("SetProductD3D",gTimer::eTicks,REPEAT);

		for(int count=REPEAT;count--;)
		{
			((Mat4 *)&m3)->SetProductD3D(m1,m2);
		}	
	}

	const Vec4 v4(RandomUnit4());

	volatile Vec4 v4t;

	{
	ulong addr = (ulong) &v4t;
	int offset = addr & 15;
	Log::Printf("v4t aligned = %s\n", (offset == 0) ? "yes" : "no");
	}

	// 63 ticks
	{
		gTimer::AutoTimer at("Multiply",gTimer::eTicks,REPEAT);

		for(int count=REPEAT;count--;)
		{
			*((Vec4 *)&v4t) = m1 * v4;
		}	
	}

	// 65 ticks
	{
		gTimer::AutoTimer at("D3DXVec4Transform",gTimer::eTicks,REPEAT);

		for(int count=REPEAT;count--;)
		{
			D3DXVec4Transform( (D3DXVECTOR4 *) &v4t, (const D3DXVECTOR4 *) &v4, (const D3DXMATRIX *) &m1 );
		}	
	}
}

#endif //} // tests

//=================================================================================

#if 0

#include <d3dx8math.h>
//#include "Core/XG.h"

#ifdef _DEBUG
#pragma comment(lib,"d3dx8d.lib")
#else
#pragma comment(lib,"d3dx8.lib")
#endif

float Mat4::GetDeterminant() const
{
#ifdef _XBOX
	return XGMatrixfDeterminant( (const XGMATRIX *)this );
#else
	return D3DXMatrixfDeterminant( (const D3DXMATRIX *)this );
#endif
}

void Mat4::GetTranspose(Mat4 * pMat) const
{
#ifdef _XBOX
	XGMatrixTranspose( (XGMATRIX *)pMat, (const XGMATRIX *)this );
#else
	D3DXMatrixTranspose( (D3DXMATRIX *)pMat, (const D3DXMATRIX *)this );
#endif
}

float Mat4::GetInverse(Mat4 * pMat) const
{
	float det;
	
#ifdef _XBOX
	if ( XGMatrixInverse( (XGMATRIX *)pMat, &det, (const XGMATRIX *)this ) == NULL )
		return 0.f;
	else
		return det;
#else
	if ( D3DXMatrixInverse( (D3DXMATRIX *)pMat, &det, (const D3DXMATRIX *)this ) == NULL )
		return 0.f;
	else
		return det;
#endif

}

void Mat4::SetProduct(const Mat4 &m1,const Mat4 &m2)
{
	// SetMultiple : this = m1 * m2
	// with D3DXMatrixMultiply, either m1 or m2 can be "this" and it's fine!
#ifdef _XBOX
	XGMatrixMultiply( (XGMATRIX *)this, (const XGMATRIX *)&m2, (const XGMATRIX *)&m1 );
#else
	D3DXMatrixMultiply( (D3DXMATRIX *)this, (const D3DXMATRIX *)&m2, (const D3DXMATRIX *)&m1 );
#endif
}

#endif // D3D calls

static __forceinline void RawProduct(float * m3,const float * m1,const float * m2)
{
    m3[0]=m1[0]*m2[0]+m1[1]*m2[4]+m1[2]*m2[8]+m1[3]*m2[12];
    m3[1]=m1[0]*m2[1]+m1[1]*m2[5]+m1[2]*m2[9]+m1[3]*m2[13];
    m3[2]=m1[0]*m2[2]+m1[1]*m2[6]+m1[2]*m2[10]+m1[3]*m2[14];
    m3[3]=m1[0]*m2[3]+m1[1]*m2[7]+m1[2]*m2[11]+m1[3]*m2[15];
    m3[4]=m1[4]*m2[0]+m1[5]*m2[4]+m1[6]*m2[8]+m1[7]*m2[12];
    m3[5]=m1[4]*m2[1]+m1[5]*m2[5]+m1[6]*m2[9]+m1[7]*m2[13];
    m3[6]=m1[4]*m2[2]+m1[5]*m2[6]+m1[6]*m2[10]+m1[7]*m2[14];
    m3[7]=m1[4]*m2[3]+m1[5]*m2[7]+m1[6]*m2[11]+m1[7]*m2[15];
    m3[8]=m1[8]*m2[0]+m1[9]*m2[4]+m1[10]*m2[8]+m1[11]*m2[12];
    m3[9]=m1[8]*m2[1]+m1[9]*m2[5]+m1[10]*m2[9]+m1[11]*m2[13];
    m3[10]=m1[8]*m2[2]+m1[9]*m2[6]+m1[10]*m2[10]+m1[11]*m2[14];
    m3[11]=m1[8]*m2[3]+m1[9]*m2[7]+m1[10]*m2[11]+m1[11]*m2[15];
    m3[12]=m1[12]*m2[0]+m1[13]*m2[4]+m1[14]*m2[8]+m1[15]*m2[12];
    m3[13]=m1[12]*m2[1]+m1[13]*m2[5]+m1[14]*m2[9]+m1[15]*m2[13];
    m3[14]=m1[12]*m2[2]+m1[13]*m2[6]+m1[14]*m2[10]+m1[15]*m2[14];
    m3[15]=m1[12]*m2[3]+m1[13]*m2[7]+m1[14]*m2[11]+m1[15]*m2[15];
}

void Mat4::SetProduct(const Mat4 &m1,const Mat4 &m2)
{	
	// this version can't work on self at all :
	//ASSERT( &m1 != this && &m2 != this );
	//RawProduct(GetData(),m1.GetData(),m2.GetData());
	float m3[16];
	RawProduct(m3,m1.GetData(),m2.GetData());
	float * dest = GetData();
	dest[0 ] = m3[0 ];
	dest[1 ] = m3[1 ];
	dest[2 ] = m3[2 ];
	dest[3 ] = m3[3 ];
	dest[4 ] = m3[4 ];
	dest[5 ] = m3[5 ];
	dest[6 ] = m3[6 ];
	dest[7 ] = m3[7 ];
	dest[8 ] = m3[8 ];
	dest[9 ] = m3[9 ];
	dest[10] = m3[10];
	dest[11] = m3[11];
	dest[12] = m3[12];
	dest[13] = m3[13];
	dest[14] = m3[14];
	dest[15] = m3[15];
}


float Mat4::GetDeterminant() const
{
	double value =
		m_x.w * m_y.z * m_z.y * m_w.x-m_x.z * m_y.w * m_z.y * m_w.x-m_x.w * m_y.y * m_z.z * m_w.x+m_x.y * m_y.w * m_z.z * m_w.x+
		m_x.z * m_y.y * m_z.w * m_w.x-m_x.y * m_y.z * m_z.w * m_w.x-m_x.w * m_y.z * m_z.x * m_w.y+m_x.z * m_y.w * m_z.x * m_w.y+
		m_x.w * m_y.x * m_z.z * m_w.y-m_x.x * m_y.w * m_z.z * m_w.y-m_x.z * m_y.x * m_z.w * m_w.y+m_x.x * m_y.z * m_z.w * m_w.y+
		m_x.w * m_y.y * m_z.x * m_w.z-m_x.y * m_y.w * m_z.x * m_w.z-m_x.w * m_y.x * m_z.y * m_w.z+m_x.x * m_y.w * m_z.y * m_w.z+
		m_x.y * m_y.x * m_z.w * m_w.z-m_x.x * m_y.y * m_z.w * m_w.z-m_x.z * m_y.y * m_z.x * m_w.w+m_x.y * m_y.z * m_z.x * m_w.w+
		m_x.z * m_y.x * m_z.y * m_w.w-m_x.x * m_y.z * m_z.y * m_w.w-m_x.y * m_y.x * m_z.z * m_w.w+m_x.x * m_y.y * m_z.z * m_w.w;
	return value;
}

void Mat4::GetTranspose(Mat4 * pMat) const
{
	ASSERT( pMat != this );
	pMat->m_x.x = m_x.x ; 
	pMat->m_x.y = m_y.x ; 
	pMat->m_x.z = m_z.x ; 
	pMat->m_x.w = m_w.x ; 
	pMat->m_y.x = m_x.y ; 
	pMat->m_y.y = m_y.y ; 
	pMat->m_y.z = m_z.y ; 
	pMat->m_y.w = m_w.y ; 
	pMat->m_z.x = m_x.z ; 
	pMat->m_z.y = m_y.z ; 
	pMat->m_z.z = m_z.z ; 
	pMat->m_z.w = m_w.z ; 
	pMat->m_w.x = m_x.w ; 
	pMat->m_w.y = m_y.w ; 
	pMat->m_w.z = m_z.w ; 
	pMat->m_w.w = m_w.w ; 
}

float Mat4::GetInverse(Mat4 * pMat) const
{
	float det = GetDeterminant();
	if ( det == 0.f )
		return 0.f;

	float scale = 1.f/det;
	float m00 = m_x.x;	float m10 = m_y.x;	float m20 = m_z.x;	float m30 = m_w.x; 
	float m01 = m_x.y;	float m11 = m_y.y;	float m21 = m_z.y;	float m31 = m_w.y;
	float m02 = m_x.z;	float m12 = m_y.z;	float m22 = m_z.z;	float m32 = m_w.z;
	float m03 = m_x.w;	float m13 = m_y.w;	float m23 = m_z.w;	float m33 = m_w.w;

    pMat->m_x.x = ( m12*m23*m31 - m13*m22*m31 + m13*m21*m32 - m11*m23*m32 - m12*m21*m33 + m11*m22*m33 ) * scale;    
    pMat->m_x.y = ( m03*m22*m31 - m02*m23*m31 - m03*m21*m32 + m01*m23*m32 + m02*m21*m33 - m01*m22*m33 ) * scale;    
    pMat->m_x.z = ( m02*m13*m31 - m03*m12*m31 + m03*m11*m32 - m01*m13*m32 - m02*m11*m33 + m01*m12*m33 ) * scale;    
    pMat->m_x.w = ( m03*m12*m21 - m02*m13*m21 - m03*m11*m22 + m01*m13*m22 + m02*m11*m23 - m01*m12*m23 ) * scale;    
    pMat->m_y.x = ( m13*m22*m30 - m12*m23*m30 - m13*m20*m32 + m10*m23*m32 + m12*m20*m33 - m10*m22*m33 ) * scale;    
    pMat->m_y.y = ( m02*m23*m30 - m03*m22*m30 + m03*m20*m32 - m00*m23*m32 - m02*m20*m33 + m00*m22*m33 ) * scale;    
    pMat->m_y.z = ( m03*m12*m30 - m02*m13*m30 - m03*m10*m32 + m00*m13*m32 + m02*m10*m33 - m00*m12*m33 ) * scale;    
    pMat->m_y.w = ( m02*m13*m20 - m03*m12*m20 + m03*m10*m22 - m00*m13*m22 - m02*m10*m23 + m00*m12*m23 ) * scale;    
    pMat->m_z.x = ( m11*m23*m30 - m13*m21*m30 + m13*m20*m31 - m10*m23*m31 - m11*m20*m33 + m10*m21*m33 ) * scale;    
    pMat->m_z.y = ( m03*m21*m30 - m01*m23*m30 - m03*m20*m31 + m00*m23*m31 + m01*m20*m33 - m00*m21*m33 ) * scale;    
    pMat->m_z.z = ( m01*m13*m30 - m03*m11*m30 + m03*m10*m31 - m00*m13*m31 - m01*m10*m33 + m00*m11*m33 ) * scale;    
    pMat->m_z.w = ( m03*m11*m20 - m01*m13*m20 - m03*m10*m21 + m00*m13*m21 + m01*m10*m23 - m00*m11*m23 ) * scale;    
    pMat->m_w.x = ( m12*m21*m30 - m11*m22*m30 - m12*m20*m31 + m10*m22*m31 + m11*m20*m32 - m10*m21*m32 ) * scale;    
    pMat->m_w.y = ( m01*m22*m30 - m02*m21*m30 + m02*m20*m31 - m00*m22*m31 - m01*m20*m32 + m00*m21*m32 ) * scale;    
    pMat->m_w.z = ( m02*m11*m30 - m01*m12*m30 - m02*m10*m31 + m00*m12*m31 + m01*m10*m32 - m00*m11*m32 ) * scale;    
    pMat->m_w.w = ( m01*m12*m20 - m02*m11*m20 + m02*m10*m21 - m00*m12*m21 - m01*m10*m22 + m00*m11*m22 ) * scale;    
    
    return det;
}

END_CB

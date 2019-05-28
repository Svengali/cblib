#include "cblib/Vec3U.h"
#include "cblib/Vec2U.h"
#include "cblib/Mat3.h"
#include "cblib/Mappers.h"
#include "cblib/FloatUtil.h"

#if 0

#include "Renderer/gVertex.h"

bool ComputeUVToBaryMaker(
		const Vec2 & uv0,
		const Vec2 & uv1,
		const Vec2 & uv2,
		gBaryMaker * pMaker)
{
	// multiply [u,v,1] by Matrix and you'll get barys out
	
	Vec2 e0 = uv2 - uv1;
	Vec2 e1 = uv0 - uv2;
	Vec2 e0p = MakePerpCW(e0);
	Vec2 e1p = MakePerpCW(e1);
	float c0 = faverage( e0p * uv1 , e0p * uv2 );
	float c1 = faverage( e1p * uv0 , e1p * uv2 );
	float m0 = e0p * uv0 - c0;
	float m1 = e1p * uv1 - c1;
	
	pMaker->r[0] = e0p / m0;
	pMaker->c[0] = -c0 / m0;
	pMaker->r[1] = e1p / m1;
	pMaker->c[1] = -c1 / m1;
		
	#ifdef DO_ASSERTS
	{
		Vec2 bary0 = pMaker->Compute(uv0);
		Vec2 bary1 = pMaker->Compute(uv1);
		Vec2 bary2 = pMaker->Compute(uv2);
		ASSERT( Vec2::Equals(bary0,Vec2(1,0)) );
		ASSERT( Vec2::Equals(bary1,Vec2(0,1)) );
		ASSERT( Vec2::Equals(bary2,Vec2(0,0)) );
	}
	#endif
	
	return true;
}

bool ComputeUVToPosMatrix(
		const gVertexNDUV2 & V0,
		const gVertexNDUV2 & V1,
		const gVertexNDUV2 & V2,
		Mat3 * pMatrix)
{
	// multiply [u,v,1] by Matrix and you'll get positions out
	
	const Vec2 & uv0 = V0.uv2;
	const Vec2 & uv1 = V1.uv2;
	const Vec2 & uv2 = V2.uv2;
	
	Vec2 e0 = uv2 - uv1;
	Vec2 e1 = uv0 - uv2;
	Vec2 e2 = uv1 - uv0;
	Vec2 e0p = MakePerpCW(e0);
	Vec2 e1p = MakePerpCW(e1);
	Vec2 e2p = MakePerpCW(e2);
	float c0 = faverage( e0p * uv1 , e0p * uv2 );
	float c1 = faverage( e1p * uv0 , e1p * uv2 );
	float c2 = faverage( e2p * uv1 , e2p * uv0 );
	
	float m0 = e0p * uv0 - c0;
	float m1 = e1p * uv1 - c1;
	float m2 = e2p * uv2 - c2;
	
	Mat3 baryMatrix;
	baryMatrix.RowX() = Vec3( e0p.x, e0p.y, - c0) / m0;
	baryMatrix.RowY() = Vec3( e1p.x, e1p.y, - c1) / m1;
	baryMatrix.RowZ() = Vec3( e2p.x, e2p.y, - c2) / m2;
	
	#ifdef DO_ASSERTS
	{
		Vec3 bary0 = baryMatrix.Rotate( Vec3(uv0.x,uv0.y,1.f) );
		Vec3 bary1 = baryMatrix.Rotate( Vec3(uv1.x,uv1.y,1.f) );
		Vec3 bary2 = baryMatrix.Rotate( Vec3(uv2.x,uv2.y,1.f) );
		ASSERT( Vec3::Equals(bary0,Vec3(1,0,0)) );
		ASSERT( Vec3::Equals(bary1,Vec3(0,1,0)) );
		ASSERT( Vec3::Equals(bary2,Vec3(0,0,1)) );
	}
	#endif
	
	Mat3 posMatrix;
	posMatrix.SetColumnX( V0.pos );
	posMatrix.SetColumnY( V1.pos );
	posMatrix.SetColumnZ( V2.pos );
	
	pMatrix->SetProduct( posMatrix, baryMatrix );
	
	#ifdef DO_ASSERTS
	{
		Vec3 pos0 = pMatrix->Rotate( Vec3(uv0.x,uv0.y,1.f) );
		Vec3 pos1 = pMatrix->Rotate( Vec3(uv1.x,uv1.y,1.f) );
		Vec3 pos2 = pMatrix->Rotate( Vec3(uv2.x,uv2.y,1.f) );
		ASSERT( Vec3::Equals(pos0,V0.pos,0.01f) );
		ASSERT( Vec3::Equals(pos1,V1.pos,0.01f) );
		ASSERT( Vec3::Equals(pos2,V2.pos,0.01f) );
	}
	#endif
	
	return true;
}

bool ComputeUVToNormalVectors(
		const gVertexNDUV2 & V0,
		const gVertexNDUV2 & V1,
		const gVertexNDUV2 & V2,
		Vec3 * pU,
		Vec3 * pV)
{
	const Vec3 P10 = V1.pos - V0.pos;
	const Vec3 P20 = V2.pos - V0.pos;
	const float u10 = V1.uv2.x - V0.uv2.x;
	const float u20 = V2.uv2.x - V0.uv2.x;
	const float v10 = V1.uv2.y - V0.uv2.y;
	const float v20 = V2.uv2.y - V0.uv2.y;
	
	float denom = u10 * v20 - u20 * v10;
	
	// @@ have a small tolerance (smaller than EPSILON) ?
	if ( denom == 0.f )
		return false;
	
	float invDenom = 1.f / denom;

	pU->SetScaled( P10,  v20 * invDenom );
	pU->AddScaled( P20, -v10 * invDenom );
	
	pV->SetScaled( P10, -u20 * invDenom );
	pV->AddScaled( P20,  u10 * invDenom );
	
	float lenSqrU = pU->LengthSqr();
	float lenSqrV = pV->LengthSqr();
	
	// @@ have a small tolerance (smaller than EPSILON) ?
	if ( lenSqrU == 0.f || lenSqrV == 0.f )
		return false;
	
	// scale to invert their lengths :
	*pU *= ( 1.f / lenSqrU );
	*pV *= ( 1.f / lenSqrV );
	
	return true;
}

//----------------------------------------------------------------------------------

#endif

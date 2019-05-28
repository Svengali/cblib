#pragma once

#if 0

/***

from Galaxy :

some handy triangle uv-surface position maths

***/

class Vec2;
class Vec3;
class gVertexNDUV2;
class Mat3;
class Frame3;

//---------------------------------------------------
// UV/Pos/Bary triangle mapping routines :

// gBaryMaker converts a "uv" into a barycentric coord {b0,b1,b2}
//	(it just returns the first two and b2 = 1 - b0 - b1; )
struct gBaryMaker
{
	const Vec2	Compute(const Vec2 & uv) const
	{
		return Vec2( r[0] * uv + c[0] , r[1] * uv + c[1] );
	}
	float	Compute0(const Vec2 & uv) const
	{
		return r[0] * uv + c[0];
	}
	float	Compute1(const Vec2 & uv) const
	{
		return r[1] * uv + c[1];
	}
	
	Vec2	r[2];
	float	c[2];
};


// make a gBaryMaker from 3 uv coordinates of a triangle
bool ComputeUVToBaryMaker(
		const Vec2 & uv0,
		const Vec2 & uv1,
		const Vec2 & uv2,
		gBaryMaker * pMaker);

// create a matrix that will tranform a uv into a world-space position
//	multiply the matrix by {u,v,1} and you get a position out
bool ComputeUVToPosMatrix(
		const gVertexNDUV2 & V0,
		const gVertexNDUV2 & V1,
		const gVertexNDUV2 & V2,
		Mat3 * pMatrix);
			
// get the surface tangent vectors in world space
//	that are along the u & v texture axes
bool ComputeUVToNormalVectors(
		const gVertexNDUV2 & V0,
		const gVertexNDUV2 & V1,
		const gVertexNDUV2 & V2,
		Vec3 * pU,
		Vec3 * pV);

#endif

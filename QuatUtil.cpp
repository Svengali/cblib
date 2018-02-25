#include "cblib/Base.h"
#include "cblib/QuatUtil.h"
#include "cblib/Vec3U.h"
#include "cblib/Rand.h"
#include "cblib/Mat3Util.h"

START_CB

//namespace QuatUtil
//{


// @@ : use this instead of clib acos ?
//	-> or use the hardware atanf ?
/*
 polynomial approximations of asin() don't work well
	because its slopes are infinite at -1 and 1

	asin is ODD, so asin(-x) = - asin(x)
	asin(0) = 0
	asin'(0) = 1
	so near 0, asin(x) is nearly = x
	this is actually not bad up to like 0.5,
	 where asin(0.5) = 0.5236

 for the purposes of Slerp , we could actually use an approximate
  acos/asin JUST FOR SMALL "cos_theta" ; for large cos_theta, a
  lerp is actually pretty good, so just do that (blend it in)

*/

/*
inline static float myasin(const float x)
{
	static const float c1 = 0.892399f;
	static const float c3 = 1.693204f;
	static const float c5 =-3.853735f;
	static const float c7 = 2.838933f;

	const float x2 = x * x;
	const float d = x * (c1 + x2 * (c3 + x2 * (c5 + x2 * c7)));

	return d;
}


inline static float myacos(const float x)
{
	return HALF_PI - myasin(x);
}
*/

	void SetSlerp(Quat * pQ,
						const Quat & q1,const Quat & q2,
						const float t)
	{
		ASSERT(pQ);
		ASSERT(q1.IsValid() && q2.IsValid());

		/*
		@@ : Casey sez :

		Don't use acos for slerping if you can avoid it!  It's death warmed over
		(acos isn't a hardware op, so it does like a square-root plus an atan2 and
		it's just awful).  Use atan2 directly (since that's a Pentium op).  You
		get more info out of an atan2 anyways and your slerp routine has less
		if'ing and negating and all that crap.

		*/

		// cosine theta = dot product of A and B
		float cos_theta = q1.Component4Dot(q2);

		// if B is on opposite hemisphere from A, use -B instead
		float sign;
 		if ( cos_theta < 0.f )
		{
			cos_theta = -cos_theta;
			sign = -1.f;
		}
		else
		{
			sign = 1.f;
		}

		float c1,c2;
		if ( cos_theta > 1.f - EPSILON )
		{
			// if q2 is (within precision limits) the same as q1,
			// just linear interpolate between A and B.

			c2 = t;
			c1 = 1.f - t;
 		}
		else
		{
 			//float theta = gFloat::ArcCosTable(cos_theta);
			// faster than table-based :
 			//const float theta = myacos(cos_theta);
 			const float theta = acosf(cos_theta);
 			const float sin_theta = sinf(theta);
			const float t_theta = t*theta;
			const float inv_sin_theta = 1.f / sin_theta;
 			c2 = sinf(        t_theta) * inv_sin_theta;
 			c1 = sinf(theta - t_theta) * inv_sin_theta;
 		}

		c2 *= sign; // or c1 *= sign
						// just affects the overrall sign of the output

		// interpolate
		pQ->SetWeightedSum( c1, q1, c2, q2 );
	}

	void SetRandom(Quat * pQ)
	{
		ASSERT(pQ);
		Vec3 axis;
		SetRandomNormal(&axis);
		//float angle = Rand::GetAngle();
		float angle = frandunit() * CBPI;
		pQ->SetFromAxisAngle(axis,angle);
	}

	// SQuad
	//	is just the DeCasteljau formulation of Bezier
	//	applied to Quats using Slerp instead of Lerp
	/*! Squad : just like cubic Bezier, but made of Slerps instead of Lerps
			t = 0 gives you q0
			t = 1 gives you q3
			q1 and q2 are not passed through; they pull the intermediate quats in their direction
	*/
	void SetSphericalBezier(Quat * pQ,
		const Quat & q0,const Quat & q1,const Quat & q2,const Quat & q3,
		const float t)
	{
		ASSERT(pQ);

		Quat q01,q12,q23,q012,q123;

		SetSlerp(&q01, q0, q1, t);
		SetSlerp(&q12, q1, q2, t);
		SetSlerp(&q23, q2, q3, t);
		
		SetSlerp(&q012, q01, q12, t);
		SetSlerp(&q123, q12, q23, t);

		SetSlerp(pQ, q012, q123, t);
	}

	
	/*! SetRotationArc
		makes the "simplest" quat Q
		such that normalTo = Q * normalFm
	*/
	void SetRotationArc(Quat * pQ, const Vec3 & normalFm, const Vec3 & normalTo )
	{
		ASSERT( pQ );
		ASSERT( normalFm.IsNormalized() && normalTo.IsNormalized() );

		const float dot = normalFm * normalTo;

		// we have trouble when the vectors are nearly opposite
		if ( dot <= (-1.f + EPSILON) )
		{
			// just get any axis that's perp to Fm
			Vec3 perp1,perp2;
			GetTwoPerp(normalFm,&perp1,&perp2);
			//pQ->SetFromAxisAngle( perp1, PI );
			pQ->SetComponents(perp1,0.f);
		}
		// cbloom :
		// very large dots can still be quite big differences in angle
		// for example, cos(1 degree) = 0.999848
		// we have no degeneracy for near-parallels, so just do the work
		// simplify the case of nearly parralel
		// dot can be > 1.f because of float innacuracy
		else if ( dot >= 1.f )
		{
			pQ->SetIdentity();
		}
		else
		{
			const Vec3 cross = MakeCross(normalFm,normalTo);

			/*
			Vec3 axis = cross;
			axis.NormalizeSafe();
			const float angle = acosf(dot); // in 0 -> pi

			Matrix3 mat;

			SetRotation(&mat,axis,angle);
			const Vec3 test = mat * normalFm;
			const float dotTest = test * normalTo;
			ASSERT( dotTest >= 0.96f );

			Quat qFromMat;
			qFromMat.SetFromMatrix(mat);
			const Vec3 test2 = qFromMat.Rotate( normalFm );
			const float dotTest2 = test * normalTo;
			ASSERT( dotTest2 >= 0.96f );

			Quat qTest;
			qTest.SetFromAxisAngle(axis,angle);
			*/

			pQ->SetComponents( cross, (1.f + dot) );
			pQ->Normalize();
		}

		#ifdef DO_ASSERTS
		{
			const Vec3 result = pQ->Rotate(normalFm);
			const float dotResult = result * normalTo;
			ASSERT( dotResult >= 1.f - 2 * EPSILON );
		}
		#endif // DO_ASSERTS
	}

	void SetRotationArcUnNormalized(Quat * pQ, const Vec3 & from, const Vec3 & to )
	{
		ASSERT( pQ );
		const float fromLenSqr = from.LengthSqr();
		const float toLenSqr = to.LengthSqr();
		const float lenProduct = sqrtf( fromLenSqr * toLenSqr );
		const float dot = from * to;
		const Vec3 cross = MakeCross(from,to);
		pQ->SetComponents( cross, lenProduct + dot );
		pQ->Normalize();
	
		#ifdef DO_ASSERTS
		{
			const Vec3 result = pQ->Rotate(from);
			const float dotResult = (result * to) / lenProduct;
			ASSERT( dotResult >= 1.f - 2 * EPSILON );
		}
		#endif // DO_ASSERTS
	}

	// *pQ = q ^ power
	void SetPower(Quat * pQ,const Quat & q,const float power)
	{
		ASSERT( pQ != NULL );
		ASSERT( q.IsValid() );
		ASSERT( fisvalid(power) );
		Vec3 axis;
		float angle;
		q.GetAxisAngleMod2Pi(&axis,&angle);
		pQ->SetFromAxisAngle(axis, angle * power );
	}

//};

END_CB

/*
Quaternion fast_simple_rotation(const Vector3 &a, const Vector3 &b) {
    Vector3 axis = cross_product(a, b);
    float dot = dot_product(a, b);
    if (dot < -1.0f + DOT_EPSILON) return Quaternion(0, 1, 0, 0);
    
    Quaternion result(axis.x * 0.5f, axis.y * 0.5f, axis.z * 0.5f, 
                      (dot + 1.0f) * 0.5f);
    fast_normalize(&result);

    return result;
}
*/
#include "cblib/Base.h"
#include "cblib/Mat2.h"

START_CB

float Mat2U::GetInverse(const Mat2 & fm,Mat2 * pTo)
{
	ASSERT( fm.IsValid() );
	ASSERT( pTo != NULL );

	const float det = fm.GetDeterminant();
	if ( det == 0.f )
		return 0.f;
	const float invDet = 1.f / det;
	
	(*pTo)[0][0] =   fm[1][1] * invDet;
	(*pTo)[1][1] =   fm[0][0] * invDet;
	(*pTo)[0][1] = - fm[1][0] * invDet;
	(*pTo)[1][0] = - fm[0][1] * invDet;

	ASSERT( pTo->IsValid() );

	return det;
}

END_CB

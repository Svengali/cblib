#include "Base.h"
#include "Vec3.h"
#include "Log.h"
#include <stddef.h>

START_CB

COMPILER_ASSERT( sizeof(Vec3) == 12 );
COMPILER_ASSERT( MEMBER_OFFSET(Vec3,x) == 0 );
COMPILER_ASSERT( MEMBER_OFFSET(Vec3,y) == 4 );
COMPILER_ASSERT( MEMBER_OFFSET(Vec3,z) == 8 );

//-------------------------------------------------------------------
//statics 

const Vec3 Vec3::zero(0,0,0);
const Vec3 Vec3::unitX(1,0,0);
const Vec3 Vec3::unitY(0,1,0);
const Vec3 Vec3::unitZ(0,0,1);
const Vec3 Vec3::unitXneg(-1,0,0);
const Vec3 Vec3::unitYneg(0,-1,0);
const Vec3 Vec3::unitZneg(0,0,-1);

//-------------------------------------------------------------------

#pragma warning(disable : 4505) // unreferenced static removed

static void Vec3Test()
{
	Vec3 a(0,1,7),b(2,3,1);

	a = b;
}

//-------------------------------------------------------------------

void Vec3::Log() const //!< writes xyz to Log; does NOT add a \n !
{
	// compact display :
	//lprintf("{%1.3f,%1.3f,%1.3f}",x,y,z);
	lprintf("{%1.6ff,%1.6ff,%1.6ff}",x,y,z);
}

//-------------------------------------------------------------------
END_CB

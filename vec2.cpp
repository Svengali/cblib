#include "Base.h"
#include "Vec2.h"
#include "Log.h"

START_CB

//-------------------------------------------------------------------
//statics 

const Vec2 Vec2::zero(0,0);
const Vec2 Vec2::unitX(1,0);
const Vec2 Vec2::unitY(0,1);
const Vec2 Vec2::unitXneg(-1,0);
const Vec2 Vec2::unitYneg(0,-1);

//-------------------------------------------------------------------

#pragma warning(disable : 4505) // unreferenced static removed

static void Vec2Test()
{
	Vec2 a(0,1),b(2,3);

	a = b;
}

void Vec2::Log() const //!< writes xyz to Log; does NOT add a \n !
{
	lprintf("{%1.3f,%1.3f}",x,y);
}

//-------------------------------------------------------------------
END_CB

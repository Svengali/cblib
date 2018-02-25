#include "Base.h"
#include "cblib/Vec2.h"

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

END_CB

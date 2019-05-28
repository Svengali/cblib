#include "cblib/Vec2i.h"
#include "cblib/Log.h"

START_CB

//-------------------------------------------------------------------
//statics 

Vec2i Vec2i::zero(0,0);
Vec2i Vec2i::unitX(1,0);
Vec2i Vec2i::unitY(0,1);
Vec2i Vec2i::unitXneg(-1,0);
Vec2i Vec2i::unitYneg(0,-1);

//-------------------------------------------------------------------

#pragma warning(disable : 4505) // unreferenced static removed

static void Vec2iTest()
{
	Vec2i a(0,1),b(2,3);

	a = b;
}

void Vec2i::Log() const //!< writes xyz to Log; does NOT add a \n !
{
	lprintf("{%d,%d}",x,y);
}

// Area is a free function
// Area is *signed* based on handedness
// (this is actually triangle area TIMES TWO)
//  if "abc" is *COUNTER-CLOCKWISE* then Area is positive
int64 Area(const Vec2i & a,const Vec2i &b,const Vec2i &c)
{
	int64 bx = b.x - a.x;
	int64 by = b.y - a.y;
	int64 cx = c.x - a.x;
	int64 cy = c.y - a.y;

	return bx*cy - by*cx;
}

//-------------------------------------------------------------------

END_CB

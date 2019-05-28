#include "Base.h"
#include "cblib/Vec4.h"
#include "cblib/Log.h"

START_CB

//-------------------------------------------------------------------
//statics 

const Vec4 Vec4::zero(0,0,0,0);
const Vec4 Vec4::unitX(1,0,0,0);
const Vec4 Vec4::unitY(0,1,0,0);
const Vec4 Vec4::unitZ(0,0,1,0);
const Vec4 Vec4::unitXneg(-1,0,0,0);
const Vec4 Vec4::unitYneg(0,-1,0,0);
const Vec4 Vec4::unitZneg(0,0,-1,0);

//-------------------------------------------------------------------

#pragma warning(disable : 4505) // unreferenced static removed

static void Vec4Test()
{
	Vec4 a(0,1,7,0),b(2,3,1,0);

	a = b;
}

//-------------------------------------------------------------------

void Vec4::Log() const //!< writes xyz to Log; does NOT add a \n !
{
	lprintf("{%1.3f,%1.3f,%1.3f,%1.3f}",x,y,z,w);
}

//-------------------------------------------------------------------END_CB

END_CB

#include "Base.h"
#include "cblib/VecN.h"

START_CB

//-------------------------------------------------------------------

#pragma warning(disable : 4505) // unreferenced static removed

static void VecNTest()
{
	VecN<7> a,b;

	a.MutableVec3(0) = Vec3::zero;

	a = b;
}

//-------------------------------------------------------------------

END_CB

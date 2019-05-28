#pragma once

/**

MiniBall

builds a really minimal bounding gSphere

for pre-processing use only

***/

#include "cblib/Vec3.h"
#include "cblib/Sphere.h"

namespace MiniBall
{
	//-----------------------------------------

	void Make(cb::Sphere * pS,
				const cb::Vec3 * pVerts,
				const int numVerts);

	//-----------------------------------------
};


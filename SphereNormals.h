#pragma once

#include "cblib/Vec3.h"

START_CB

	enum
	{
		c_numSphereNormals = 258
	};		

	const Vec3 & GetSphereNormal(const int i);

END_CB

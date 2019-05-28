#pragma once

#include "cblib/Vec3.h"

START_CB

	extern const int c_numSphereNormals;
	extern const int c_numSpherePositiveNormals;

	const Vec3 & GetSphereNormal(const int i);

	extern const int c_numSphereNormalsSimple;
	const Vec3 & GetSphereNormalSimple(const int i);
	
END_CB

#pragma once

#include "cblib/BmpImage.h"

START_CB

bool LoadJpeg(const char *filename, BmpImage *result);

// LoadGeneric should be called without extension and we'll find it
bool LoadGeneric(const char *filename, BmpImage *result);

END_CB

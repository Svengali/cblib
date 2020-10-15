#pragma once

#include "BmpImage.h"

START_CB

bool LoadJpeg(const char *filename, BmpImage *result);

bool LoadPNG(const char *filename, BmpImage *result);

bool LoadGIF(const char *filename, BmpImage *result);

// quality in [0,100]
// quality <= 0 reads it from the last number in the file name
bool WriteJpeg(const char *filename, const BmpImage *source,int jqval);

bool WritePNG(const char *filename, const BmpImage * source,int zlevel = 7);

END_CB

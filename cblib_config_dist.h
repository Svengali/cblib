#pragma once

#define CBLIB_USE_JPEGLIB 0
#define CBLIB_USE_PNGLIB  0

// override operator new in the global namespace to track allocs
//	disable here if you don't want it
#define CB_DO_REPLACE_OPERATOR_NEW 0
#define CB_DO_REPLACE_MALLOC	0

#define CBLIB_PROFILE_ENABLED  0

#define CBLIB_TWEAKVAR_ENABLED 1

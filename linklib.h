#pragma once

/**

Just include "linklib" in your client file to bring in the library

**/

#include <cblib/inc.h>

#ifdef _DEBUG
#pragma comment(lib,"c:/src/cblib/cbd.lib")
#else
#pragma comment(lib,"c:/src/cblib/cb.lib")
#endif


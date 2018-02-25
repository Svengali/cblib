#pragma once

/*
#pragma warning(disable : 4127) // conditional expression is constant
#pragma warning(disable : 4100) // unreferenced formal parameter


// fucking MS STL is full of warnings, so I can't really use level 4
#pragma warning(push, 3)
#pragma warning(disable : 4663)
#pragma warning(disable : 4514)
#pragma warning(disable : 4018)
#pragma warning(disable : 4245)
*/

//#pragma warning(push,1)

// this header should maybe go away completely?

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "cblib/Base.h"
#include "cblib/Util.h"
#include "cblib/Rand.h"
#include "cblib/Journal.h"
#include "cblib/Log.h"
#include "cblib/String.h"
#include "cblib/File.h"
#include "cblib/FloatUtil.h"
#include "cblib/stl_basics.h"
#include "cblib/vector.h"
#include "cblib/RefCounted.h"
#include "cblib/SPtr.h"

#define WIN32_LEAN_AND_MEAN

//#pragma warning(pop)

//#define DEBUG_LOG // @@@@ TOGGLE HERE

//-----------------------------------------------------------

//#pragma warning(pop)

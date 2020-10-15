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

#include "Base.h"
#include "Util.h"
#include "Rand.h"
#include "Journal.h"
#include "Log.h"
#include "String.h"
#include "File.h"
#include "FloatUtil.h"
#include "stl_basics.h"
#include "vector.h"
#include "RefCounted.h"
#include "SPtr.h"

#define WIN32_LEAN_AND_MEAN

//#pragma warning(pop)

//#define DEBUG_LOG // @@@@ TOGGLE HERE

//-----------------------------------------------------------

//#pragma warning(pop)

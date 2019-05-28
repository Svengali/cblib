#pragma once

#include "Base.h"
#include "safeprintf.h"
#include "String.h"
//#include "Log.h"
#include <stdio.h>
#include <ctype.h>

START_CB

//=========================================================================================================

// helper - set mode for wchar conversion to chars :
extern void autoPrintfSetWCharAnsi(bool ansi); // else console
extern String autoPrintfWChar(const wchar_t * ws);

extern void MakeAutoPrintfINL();

//=========================================================================================================

#ifdef _MSC_VER
#define FMT_I64 "I64"
#else
#define FMT_I64  "ll"
#endif

//=========================================================================================================

template <typename T1>
inline const String ToString( T1 arg1 );
// base template is unimplemented - this gives you a compile time error if you try to ToString something unhandled
/*
{
	return String("UNKNOWN TYPE");
}
*/

//template <>
inline const String ToString( char * arg1 )
{
	return String(arg1);
}

//template <>
inline const String ToString( wchar_t * arg1 )
{
	return autoPrintfWChar(arg1);
}

//template <>
inline const String ToString( const String rhs )
{
	return rhs;
}

// @@ TODO : these ToStrings are a lot slower & fatter than necessary

//template <>

inline const String ToString(const int i)
{
	char temp[20];
	itoa(i,temp,10);
	return String(temp);
}

//template <>
inline const String ToString( int64 i )
{
	return StringPrintf("%" FMT_I64 "d",i);
}

//template <>
inline const String ToString( unsigned int i )
{
	return StringPrintf("%u",i);
}

//template <>
inline const String ToString( unsigned __int64 i )
{
	return StringPrintf("%" FMT_I64 "u",i);
}

//template <>
inline const String ToString( float f )
{
	return StringPrintf("%f",f);
}

//template <>
inline const String ToString( double f )
{
	return StringPrintf("%f",f);
}

//template <>
inline const String ToString( void * p )
{
	return StringPrintf("%p",p);
}

inline String ToString(bool b)
{
	return b ? String("true") : String("false");
}

//===========================================================================

/*
inline ESafePrintfType safeprintf_type( const String rhs )
{
	return safeprintf_cbstring;
}
*/

//===========================================================================
// autoArgConvert changes types into printf-able type
//	if arg is a basic type it is passed through untouched
//	otherwise ToString() is called on it

template <typename T>
inline String autoArgConvert( T arg )
{
	return ToString(arg);
}

//template <>
inline int autoArgConvert(const int arg)
{
	return arg;
}

//template <>
inline char autoArgConvert(const char arg)
{
	return arg;
}

inline wchar_t autoArgConvert(const wchar_t arg)
{
	return arg;
}

//template <>
inline short autoArgConvert(const short arg)
{
	return arg;
}

//template <>
inline long autoArgConvert(const long arg)
{
	return arg;
}

//template <>
inline __int64 autoArgConvert(const __int64 arg)
{
	return arg;
}

//template <>
inline unsigned int autoArgConvert(const unsigned int arg)
{
	return arg;
}

//template <>
inline unsigned char autoArgConvert(const unsigned char arg)
{
	return arg;
}

//template <>
inline unsigned short autoArgConvert(const unsigned short arg)
{
	return arg;
}

//template <>
inline unsigned long autoArgConvert(const unsigned long arg)
{
	return arg;
}

//template <>
inline const unsigned __int64 autoArgConvert(const unsigned __int64 arg)
{
	return arg;
}

//template <>
inline const float autoArgConvert(const float arg)
{
	return arg;
}

//template <>
inline const double autoArgConvert(const double arg)
{
	return arg;
}

//template <>
inline const char * autoArgConvert(const char * arg)
{
	return arg;
}

//template <>
inline const wchar_t * autoArgConvert(const wchar_t * arg)
{
	return arg;
}

//template <>
inline char * autoArgConvert(char * arg)
{
	return arg;
}

//template <>
inline wchar_t * autoArgConvert(wchar_t * arg)
{
	return arg;
}

//template <>
inline int * autoArgConvert(int * arg)
{
	return arg;
}

//template <>
inline void * autoArgConvert(void * arg)
{
	return arg;
}

//===========================================================================
// autoprintf_StringToChar :
//	converts String to char *
//	passes through everything else

template <typename T>
inline T autoprintf_StringToChar (const T & rhs)
{
	return rhs;
}

//template <>
inline const char * autoprintf_StringToChar (const String & rhs)
{
	return rhs.CStr();
}

//===========================================================================
// autoToStringFunc is the big worker function

extern String autoToStringFunc(int nArgs, ... );

//===========================================================================
// this defines autoToString :

#include "autoprintf.inl"

//===========================================================================
// adapters to convert autoToString to 

class autoprintf_PrintString
{
public:
	void operator << (const String & rhs)
	{
		fputs(rhs.CStr(),stdout);
	}
};


#define autoprintf  autoprintf_PrintString() << cb::autoToString

/*
#define VA1	__va_arg1
#define VA2 __va_arg1, __va_arg2
#define VA3 __va_arg1, __va_arg2, __va_arg3
#define VA4 __va_arg1, __va_arg2, __va_arg3, __va_arg4

// arg, can't vararg macro !
#define autosprintf(str,fmt,VA1) sprintf(str, StringToChar() << autoToString(fmt,VA1) )
#define autosprintf(str,fmt,VA2) sprintf(str, StringToChar() << autoToString(fmt,VA2) )
#define autosprintf(str,fmt,VA3) sprintf(str, StringToChar() << autoToString(fmt,VA3) )
//#define autofprintf(file,fmt,...) WriteString(file) << autoToString(fmt,...)
*/

#define APF_CALLER	autosprintf
#define APF_CALLER_PREARG	char * toString,
#define APF_CALL_SUB(s)		strcpy( toString, s.CStr() )

#include "autoprintfsub.inc"

#undef APF_CALLER
#undef APF_CALLER_PREARG
#undef APF_CALL_SUB

#define APF_CALLER	autosnprintf
#define APF_CALLER_PREARG	char * toString, int toStringSize, 
#define APF_CALL_SUB(s)		strlcpy( toString, s.CStr() , toStringSize)

#include "autoprintfsub.inc"

#undef APF_CALLER
#undef APF_CALLER_PREARG
#undef APF_CALL_SUB


#define APF_CALLER	autofprintf
#define APF_CALLER_PREARG	FILE * toFile,
#define APF_CALL_SUB(s)		fputs( s.CStr() , toFile )

#include "autoprintfsub.inc"

#undef APF_CALLER
#undef APF_CALLER_PREARG
#undef APF_CALL_SUB

//===========================================================================

END_CB

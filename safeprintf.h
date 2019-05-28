#pragma once

#include "cblib/Base.h"
#include <stdio.h>

/****

safeprintf 4-12-07

"safeprintf" is an identical drop in for "printf"

(similarly safesprintf -> sprintf , safefprintf -> fprintf ).

It detects argument errors at run time and makes a nice message (if noisy mode is on).

With or without noisy mode, it blocks printf from being called with the bad args.

A stronger safe printing template would detect problems at compile time, but you can't do that with a 
printf drop-in.  Drop-in compatibility with printf is crucial to me.



---------------------------------------------
DIFFERENCE IN FUNCTIONALITY : 

Regular printf will take classes in the varargs and convert them to whatever type you try to print as.
In some cases people use this benignly, such as with an HWND :

printf("Window handle = %08X\n",GetWindow());

if you call that with safeprintf it will generate an error saying an HWND is not an int.
You have to explicitly cast that for safeprintf :

printf("Window handle = %08X\n",(int)GetWindow());

---------------------------------------------
ADDING TYPES :

if you want safeprintf to treat a certain type as a valid int (or float or whatever) arg,
you can implement 

safeprintf_type()

on your custom type.  Generally it's preferred to explicitly cast when you call, but in some cases if you
have types that are drop-in replacements for basic types you may want to do this.

eg. you could provide :

template <> inline ESafePrintfType safeprintf_type(const HWND arg) { return safeprintf_int32; }

---------------------------------------------

You can wrap your own varargs printing functions to make them safe using the "safeprintf.inc" include.
See the bottom of this file for examples.

****/

START_CB

//=======================================================================================

enum ESafePrintfType
{
	safeprintf_none = 0,
	safeprintf_unknown,
	safeprintf_charptr,
	safeprintf_wcharptr,
	safeprintf_float,
	safeprintf_ptrint,
	safeprintf_ptrvoid,
	safeprintf_int32,
	safeprintf_int64,
	safeprintf_uint32,
	safeprintf_uint64
	//safeprintf_basetypes_count
};
const char * c_safeprintftypenames[];

template < typename t_arg >
inline ESafePrintfType safeprintf_type(const t_arg arg)
{
	return safeprintf_unknown;
}

template <>
inline ESafePrintfType safeprintf_type(const int arg)
{
	return safeprintf_int32;
}

template <>
inline ESafePrintfType safeprintf_type(const char arg)
{
	return safeprintf_int32;
}

template <>
inline ESafePrintfType safeprintf_type(const wchar_t arg)
{
	return safeprintf_int32;
}

template <>
inline ESafePrintfType safeprintf_type(const short arg)
{
	return safeprintf_int32;
}

template <>
inline ESafePrintfType safeprintf_type(const long arg)
{
	return safeprintf_int32;
}

template <>
inline ESafePrintfType safeprintf_type(const __int64 arg)
{
	return safeprintf_int64;
}

template <>
inline ESafePrintfType safeprintf_type(const unsigned int arg)
{
	return safeprintf_uint32;
}

template <>
inline ESafePrintfType safeprintf_type(const unsigned char arg)
{
	return safeprintf_int32;
}

template <>
inline ESafePrintfType safeprintf_type(const unsigned short arg)
{
	return safeprintf_int32;
}

template <>
inline ESafePrintfType safeprintf_type(const unsigned long arg)
{
	return safeprintf_uint32;
}

template <>
inline ESafePrintfType safeprintf_type(const unsigned __int64 arg)
{
	return safeprintf_uint64;
}

template <>
inline ESafePrintfType safeprintf_type(const float arg)
{
	return safeprintf_float;
}

template <>
inline ESafePrintfType safeprintf_type(const double arg)
{
	return safeprintf_float;
}

template <>
inline ESafePrintfType safeprintf_type(const char * arg)
{
	return safeprintf_charptr;
}

template <>
inline ESafePrintfType safeprintf_type(const wchar_t * arg)
{
	return safeprintf_wcharptr;
}

template <>
inline ESafePrintfType safeprintf_type(char * arg)
{
	return safeprintf_charptr;
}

template <>
inline ESafePrintfType safeprintf_type(wchar_t * arg)
{
	return safeprintf_wcharptr;
}

template <>
inline ESafePrintfType safeprintf_type(int * arg)
{
	return safeprintf_ptrint;
}

template <>
inline ESafePrintfType safeprintf_type(void * arg)
{
	return safeprintf_ptrvoid;
}

//=======================================================================================

//default setup is (true,true,false,false)
extern void safeprintf_setoptions(bool noisy,bool checkintsize,bool checkintasfloat,bool checkintunsigned);

extern ESafePrintfType safeprintf_fmttype(const char fmt, bool wide);
extern const char * safeprintf_fmtskipwidth(const char * ptr, bool * pWide);
extern ESafePrintfType safeprintf_findfmtandadvance(const char ** ptr);
extern void safeprintf_throwerror(const char *fmt_base,const char *fmt,ESafePrintfType fmttype,ESafePrintfType argtype);
extern void safeprintf_throwsyntaxerror(const char *fmt_base,const char *fmt);

template < typename t_arg >
inline const char * checkarg(const char *fmt_base,const char *fmt,const t_arg & arg)
{
	if ( ! fmt )
		return NULL;
		
	const char * ptr = fmt;
	
	ESafePrintfType fmttype = safeprintf_findfmtandadvance(&ptr);
	
	// it's perfectly valid to just not have %'s for your args
	if ( fmttype == safeprintf_none )
	{
		return NULL;
	}
	else if ( fmttype == safeprintf_unknown )
	{
		// syntax error failure (failed to parse a type from % in fmt)
		safeprintf_throwsyntaxerror(fmt_base,fmt);
	}
	else
	{
		ESafePrintfType argtype = safeprintf_type(arg);
		
		if ( fmttype != argtype )
		{
			safeprintf_throwerror(fmt_base,fmt,fmttype,argtype);
		}
	}
	
	return ptr;
}

//=============================================================================================

#define SPI_SAFEDECL int safeprintf
#define SPI_CALLRAW return printf
#define SPI_PREARG
#define SPI_CALLARG fmt
#define SPI_BADRETURN return 0;
#include "safeprintf.inc"
#undef SPI_SAFEDECL
#undef SPI_CALLRAW
#undef SPI_PREARG
#undef SPI_CALLARG
#undef SPI_BADRETURN

#define SPI_SAFEDECL int safesprintf
#define SPI_CALLRAW return sprintf
#define SPI_PREARG char * into,
#define SPI_CALLARG into,fmt
#define SPI_BADRETURN return 0;
#include "safeprintf.inc"
#undef SPI_SAFEDECL
#undef SPI_CALLRAW
#undef SPI_PREARG
#undef SPI_CALLARG
#undef SPI_BADRETURN

#define SPI_SAFEDECL int safefprintf
#define SPI_CALLRAW return fprintf
#define SPI_PREARG FILE * into,
#define SPI_CALLARG into,fmt
#define SPI_BADRETURN return 0;
#include "safeprintf.inc"
#undef SPI_SAFEDECL
#undef SPI_CALLRAW
#undef SPI_PREARG
#undef SPI_CALLARG
#undef SPI_BADRETURN

//=============================================================================================

extern void test_safeprintf();

END_CB

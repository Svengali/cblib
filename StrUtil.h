#pragma once

#include "cblib/Base.h"
#include "cblib/Util.h"
#include <crtdefs.h>

//#define strncpy	cb::strlcpy

//-----------------------------------------------------------------
// wchar str look-alikes : !!
//	these have to be in global namespace so that they're at the same level as the regular str funcs
//	if I put them in cb:: then overloading doesn't work because only the cb:: versions are searched
	
inline void strcat(wchar_t * to,const wchar_t * fm) { wcscat(to,fm); }
inline void strcpy(wchar_t * to,const wchar_t * fm)	{ wcscpy(to,fm); }
inline size_t strlen(const wchar_t * str) { return wcslen(str); }
inline wchar_t * strchr(wchar_t * str,wchar_t chr) { return wcschr(str,chr); }
inline const wchar_t * strchr(const wchar_t * str,wchar_t chr) { return wcschr(str,chr); }
inline wchar_t * strrchr(wchar_t * str,wchar_t chr) { return wcsrchr(str,chr); }
inline const wchar_t * strrchr(const wchar_t * str,wchar_t chr) { return wcsrchr(str,chr); }
inline int strcmp(const wchar_t * s1,const wchar_t * s2) { return wcscmp(s1,s2); }
inline int stricmp(const wchar_t * p1,const wchar_t * p2) { return wcsicmp(p1,p2); }
inline int strnicmp(const wchar_t * p1,const wchar_t * p2,size_t s) { return wcsnicmp(p1,p2,s); }


//-----------------------------------------------------------------



START_CB

//-----------------------------------------------------------
// char * string utils

// use countof instead of sizeof
//	(MSVC stdlib.h has _countof)
#define countof(buf)	ARRAY_SIZE(buf)

// skipprefix : if str starts with pre, return str+strlen(pre)
const char * skipprefix(const char *str,const char *pre);
const char * skipprefixi(const char *str,const char *pre);

int get_first_number(const char * str);
int get_last_number(const char * str);

// argstr supports -oxxx or "-o xxx" or "-o" "xxx" or "-o=xxx"
//	argstr returns a direct pointer into the char * argv data
const char * argstr(int argc,const char * const argv[],int & i);

int argint(int argc,const char * argv[],int & i);
double argfloat(int argc,const char * argv[],int & i);

// macro_tolower should be safe and is nice for constants
#define macro_tolower(c)	( ( (c) >= 'A' && (c) <= 'Z' ) ? ( (c) | 0x20 ) : (c) )

// unsafe_tolower works on alpha chars and also leaves number digits alone
#define unsafe_tolower(c)	( (c) | 0x20 )

void sprintfcommas( char * into,int64 number);
void sprintfcommasf(char * into,double number,int numdecimals);
   
bool iswhitespace(char c);

// expandStrEscapes writes "to" from "fm" for codes like \n and \t
void expandStrEscapes(char *to,const char *fm);

const char * skipwhitespace(const char * str);
char * skipwhitebackwards(char * ptr,char * pStart);

inline const char * skipwhitebackwards(const char * ptr,const char * pStart)
{
	return (const char *) skipwhitebackwards((char *)ptr,(char *)pStart);
}

// returns true if the beginning of str matches pre
bool strpresame(const char * str,const char * pre);
bool stripresame(const char * str,const char * pre);
bool strtpresame(const char * str,const char * pre);

// returns length that prefix is the same , return is in [0,MIN(strlen(str1),strlen(str2)]
int strpresamelen(const char * str1,const char * str2);

int strlen32(const char * str);

inline int strlen32(const wchar_t * str) { return cb::check_value_cast<int>( wcslen(str) ); }

// strrep changed fm to to
void strrep(char * str,char fm,char to);

// insert "fm" at to : ; returns ptr of end of insertion
char * strins(char *to,const char *fm);

// count # of occurence of 'c' in str
int strcount(const char * str,char c);

// strrchr or end
//	to go to the end of a token section
inline const char * strrchrorend(const char * str,char c)
{
	const char * ptr = strrchr(str,c);
	if ( ptr )
		return ptr;
	else
		return str + strlen(str);
}
inline char * strrchrorend(char * str,char c)
{
	char * ptr = strrchr(str,c);
	if ( ptr )
		return ptr;
	else
		return str + strlen(str);
}

#define errputs(str) fprintf(stderr,"%s\n",str)

// strlcpy is a semi-standard name for what you think strncpy is
void strlcpy(char * to,const char *fm,int count);

// NOTE : if you "USE_CB" and just call strncpy() it will call *this* not the library one
// override clib and use same signature :
inline char * strncpy ( char * destination, const char * source, size_t num )
{
	strlcpy(destination,source,(int)num);
	return destination;
}

// tokenchar promotes a char like we do for the CRC - makes it upper case & back slashes
inline char tokenchar(char c)
{
	// '/' is 47 = 0x2F
	// '\' is 92 = 0x5C
	// / is unaffected by unsafe_tolower, but \ is, so prefer /
	if ( c == '\\' )
		c = '/';

	//return (char)toupper(c);
	return unsafe_tolower(c);
}

// munge str to tokenchars
void strtokenchar(char * ptr);

// strtcmp is a strcmp like a "Token" - that is, case & slashes don't affect it
int strtcmp(const char * p1,const char * p2);

inline bool strtsame(const char * s1,const char * s2)
{
	return strtcmp(s1,s2) == 0;
}

const char * stristr(const char * search_in,const char * search_for);
const char * strrstr(const char * search_in,const char * search_for);
const char * strristr(const char * search_in,const char * search_for);
const char * strichr(const char * search_in,const char search_for);

// find the first char the matches anything in search_set :
const char * strchrset(const char * search_in,const char * search_set);

inline char * strchrset(char * search_in,const char * search_set)
{
	return (char *)strchrset((const char *)search_in,search_set);
}

// strchr with maximum search distance :
const char * strchrlimit(const char * ptr,char c1,int limit);

const char * strstr2(const char * search_in,const char * search_for1,const char * search_for2);

const char * strchr2(const char * ptr,char c1,char c2);
const char * strrchr2(const char * ptr,char c1,char c2);
const char * strchreol(const char * ptr);
const char * skipeols(const char *ptr);

// strnextline puts you at the start of the next line :
const char * strnextline(const char * ptr);

// const adapters
inline char * strchr2(char * ptr,char c1,char c2) { return (char *) strchr2( (const char *)ptr,c1,c2 ); }
inline char * strchrlimit(char * ptr,char c1,int limit) { return (char *) strchrlimit( (const char *)ptr,c1,limit ); }
inline char * strrchr2(char * ptr,char c1,char c2) { return (char *) strrchr2( (const char *)ptr,c1,c2 ); }
inline char * strchreol(char * ptr) { return (char *) strchreol( (const char *)ptr ); }
inline char * skipeols(char *ptr) { return (char *) skipeols( (const char *)ptr ); }
inline char * strnextline(char * ptr) { return (char *)strnextline((const char *)ptr); }

// "orend" returns strend(str) instead of NULL if not found
const char * strchrorend(const char * ptr,char c);
const char * strstrorend(const char * ptr,const char * substr);

inline char * strchrorend(char * ptr,char c) { return (char *) strchrorend((const char*)ptr,c); }
inline char * strstrorend(char * ptr,const char * substr) { return (char *) strstrorend((const char*)ptr,substr); }

// is the entire string at ptr a number?
//	"-99.4e7" returns true
//	"33 xx" returns false
// NOTE : isnumber returns true for a totally empty string ; meh? check that separately
bool isnumber(const char * ptr);

bool isallwhitespace( const char * pstr );
void killtailingwhite( char * ptr );
void killtailingwhiteandcomments( char * ptr ); // also kills tailing /* comments */

// returns the tok between tokChar,tokChar pair ; eg. tokChar ='"'
//	mutates str by putting a 0 at second occurance of tokChar
char * strextracttok(char * str, char tokChar, char ** pAfter);

//strstrend : handy call for when you want to find a substr and get the pointer to the END of it :
inline const char * strstrend(const char * search_in,const char * search_for)
{
	const char * ptr = strstr(search_in,search_for);
	if ( ! ptr ) return NULL;
	return ptr + strlen(search_for);
}

inline const char * stristrend(const char * search_in,const char * search_for)
{
	const char * ptr = stristr(search_in,search_for);
	if ( ! ptr ) return NULL;
	return ptr + strlen(search_for);
}

//strstrend : handy call for when you want to find a substr and get the pointer to the END of it :
inline char * strstrend(char * search_in,const char * search_for)
{
	return (char *)strstrend( (const char *)search_in, search_for );
}

inline char * stristrend(char * search_in,const char * search_for)
{
	return (char *)stristrend( (const char *)search_in, search_for );
}

inline char * skipwhitespace(char * str)
{
	return (char *)skipwhitespace( (const char *)str );
}
inline char * stristr(char * search_in,const char * search_for)
{
	return (char *)stristr( (const char *)search_in, search_for );
}
inline char * strrstr(char * search_in,const char * search_for)
{
	return (char *)strrstr( (const char *)search_in, search_for );
}
inline char * strristr(char * search_in,const char * search_for)
{
	return (char *)strristr( (const char *)search_in, search_for );
}
	
inline const char * strend(const char * ptr) // strend points you at the null
{
	return ptr + strlen(ptr);
}
inline char * strend(char * ptr)
{
	return ptr + strlen(ptr);
}
	
inline bool strisame(const char * s1,const char * s2)
{
	return stricmp(s1,s2) == 0;
}

inline bool strsame(const char * s1,const char * s2)
{
	return strcmp(s1,s2) == 0;
}


wchar_t * strins(wchar_t *to,const wchar_t *fm);
void strrep(wchar_t * str,wchar_t fm,wchar_t to);
bool stripresame(const wchar_t * str,const wchar_t * pre);
bool strpresame(const wchar_t * str,const wchar_t * pre);
int strpresamelen(const wchar_t * str1,const wchar_t * str2);
void killtailingwhite( wchar_t * ptr );
const wchar_t * skipwhitespace(const wchar_t * str);
bool iswhitespace(wchar_t c);
void strncpy(wchar_t * to,const wchar_t *fm,int count);

inline bool strsame(const wchar_t * s1,const wchar_t * s2)
{
	return strcmp(s1,s2) == 0;
}

inline bool strisame(const wchar_t * p1,const wchar_t * p2)
{
	return stricmp(p1,p2) == 0;
}

END_CB

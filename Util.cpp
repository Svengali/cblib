#include "cblib/Util.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <typeinfo.h>

START_CB

void * mymalloc(size_t s)
{
	void * ret = malloc(s);
	if ( ret == NULL && s > 0 )
	{
		FAIL("malloc failed");
	}
	return ret;
}

//-----------------------------------------------------------

bool iswhitespace(char c)
{
	//return ( c== ' ' || c== '\t' || c== '\n' || c== '\r' );
	return !! ::isspace(c);
}

const char * skipwhitespace(const char * str)
{
	ASSERT( str != NULL );

	while( iswhitespace(*str) )
		str++;

	return str;
}

void killtailingwhite( char * ptr )
{
	char * pend = ptr + strlen(ptr) - 1;
	while( pend >= ptr )
	{
		if ( iswhitespace(*pend) )
		{
			*pend = 0;
		}
		else
		{
			break;
		}
		pend--;
	}
}

int strpresamelen(const char * str1,const char * str2)
{
	int matchLen = 0;
	while( str1[matchLen] == str2[matchLen] )
	{
		if ( str1[matchLen] == 0 )
			return matchLen;
		
		matchLen++;
	}
	return matchLen;
}

bool strpresame(const char * str,const char * pre)
{
	while(*pre)
	{
		if ( *str != *pre )
			return false;
		str++;
		pre++;
	}
	return true;
}

bool stripresame(const char * str,const char * pre)
{
	while(*pre)
	{
		if ( toupper(*str) != toupper(*pre) )
			return false;
		str++;
		pre++;
	}
	return true;
}

void strncpy(char * to,const char *fm,int count)
{
	::strncpy(to,fm,count);
	to[count-1] = 0;
}

// tokenchar promotes a char like we do for the CRC - makes it upper case & back slashes
char tokenchar(char c)
{
	if ( c == '/' )
		return '\\';
	else
		return (char)toupper(c);
}
	
// strtcmp is a strcmp like a "Token" - that is, case & slashes don't affect it
int strtcmp(const char * p1,const char * p2)
{
	for(;;)
	{
		const char c1 = tokenchar(*p1++);
		const char c2 = tokenchar(*p2++);
		if ( c1 != c2 )
		{
			return int(c1) - int(c2);
		}
		if ( c1 == 0 )
		{
			// c2 == 0
			//	also
			return 0;
		}
		// continue
	}
}	

const char * extensionpart(const char * path)
{
	const char * dot = strrchr(path,'.');
	if ( dot == NULL )
		return path + strlen(path);
	return (dot+1);
}
	
const char * stristr(const char * search_in,const char * search_for)
{
	while ( *search_in )
	{
		if ( toupper(*search_in) == toupper(*search_for) )
		{
			const char * in_ptr = search_in + 1;
			const char * for_ptr= search_for + 1;
			while ( *for_ptr && toupper(*for_ptr) == toupper(*in_ptr) )
			{
				for_ptr++; in_ptr++;
			}
			if ( ! *for_ptr) 
				return search_in;
		}
		search_in++;
	}

	return NULL;
}

const char * strristr(const char * search_in,const char * search_for)
{
	const char * search_in_ptr = search_in + strlen(search_in) - 1;
	while ( search_in_ptr >= search_in )
	{
		if ( toupper(*search_in_ptr) == toupper(*search_for) )
		{
			const char * in_ptr = search_in_ptr + 1;
			const char * for_ptr= search_for + 1;
			while ( *for_ptr && toupper(*for_ptr) == toupper(*in_ptr) )
			{
				for_ptr++; in_ptr++;
			}
			if ( ! *for_ptr) 
				return search_in_ptr;
		}
		search_in_ptr--;
	}

	return NULL;
}

const char * strrstr(const char * search_in,const char * search_for)
{
	const char * search_in_ptr = search_in + strlen(search_in) - 1;
	while ( search_in_ptr >= search_in )
	{
		if ( (*search_in_ptr) == (*search_for) )
		{
			const char * in_ptr = search_in_ptr + 1;
			const char * for_ptr= search_for + 1;
			while ( *for_ptr && (*for_ptr) == (*in_ptr) )
			{
				for_ptr++; in_ptr++;
			}
			if ( ! *for_ptr) 
				return search_in_ptr;
		}
		search_in_ptr--;
	}

	return NULL;
}

const char * strichr(const char * search_in,const char search_for)
{
	while ( *search_in )
	{
		if ( toupper(*search_in) == toupper(search_for) )
		{
			return search_in;
		}
		search_in++;
	}

	return NULL;
}

// strrep changed fm to to
void strrep(char * str,char fm,char to)
{
	while(*str)
	{
		if ( *str == fm )
			*str = to;
		str++;
	}
}

// count # of occurence of 'c' in str
int strcount(const char * str,char c)
{
	int count = 0;
	while(*str)
	{
		if ( *str == c )
			count++;
		str++;
	}
	return count;
}

const char * filepart(const char * path)
{
	const char * p1 = strrchr(path,'\\');
	const char * p2 = strrchr(path,'/');

	const char * p = MAX(p1,p2);

	if ( p != NULL )
	{
		ASSERT( *p == '\\' || *p == '/' );
		return (p+1);
	}

	return path;
}

/*
const char * skipspaces(const char * str)
{
	ASSERT( ::isspace(' ') );
	ASSERT( ::isspace('\t') );
	ASSERT( ::isspace('\r') );
	ASSERT( ::isspace('\n') );
	ASSERT( str != NULL );
	while ( *str )
	{
		if ( !::isspace( *str ) )
		{
			return str;
		}
		str++;
	}
	return str;
}
*/

bool isallwhitespace( const char * pstr )
{
	if ( pstr==NULL )
	{
		return true;
	}

	const char * pFirstNonSpace = skipwhitespace(pstr);

	return ( *pFirstNonSpace == 0 );
}


char * SkipCppComments(const char * ptr )
{
	if ( ptr[0] == '/' && ptr[1] == '/' )
	{
		// C++ - style comment, skip this line 
		ptr += 2;
		while ( *ptr != 0 && *ptr != '\n' )
		{
			ptr++;
		}
	}
	return const_cast<char *>(ptr);
}

char * FindMatchingBrace(const char * start)
{
	const char * ptr = start;
	ASSERT( *ptr == '{' );
	ptr++;
	int depth = 0;
	for(;;)
	{
		ptr = SkipCppComments(ptr);
		if ( *ptr == 0 )
		{
			//lrintf("Unmatched Braces : %s\n",start);
			return NULL;
		}
		if ( *ptr == '{' )
		{
			depth++;
		}
		if ( *ptr == '}' )
		{
			if ( depth == 0 )
			{
				// that's it
				ptr++;
				return const_cast<char *>(ptr);
			}
			depth--;
		}
		ptr++;
	}
}

char * MultiLineStrChr(const char * start,int c)
{
	const char * ptr = start;
	for(;;)
	{
		if ( *ptr == c )
			return const_cast<char *>(ptr);
		ptr = SkipCppComments(ptr);
		if ( *ptr == 0 )
		{
			return NULL;
		}
		ptr++;
	}
}

char * TokToComma(char * ptr)
{
	char * comma = strchr(ptr,',');
	if ( comma == NULL )
		return NULL;

	// skip over { } ; eg. handle "{a,b,c}," - I want to go to the comma
	//	after the ending brace, not any , inside
	char * brace = strchr(ptr,'{');
	if ( brace != NULL && brace < comma )
	{
		char * brace2 = FindMatchingBrace(brace);
		if ( brace2 == NULL )
		{
			return NULL;
		}
		char * next_comma = strchr(brace2,',');
		if ( next_comma == NULL )
			return NULL;
	}

	*comma = 0;
	return comma+1;
}

//-----------------------------------------------------------

const int c_bitTable[256] =
{
	0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
};

int GetNumBitsSet(uint64 word)
{
	int ret = 0;
	while( word )
	{
		uint8 bottom = (uint8)(word & 0xFF);
		word >>= 8;
		ret += c_bitTable[bottom];
	}
	return ret;
}


int argint(int argc,char * argv[],int & i)
{
	char * cur = argv[i]+2;
	cur = skipwhitespace(cur);
	if ( *cur )
	{
		char * endptr;
		int ret = strtol(cur,&endptr,10);
		if ( endptr != cur )
			return ret;
	}

	if ( i == argc-1 )
	{
		FAIL("no int value found for arg!");
	}

	i++;
	return atoi(argv[i]);
}

double argfloat(int argc,char * argv[],int & i)
{
	char * cur = argv[i]+2;
	cur = skipwhitespace(cur);
	if ( *cur )
	{
		char * endptr;
		double ret = strtod(cur,&endptr);
		if ( endptr != cur )
			return ret;
	}

	if ( i == argc-1 )
	{
		FAIL("no int value found for arg!");
	}

	i++;
	return atof(argv[i]);
}

const char * type_name(const type_info & ti)
{
	return ti.name();
}

END_CB

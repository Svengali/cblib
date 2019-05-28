#include "StrUtil.h"
#include "Log.h"

#include <ctype.h>

START_CB

// -999.999
bool isnumber(const char * ptr)
{
	ptr = skipwhitespace(ptr);
	if ( *ptr == '-' || *ptr == '+' ) ptr++;
	while( isdigit(*ptr) ) ptr++;
	if ( *ptr == '.' ) ptr++;
	while( isdigit(*ptr) ) ptr++;
	if ( tolower(*ptr) == 'e' )
	{
		// optional exponent
		ptr++;
		if ( *ptr == '-' || *ptr == '+' ) ptr++;
		while( isdigit(*ptr) ) ptr++;
	}
	ptr = skipwhitespace(ptr);
	if ( *ptr != 0 )
		return false;
	else
		return true;	
}

const char * strstr2(const char * search_in,const char * search_for1,const char * search_for2)
{
	const char * s1 = strstr(search_in,search_for1);
	const char * s2 = strstr(search_in,search_for2);
	if ( s1 == NULL ) return s2;
	else if ( s2 == NULL ) return s1;
	return MIN(s1,s2);
}

const char * skipprefixi(const char *str,const char *pre)
{
	if ( stripresame(str,pre) )
		str += strlen(pre);
	return str;
}

const char * skipprefix(const char *str,const char *pre)
{
	if ( strpresame(str,pre) )
		str += strlen(pre);
	return str;
}

// strchr with maximum search distance :
const char * strchrlimit(const char * ptr,char c1,int limit)
{
	for(int i=0;i<limit;i++)
	{
		if ( ptr[i] == c1 ) return ptr+i;
		if ( ptr[i] == 0 ) return NULL;
	}
	return NULL;
}

char * skipwhitebackwards(char * ptr,char * pStart)
{
	while ( ptr > pStart && iswhitespace(*ptr) )
	{
		--ptr;
	}
	return ptr;
}

int get_last_number(const char * str)
{
	const char * ptr = strend(str) - 1;
	while( ptr > str && ! isdigit(*ptr) )
		ptr --;
	if ( ! isdigit(*ptr) ) return 0;
	while ( ptr > str && isdigit(ptr[-1]) ) ptr--;
	return atoi(ptr);
}
		
		
int get_first_number(const char * str)
{
	const char * ptr = str;
	while ( *ptr && ! isdigit(*ptr) )
		ptr++;
		
	return atoi(ptr);
}

int strlen32(const char * str)
{
	return check_value_cast<int>( strlen(str) );
}

// strnextline find end of line then go to the next one
const char * strnextline(const char * ptr)
{
	ptr = strchreol(ptr);
	if ( ptr == NULL )
		return NULL;
	return skipeols(ptr);
}

// strextracttok :
//	find the next two occurances of tokChar  (tokChar is often " for example)
// return the char * to the tok
// set pAfter to the string after the end of tok
// * MODIFIES STR by putting in a null at the end of tok
char * strextracttok(char * str, char tokChar, char ** pAfter)
{
	char * start = strchr(str,tokChar);
	if ( start == NULL )
		return NULL;
	char * ret = start+1;
	char * end = strchr(ret,tokChar);
	if ( end == NULL )
		return NULL;
	*end = 0;
	if ( pAfter )
		*pAfter = end+1;
	
	return ret;
}

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

void killtailingwhiteandcomments( char * ptr )
{
	char * pend = ptr + strlen(ptr) - 1;
	while( pend >= ptr )
	{
		if ( iswhitespace(*pend) )
		{
			*pend = 0;
		}
		else if ( (pend - ptr) >= 4 && *pend == '/'&& pend[-1] == '*' )
		{
			// saw an end-of-comment, look for a beginn
			char * look = pend-2;
			while ( ! (*look == '*' && look[-1] == '/') )
			{
				look--;
				if ( look <= ptr ) return;
			}
			ASSERT( *look == '*' );
			look--;
			ASSERT( *look == '/' );
			*look = 0;
			pend = look;
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

bool strtpresame(const char * str,const char * pre)
{
	while(*pre)
	{
		if ( tokenchar(*str) != tokenchar(*pre) )
			return false;
		str++;
		pre++;
	}
	return true;
}

void strlcpy(char * to, const char * fm, int maxLen)
{
	int len = strlen32(fm) + 1;
	len = MIN(len,maxLen);
	memcpy(to,fm,len);
	to[maxLen-1] = 0;
}

/*
void strncpy(char * to,const char *fm,int count)
{
	::strncpy(to,fm,count);
	to[count-1] = 0;
}
*/
	
// munge str to tokenchars
void strtokenchar(char * ptr)
{
	while(*ptr)
	{
		*ptr = tokenchar(*ptr);
		ptr++;
	}
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

// strchrset : find the first occurance of anything in search_set
const char * strchrset(const char * search_in,const char * search_set)
{
	// set flags :
	char mask[256] = { 0 };
	while( *search_set )
	{
		mask[ (uint8)*search_set ] = 1;
		search_set++;
	}
	
	while( *search_in )
	{
		if ( mask[ (uint8)*search_in ] )
			return search_in;
		search_in++;
	}
	
	return NULL;
}

// strchr2 : find the first occurance of c1 or c2
const char * strchr2(const char * ptr,char c1,char c2)
{	
	while ( *ptr )
	{
		if ( *ptr == c1 || *ptr == c2 )
			return ptr;
		ptr++;
	}
	
	return NULL;
}

const char * strrchr2(const char * ptr,char c1,char c2)
{	
	const char * end = strend(ptr);
	end--;
	while ( end >= ptr )
	{
		if ( *end == c1 || *end == c2 )
			return end;
		end--;
	}
	
	return NULL;
}

// strchreol : find first eol char :
const char * strchreol(const char * ptr)
{
	return strchr2(ptr,'\r','\n');
}

const char * skipeols(const char *ptr)
{
	while( *ptr == '\r' || *ptr == '\n' )
	{
		ptr++;
	}
	return ptr;
}

// strrep changes fm to to
void strrep(char * str,char fm,char to)
{
	while(*str)
	{
		if ( *str == fm )
			*str = to;
		str++;
	}
}

// insert "fm" at "to" , shifts down the remainder of "to" - 
//	you better make sure enough space was allocated or this trashes memory
char * strins(char *to,const char *fm)
{
	int tolen = strlen32(to);
	int fmlen = strlen32(fm);
	char *newto,*oldto;

	newto = to+fmlen+tolen; oldto = to+tolen;
	tolen++;
	while(tolen--) *newto-- = *oldto--;

	while(fmlen--) *to++ = *fm++;

	return to;
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

// strchrorend : like strchr but returns the end of the string instead of NULL if none found
const char * strchrorend(const char * ptr,char c)
{
	const char * ret = strchr(ptr,c);
	return ret ? ret : strend(ptr);
}

const char * strstrorend(const char * ptr,const char * substr)
{
	const char * ret = strstr(ptr,substr);
	return ret ? ret : strend(ptr);
}


//-----------------------------------------------------------

// argstr supports -oxxx or "-o xxx" or "-o" "xxx" or "-o=xxx"
//	argstr returns a direct pointer into the char * argv data
const char * argstr(int argc,const char * const argv[],int & i)
{
	const char * cur = argv[i]+2;
	cur = skipwhitespace(cur);
	if ( *cur == '=' ) cur++; // support -o=r:\t.rmv style args as well
	cur = skipwhitespace(cur);
	if ( *cur )
	{
		return cur;
	}

	if ( i == argc-1 )
	{
		lprintf("no value found for arg!\n");
		i = argc;
		return 0;
	}

	i++;
	return argv[i];
}

// argint/argfloat :
//	read a value from command line args
//	works for either "-c5" or "-c 5"
//	eg it can look in current arg or next arg - thus you pass in argi and it might increment
int argint(int argc,const char * argv[],int & i)
{
	const char * cur = argv[i]+2;
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

double argfloat(int argc,const char * argv[],int & i)
{
	const char * cur = argv[i]+2;
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


// sprintfcommas : print number with commas at thousands
void sprintfcommas(char * into,int64 number)
{
	if ( number < 0 )
	{
		number = -number;
		*into++ = '-';
	}
	else if ( number == 0 )
	{
		strcpy(into,"0");
		return;
	}

	int64 place = 1000;
	while ( number >= place )
	{
		place *= 1000;
	}
	// number < place and number >= place/1000
	place /= 1000;
	
	uint64 remainder = number;
	
	char * ptr = into;
	bool first = true;
	
	while( place > 0 )
	{
		int cur = (int)(remainder/place);
		remainder -= cur*place;
	
		if ( ! first )
		{
			*ptr++ = ',';
			sprintf(ptr,"%03d",cur);		
		}
		else
		{
			first = false;
			sprintf(ptr,"%d",cur);
		}
		ptr += strlen(ptr);
		
		place /= 1000;
		
		if ( (ptr - into) > 30 )
			break;
	}
}

void sprintfcommasf(char * into,double number,int numdecimals)
{
	if ( number < 0 )
	{
		*into++ = '-';
		number = - number;
	}
	
	uint64 intpart = (uint64)number;
	double fraction = number - intpart;
	
	sprintfcommas(into,intpart);
	
	if ( numdecimals > 0 )
	{
		char * ptr = into + strlen(into);
		//sprintf(into,"%0*d",numdecimals,(int)(fraction*(10^numdecimals)));
		ASSERT( fraction < 1.0 );
		for(int i=0;i<numdecimals;i++)
		{
			fraction *= 10.0;
			int v = (int)fraction;
			ASSERT( v >= 0 && v < 10 );
			fraction -= v;
			*ptr++ = (char)(v + '0');
			//if ( (i%3) == 2 )
			//	*ptr++ = ','
		}
		*ptr = 0;
	}
}


void strncpy(wchar_t * to,const wchar_t *fm,int count)
{
	wcsncpy(to,fm,count);
	to[count-1] = 0;
}


bool iswhitespace(wchar_t c)
{
	//return ( c== ' ' || c== '\t' || c== '\n' || c== '\r' );
	return !! ::isspace(c);
}

const wchar_t * skipwhitespace(const wchar_t * str)
{
	ASSERT( str != NULL );

	while( iswhitespace(*str) )
		str++;

	return str;
}

void killtailingwhite( wchar_t * ptr )
{
	wchar_t * pend = ptr + strlen(ptr) - 1;
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

int strpresamelen(const wchar_t * str1,const wchar_t * str2)
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

bool strpresame(const wchar_t * str,const wchar_t * pre)
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

bool stripresame(const wchar_t * str,const wchar_t * pre)
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

// strrep changed fm to to
void strrep(wchar_t * str,wchar_t fm,wchar_t to)
{
	while(*str)
	{
		if ( *str == fm )
			*str = to;
		str++;
	}
}

// insert "fm" at "to"
wchar_t * strins(wchar_t *to,const wchar_t *fm)
{
	size_t tolen = strlen(to);
	size_t fmlen = strlen(fm);
	
	wchar_t *newto,*oldto;

	newto = to+fmlen+tolen; oldto = to+tolen;
	tolen++;
	while(tolen--) *newto-- = *oldto--;

	while(fmlen--) *to++ = *fm++;

	return to;
}


// expandStrEscapes writes "to" from "fm" for codes like \n and \t
void expandStrEscapes(char *to,const char *fm)
{
	while(*fm)
	{
		if ( *fm == '\\' )
		{
			fm++;
			switch(*fm)
			{
			case 'n':	*to++ = '\n';	fm++; break;
			case 'r':	*to++ = '\r'; fm++;	break;
			case 't':	*to++ = '\t'; fm++; break;
			case '\\':	*to++ = '\\';	fm++; break;
			default:
				if ( isdigit(*fm) )
				{
					int v = 0;
					while( isdigit(*fm) && v < 26 )
					{
						v *= 10;
						v += *fm - '0' ; fm++;
					}
					*to++ = (char)(v & 0xFF);
				}
				else
				{
					*to++ = '\\';
					*to++ = *fm++;
				}
				break;
			}
		}
		else
		{
			*to++ = *fm++;
		}
	}
	*to = 0;
}

END_CB

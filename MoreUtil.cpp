#include "MoreUtil.h"
#include "FileUtil.h"
#include "StrUtil.h"
#include "Log.h"

START_CB

#define SingleQuote ((char)39)	// the ''' char	

bool IsStringInVecI(const char * str, const vector<String> & vec)
{
	// check if it's already in onceFileDone :
	for LOOPVEC(i,vec)
	{
		if ( strisame(str,vec[i].CStr()) )
		{
			return true;
		}
	}
	return false;
}

bool IsStringInVecC(const char * str, const vector<String> & vec)
{
	// check if it's already in onceFileDone :
	for LOOPVEC(i,vec)
	{
		if ( strsame(str,vec[i].CStr()) )
		{
			return true;
		}
	}
	return false;
}

bool IsStringPrefixInVecI(const char * str, const vector<String> & vec)
{
	// check if it's already in onceFileDone :
	for LOOPVEC(i,vec)
	{
		if ( stripresame(str,vec[i].CStr()) )
		{
			return true;
		}
	}
	return false;
}

bool IsStringPrefixInVecC(const char * str, const vector<String> & vec)
{
	// check if it's already in onceFileDone :
	for LOOPVEC(i,vec)
	{
		if ( strpresame(str,vec[i].CStr()) )
		{
			return true;
		}
	}
	return false;
}

bool IsStringPortionInVecI(const char * str, const vector<String> & vec)
{
	// check if it's already in onceFileDone :
	for LOOPVEC(i,vec)
	{
		if ( stristr(str,vec[i].CStr()) )
		{
			return true;
		}
	}
	return false;
}

bool IsStringPortionInVecC(const char * str, const vector<String> & vec)
{
	// check if it's already in onceFileDone :
	for LOOPVEC(i,vec)
	{
		if ( strstr(str,vec[i].CStr()) )
		{
			return true;
		}
	}
	return false;
}

bool CheckExtensionAllowed(const char * baseFileName,vector<String> & extensionsAllowed)
{
	if ( ! extensionsAllowed.empty() )
	{
		// check extension filter
		const char * ext = extensionpart(baseFileName);
		if ( ! IsStringInVecI(ext,extensionsAllowed) )
		{
			lprintf_v2("extension not in filter (%s), skipping..\n",ext);
			return false;	
		}
	}
	
	return true;
}

void ParseSemicolonsToStringVec(const char * extensionFilter,vector<String> & extensionsAllowed)
{
	if ( extensionFilter != NULL )
	{
		char extWork[_MAX_PATH];
		strlcpy(extWork,extensionFilter,256);
		char * ptr = extWork;
		for(;;)
		{
			char * end = strchrorend(ptr,';');
			bool last = *end == 0;
			*end = 0;
			lprintf_v2("extension allowed : %s\n",ptr);
			extensionsAllowed.push_back( String(ptr) );
			ptr = end+1;
			if ( last ) break;
		}
	}
}

String GetSubStr(const char * str,char startDelim,char endDelim, const char ** pAfter)
{
	if ( pAfter )
		*pAfter = str;
		
	const char * p1 = strchr(str,startDelim);
	if ( ! p1 )
		return String();

	p1++;

	const char * p2 = strchr(p1,endDelim);
	if ( ! p2 )
		return String();
	
	int len = (int)(p2 - p1);
	
	if ( pAfter )
		*pAfter = p2+1;
	
	return String(String::eSubString,p1,len);
}

String KillHeadTailWhite(const char * str)
{
	str = skipwhitespace(str);
	const char * end = skipwhitebackwards( strend(str) -1,str);
	if ( end <= str )
		return String();
	int len = (int) (end - str) + 1;
	return String(String::eSubString,str,len);
}

// also remove surrounding quotes, if any :
String KillHeadTailWhiteAndQuotes(const char * str)
{
	str = skipwhitespace(str);
	const char * end = skipwhitebackwards( strend(str) -1,str);
	if ( end <= str )
		return String();
	int len = (int) (end - str) + 1;
	if ( len >= 2 )
	{
		if ( *str == '"' && str[len-1] == '"' )
		{
			str++; len -= 2;
		} 
		else if ( *str == SingleQuote && str[len-1] == SingleQuote )
		{
			str++; len -= 2;
		}
	}
	return String(String::eSubString,str,len);
}


/*
// PrintfBinary prints bits with the MSB first
void lprintfBinary(uint32 val,int bits)
{
    if ( bits == 0 )
        return;
    for(--bits;bits>=0;bits--)
    {
        int b = (val>>bits)&1;

        if ( b ) lprintf("1");
        else lprintf("0");
    }
}

void lprintfBin2C(const uint8 * data,int size,const char * name)
{
    lprintf("\nstatic const int %s_size = %d;\n",name,size);
    lprintf("static const unsigned char %s[] = \n",name);
    lprintf("{\n  ");
    for(int i=0;i<size;i++)
    {
        lprintf("0x%02X",data[i]);
        if ( i < size-1 )
            lprintf(",");
        if ( (i%30) == 29 )
            lprintf("\n  ");
    }
    lprintf("\n};\n\n");
}

void lprintfIntArray(const int * data,int size,const char * name,int columns)
{
    lprintf("\nstatic const int %s_size = %d;\n",name,size);
    lprintf("static const int %s[] = \n",name);
    lprintf("{\n  ");
    for(int i=0;i<size;i++)
    {
        lprintf("%d",data[i]);
        if ( i < size-1 )
            lprintf(",");
        if ( (i%columns) == columns )
            lprintf("\n  ");
    }
    lprintf("\n};\n\n");
}

void lprintfFloatArray(const float * data,int size,const char * name,int columns)
{
    lprintf("\nstatic const int %s_size = %d;\n",name,size);
    lprintf("static const float %s[] = \n",name);
    lprintf("{\n  ");
    for(int i=0;i<size;i++)
    {
        lprintf("%.6ff",data[i]);
        if ( i < size-1 )
            lprintf(",");
        if ( (i%columns) == (columns-1) )
            lprintf("\n  ");
    }
    lprintf("\n};\n\n");
}

void lprintfDoubleArray(const double * data,int size,const char * name,int columns)
{
    lprintf("\nstatic const int %s_size = %d;\n",name,size);
    lprintf("static const double %s[] = \n",name);
    lprintf("{\n  ");
    for(int i=0;i<size;i++)
    {
        lprintf("%.6f",data[i]);
        if ( i < size-1 )
            lprintf(",");
        if ( (i%columns) == (columns-1) )
            lprintf("\n  ");
    }
    lprintf("\n};\n\n");
}

void lprintStringWithLength(const char *data, int len)
{
    for (int i=0; i < len; ++i)
    {
        rawlprintf("%c", data[i]);
	}
}
/**/


// skipcppcomments skips comments right at ptr
char * SkipCppComments(char * ptr)
{
	if ( *ptr == '/' )
	{
		if ( ptr[1] == '/' )
		{
			// cpp :
			ptr += 2;
			return strchrorend(ptr,'\n');
		}
		else if ( ptr[1] == '*' )
		{
			ptr += 2;
			return strstrorend(ptr,"*/");
		}
	}
	
	return ptr;
}

char * SkipCppCommentsAndWhiteSpace(char * ptr)
{
	for(;;)
	{
		char * start = ptr;
		ptr = skipwhitespace(ptr);
		ptr = SkipCppComments(ptr);
		if ( start == ptr )
			return ptr;
	}
}


char * FindMatchingBrace(const char * start, char open, char close)
{
	const char * ptr = start;
	ASSERT( *ptr == open );
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
		if ( *ptr == open )
		{
			depth++;
		}
		if ( *ptr == close )
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

char * MultiLineStrChr_SkipCppComments(const char * start,int c)
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

char * TokToComma_CurlyBraced(char * ptr)
{
	char * comma = strchr(ptr,',');
	if ( comma == NULL )
		return NULL;

	// skip over { } ; eg. handle "{a,b,c}," - I want to go to the comma
	//	after the ending brace, not any , inside
	char * brace = strchr(ptr,'{');
	if ( brace != NULL && brace < comma )
	{
		char * brace2 = FindMatchingBrace(brace,'{','}');
		if ( brace2 == NULL )
		{
			return NULL;
		}
		char * comma = strchr(brace2,',');
		if ( comma == NULL )
			return NULL;
	}

	*comma = 0;
	return comma+1;
}

// myatof handles commas like "1,240.50"
double myatof(const char * input)
{
	char buffer[80];
	char * pto = buffer;

	const char * ptr = input;
	ptr = skipwhitespace(ptr);
	
	if ( *ptr == '$' )
		ptr++;
	if ( *ptr == '+' )
		ptr++;
	
	if ( *ptr == '-' )
	{
		*pto++ = *ptr++;
	}
	
	while( (*ptr >= '0' && *ptr <= '9') || *ptr == ',' || *ptr == '.' )
	{
		if ( *ptr == ',' )
			ptr++;
		else
			*pto++ = *ptr++;		
	}
	*pto = 0;
	
	double ret = atof(buffer);
	
	return ret;
}

void itoacommas(int number,char * into)
{
	if ( number == 0 )
	{
		into[0] = '0';
		into[1] = 0;
		return;
	}
	
	bool neg = false;
	if ( number < 0 )
	{
		number = -number;
		neg = true;
	}
	
	char * ptr = into;	

	int numdigits = 0;

	while( number > 0 )
	{
		int next = number/10;
		int cur = number - (next*10);
		number = next;
		
		if ( numdigits == 3 )
		{
			*ptr++ = ',';
			numdigits = 0;
		}
		
		*ptr++ = (char) ( cur + '0' );
		numdigits++;
	}
	
	if ( neg )
	{
		*ptr++ = '-';
	}
	
	*ptr = 0;
	
	strrev(into);
}


void itoacommas64(int64 number,char * into)
{
	if ( number == 0 )
	{
		into[0] = '0';
		into[1] = 0;
		return;
	}
	
	bool neg = false;
	if ( number < 0 )
	{
		number = -number;
		neg = true;
	}
	
	char * ptr = into;	

	int numdigits = 0;

	while( number > 0 )
	{
		int64 next = number/10;
		int64 cur = number - (next*10);
		number = next;
		
		if ( numdigits == 3 )
		{
			*ptr++ = ',';
			numdigits = 0;
		}
		
		*ptr++ = (char) ( cur + '0' );
		numdigits++;
	}
	
	if ( neg )
	{
		*ptr++ = '-';
	}
	
	*ptr = 0;
	
	strrev(into);
}

//=====================================================

bool stripresameadvance( char ** pptr , const char * match )
{
	char *ptr = *pptr;
	ptr = skipwhitespace(ptr);
	if ( stripresame(ptr,match) )
	{
		ptr += strlen(match);
		ptr = skipwhitespace(ptr);
		*pptr = ptr;
		return true;
	}
	return false;
}

// stripresameadvanceskipwhite :
//	returns true if string at *pptr has a preisame match with "match"
//	skips past match if so
//	also skips pre and post white
bool stripresameadvance_skipcpp( char ** pptr , const char * match )
{
	char *ptr = *pptr;
	ptr = SkipCppCommentsAndWhiteSpace(ptr);
	if ( stripresame(ptr,match) )
	{
		ptr += strlen(match);
		ptr = SkipCppCommentsAndWhiteSpace(ptr);
		*pptr = ptr;
		return true;
	}
	return false;
}

// toktoadvance :
//	destructive tokenize starting at *pptr
//	up to any char in "endset" (eg. endset is something like ")},;")
//	fills ptokchar with the char of endset that was found
//	returned pointer is the token without head&tail white
//	advances *pptr to after the endset char
//	also skips pre and post white
char * toktoadvance_skipcpp( char ** pptr, const char * endset , char * ptokchar )
{
	char *ptr = *pptr;
	
	ptr = SkipCppCommentsAndWhiteSpace(ptr);
	char * start = ptr;
	
	ptr = strchrset(ptr,endset);
	if ( ptr == NULL )
		return NULL;
	
	if ( ptokchar )
		*ptokchar = *ptr;
	*ptr = 0;
	ptr++;
	ptr = SkipCppCommentsAndWhiteSpace(ptr);
	*pptr = ptr;
	
	killtailingwhiteandcomments(start);		
	return start;
}


size_t CountNumSame(const void * buf1,const void * buf2,size_t size)
{
	const uint8 * ptr1 = (const uint8 *) buf1;
	const uint8 * ptr2 = (const uint8 *) buf2;

	ASSERT_RELEASE( ptr1 && ptr2 );
	
	for(size_t numSame = 0;numSame < size;numSame++)
	{
		if ( ptr1[numSame] != ptr2[numSame] )
			return numSame;
	}	
	
	return size;
}

bool AreBuffersSame(const void * buf1,const void * buf2,size_t size)
{
	size_t sameLen = CountNumSame(buf1,buf2,size);
	
	if ( sameLen == size )
		return true;
		
	lprintf("same : %a/%a\n",sameLen,size);
	return false;
}


// nexttok :
//	return next * , modifies str
//	won't break quoted strings
//	so eg.   one, "two a, two b", three
//	is tokenized correctly
char * nexttok_skipquotes(char *str, char tok_delim)	/** modifies str! **/
{
	for(;;) 
	{
		if ( *str == tok_delim )
		{
			*str++ = 0;
			return skipwhitespace(str);
		}
	
		switch( *str ) 
		{
			case '"': // quote - seek to next quote
				str++;
				while(*str != '"') { if ( *str == 0 ) return NULL; str++; }
				break;
			case SingleQuote:		
				str++;
				while(*str != SingleQuote) { if ( *str == 0 ) return NULL; str++; }
				break;	
			case 0:
				return NULL;

			// tok delims :
			//case ' ': 
			//case '\t':
			//case '\r':
			case '\n': 
				//*str++ = 0;
				//return skipwhitespace(str);
				return str;
			default:
				break;
		}
		str++;
	}
}

END_CB

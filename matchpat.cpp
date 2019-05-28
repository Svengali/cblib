/***************
 * 
 * todo :
 *
 *		AND 
 *
 ************/

#include <cblib/matchpat.h>
#include <cblib/Util.h>
#include <cblib/FileUtil.h>
#include <string.h>
#include <ctype.h>

//#pragma warning(disable : 4018) // signed/unsigned compare
#pragma warning(disable : 4244) // conversion float/int/double

START_CB

/****

Pattern Spec :

A Pattern consists of alternating Tokens and Wilds

A Wild, '*' or '#?' matches any number of any character

All Tokens must be matched.

A Token consists of:

    raw characters
or  ranges of characters: [x-y] matches x,y,and all values between
or  lists of characters:  [a,b,c] matchs a or b or c
or  a single arbitrary character : ?

Tokens may be preceeded by a boolean-not '~'

Tokens may be OR-ed by listing options as (x|y|z)

Tokens may be AND-ed by listing options as {x&y&z}

Does not support #x functionality except for #?

` as a char to indicate that next is a raw literal:
    i.e. `~ to match the ~ character and `` to match the ` character

--------------------

Reserved characters:

`
*
?
(
|
&
)
[
]
{
}
~

--------------------

In RenameByPat , a *$ consumes a wild

eg.

Rename *hello* -> *$poop*
Takes "my hello world" -> "poop world"

--------------------

occasionally deeply recursive

can makes heavy use of the stack when multiple * or (||) branches are
involved.

--------------------

MakePattern could be a lot smarter

	` should only escape if the next char is a wild char
	[] should only be treated as brackets if there's a comma
	() etc

basically invalid wild sequences automatically become literals

****/


typedef uint16 patchar;
typedef patchar * pattern;

#define WANY  256 //*
#define WCHAR 257 //?
#define WNOT  258 //~
#define WBRA  259 //[
#define WKET  260 //]

#define PBASE 0x1000 // ( | & )
#define PMASK 0x0FFF
#define PTYPE 0xF000
#define POPENOR  0x1000
#define POPENAND 0x2000
#define PDONEOR  0x3000
#define PDONEAND 0x4000
#define POR   	 0x5000
#define PAND  	 0x6000

const patchar StaticPat_Any_Array[] = { WANY, 0 };
pattern StaticPat_Any = (pattern) StaticPat_Any_Array;

char * MakePatternError = NULL;

bool IsWildChar(char c)
{
	switch(c)
	{
	case '`':
	case '[':
	case ']':
	case '(':
	case ')':
	case '{':
	case '}':
	case '|':
	case '&':
	case '?':
	case '~':
	case '*':
	case '#':
		return true;
	
	default:
		return false;
	}
}
	
void GetNonWildPrefix(wchar_t * to, const wchar_t * fm)
{
	while( *fm && ! IsWildChar((char)*fm) )
	{
		*to++ = *fm++;
	}
	*to = 0;
}

bool strmatch3seq(const char * str,const char c1,const char c2,const char c3)
{
	const char * ptr = strchr(str,c1);
	if ( ! ptr )
		return false;
	
	ptr = strchr(ptr,c2);
	if ( ! ptr )
		return false;
		
	ptr = strchr(ptr,c3);
	if ( ! ptr )
		return false;
		
	return true;
}

bool strmatch2seq(const char * str,const char c1,const char c2)
{
	const char * ptr = strchr(str,c1);
	if ( ! ptr )
		return false;
	
	ptr = strchr(ptr,c2);
	if ( ! ptr )
		return false;
		
	return true;
}

pattern MakePattern(const char * SignedStr)
{
	pattern ret;
	int ri,nestOr,nestAnd;
	int bras,kets;

	const uint8 * str = (const uint8 *) SignedStr;

	MakePatternError = NULL;

	if ( (ret = (pattern) malloc(sizeof(patchar)*(strlen(SignedStr)*2+256))) == NULL )
		{ MakePatternError = "malloc"; return(NULL);}

	bras=kets=nestOr=nestAnd=ri=0;

	/* handle ` parsing */
	/* turn wild chars into masked (256 or more) */

	while(*str)
	{
	switch(*str++)
		{
		case '`':
			if ( IsWildChar(*str) )
			{
				// literal :
				ret[ri++] = *str++;				
			}
			else
			{
				// not wild, just take the raw char :
				ret[ri++] = str[-1];
			}
			break;

		case '[':
		
			if ( strmatch2seq((const char *)str,'-',']') ||
				strmatch2seq((const char *)str,',',']') )
			{
				ret[ri++] = WBRA; bras++;
			}
			else
			{
				// not wild, just take the raw char :
				ret[ri++] = str[-1];
			}
			break;
			
		case ']':
			if ( bras > kets )
			{
				ret[ri++] = WKET; kets++; 
			}
			else
			{
				// not wild, just take the raw char :
				ret[ri++] = str[-1];
			}	
			break;

		case '(':
			if ( strmatch2seq((const char *)str,'|',')') )
			{
				nestOr++;
				ret[ri++] = POPENOR + nestOr; 
			}
			else
			{
				// not wild, just take the raw char :
				ret[ri++] = str[-1];
			}
			break;
			
		case ')':
			if ( nestOr > 0 )
			{
				ret[ri++] = PDONEOR + nestOr; 
				nestOr--; 
			}
			else
			{
				// not wild, just take the raw char :
				ret[ri++] = str[-1];
			}			
			break;
		
		case '{':
			if ( strmatch2seq((const char *)str,'&','}') )
			{
				nestAnd++;
				ret[ri++] = POPENAND + nestAnd; 
			}
			else
			{
				// not wild, just take the raw char :
				ret[ri++] = str[-1];
			}
			break;
			
		case '}':
			if ( nestAnd > 0 )
			{
				ret[ri++] = PDONEAND + nestAnd; 
				nestAnd--; 
			}
			else
			{
				// not wild, just take the raw char :
				ret[ri++] = str[-1];
			}			
			break;
			
		case '|':
			if ( nestOr > 0 )
			{
				ret[ri++] = POR   + nestOr;
			}
			else
			{
				// not wild, just take the raw char :
				ret[ri++] = str[-1];
			}			
			break;
		case '&':
			if ( nestAnd > 0 )
			{
				ret[ri++] = PAND   + nestAnd;
			}
			else
			{
				// not wild, just take the raw char :
				ret[ri++] = str[-1];
			}			
			break;

		case '#':
			if ( *str == '?' ) 
			{
				str++;
				ret[ri++] = WANY;
			}
			else
			{
				// not wild, just take the raw char :
				ret[ri++] = str[-1];
			}			
			break;
			
		case '?':
			ret[ri++] = WCHAR; 
			break;
		case '~':
			ret[ri++] = WNOT; 
			break;
		case '*':
			ret[ri++] = WANY; 
			break;

		default: // literal
			ret[ri++] = str[-1];
			break;
		}
	}
	
	ret[ri] = 0;

	if ( nestOr != 0 ) { free(ret); MakePatternError = "unmatched parens"; return(NULL); }
	if ( nestAnd != 0 ) { free(ret); MakePatternError = "unmatched braces"; return(NULL); }
	if ( bras != kets) { free(ret); MakePatternError = "unmatched bra/ket"; return(NULL); }

	return(ret);
}

void FreePattern(pattern pat)
{
	if ( pat ) free(pat);
}

int PatternLen(pattern pat)
{
	int len = 0;
	while( pat[len] ) len++;
	return len;
}

bool IsWild(const char *Str)
{

	pattern pat = MakePattern(Str);
	
	if ( ! pat )
		return false;
		
	bool isWild = false;
	
	pattern ptr = pat;
	while(*ptr)
	{
		if ( *ptr >= 256 )
		{
			isWild = true;
			break;
		}
		
		ptr++;
	}
	
	FreePattern(pat);

	return isWild;
}


bool MatchPatternSub(const uint8 *VsStr,pattern PatStr,bool DoUpr)
{
#define LIT(zchar) ( DoUpr ? toupper(zchar) : zchar )

for(;;)
  {
  switch( *PatStr )
    {
    case WCHAR: /* match any one */
      PatStr++;
      if ( *VsStr == 0 ) return(0);
      VsStr++;
      break;
  
    case WBRA: /* match several */
      PatStr++;
      if ( PatStr[1] == '-' ) /* range */
        {
        if ( LIT(*VsStr) < LIT(PatStr[0]) || LIT(*VsStr) > LIT(PatStr[2]) )
          return(0);
        VsStr++;
        PatStr += 3;
        }
      else /* list */
        {
        while ( LIT(*PatStr) != LIT(*VsStr) )
          {
			PatStr++;
         	if ( *PatStr == WKET ) return(0);
			if ( LIT(*PatStr) == ',' ) PatStr++;
          }
        VsStr++;
        }
      while( *PatStr != WKET ) PatStr++;
      PatStr++;
      break;
    
    case WNOT: /* NOT */
      PatStr++;
      return( ! MatchPatternSub(VsStr,PatStr,DoUpr) );
  
    case WANY: /* WILD */
      PatStr++;
      if ( *PatStr == 0 ) return(1);

      while(*VsStr)
        {
        if ( MatchPatternSub(VsStr,PatStr,DoUpr) ) return(1);
        VsStr++;
        }
      return( MatchPatternSub(VsStr,PatStr,DoUpr) );
  
    default: /* raw character */

			if ( *PatStr >= PBASE ) /* and/or branch */
				{
				int nest;
				nest = *PatStr & PMASK;
				switch(*PatStr & PTYPE)
					{
					case POPENOR:
			      do
			        {
			        PatStr++;
			        if ( MatchPatternSub(VsStr,PatStr,DoUpr) ) return(1);
			        while ( *PatStr != (POR+nest) && *PatStr != (PDONEOR+nest) ) PatStr++;
			        } while( *PatStr != (PDONEOR+nest) );
			      return(0);

					case POPENAND:
			      do
			        {
			        PatStr++;
			        if ( ! MatchPatternSub(VsStr,PatStr,DoUpr) ) return(0);
			        while ( *PatStr != (PAND+nest) && *PatStr != (PDONEAND+nest) ) PatStr++;
			        } while( *PatStr != (PDONEAND+nest) );
			      return(1);

					case PDONEOR: /* left-overs from POPEN */
					case POR:
			      while ( *PatStr != (PDONEOR+nest) ) PatStr++;
			      PatStr++;
						break;

					case PDONEAND: /* left-overs from POPEN */
					case PAND:
			      while ( *PatStr != (PDONEAND+nest) ) PatStr++;
			      PatStr++;
						break;
					default:
						return(0); // error!
						break;	
					}
				}
			else /* raw character */
				{
	      if ( LIT(*VsStr) != LIT(*PatStr) ) 
			return(0);
	      if ( *VsStr == 0 && *PatStr == 0 ) return(1);
	      VsStr++; PatStr++;
				}
      break;
    }
  }
  //return(0);
}

bool MatchPatternNoCase(const char *str,pattern Pat)
{
	return MatchPatternSub((const uint8 *)str,Pat,true);
}
bool MatchPatternWithCase(const char *str,pattern Pat)
{
	return MatchPatternSub((const uint8 *)str,Pat,false);
}

//bool MatchPatternNoCase(char *str,pattern Pat) { return(MatchPatternSub(str,Pat,1)); }

/**

temp routine, turns all wilds into *
rarely does cause unexpected not-nice behavior

ren zz*?e zz*.*e

should do: zzqae -> zzq.ae
but instead does: zzqae -> zzqa.e

**/

void RenameByPatSimplify(pattern Fm,pattern To)
{
for(;;)
  {
  switch( *Fm )
    {
    case WBRA:
      *To++ = WANY;
      while ( *Fm != WKET ) Fm++;
      break;
    case WCHAR:
      *To++ = WANY;
      break;
    case WNOT:
      *To++ = WANY;
      *To = 0;
      return;
    case 0:
      *To = 0;
      return;
      
	default:
		if ( *Fm & PTYPE )
		{
			int nest = *Fm & PMASK;
			*To++ = WANY;
			while ( *Fm != (PDONEAND+nest) && *Fm != (PDONEOR+nest) ) 
			{
				Fm++;
			}
		}
		else
		{
			*To++ = *Fm;
		}
	break;
    }
  Fm++;
  }
}

/**

given the first three (FmPat,FmStr,ToPat), fill in the last (ToStr)

**/

bool RenameByPat(pattern FmPat,const char *FmStr,pattern ToPat,char *ToStr,bool keepCase)
{
// unused, for debug watch :
const char * FmStr_in = FmStr; FmStr_in;
char * ToStr_in = ToStr; ToStr_in;

// should be called with FmStr matching FmPat :
ASSERT( MatchPatternNoCase(FmStr,FmPat) );

int FmPatLen = PatternLen(FmPat);
int ToPatLen = PatternLen(ToPat);
STACK_ARRAY(FmPatT,patchar,FmPatLen+16);
STACK_ARRAY(ToPatT,patchar,ToPatLen+16);

RenameByPatSimplify(FmPat,FmPatT);
RenameByPatSimplify(ToPat,ToPatT);

/** now only literals and * characters remain **/

FmPat = FmPatT;
ToPat = ToPatT;
*ToStr = 0;

// verify again after RenameByPatSimplify :
ASSERT( MatchPatternNoCase(FmStr,FmPat) );

for(;;)
  {
  String toBuf;
  String fmBuf;
  String fmPatBuf;
  
  while ( *ToPat != WANY && *ToPat != 0 )
    {      
    toBuf += *ToPat++;
    }
    
  ASSERT( MatchPatternNoCase(FmStr,FmPat) );
  while ( *FmPat != WANY && *FmPat != 0 )
    {      
    fmBuf += *FmStr++;
    fmPatBuf += *FmPat++;
    }

	// flush toBuf to ToStr
	if ( keepCase )
	{
	  strcpyKeepCase(ToStr,toBuf.CStr(),fmBuf.CStr(),fmPatBuf.Length());
	  ASSERT( strlen32(ToStr) == toBuf.Length() );
	  ToStr = strend(ToStr);
	}
	else
	{
	  *ToStr = 0;
      strcat(ToStr,toBuf.CStr());
	  ToStr = strend(ToStr);
	}

    if ( ! *ToPat )
      {
      break;   /* ok for To to end early, it's a crop */
      }
      
    if (! *FmPat )
      {
      return (0); 	/* not Ok for Fm to end early, it's an underflow of input */
      }
      
	// now FmPat and ToPat both point at starts
	FmPat++; ToPat++; /* skip the stars */
         
    if ( ! *ToPat && ! *FmPat ) // both end
      {
      *ToStr = 0;
      // FmPat was a star , that eats rest of FmStr :
      strcat(ToStr,FmStr);
      break;
      }
      
	if ( *ToPat == '$' ) // ToPat was *$ - that means eat Fm match without putting anything
	{
		ToPat++; // skip the $
		
	  while ( *FmStr && ! MatchPatternNoCase(FmStr,FmPat) ) FmStr++;
	}
	else
	{
	  // copy the * to the * :
	  while ( *FmStr && ! MatchPatternNoCase(FmStr,FmPat) ) *ToStr++ = *FmStr++;
	}
	
	if ( ! *FmStr )
	  {
	  // if remainder of ToPat is literals, put them on
	  // as in "zr * *.png"
	  
	  while( *ToPat != 0 )
	  {
		if ( *ToPat == WANY ) return 0; // bad
		
		*ToStr++ = (char) *ToPat++;
	  }
	  *ToStr = 0;
	  
	  return (1);
	  }
	  
    if ( ! *ToPat )
      {
      *ToStr = 0;
      break;   /* ok for To to end early, it's a crop */
      }
  }

return (1);
}


pattern chshMakePattern(const char * spec)
{
	// does file exist :
	if ( FileExistsMatch(spec) )
	{		
		pattern ret;
		
		MakePatternError = NULL;
		
		int len = strlen32(spec);

		if ( (ret = (pattern) malloc(sizeof(patchar)*(len+1))) == NULL )
			{ MakePatternError = "malloc"; return(NULL);}

		//	manually build a pattern that's just all literals
		
		for(int i=0;i<len;i++)
		{
			ret[i] = ((const uint8 *)spec)[i];
		}
		ret[len] = 0;
		
		return ret;		
	}
	
	char specpath[1024];
	getpathpart(spec,specpath);
	const char * specfile = filepart(spec);
	
	if ( ! FileExistsMatch(specpath) )
	{
		return MakePattern(spec);
	}

	// path exists
	// put path on literally and then tack on the pattern :
	
	pattern filepartPat = MakePattern(specfile);
	if ( ! filepartPat )
		return NULL;
	
	pattern ret;
		
	if ( (ret = (pattern) malloc(sizeof(patchar)*2048)) == NULL )
		{ MakePatternError = "malloc"; return(NULL);}

	int specpathlen = strlen32(specpath);
	int i = 0;
	for(;i<specpathlen;i++)
	{
		ret[i] = ((const uint8 *)specpath)[i];
	}
	//ret[i] = '\\';
	//i++;
	
	for(int j=0;;i++,j++)
	{
		ret[i] = filepartPat[j];
		if ( filepartPat[j] == 0 )
			break;
	}
	
	FreePattern(filepartPat);
	
	return ret;
}


bool RenameByPat(pattern FmPat,const wchar_t *FmStr,
						pattern ToPat,wchar_t *ToStr, bool keepCase)
{
	char To[1024];

#pragma PRAGMA_MESSAGE("RenameByPat unicode not real")

	char ansi[1024];
	char oem[1024];
	
	StrConv_UnicodeToConsole(oem,FmStr,1024);
	StrConv_UnicodeToAnsi(ansi,FmStr,1024);

	bool ret = false;

	if ( MatchPatternNoCase(oem,FmPat) )
	{
		if ( RenameByPat(FmPat,oem,ToPat,To,keepCase) )
		{
			MakeUnicodeNameFullMatch(ToStr,To,1024,eMatch_Console);

			//StrConv_OemToUnicode(ToStr,To,1024);
			ret = true;
		}
	}
	else if ( MatchPatternNoCase(ansi,FmPat) )
	{
		if ( RenameByPat(FmPat,ansi,ToPat,To,keepCase) )
		{
			MakeUnicodeNameFullMatch(ToStr,To,1024,eMatch_Ansi);
		
			//StrConv_AnsiToUnicode(ToStr,To,1024);
			ret = true;
		}
	}
	
	if ( ! ret )
		return false;
	
	// if we only changed dirs then keep the correct unicode name :

	if ( strisame( filepart(ansi), filepart(To) ) ||
			strisame( filepart(oem), filepart(To) ) )
	{
		strcpy( filepart(ToStr) , filepart(FmStr) );
	}
	
	return true;
}

bool chshMatchPattern(const wchar_t *VsStr,pattern PatStr)
{
	char ansi[1024];
	char oem[1024];
	
	StrConv_UnicodeToConsole(oem,VsStr,1024);
	StrConv_UnicodeToAnsi(ansi,VsStr,1024);
	
	return
		MatchPatternNoCase(ansi,PatStr) || 
		MatchPatternNoCase(oem,PatStr); 
}

//---------------------------------------------------------

// strcpy "putString" to "into"
//  but change its case to match the case in src
// putString should be mixed case , the way you want it to be if src is mixed case
void strcpyKeepCase(
		char * into,
		const char * putString,
		const char * src,
		int srcLen)
{	
	// okay, I have a match
	// what type of case is "src"
	//	all lower
	//	all upper
	//	first upper
	//	mixed
	
	int numLower = 0;
	int numUpper = 0;
	
	for(int i=0;i<srcLen;i++)
	{
		ASSERT( src[i] != 0 );
		if ( isalpha(src[i]) )
		{
			if ( isupper(src[i]) ) numUpper++;
			else numLower++;
		}
	}
	
	// non-alpha :
	if ( numLower+numUpper == 0 )
	{
		strcpy(into,putString);
	}
	else if ( numLower == 0 )
	{
		// all upper :
		while( *putString )
		{
			*into++ = toupper( *putString ); putString++;
		}
		*into = 0;
	}
	else if ( numUpper == 0 )
	{
		// all lower :
		while( *putString )
		{
			*into++ = tolower( *putString ); putString++;
		}
		*into = 0;
	}
	else if ( numUpper == 1 && isalpha(src[0]) && isupper(src[0]) )
	{
		// first upper then low
		
		if( *putString ) //&& isalpha(*putString) )
		{
			*into++ = toupper( *putString ); putString++;
		}
		while( *putString )
		{
			*into++ = tolower( *putString ); putString++;
		}
		*into = 0;
	}
	else
	{
	
		/*
		// fully mixed :
		if ( strlen(putString) == srcLen )
		{
			// same len , transfer case
			// this is not a good idea, eg.
			//	WoaBy -> HeMen should not transfer case letter by letter
			while( *putString )
			{
				ASSERT(*src);
				if ( isalpha(*src) )
				{
					if ( islower(*src) )
						*into = tolower( *putString );
					else
						*into = toupper( *putString );
				}
				else *into = *putString;
				
				into++;
				src++;
				putString++;
			}
			*into = 0;
		}
		else
		/**/
		
		{
			// just copy putString - it should be mixed	
			strcpy(into,putString);
		}
	}
}
						
END_CB

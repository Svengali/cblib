#pragma once

#include "Base.h"
#include <string.h>

START_CB

typedef uint16 * pattern; /** outside of matchpat.c , patterns are const **/

extern pattern StaticPat_Any; /* "*" */

extern char * MakePatternError; /*  if MakePat returns null, this has more info */

pattern MakePattern(const char *str); /* returns from malloc */
void FreePattern(pattern pat);
int PatternLen(pattern pat);


bool IsWildChar(char c);
bool IsWild(const char *Str);
bool MatchPatternNoCase(const char *VsStr,pattern PatStr);
bool MatchPatternWithCase(const char *VsStr,pattern PatStr);

bool RenameByPat(pattern FmPat,const char *FmStr,
						pattern ToPat,char *ToStr, bool keepCase);
  /** given the first three, fill in the last **/

bool RenameByPat(pattern FmPat,const wchar_t *FmStr,
						pattern ToPat,wchar_t *ToStr, bool keepCase);

pattern chshMakePattern(const char * spec);
bool chshMatchPattern(const wchar_t *VsStr,pattern PatStr);

void GetNonWildPrefix(wchar_t * to, const wchar_t * fm);

//---------------------------------------------------------

// strcpy "putString" to "into"
//  but change its case to match the case in src
// putString should be mixed case , the way you want it to be if src is mixed case
void strcpyKeepCase(
		char * into,
		const char * putString,
		const char * src,
		int srcLen);
		
//---------------------------------------------------------

END_CB

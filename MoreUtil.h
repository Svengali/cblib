#pragma once

#include "Util.h"
#include "String.h"
#include "vector.h"

START_CB

//---------------------------------------------------
// Log helpers :
// see LogUtil.h instead

bool IsStringInVecI(const char * str, const vector<String> & vec);
bool IsStringInVecC(const char * str, const vector<String> & vec);
bool IsStringPrefixInVecI(const char * str, const vector<String> & vec);
bool IsStringPrefixInVecC(const char * str, const vector<String> & vec);
bool IsStringPortionInVecI(const char * str, const vector<String> & vec);
bool IsStringPortionInVecC(const char * str, const vector<String> & vec);
bool CheckExtensionAllowed(const char * baseFileName,vector<String> & extensionsAllowed);
// extensionFilter is ";" separated like "cpp;c;h"
void ParseSemicolonsToStringVec(const char * extensionFilter,vector<String> & extensionsAllowed);

//-----------------------------------------------------------

// stripresameadvanceskipwhite :
//	returns true if string at *pptr has a preisame match with "match"
//	skips past match if so
//	also skips pre and post white
bool stripresameadvance_skipcpp( char ** pptr , const char * match );
bool stripresameadvance( char ** pptr , const char * match );

// toktoadvance :
//	destructive tokenize starting at *pptr
//	up to any char in "endset" (eg. endset is something like ")},;")
//	fills ptokchar with the char of endset that was found
//	returned pointer is the token without head&tail white
//	advances *pptr to after the endset char
//	also skips pre and post white
char * toktoadvance_skipcpp( char ** pptr, const char * endset , char * ptokchar = NULL);

inline char * toktoeoladvance_skipcpp( char ** pptr, char * ptokchar = NULL)
{
	return toktoadvance_skipcpp(pptr,"\r\n",ptokchar);
}

inline bool stripresameadvance( char const ** pptr , const char * match )
{
	return stripresameadvance( (char **) pptr, match );
}
inline bool stripresameadvance_skipcpp( char const ** pptr , const char * match )
{
	return stripresameadvance_skipcpp( (char **) pptr, match );
}
//---------------------------------------------------

// copy str to a String but without the white on head or tail :
String KillHeadTailWhite(const char * str);
// also remove surrounding quotes, if any :
String KillHeadTailWhiteAndQuotes(const char * str);

// GetSubStr : return string in between two delims, not including the delims
//	does not kill head/tail white for you
//	GetSubStr is non-destructive
String GetSubStr(const char * str,char startDelim,char endDelim, const char ** pAfter);

// SkipCppComments can be used on a single-line, in which case it only works for C++ style // comments
//	or it can be used on a while file buf, in which case it works on C-style /* */ comments too
// SkipCppComments does not skip white space, it only skips a comment that occurs right at *ptr
char * SkipCppComments(char * ptr );

inline const char * SkipCppComments(const char * ptr ) { return (const char *) SkipCppComments( (char *) ptr ); }

char * SkipCppCommentsAndWhiteSpace(char * ptr );

inline const char * SkipCppCommentsAndWhiteSpace(const char * ptr ) { return (const char *) SkipCppCommentsAndWhiteSpace( (char *) ptr ); }


// FindMatchingBrace should be called with *start == open
//	returns pointer *past* the last matching brace
char * FindMatchingBrace(const char * start, char open, char close);

// MultiLineStrChr
char * MultiLineStrChr_SkipCppComments(const char * start,int c);
char *TokToComma_CurlyBraced( char *ptr );
char *TokToComma( char *ptr );

// next = nexttok_skipquotes(cur,',') for csv parsing
char * nexttok_skipquotes(char *str, char tok_delim);	/** modifies str! **/

// myatof handles commas like "1,240.50"
double myatof(const char * input);

void itoacommas(int number,char * into);
void itoacommas64(int64 number,char * into);

//---------------------------------------------------

struct VarianceAccumulator
{
	double count;
	double sum;
	double sumAbs;
	double sumSqr;
	double lo,hi;

	VarianceAccumulator() : count(0), sum(0), sumAbs(0), sumSqr(0), lo(3.402823466e+38F), hi(-3.402823466e+38F) { }
	
	void Reset()
	{
		ZERO_VAL(*this);
	}
	
	void Add(double x)
	{
		count += 1;
		sum += x;
		sumAbs += fabs(x);
		sumSqr += x*x;
		lo = MIN(lo,x);
		hi = MAX(hi,x);
	}
	
	void Add(const VarianceAccumulator & rhs)
	{
		count += rhs.count;
		sum += rhs.sum;
		sumAbs += rhs.sumAbs;
		sumSqr += rhs.sumSqr;
		lo = MIN(lo,rhs.lo);
		hi = MIN(hi,rhs.hi);
	}
	
	
	void operator += (double rhs)
	{
		Add(rhs);
	}
	
	void operator += (const VarianceAccumulator & rhs)
	{
		Add(rhs);
	}
	
	double GetMean()   const { if ( count == 0 ) return 0; return sum/count; }
	double GetL1Mean() const { if ( count == 0 ) return 0; return sumAbs/count; }
	double GetL2Mean() const { if ( count == 0 ) return 0; return sqrt(sumSqr/count); }
	double GetVariance() const
	{
		if ( count <= 1 ) return 0;
		//double V = (sumSqr/count - fsquare(sum/count))*count/(count-1);
		double V = (sumSqr - sum*sum/count)/(count-1.0);
		// V should be >= 0 but can go slightly neg due to floating point
		return MAX(V,0);
	}
	double GetSDev() const { return sqrt(GetVariance()); }
};

//---------------------------------------------------

size_t CountNumSame(const void * buf1,const void * buf2,size_t size);
bool AreBuffersSame(const void * buf1,const void * buf2,size_t size);

END_CB

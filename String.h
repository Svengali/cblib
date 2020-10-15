#pragma once

#include "Base.h"
#include "Util.h"
#include "StrUtil.h"
#include "safeprintf.h"
//#include "vector.h" // ICK this is just for function protos
//#include "stl_basics.h" // need vecto
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**

"String" is a COW String, so you can share it easily and not waste memory.
String is basically a smart pointer to the char data.

WARNING : String is NOT thread safe!  Any usage of Strings that may be shared across
threads must be manually protected with critical sections.

The biggest advantage of COW is that it makes vector< String > much more efficient,
due to the many uses of operator= (COW is a huge optimization for operator=)

Also you can just pass it around by value and any function in the chain can modify a copy (or not) and it's all fast

-----------------

NOTE : using temporary stack Strings as arguments to printf() and such is indeed perfectly safe,
because C++ says that temporary returned objects live until the whole line finishes, so you can do
printf("%s",TempString().CStr());

-----------------

I specifically do NOT provide operator char * and other such unsafe conversions.

-----------------

NOTE : you can hammer on the char data of the string all you want AS LONG AS YOU DONT CHANGE LENGTH.
Any adding or removing of nulls must be done through the APIs (Append/Truncate/etc.)

**/

START_CB

template <class t_entry> class vector;
//typedef vector<char> vector_char;

struct StringData;

/*
namespace std {
template<class T> class vector;
};
*/

class String
{
public:	
	enum EEmpty		{ eEmpty };
	enum EReserve	{ eReserve };
	enum ESubString	{ eSubString };
	enum EConcat	{ eConcat };
	enum EPrintf	{ ePrintf };

			 String();	// makes an empty string.
	         String( const String &str );
	explicit String( const EEmpty e );
	explicit String( const char *const pStr );
	//explicit String( const char c );
	explicit String( const EReserve e, const int reserve );

	// @@ this ePrintf sucks, but if I don't do it it makes the basic char * constructor
	//	ambiguous; I could get rid of that and always go through here, but that's lame too
	//explicit String( const EPrintf e, const char *format, ... );

	         String( const EReserve e, const char * const pStr, const int reserve);
	         String( const ESubString e, const char * const pStr, const int len);
	         String( const EConcat e, const char * const pStr1, const char * const pStr2);

	~String();

	/** Return a reference to an empty String. */
	static const String& GetEmpty();
	bool IsEmpty() const { return Length() == 0; }

	// Assigns :
	void Set( const char *const pStr );
	void operator =(const String& str);
	void operator =( const char * const pStr ) { Set(pStr); }

	//Appends
	void Append( const char *const pStr );
	void Append( const char c);
	void Append( const String &str) { Append(str.CStr()); }

	//Insert :
	void Insert(const int at, const char *const pStr );
	void Insert(const int at, const char c);
	//void Insert(const int at, const String &str) { Append(str.CStr()); }

	// FindReplace returns the number of times "from" was found
	int FindReplace(const char * const from, const char * const to );

	void operator +=( const char * const  pStr  ) { Append(pStr); }
	void operator +=( const String &str   ) { Append(str); }
	void operator +=( const char c ) { Append(c); }

	// call Printf() or CatPrintf()
	//	they will redirect to these
	void rawPrintf( const char *format, ... );
	// catprintf sticks it on the back
	void rawCatPrintf( const char *format, ... );

	// convert pointer in me to an Index (throws if bad)
	int Index(const char * ptr) const;

	char GetChar( const int index ) const;
	char operator []( const int index ) const;
	
	// SetCharProxy handles doing str[i] = 0;
	class SetCharProxy;
	SetCharProxy operator []( const int index );

	char operator []( const char * ptr ) const;
	SetCharProxy operator []( const char * ptr );
	
	char PopBack();
	
	void Truncate( const int newLength ); 
	//! SetChar(index,0) truncates to length "index"
	//! Truncate takes care of truncation.
	void SetChar( const int index, const char c );
	// Split is like SetChar(index,0) - returns the tail that's cut off
	String Split( const int index );

	//Comparisons
	bool operator ==( const String &str ) const;	
	bool operator !=( const String &str ) const;
	bool operator < ( const String &str ) const;
		
	bool operator ==( const char *const pStr ) const;	
	bool operator !=( const char *const pStr ) const;
	bool operator < ( const char *const pStr ) const;		

	//! Change the amount reserved for string.
	//! Does not truncate 
	void Reserve( const int amount );

	//! @name Get at the string in a (const char *) kind of way
	const char *CStr( void ) const;
	
	// WriteableCStr : do NOT set nulls !
	//	eh. actually you CAN set nulls, but if you do, then do not call any String() interfaces
	//	 until you call FixLength
	//  eg. :
	//		char * buf = str.WriteableCStr(maxlength);
	//		.. jam on buf but do not touch str ! ..
	//		str.FixLength();
	char * WriteableCStr( int size = 0 );
	void FixLength();
	
	// CB - try the automatic conversion and see how it works out
	// this is horrific, it allows conversion of String to bool !!
	//const char * operator ()() const { return CStr(); }
	//operator const char * () const { return CStr(); }

	//! The length in chars till the first 0 or the end of the buffer
	int Length( void ) const;

	//! The length of the buffer
	int Capacity( void ) const;

	void Clear();
	void Release();

	bool IsValid() const;

	// fast swap :
	//void Swap(String * pOther);
	void Swap(String & rhs);
	
	void WriteBinary(FILE * fp) const;
	void ReadBinary(FILE * fp);
	void WriteText(FILE * fp) const; // just writes the string, no delimiters
	//void ReadText(FILE * fp);

	// Printf & CatPrintf :

#define SPI_SAFEDECL void Printf
#define SPI_CALLRAW rawPrintf
#define SPI_PREARG
#define SPI_CALLARG fmt
#define SPI_BADRETURN
#include "safeprintf.inc"
#undef SPI_SAFEDECL
#undef SPI_CALLRAW
#undef SPI_PREARG
#undef SPI_CALLARG
#undef SPI_BADRETURN

#define SPI_SAFEDECL void CatPrintf
#define SPI_CALLRAW rawCatPrintf
#define SPI_PREARG 
#define SPI_CALLARG fmt
#define SPI_BADRETURN
#include "safeprintf.inc"
#undef SPI_SAFEDECL
#undef SPI_CALLRAW
#undef SPI_PREARG
#undef SPI_CALLARG
#undef SPI_BADRETURN

private:

	StringData *	m_pData;
	const vector<char> & Readable() const;
	vector<char> & Writeable();
	vector<char> & WriteableEmpty(const int reserve = 16);
};

//---------------------------------------------------------------------------

class String::SetCharProxy
{
public:
	SetCharProxy(String * str,const int i) : 
		m_str(str), m_index(i)
	{
	
	}
	
	operator char () const
	{
		return m_str->GetChar(m_index);
	}
	
	void operator = (const char c)
	{
		m_str->SetChar( m_index, c );	
	}
	void operator = (const String::SetCharProxy & rhs)
	{
		const char c = rhs;
		*this = c;
	}
	

private:
	String * m_str;
	int m_index;
};

//---------------------------------------------------------------------------

bool operator ==( const char * const pStr, const String &str );
bool operator !=( const char * const pStr, const String &str );
bool operator < ( const char * const pStr, const String &str );

// non-member Cat functions
inline const String operator +(const String & s1,const String & s2)
{
	return String(String::eConcat,s1.CStr(),s2.CStr());
}
inline const String operator +(const String & s1,const char * s2)
{
	return String(String::eConcat,s1.CStr(),s2);
}
inline const String operator +(const char * s1,const String & s2)
{
	return String(String::eConcat,s1,s2.CStr());
}

inline char String::operator []( const int i ) const
{
	return GetChar(i);
}

inline String::SetCharProxy String::operator [] (const int i)
{
	return SetCharProxy(this,i);
}

inline char String::operator []( const char * ptr ) const
{
	return GetChar( Index(ptr) );
}

inline String::SetCharProxy String::operator []( const char * ptr ) 
{
	return SetCharProxy(this, Index(ptr) ); 
}
	
inline bool LessI(const String &s1,const String &s2)
{
	return stricmp(s1.CStr(),s2.CStr()) < 0;
}

inline bool EqualsI(const String &s1,const String &s2)
{
	return strisame(s1.CStr(),s2.CStr());
}

struct LessIFunc // : public binary_function
{
	bool operator() (const String &s1,const String &s2) const
	{
		return LessI(s1,s2);
	}
};

String rawStringPrintf(const char *fmt, ...);
String StringRawPrintfVA(const char *pFormat, va_list varargs);
 
#define SPI_SAFEDECL String StringPrintf
#define SPI_CALLRAW return rawStringPrintf
#define SPI_PREARG 
#define SPI_CALLARG fmt
#define SPI_BADRETURN return String("invalid");
#include "safeprintf.inc"
#undef SPI_SAFEDECL
#undef SPI_CALLRAW
#undef SPI_PREARG
#undef SPI_CALLARG
#undef SPI_BADRETURN

//---------------------------------------------------------------------------

END_CB

//---------------------------------------------------------------------------
// overload swap<> on String
// need to make sure we see the generic template decl before we overload

/*
#include "stl_basics.h"

CB_STL_BEGIN

template<> inline
void swap<cb::String>(cb::String& _Left, cb::String& _Right)
{	// exchange values stored at _Left and _Right
	_Left.Swap(&_Right);
}

CB_STL_END
/**/

CB_DEFINE_MEMBER_SWAP(String,Swap);

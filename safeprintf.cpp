#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#include "safeprintf.h"
#include <ctype.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h> // for OutputDebugString

/**

When there's an error the run-time message looks like this :

safeprintf type error; wanted (charptr), found (float)
safeprintf err in fmt "%s/%d = %s"
safeprintf err at : " = %s"

----

ARG : one really annoying things is that enums count as "classes" so you have to cast them to int
to make them printf normally :(

**/

START_CB

const char * c_safeprintftypenames[] =
{
	"none",
	"unknown",
	"charptr",
	"wcharptr",
	"float",
	"ptrint",
	"ptrvoid",
	"int32",
	"int64",
	"uint32",
	"uint64"
};

ESafePrintfType safeprintf_fmttype(const char fmt, bool wide)
{
	/**

	c int or wint_t When used with printf functions, specifies a single-byte character; when used with wprintf functions, specifies a wide character. 
	C int or wint_t When used with printf functions, specifies a wide character; when used with wprintf functions, specifies a single-byte character. 
	d int Signed decimal integer. 
	i int  Signed decimal integer. 
	o int  Unsigned octal integer. 
	u int  Unsigned decimal integer. 
	x int Unsigned hexadecimal integer, using "abcdef." 
	X int Unsigned hexadecimal integer, using "ABCDEF." 
	e  double Signed value having the form [ – ]d.dddd e [sign]ddd where d is a single decimal digit, dddd is one or more decimal digits, ddd is exactly three decimal digits, and sign is + or –. 
	E double Identical to the e format except that E rather than e introduces the exponent. 
	f double Signed value having the form [ – ]dddd.dddd, where dddd is one or more decimal digits. The number of digits before the decimal point depends on the magnitude of the number, and the number of digits after the decimal point depends on the requested precision. 
	g double Signed value printed in f or e format, whichever is more compact for the given value and precision. The e format is used only when the exponent of the value is less than –4 or greater than or equal to the precision argument. Trailing zeros are truncated, and the decimal point appears only if one or more digits follow it. 
	G double Identical to the g format, except that E, rather than e, introduces the exponent (where appropriate). 
	n  Pointer to integer  Number of characters successfully written so far to the stream or buffer; this value is stored in the integer whose address is given as the argument. 
	p Pointer to void Prints the address of the argument in hexadecimal digits. 
	s String  When used with printf functions, specifies a single-byte–character string; when used with wprintf functions, specifies a wide-character string. Characters are printed up to the first null character or until the precision value is reached. 
	S 

	*/
	switch( tolower(fmt) )
	{
	case 'c': 
	case 'd':
	case 'i':
	case 'o':
		if ( wide )
			return safeprintf_int64;
		else
			return safeprintf_int32;
	case 'u':
	case 'x':
		if ( wide )
			return safeprintf_uint64;
		else
			return safeprintf_uint32;
	case 'z':
		if ( sizeof(size_t) == sizeof(int64) )
			return safeprintf_uint64;
		else
			return safeprintf_uint32;		
	case 'e':
	case 'f':
	case 'g':
		return safeprintf_float;
	case 'n':
		return safeprintf_ptrint;
	case 'p':
		return safeprintf_ptrvoid;
	case 's':
		if ( fmt == 'S' )
			return safeprintf_wcharptr;
		else
			return safeprintf_charptr;
	default:
		return safeprintf_unknown;
	}	
}

const char * safeprintf_fmtskipwidth(const char * ptr, bool * pWide)
{
	*pWide = false;
			
	// h | l | I | I32 | I64
	if ( *ptr == 'h' || *ptr == 'l' || *ptr == 'w' )
	{
		// char / string modifiers
		return ptr+1;
	}
	else if ( *ptr == 'I' )
	{
		if ( ptr[1] == '3' && ptr[2] == '2' )
		{
			return ptr+3;
		}
		else if ( ptr[1] == '6' && ptr[2] == '4' )
		{
			*pWide = true;
			return ptr+3;
		}
		else
		{
			// (size_t) size
			if ( sizeof(size_t) == sizeof(int64) )
				*pWide = true;

			return ptr+1;
		}
	}
	else
	{
		return ptr;
	}
}

ESafePrintfType safeprintf_findfmtandadvance(const char ** pptr)
{
	const char * ptr = *pptr;
	const char * startPtr = ptr;
	REFERENCE_TO_VARIABLE(startPtr);
	// find the next % arg :
	for(;;)
	{
		ptr = strchr(ptr,'%');
		if ( ! ptr )
		{
			*pptr = NULL;
			return safeprintf_none;
		}
		ASSERT( ptr[0] == '%' );
		if ( ptr[1] == '%' )
		{
			ptr += 2;
			continue;
		}
		else
		{
			break;
		}
	}
	ASSERT( ptr[0] == '%' );
	ptr++;
	while( ! isalpha(*ptr) )
	{
		if ( ! *ptr )
		{
			*pptr = NULL;
			return safeprintf_unknown;
		}
		ptr++;
	}
	ASSERT( isalpha(*ptr) );
	
	bool wide = false;
	ptr = safeprintf_fmtskipwidth(ptr,&wide);
	
	ESafePrintfType fmttype = safeprintf_fmttype(*ptr,wide);
	
	*pptr = ptr+1;
	
	return fmttype;
}

//-----------------------------------------------------------------------------
// config :

bool safeprintf_checkintunsigned = false;
bool safeprintf_checkintasfloat = false;
bool safeprintf_checkintsize = true;
bool safeprintf_noisy = true;

void safeprintf_setoptions(bool noisy,bool checkintsize,bool checkintasfloat,bool checkintunsigned)
{
	safeprintf_checkintunsigned = checkintunsigned;
	safeprintf_checkintsize = checkintsize;
	safeprintf_checkintasfloat = checkintasfloat;
	safeprintf_noisy = noisy;
}

void safeprintf_throwsyntaxerror(const char *fmt_base,const char *fmt)
{
	if ( safeprintf_noisy )
	{
		char buffer[4096];
		buffer[0] = 0;
		sprintf(buffer+strlen(buffer),"safeprintf syntax err in fmt \"%s\"\n",fmt_base);
		sprintf(buffer+strlen(buffer),"safeprintf err at : \"%s\"\n",fmt);
		
		// can't call Log cuz it might use me
		fputs(buffer,stderr);
		OutputDebugString(buffer);
		
		ASSERT_BREAK();
	}

	THROW;
}

static inline bool isIntFmt(ESafePrintfType fmt)
{
	return fmt >= safeprintf_int32;
}
static inline int getIntFmtWidth(ESafePrintfType fmt)
{
	if ( fmt == safeprintf_int32 || fmt == safeprintf_uint32 )
		return 0;
	else
		return 1;
}
static inline int getIntFmtSign(ESafePrintfType fmt)
{
	if ( fmt == safeprintf_int32 || fmt == safeprintf_int64 )
		return 0;
	else
		return 1;
}

void safeprintf_throwerror(const char *fmt_base,const char *fmt,ESafePrintfType fmttype,ESafePrintfType argtype)
{
	ASSERT( fmttype != argtype );
	
	if ( isIntFmt(fmttype) && isIntFmt(argtype) )
	{
		// %d vs %u / etc was passed an int ; can be benign ?
		
		bool sizeOk =
		 ( ! safeprintf_checkintsize ||
			 getIntFmtWidth(fmttype) == getIntFmtWidth(argtype) );
		
		bool signOk =
		 ( ! safeprintf_checkintunsigned ||
			 getIntFmtSign(fmttype) == getIntFmtSign(argtype) );
		
		if ( sizeOk && signOk )
			return; // no error		
	}
	
	if( fmttype == safeprintf_float &&
		(argtype == safeprintf_int32 ||
		 argtype == safeprintf_int64 ||
		 argtype == safeprintf_uint32 ||
		 argtype == safeprintf_uint64) )
	{
		// %f was passed an int ; can be benign ?
		if ( ! safeprintf_checkintasfloat )
			return;
	}
	
	if ( safeprintf_noisy )
	{
		char buffer[4096];
		buffer[0] = 0;
		sprintf(buffer+strlen(buffer),"safeprintf type error; wanted (%s), found (%s)\n",c_safeprintftypenames[fmttype],c_safeprintftypenames[argtype]);
		sprintf(buffer+strlen(buffer),"safeprintf err in fmt \"%s\"\n",fmt_base);
		sprintf(buffer+strlen(buffer),"safeprintf err at : \"%s\"\n",fmt);
		
		// can't call Log cuz it might use me
		fputs(buffer,stderr);
		OutputDebugString(buffer);
		
		ASSERT_BREAK();
	}

	THROW;
}

void test_safeprintf()
{
	safeprintf("hello %d\n",1);
	safeprintf("hello %03d\n",1);
	safeprintf("hello % 3d\n",1,2);
	safeprintf("hello %+3d\n",1);
	safeprintf("hello %d,%f\n",1);
	safeprintf("hello %03d,%f\n",1);
	safeprintf("hello % 3d,%f\n",1);
	safeprintf("hello %+3d,%f\n",1);
	safeprintf("hello %d,%f\n",1,2);
	safeprintf("hello %d %d %s\n",1,2,"test");
	safeprintf("hello %d %S %s\n",1,2,"test");
}

END_CB





// playing around with "Printfer" here :
//	not really much advantage over just calling CatPrintf a few times

//===============================================================================
/*

class Printfer;

template <typename T1>
class Printfer1;

template <typename T1, typename T2>
class Printfer2;

template <typename T1, typename T2, typename T3>
class Printfer3;

template <typename T1, typename T2, typename T3>
class Printfer3
{
public:
	Printfer3(const char * fmt,T1 v1,T2 v2,T3 v3) : m_fmt(fmt), m_v1(v1), m_v2(v2), m_v3(v3)
	{
	}

	operator String() const
	{
		String ret;
		ret.Printf(m_fmt,m_v1,m_v2,m_v3);
		return ret;
	}
	
	const char * m_fmt;
	T1 m_v1;
	T2 m_v2;
	T3 m_v3;
};

template <typename T1, typename T2>
class Printfer2
{
public:
	Printfer2(const char * fmt,T1 v1,T2 v2) : m_fmt(fmt), m_v1(v1), m_v2(v2)
	{
	}

	operator String() const
	{
		String ret;
		ret.Printf(m_fmt,m_v1,m_v2);
		return ret;
	}
	
	template <typename T3>
	Printfer3<T1,T2,T3> operator << (T3 v3)
	{
		return Printfer3<T1,T2,T3>(m_fmt,m_v1,m_v2,v3);
	}

	Printfer3<T1,T2,const char *> operator << (const String & str)
	{
		return (*this) << str.CStr();
	}
	
	const char * m_fmt;
	T1 m_v1;
	T2 m_v2;
};

template <typename T1>
class Printfer1
{
public:
	Printfer1(const char * fmt,T1 v1) : m_fmt(fmt), m_v1(v1)
	{
	}
	
	operator String() const
	{
		String ret;
		ret.Printf(m_fmt,m_v1);
		return ret;
	}
	
	template <typename T2>
	Printfer2<T1,T2> operator << (T2 v2)
	{
		return Printfer2<T1,T2>(m_fmt,m_v1,v2);
	}

	Printfer2<T1,const char *> operator << (const String & str)
	{
		return (*this) << str.CStr();
	}
	
	const char * m_fmt;
	T1 m_v1;
};
	
class Printfer
{
public:

	Printfer(const char * fmt) : m_fmt(fmt)
	{
	}

	operator String() const
	{
		String ret;
		ret.Printf(m_fmt);
		return ret;
	}

	template <typename T1>
	Printfer1<T1> operator << (T1 v1)
	{
		return Printfer1<T1>(m_fmt,v1);
	}
	
	//template <String>
	Printfer1<const char *> operator << (const String & str)
	{
		return (*this) << str.CStr();
	}
	
	const char * m_fmt;
};

String operator << (const String & s1,const String & s2)
{
	return String(String::eConcat,s1.CStr(),s2.CStr());
}

void Printfer_Test()
{
	String s1 = Printfer("hello world");
	String s2 = Printfer("hello world %d") << 1;
	s1 += Printfer(" more world %d %s") << 1 << "test";

	String s3 = Printfer("hello world %s") << s1;
	String s4 = Printfer("hello world %d %s") << 1 << s1;
	
	String s5 = (Printfer("stuff %d %d") << 1 << 2) << (Printfer(" floats %3f %s") << 1.7 << "cool");
	
}

*/
//===============================================================================

#pragma once

#ifdef _DEBUG
#define DO_ASSERTS
#endif

// @@@@ TODO : should have a release build with ASSERTS where the assert just logs

#define _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS

#pragma warning(disable : 4127) // conditional is constant
#pragma warning(disable : 4100) // unreferenced formal parameter
#pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union
#pragma warning(disable : 4505) // unreferenced local function has been removed
#pragma warning(disable : 4702) // unreachable code

#pragma warning(disable : 4996) // deprecated POSIX

//#pragma pack(4) // default is 8

// my namespace :
#define USE_CB		using namespace cb;
#define NS_CB		cb
#define START_CB	namespace cb {
#define END_CB		};

START_CB

//-------------------------------------------------------------------

// define NULL here so that stdlib.h isn't necessary
#ifndef NULL
#define NULL 0
#endif

//  types.
typedef char   				int8;
typedef short				int16;
typedef int					int32;
typedef __int64        		int64;

typedef unsigned char   	uint8;
typedef unsigned short		uint16;
typedef unsigned int		uint32;
typedef unsigned __int64	uint64;

#ifdef  _WIN64
#define CB_64
#endif

//-----------------------------------------------------------

#define CLASS_ABSTRACT_BASE class __declspec(novtable)

#define DECL_ALIGN(x)	__declspec(align(x))

//! Silences compiler warning about unused parameters/variables
#define UNUSED_PARAMETER(x)	x
#define REFERENCE_TO_VARIABLE	UNUSED_PARAMETER

//! Disallows the compiler defined default ctor
#define FORBID_DEFAULT_CTOR(x) x()

//! Disallows the compiler defined copy ctor
#define FORBID_COPY_CTOR(x)    x(const x&)

//! Disallows the compiler defined assignment operator
#define FORBID_ASSIGNMENT(x)   void operator=(const x&)

#define FORBID_CLASS_STANDARDS(x)	\
	FORBID_ASSIGNMENT(x);	\
	FORBID_COPY_CTOR(x)	

#define MAKE_COMPARISONS_FROM_LESS_AND_EQUALS(this_type)	\
	bool operator <= (const this_type & other) const { return ! ( other < *this ); } \
	bool operator >= (const this_type & other) const { return ! ( *this < other ); } \
	bool operator >  (const this_type & other) const { return ( other < *this ); }	\
	bool operator != (const this_type & other) const { return ! ( *this == other ); }

//#define ARRAY_SIZE(data)	(sizeof(data)/sizeof(data[0]))

#ifdef ARRAY_SIZE
#undef ARRAY_SIZE
#endif

template<typename T, size_t N> char (&ArrayCountObj(const T (&)[N]))[N];
#define ARRAY_SIZE(arr)    (sizeof(NS_CB::ArrayCountObj(arr)))

// MEMBER_OFFSET tells you the offset of a member in a type
//#define MEMBER_OFFSET(type,member)	( (size_t) &(((type *)0)->member) )
#define MEMBER_OFFSET(type,member)	offsetof(type,member)

// MEMBER_SIZE tells you the size of a member in a type
#define MEMBER_SIZE(type,member)  ( sizeof( ((type *) 0)->member) )

// MEMBER_TO_OWNER takes a pointer to a member and gives you back the base of the object
//	you should then ASSERT( &(ret->member) == ptr );
#define MEMBER_TO_OWNER(type,member,ptr)	(type *)( ((char *)ptr) - MEMBER_OFFSET(type,member) )

//-------------------------------------------------------------------

#define _Stringize( L )			#L
#define _DoMacro1( M, X )		M(X)
#define _DoMacro2( M, X,Y )		M(X,Y)

#define STRINGIZE(M)			_DoMacro1( _Stringize, M )
#define LINE_STRING				STRINGIZE( __LINE__ )
#define MACRO_INDIRECT(A)		A


#define STRING_JOIN(arg1, arg2)				STRING_JOIN_DELAY(arg1, arg2)
#define STRING_JOIN_DELAY(arg1, arg2)		STRING_JOIN_IMMEDIATE(arg1, arg2)
#define STRING_JOIN_IMMEDIATE(arg1, arg2)	arg1 ## arg2

#define PRAGMA_MESSAGE(str)		message( __FILE__ "(" LINE_STRING ") : message: " str)
	// don't put a semicolon after a gPragmaMessage
	// use like this :
	//#pragma gPragmaMessage("my text")

// __LINE__ is broken in MSVC with /ZI , but __COUNTER__ is an MSVC extension that works

#ifdef _MSC_VER
#define NUMBERNAME(name)	STRING_JOIN(name,__COUNTER__)
#else
#define NUMBERNAME(name)	STRING_JOIN(name,__LINE__)
#endif

//-----------------------------------------------------------

#define MIN(a,b)	( (a) < (b) ? (a) : (b) )
#define MAX(a,b)	( (a) > (b) ? (a) : (b) )

#define SETMIN(a,b)	a = MIN(a,b)
#define SETMAX(a,b)	a = MAX(a,b)

#define MIN3(a,b,c)	MIN(MIN(a,b),c)
#define MAX3(a,b,c)	MAX(MAX(a,b),c)

#define ABS(a)		( ((a) < 0) ? -(a) : (a) )

//#define ZERO(ptr)		memset(ptr,0,sizeof(*ptr))
#define ZERO_PTR(ptr)		memset(ptr,0,sizeof(*ptr))
#define ZERO_VAL(obj)		memset(&(obj),0,sizeof(obj))

#define LOOP(var,count)	(int var=0;(var)<(count);var++)
#define LOOPBACK(var,count)	(int var=(count)-1;(var)>=0;var--)
#define LOOPVEC(var,vec)    (int var=0; (var) < (int)vec.size(); var++)
#define LOOPVECBACK(var,vec)    (int var= (int)vec.size() -1; (var)>=0; var--)

//-------------------------------------------------------------------
// crazy macros : DO_ONCE and AT_STARTUP

// Macro for static-initialization one-liners.
// !! Only works in statically-linked .obj files. !!
#define AT_STARTUP(some_code)	\
namespace { static struct STRING_JOIN(AtStartup_,__LINE__) { STRING_JOIN(AtStartup_,__LINE__)() { some_code; } } NUMBERNAME(AtStartup_) ## Instance; };

#define AT_SHUTDOWN(some_code)	\
namespace { static struct STRING_JOIN(AtShutdown_,__LINE__) { ~STRING_JOIN(AtShutdown_,__LINE__)() { some_code; } } NUMBERNAME(AtShutdown_) ## Instance; };

#define DO_ONCE(some_code)	\
do { static bool once = true; if ( once ) { once = false; { some_code; } } } while(0)

#define DO_N_TIMES(count,some_code)	\
do { static int counter = 0; if ( counter++ < (count) ) { some_code; } } while(0)

//-----------------------------------------------------------

void AssertMessage(const char * fileName,const int line,const char * message);

#ifdef _CPPUNWIND
#define TRY		try
#define THROW	throw 0
#define CATCH	catch(...)
#else
#define TRY		__try
#define THROW	*((char *)0) = 0
#define CATCH	__except( 1 ) // EXCEPTION_EXECUTE_HANDLER )
#endif

#if 1 // def _DEBUG
#ifdef CB_64
#define ASSERT_BREAK()	__debugbreak()
#else
#define ASSERT_BREAK()	__asm { int 3 }
#endif
#else
#define ASSERT_BREAK()	THROW
#endif // _DEBUG

//! FAIL() is to be used with non-conditional errors of the "should not get here" type; it takes a char * argument
#define FAIL(str)		do { NS_CB::AssertMessage(__FILE__,__LINE__,str); ASSERT_BREAK(); } while(0)

// ASSERT_RELEASE and ASSERT_RELEASE_THROW are almost identical at the moment
#define ASSERT_RELEASE(exp)		do { if ( ! (exp) ) { NS_CB::AssertMessage(__FILE__,__LINE__,#exp); ASSERT_BREAK(); } } while(0)
#define ASSERT_RELEASE_THROW(exp)		do { if ( ! (exp) ) { NS_CB::AssertMessage(__FILE__,__LINE__,#exp); THROW; } } while(0)

#ifdef DO_ASSERTS

#define ASSERT(exp)		do { if ( ! (exp) ) { NS_CB::AssertMessage(__FILE__,__LINE__,#exp);ASSERT_BREAK(); } } while(0)
#define DURING_ASSERT(exp)	exp

#else

#define ASSERT(exp)
#define DURING_ASSERT(exp)

#endif // DO_ASSERTS


#define ASSERT_LOW(exp)
#define ASSERT_MED(exp)		ASSERT(exp)

//-------------------------------------------------------------------
//! compiler_assert : compile time "assert"
//#define COMPILER_ASSERT(exp)	extern char _dummy_array[ (exp) ? 1 : -1 ]

// better version of COMPILER_ASSERT stolen from boost :

template <bool x> struct COMPILER_ASSERT_FAILURE;

template <> struct COMPILER_ASSERT_FAILURE<true> { enum { value = 1 }; };

template<int x> struct compiler_assert_test{};

// __LINE__ macro broken when -ZI is used see Q199057
// fortunately MSVC ignores duplicate typedef's.
#define COMPILER_ASSERT( B ) \
   typedef NS_CB::compiler_assert_test<\
      sizeof(NS_CB::COMPILER_ASSERT_FAILURE< (bool)(B) >)\
      > compiler_assert_typedef_

//-------------------------------------------------------------------

#ifdef _DEBUG
#define CANT_GET_HERE()		FAIL("Can't get here")
#else
#define CANT_GET_HERE()		__assume(0)
#endif

#define NO_DEFAULT_CASE		default: { CANT_GET_HERE(); } break;
			
//-------------------------------------------------------------------

END_CB

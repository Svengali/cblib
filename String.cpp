#include "String.h"
#include "Base.h"
#include "stl_basics.h"
#include "File.h"
#include "FileUtil.h"
#include "vector.h"
#include "Log.h"

#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#pragma warning(disable : 4127) // conditional is constant; but it isn't !

START_CB

//! Printf strings are not meant to exceed this value
//const int c_bigAssSize = 2048;

//}{= StringData ======================================================================================

/*!

StringData manages the COW for String (COW = Copy On Write)

it's a refcounted vector<char>

you should use Readable() and Writeable() to get at m_vec
	in fact, only the constructors of String and those two functions
	should ever touch m_vec

The biggest advantage of COW is that it makes vector< String > much efficient,
due to the many uses of operator= (COW is a huge optimization for operator=)

To hold a unique/writable String requires TWO allocations : one for StringData and one
in the vector<> to point at the char data.

Any COW string needs two allocations, unless you pack the ref count into the char data, which
actually isn't a terrible idea...


*/

#pragma pack(push)
#pragma pack(4)

struct StringData
{
public:

	// constructed with one ref
	StringData() : m_refCount(1)
	{
		// when you just construct vector<> it does no allocations
	}
	
	enum ECOW { eCOW };
	
	// constructed with one ref
	// COW : break link to old data; copy it and de-ref it
	StringData( ECOW e, StringData * pOldData ) : m_refCount(1), m_vec(pOldData->m_vec)
	{
		pOldData->FreeRef();
	}

	enum EEmpty { eEmpty };

	// just for GetStaticEmptyStringData :
	explicit StringData( EEmpty e ) : m_refCount(1)
	{
		m_vec.resize(1);
		m_vec[0] = 0;
	}

	~StringData()
	{
		ASSERT( m_refCount == 0 );
	}

	void TakeRef()
	{
		m_refCount++;
	}
	void FreeRef()
	{
		m_refCount--;
		if ( m_refCount == 0 )
			delete this;
	}
	int GetRefCount() const
	{
		ASSERT( m_refCount > 0 );
		return m_refCount;
	}

	vector<char>	m_vec;

private:
	int				m_refCount;
};

/*

// safe version does an alloc :
static StringData * GetStaticEmptyStringData()
{
	static StringData*	s_pData = NULL;
	if (s_pData == NULL)
	{
		s_pData = new StringData(StringData::eEmpty);
		s_pData->TakeRef();
	}

	return s_pData;
}

/*/

// unsafe version :
//	make the const empty string data just point at a chunk of memory 
// this is really just to avoid a tiny alloc that leaks so that my leak check is clean

struct StringDataHammer
{
	char *		begin;
	vecsize_t	capacity;
	vecsize_t	size;
	int			refCount;
};
#pragma pack(pop)

//COMPILER_ASSERT( sizeof(cb::vector<char>) == sizeof(char *) + sizeof(vecsize_t) + sizeof(vecsize_t) );
COMPILER_ASSERT( sizeof(StringData) == sizeof(StringDataHammer) );

/*
struct Test
{
	char * ptr;
	int	x;
};
COMPILER_ASSERT( sizeof(Test) == 16 );
*/

static StringData * GetStaticEmptyStringData()
{
	//static StringData s_data; // can't do that cuz I never want to destruct
	static StringData*	s_pData = NULL;
	if (s_pData == NULL)
	{
		static StringDataHammer s_hammer = { 0 };
		s_pData = (StringData *) &s_hammer;
	
		s_hammer.begin = "";
		s_hammer.size = 1;
		s_hammer.capacity = 8;
		s_hammer.refCount = 2;

		ASSERT_RELEASE( s_pData->m_vec.size() == 1 );
		ASSERT_RELEASE( s_pData->m_vec[0] == 0 );
	}

	return s_pData;
}
/**/

//}{= constructors ======================================================================================


// Default ctor.  Make an empty string.
String::String()
	: m_pData(GetEmpty().m_pData)
{
	ASSERT(m_pData != NULL);
	m_pData->TakeRef();

	//ASSERT(IsValid());
}


String::String( const String &str )
	: m_pData(str.m_pData)
{
	ASSERT( m_pData != NULL );
	m_pData->TakeRef();

	//ASSERT( IsValid() );
}

String::String( const char * const pStr )
	: m_pData(new StringData)
{
	ASSERT( pStr != NULL );

	// can't call Set cuz we're not yet Valid
	int size = strlen32(pStr) + 1;
	m_pData->m_vec.resize( size );
	strcpy( &m_pData->m_vec[0], pStr );

	//ASSERT( IsValid() );
}

/*
String::String( const char c )
	: m_pData(new StringData)
{
	// can't call Set cuz we're not yet Valid
	m_pData->m_vec.resize( 2 );
	m_pData->m_vec[0] = c;
	m_pData->m_vec[1] = 0;

	ASSERT( IsValid() );
}
*/
	
// do NOT use s_empty here
//	make sure I'm safe to use in other static initializers
String::String( const EEmpty e ) :
	m_pData( GetStaticEmptyStringData() )
{
	ASSERT( m_pData != NULL );
	m_pData->TakeRef();

	ASSERT( Readable()[0] == 0 );

	//ASSERT( IsValid() );
}

String::String( const EReserve e, const int amount )
	: m_pData(new StringData)
{
	m_pData->m_vec.reserve( amount );
	m_pData->m_vec.push_back( 0 );

	//ASSERT( IsValid() );
}

String::String( const EReserve e, const char * const pStr, const int amount) 
	: m_pData(new StringData)
{
	ASSERT( pStr != NULL );

	m_pData->m_vec.reserve( amount );
	m_pData->m_vec.push_back( 0 );

	Set(pStr);
	
	//ASSERT( IsValid() );
}

String::String( const ESubString e, const char * const pStr, const int len)
	: m_pData(new StringData)
{
	ASSERT( pStr != NULL );

	m_pData->m_vec.assign( pStr, pStr+len );
	ASSERT( m_pData->m_vec.size32() == len );
	m_pData->m_vec.push_back( 0 );
	
	//ASSERT( IsValid() );
}

String::String( const EConcat e, const char * const pStr1, const char * const pStr2)
	: m_pData(new StringData)
{
	ASSERT( pStr1 != NULL );
	ASSERT( pStr2 != NULL );

	int l1 = strlen32(pStr1);
	int l2 = strlen32(pStr2);
	
	m_pData->m_vec.resize(l1+l2+1);
	memcpy(&m_pData->m_vec[0],pStr1,l1);
	memcpy(&m_pData->m_vec[0]+l1,pStr2,l2);
	m_pData->m_vec.back() = 0;
	
	//ASSERT( IsValid() );
}

String::~String( void )
{
	//ASSERT( IsValid() );

	m_pData->FreeRef();
	m_pData = NULL;
}

/** return a reference to an empty String. */
/*static*/ const String& String::GetEmpty()
{
	static String s_empty( String::eEmpty ); // DO use eEmpty here

	//ASSERT(s_empty.IsValid());
	return s_empty;
}

//=======================================================================================
// all utility code should use Readable() or Writeable() to get to m_vec

const vector<char> & String::Readable() const
{
	ASSERT( m_pData );
	return m_pData->m_vec;
}

// Writeable is somewhat inefficient if I'm immediately throwing away the
//	string data, as in Clear(), etc.
// MemGuard() and Writeable should also appear together
vector<char> & String::Writeable()
{
	ASSERT( m_pData );
	// if ref count is 1, I'm the only owner
	if ( m_pData->GetRefCount() != 1 )
	{
		// shared string ; break it
		StringData * pOldData = m_pData;
		m_pData = new StringData( StringData::eCOW, pOldData );
	}
	return m_pData->m_vec;
}

// WriteableEmpty may not actually give you an empty vec;
//	it gives a vec whose contents are totally undetermined
vector<char> & String::WriteableEmpty(const int reserve /* = 16 */)
{
	ASSERT( m_pData );
	// if ref count is 1, I'm the only owner
	if ( m_pData->GetRefCount() != 1 )
	{
		// shared string ; break it
		m_pData->FreeRef();
		m_pData = new StringData;

		// and do some silly crap to make sure IsValid() will pass
		m_pData->m_vec.reserve(reserve);
		m_pData->m_vec.resize(1);
		m_pData->m_vec[0] = 0;
	}
	return m_pData->m_vec;
}

//=======================================================================================

#define m_vec _dont_touch_me_below

/*
bool String::IO( const StreamPtr &inout  ) const
{
	if ( inout->GetStreamDirection() == Stream::eDirRead )
		return const_cast<String *>(this)->Read(inout);
	else
		return Write(inout);
}

bool String::Read( const StreamPtr &in )
{

	//! The length does not have the terminating 0
	int len = 0;
	
	VERIFY( in->Read( &len ) );

	// avoid doing two allocs here :
	vector<char> & vec = WriteableEmpty(len+1);

	//! Since we store the length, I do not store the 0, thus len+1
	vec.resize(len + 1);

	VERIFY( in->Read( vec.begin(), len, NULL ) );

	vec[len] = 0;

	ASSERT( IsValid() );

	return true;
}

bool String::Write( const StreamPtr &out ) const
{
	ASSERT( IsValid() );
	const int len = Length();

	//! Does not include terminating 0
	if( !out->Write( len ) ) return false;

	if( !out->Write( Readable().begin(), len ) ) return false;

	return true;
}
*/

// fast swap :
//void String::Swap(String * pOther)
void String::Swap(String & rhs)
{
	// just swap impl pointers :
	cb::Swap( m_pData, rhs.m_pData );
}

void String::operator=(const String& str)
{
	ASSERT( str.IsValid() && IsValid() );

	// this works even for assigning to self; don't special case :

	// take ref on the new one first
	str.m_pData->TakeRef();

	// free ref to old data
	m_pData->FreeRef();

	// set pointer :
	m_pData = str.m_pData;
}

bool String::operator ==( const String &str ) const
{
	// COW allows some quick equality checks; but is this common enough to be worth while ?
	if ( m_pData == str.m_pData )
		return true;
	return strcmp( CStr(), str.CStr() ) == 0;
}

bool String::operator !=( const String &str ) const
{
	// COW allows some quick equality checks; but is this common enough to be worth while ?
	if ( m_pData == str.m_pData )
		return false;
	return strcmp( CStr(), str.CStr() ) != 0;	
}	

bool String::operator < ( const String &str ) const
{
	return strcmp( CStr(), str.CStr() ) < 0;		
}

bool String::operator ==( const char *const pStr ) const
{
	ASSERT( pStr && pStr[0] == pStr[0] );
	return strcmp( CStr(), pStr ) == 0;	
}

bool String::operator !=( const char *const pStr ) const
{
	ASSERT( pStr && pStr[0] == pStr[0] );
	return strcmp( CStr(), pStr ) != 0;		
}	

bool String::operator < ( const char *const pStr ) const
{
	ASSERT( pStr && pStr[0] == pStr[0] );
	return strcmp( CStr(), pStr ) < 0;		
}

bool operator ==( const char * const pStr, const String &str )
{
	ASSERT( pStr && pStr[0] == pStr[0] );
	return str == pStr;
}

bool operator !=( const char * const pStr, const String &str )
{
	ASSERT( pStr && pStr[0] == pStr[0] );
	return str != pStr;
}

bool operator < ( const char * const pStr, const String &str )
{
	ASSERT( pStr && pStr[0] == pStr[0] );
	return strcmp( pStr, str.CStr() ) < 0;		
}

void String::Set( const char *const pStr )
{
	if( pStr != NULL )
	{
		const int len = strlen32(pStr);
		
		vector<char> & vec = WriteableEmpty(len+1);

		vec.clear();
		vec.resize( len + 1 );
		strcpy( &vec[0], pStr );
	}
	else
	{
		/*
		vector<char> & vec = WriteableEmpty();

		vec.resize(1);
		vec[0] = 0;
		*/
		
		*this = GetEmpty();
	}
}

void String::Append( const char * const pStr )
{
	vector<char> & vec = Writeable();

	int old_len = Length();
	int add_len = strlen32(pStr);
	vec.resize( old_len + add_len + 1 );
	strcpy( &vec[0] + old_len, pStr );
}

void String::Append( const char c )
{
	vector<char> & vec = Writeable();

	vec.back() = c;
	vec.push_back(0);
}

int String::Index(const char * ptr) const
{
	int index = ptr_diff_32( ptr - CStr() );
	ASSERT_RELEASE( index >= 0 && index <= Length() );
	return index;
}

void String::Insert(const int at, const char * const pStr )
{
	int old_len = Length();
	ASSERT( at >= 0 && at <= old_len );
	
	vector<char> & vec = Writeable();
	
	int add_len = strlen32(pStr);
	vec.resize( old_len + add_len + 1 );
	
	char * buf = vec.data();
	
	memmove(buf+at+add_len,buf+at,old_len-at);
	strcpy( buf+at + old_len, pStr );
}

void String::Insert(const int at, const char c )
{
	int old_len = Length();
	ASSERT( at >= 0 && at <= old_len );
	
	vector<char> & vec = Writeable();

	vec.push_back(0);
	
	char * buf = vec.data();
	
	memmove(buf+at+1,buf+at,old_len-at);
	
	buf[at] = c;
}
	
String StringRawPrintfVA(const char *pFormat, va_list varargs)
{
	// try to print to a moderate size stack array first :

	char stackBuf[1024];
	int wroteLen = vsnprintf(stackBuf,sizeof(stackBuf)-1,pFormat,varargs);
	if ( wroteLen >= 0 )
	{
		// tack it on :
		//Set(stackBuf);
		return String(stackBuf);
	}
	else
	{
		// very big string, make a dynamic alloc buffer :
	
		// could just work into my vec directly :
		//String ret;
		//vector<char> & vBuf = ret.Writeable();
		vector<char> vBuf;
		
		vsnprintfdynamic(&vBuf,pFormat,varargs);
		
		// this is an unnecessary strcpy , but also does a tighten for us :
		return String(vBuf.data());
	}	
}

String rawStringPrintf(const char *pFormat, ...)
{
	va_list argPtr;
	va_start( argPtr, pFormat );
	String s = StringRawPrintfVA(pFormat,argPtr);
	va_end( argPtr );

	return s;
}

void String::rawPrintf( const char *pFormat, ... )
{
	va_list argPtr;
	va_start( argPtr, pFormat );
	String s = StringRawPrintfVA(pFormat,argPtr);
	va_end( argPtr );
	
	*this = s;
}

void String::rawCatPrintf( const char *pFormat, ... )
{
	va_list argPtr;
	va_start( argPtr, pFormat );
	String s = StringRawPrintfVA(pFormat,argPtr);
	va_end( argPtr );
	
	*this += s;
}

char String::GetChar( const int index ) const
{
	ASSERT( IsValid() );
	ASSERT( index >= 0 );
	ASSERT( index < Length() );

	return Readable()[ index ];
}

void String::SetChar( const int index, const char c )
{
	ASSERT( IsValid() );
	ASSERT( index >= 0 );
	ASSERT( index < Length() );

	if( c == 0 )
	{
		Truncate(index);
	}
	else
	{
		Writeable()[ index ] = c;
	}
	
	ASSERT( IsValid() );
}

// Split is like SetChar(index,0) - returns the tail that's cut off
String String::Split( const int index )
{
	ASSERT( IsValid() );
	ASSERT( index >= 0 );
	ASSERT( index <= Length() );
	
	if ( index == Length() )
	{
		return String::GetEmpty();
	}
	
	// index == Length()-1 also returns empty, but cuts off one char
	
	String ret( CStr() + index + 1 );
	
	Truncate(index);
	
	return ret;
}
	
char String::PopBack()
{
	ASSERT( IsValid() );
	ASSERT( Length() > 0 );
	
	if ( Length() == 0 )
	{
		return 0;
	}

	vector<char> & vec = Writeable();
	ASSERT( vec.back() == 0 );
	char back = vec[vec.size()-2];;
	vec.pop_back();
	vec.back() = 0;

	return back;
}

//! Useful?
//! char &operator []( const int index );

void String::Reserve( const int amount )
{
	ASSERT( IsValid() );
	
	// cheat; reserve doesn't actually change data, so don't COW because of it :
	//vector<char> & vec = const_cast<vector<char> &>( Readable() );
	
	// actually, typical usage will be a Reserve() followed by writes ; if
	//	we don't break sharing at the time of the Reserve, then the old
	//	data will have unneeded extra size
	vector<char> & vec = Writeable();

	vec.reserve( amount );
}

int String::Capacity( void ) const
{
	ASSERT( IsValid() );
	return check_value_cast<int>( Readable().capacity() );
}

const char *String::CStr( void ) const
{
	//ASSERT( IsValid() );
	return &Readable()[0];
}

// WriteableCStr : do NOT set nulls !
char * String::WriteableCStr( int size )
{
	vector<char> & vec = Writeable();

	//vec.reserve( size );
	if ( size > vec.size() )
		vec.resize( size );
	
	return vec.data();
}
	
void String::Truncate( const int newLength ) //!< equivalent to what you'd imagine SetChar(index,0); would do
{
	//ASSERT( IsValid() );
	// Truncate is intentionally allowed to work on invalid strings
	//	it's my hack for jamming illegally on a string
	vector<char> & vec = Writeable();
		
	ASSERT( newLength >= 0 );
	ASSERT( newLength < vec.capacity() );
	
	vec.resize( newLength + 1 );
	vec[ newLength ] = 0;
	
	ASSERT( IsValid() );
}

void String::FixLength()
{
	//ASSERT( IsValid() );
	// FixLength is intentionally allowed to work on invalid strings
	//	it's my hack for jamming illegally on a string
	vector<char> & vec = Writeable();
	
	int newLength = strlen32(vec.data());
	
	Truncate(newLength);
}
	
int String::Length( void ) const
{
	ASSERT( IsValid() );
	return Readable().size32() - 1;
}

bool String::IsValid() const
{
	ASSERT( m_pData != NULL );
	ASSERT( m_pData->GetRefCount() >= 1 );
	ASSERT( Readable().size() > 0 );
	ASSERT( strlen( &Readable()[0] ) == (size_t)(Readable().size() - 1) );
	ASSERT( Readable()[ Readable().size() - 1 ] == 0 );
	return true;
}

void String::Clear()
{
	ASSERT( IsValid() );
	
	vector<char> & vec = WriteableEmpty();

	vec.clear();
	vec.push_back(0);
	
	ASSERT( IsValid() );
}

void String::Release()
{
	*this = GetEmpty();
}
	
void String::WriteBinary(FILE * fp) const
{
	int len = Length();
	myfwrite(&len,sizeof(len),fp);
	myfwrite(CStr(),len,fp);
}

void String::ReadBinary(FILE * fp)
{
	ASSERT( IsValid() );
	
	vector<char> & vec = WriteableEmpty();

	vec.clear();
	
	int len;
	myfread(&len,sizeof(len),fp);
	
	vec.resize(len+1);
	myfread(&vec[0],len,fp);
	vec[len] = 0;
	
	ASSERT( IsValid() );
}
	
void String::WriteText(FILE * fp) const
{
	int len = Length();
	myfwrite( CStr(), len, fp );
}

/*
void String::ReadText(FILE * fp)
{
}
*/
	
int String::FindReplace(const char * const from, const char * const to )
{
	int old_len = Length();
	
	vector<char> & vec = Writeable();
	
	int from_len = strlen32(from);
	int to_len = strlen32(to);
	
	int add_len = to_len - from_len;
	
	int replaces = 0;
	
	char * buf = vec.data();
	char * ptr = buf;
	while(*ptr)
	{
		if ( stripresame(ptr,from) )
		{
			replaces++;
			int at = (int)(ptr - buf);
				
			// cut out from and put in to
			if ( add_len > 0 )
			{
				// to_len > from_len
				// slide up to make space
				vec.resize( old_len + add_len + 1 );
				buf = vec.data();
				ptr = buf+at;
				memmove(ptr+add_len,ptr,old_len-at);
				memcpy(ptr,to,to_len);
				ptr += to_len;	
			}
			else
			{
				// from_len >= to_len
				// slide down
				memcpy(ptr,to,to_len);
				ptr += to_len;
				// add_len <= 0
				memmove(ptr,ptr-add_len,old_len-at-from_len+1);
			}

			old_len += add_len;
		}
		else
		{
			ptr++;
		}
	}
	
	FixLength();
	ASSERT( Length() == old_len );
	
	return replaces;
}

//=======================================================================================

void String_Test()
{
	String stuff("abc");
//	String z = stuff + String(String::ePrintf,"%d",1);
	String z = stuff;
	z.CatPrintf("%d",1);
	z += "test";
	puts(z.CStr());
}

END_CB

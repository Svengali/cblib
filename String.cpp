#include "cblib/String.h"
#include "cblib/Base.h"
#include "cblib/stl_basics.h"
#include "cblib/File.h"
#include "cblib/FileUtil.h"
#include "cblib/vector.h"

#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#pragma warning(disable : 4127) // conditional is constant; but it isn't !

START_CB

//! Printf strings are not meant to exceed this value
const int c_bigAssSize = 1024;

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



static StringData * GetStaticEmptyStringData()
{
	static bool s_inited = false;
	static StringData*	s_pData = NULL;
	if (s_inited == false)
	{
		s_pData = new StringData(StringData::eEmpty);
		s_pData->TakeRef();

		s_inited = true;
	}

	return s_pData;
}

//}{= constructors ======================================================================================


// Default ctor.  Make an empty string.
String::String()
	: m_pData(GetEmpty().m_pData)
{
	ASSERT(m_pData != NULL);
	m_pData->TakeRef();

	ASSERT(IsValid());
}


String::String( const String &str )
	: m_pData(str.m_pData)
{
	ASSERT( m_pData != NULL );
	m_pData->TakeRef();

	ASSERT( IsValid() );
}

String::String( const char * const pStr )
	: m_pData(new StringData)
{
	ASSERT( pStr != NULL );

	// can't call Set cuz we're not yet Valid
	int size = strlen(pStr) + 1;
	m_pData->m_vec.resize( size );
	strcpy( &m_pData->m_vec[0], pStr );

	ASSERT( IsValid() );
}

String::String( const char c )
	: m_pData(new StringData)
{
	// can't call Set cuz we're not yet Valid
	m_pData->m_vec.resize( 2 );
	m_pData->m_vec[0] = c;
	m_pData->m_vec[1] = 0;

	ASSERT( IsValid() );
}
	
// do NOT use s_empty here
//	make sure I'm safe to use in other static initializers
String::String( const EEmpty  ) :
	m_pData( GetStaticEmptyStringData() )
{
	ASSERT( m_pData != NULL );
	m_pData->TakeRef();

	ASSERT( Readable()[0] == 0 );

	ASSERT( IsValid() );
}

String::String( const EReserve , const int amount )
	: m_pData(new StringData)
{
	m_pData->m_vec.reserve( amount );
	m_pData->m_vec.push_back( 0 );

	ASSERT( IsValid() );
}

String::String( const EReserve , const char * const pStr, const int amount) 
	: m_pData(new StringData)
{
	ASSERT( pStr != NULL );

	m_pData->m_vec.reserve( amount );
	m_pData->m_vec.push_back( 0 );

	Set(pStr);
	
	ASSERT( IsValid() );
}

String::String( const ESubString , const char * const pStr, const int len)
	: m_pData(new StringData)
{
	ASSERT( pStr != NULL );

	m_pData->m_vec.assign( pStr, pStr+len );
	ASSERT( m_pData->m_vec.size() == len );
	m_pData->m_vec.push_back( 0 );
	
	ASSERT( IsValid() );
}

String::String( const EConcat , const char * const pStr1, const char * const pStr2)
	: m_pData(new StringData)
{
	ASSERT( pStr1 != NULL );
	ASSERT( pStr2 != NULL );

	int l1 = strlen(pStr1);
	int l2 = strlen(pStr2);
	
	m_pData->m_vec.resize(l1+l2+1);
	memcpy(&m_pData->m_vec[0],pStr1,l1);
	memcpy(&m_pData->m_vec[0]+l1,pStr2,l2);
	m_pData->m_vec.back() = 0;
	
	ASSERT( IsValid() );
}

/*	         
String::String(const EPrintf e, const char *pFormat, ... )
	: m_pData(new StringData)
{
	char bigAssBuffer[ c_bigAssSize ];

	va_list argPtr;

	va_start( argPtr, pFormat );
	const int length = _vsnprintf( bigAssBuffer, c_bigAssSize, pFormat, argPtr);
	va_end( argPtr );

	ASSERT( length != -1 );

	m_pData->m_vec.resize( length+1 );
	memcpy(&m_pData->m_vec[0],bigAssBuffer,length);
	m_pData->m_vec.back() = 0;
}
*/
	
String::~String( void )
{
	ASSERT( IsValid() );

	m_pData->FreeRef();
	m_pData = NULL;
}

/** return a reference to an empty String. */
/*static*/ const String& String::GetEmpty()
{
	static String s_empty( String::eEmpty ); // DO use eEmpty here

	ASSERT(s_empty.IsValid());
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
void String::Swap(String * pOther)
{
	// just swap impl pointers :
	cb::Swap( m_pData, pOther->m_pData );
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
		const int len = strlen(pStr);
		
		vector<char> & vec = WriteableEmpty(len+1);

		vec.clear();
		vec.resize( len + 1 );
		strcpy( &vec[0], pStr );
	}
	else
	{
		vector<char> & vec = WriteableEmpty();

		vec.resize(1);
		vec[0] = 0;
	}
}

void String::Append( const char * const pStr )
{
	vector<char> & vec = Writeable();

	int old_len = Length();
	int add_len = strlen(pStr);
	vec.resize( old_len + add_len + 1 );
	strcpy( &vec[0] + old_len, pStr );
}

void String::Append( const char c )
{
	vector<char> & vec = Writeable();

	vec.back() = c;
	vec.push_back(0);
}

void String::rawPrintf( const char *pFormat, ... )
{
	char bigAssBuffer[ c_bigAssSize ];

	va_list argPtr;

	va_start( argPtr, pFormat );
	const int length = _vsnprintf( bigAssBuffer, c_bigAssSize, pFormat, argPtr);
	va_end( argPtr );

	ASSERT( length != -1 );
	UNUSED_PARAMETER(length);

	Set(bigAssBuffer);
}

void String::rawCatPrintf( const char *pFormat, ... )
{
	char bigAssBuffer[ c_bigAssSize ];

	va_list argPtr;

	va_start( argPtr, pFormat );
	const int length = _vsnprintf( bigAssBuffer, c_bigAssSize, pFormat, argPtr);
	va_end( argPtr );

	ASSERT( length != -1 );
	UNUSED_PARAMETER(length);

	Append(bigAssBuffer);
}

char String::operator []( const int index ) const
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

void String::Truncate( const int newLength ) //!< equivalent to what you'd imagine SetChar(index,0); would do
{
	ASSERT( IsValid() );
	ASSERT( newLength >= 0 );
	ASSERT( newLength < Length() );

	vector<char> & vec = Writeable();
		
	vec[ newLength ] = 0;
	vec.resize( newLength + 1 );
	
	ASSERT( IsValid() );
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
	return Readable().capacity();
}

const char *String::CStr( void ) const
{
	//ASSERT( IsValid() );
	return &Readable()[0];
}

int String::Length( void ) const
{
	ASSERT( IsValid() );
	return Readable().size() - 1;
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

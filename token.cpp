#include "Token.h"
#include "String.h"
#include "File.h"

START_CB

#ifdef DEBUG_STRINGKEY_HOLD_STRING
#pragma PRAGMA_MESSAGE("StringKey Debug Enabled : hold a String")
#endif

// @@ toggle here :
//#define DO_STRINGKEY_UNIQUENESS_TEST

#ifdef DO_STRINGKEY_UNIQUENESS_TEST //{

#pragma PRAGMA_MESSAGE("Token : uniqueness debug mode ON")

static void DoStringKeyUniquenessTest(const Token & key, const String & string );

#else //}{

//#pragma gPragmaMessage("Token : uniqueness debug mode OFF")

#define DoStringKeyUniquenessTest(key,string)

#endif // } DO_STRINGKEY_UNIQUENESS_TEST

//----------------------------------------------------------------------
//! Static things

/*static*/ const Token & Token::Empty()
{
	static Token s_empty(Token::eEmpty);
	return s_empty;
}

//----------------------------------------------------------------------
//! Class implementation

// don't use any statics in this, since Token(eEmpty) can be made from other static intializers
Token::Token( const EEmpty e ) :
	//m_hash( String( String::eEmpty ).CStr() )
	m_hashVal( TokenHash( "" ) )
{
	// m_string constructs to empty which is right
}

Token::Token( const char * const pStr ) :
	m_hashVal( TokenHash( pStr ) )
{
	#ifdef DEBUG_STRINGKEY_HOLD_STRING
	m_string = pStr;
	#endif
	DoStringKeyUniquenessTest(*this, String(pStr) );	
}

Token::Token( const String & str ) :
	m_hashVal( TokenHash( str.CStr() ) )
{
	#ifdef DEBUG_STRINGKEY_HOLD_STRING
	m_string = str;
	#endif
	DoStringKeyUniquenessTest(*this, str );
}

Token::Token( const uint32 crc ) : m_hashVal(crc)
{	
	#ifdef DEBUG_STRINGKEY_HOLD_STRING
	static const String c_str("Token::uint32");
	m_string = c_str;
	#endif
}

// todo: Token.DebugGetString() called in MSVC debug watch
//  does not work (access violation, bogus return)
// might have to put string backing const char* member into Token
const char * const Token::DebugGetString( void ) const
{
	#ifdef DEBUG_STRINGKEY_HOLD_STRING
	return m_string.CStr();
	#else
	return "<unknown>";
	#endif
}

void Token::SetHash(uint32 h)
{
	#ifdef DEBUG_STRINGKEY_HOLD_STRING
	static const String c_str("Token::SetHash");
	m_string = c_str;
	#endif	
	m_hashVal = h;
}

void Token::WriteBinary(FILE * fp, bool writeString) const
{
	myfwrite(&m_hashVal,sizeof(m_hashVal),fp);
	
	#ifdef DEBUG_STRINGKEY_HOLD_STRING
	if ( writeString ) 
	{
		m_string.WriteBinary(fp);
	}
	else
	#endif
	{
		// write a length of 0
		int len = 0;
		myfwrite(&len,sizeof(len),fp);

	}
}

void Token::ReadBinary(FILE * fp)
{
	myfread(&m_hashVal,sizeof(m_hashVal),fp);
	
	#ifdef DEBUG_STRINGKEY_HOLD_STRING
	
	m_string.ReadBinary(fp);
	
	#else
	
	// just read len and ignore it :
	int len = 0;
	myfread(&len,sizeof(len),fp);
	// skip it :
	fseek64(fp,len,SEEK_CUR);
		
	#endif
}
	
END_CB

//==================================================================================

#ifdef DO_STRINGKEY_UNIQUENESS_TEST //{

#include "TokenHash.h"
//#include <hash_map>
//#include <map>

//typedef vecsortedpair< vector< std::pair<Token,String> > > aStringKeyString;
typedef std::hash_map< cb::Token,cb::String > aStringKeyString;
//typedef std::map< cb::Token,cb::String > aStringKeyString;

// do NOT put the array instance here, because it'll be constructed
//  at whatever time; instead construct in the function and set this pointer
//static aStringKeyString * s_pArray = NULL; 

START_CB
static void DoStringKeyUniquenessTest(const Token & key, const String & str )
{
	// this is static, it's a leak :
	static aStringKeyString array;
	//s_pArray = &array;

	aStringKeyString::const_iterator itFound = array.find(key);

	if ( itFound == array.end() )
	{
		array.insert( aStringKeyString::value_type(key,str) );
	}
	else
	{
		// found a string !
		// make sure they're case-insensitive equals
		bool bEqual = strtsame(itFound->second.CStr(),str.CStr());

		ASSERT_RELEASE( bEqual );
	}
}
END_CB

#endif // } DO_STRINGKEY_UNIQUENESS_TEST

//==================================================================================

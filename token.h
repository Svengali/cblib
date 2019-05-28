
//=============================================================
/*

	Token was gStringKey

	Token is basically just a CRC on a string

	Token is case INsensitive !!
		-> also insensitive on slashes
		
	it's useful for fast indexing of strings; use it instead of
	strings for making maps on strings.

	includes a compile-time-optional test mode to make sure that two
	strings never get the same string.  With a CRC32 the change of
	this is roughly 1 in 4 billion. (though you only need 64k strings for
	the chance of *any* collision to become quite high)

	Notez : gTokens should generally not be created at runtime; const strings should be loaded
	into static gTokens like this :

	bool DoQuery()
	{
		static Token s_mygToken("hello world");
		return Index.LookupToken(s_mygToken);
	}

	The reason is that Token creation is actually rather expensive, better to do it once than
	many times.

	Use "TokenHash.h" if you want to use them in hash_map<>
	
*/

#pragma once

#include "cblib/Base.h"
#include "cblib/Util.h"
//#include "cblib/CRC.h"
#include "cblib/Hashes.h"

// @@ toggle here :
#define DEBUG_STRINGKEY_HOLD_STRING
// DEBUG_STRINGKEY_HOLD_STRING causes apparent
//	memory leaks due to gStrings hanging around
//	in static Token's

#ifdef DEBUG_STRINGKEY_HOLD_STRING
#include "cblib/String.h"
#endif

START_CB

class String;

inline uint32 TokenHash(const char * str)
{
	// TokenHash is insensitive like "tokenchar"
	return FNVHashStrInsensitive(str);
}

class Token
{
public:

	/*! Token::s_empty
  		An empty Token.  
        Is asserted to not clash with any other created Token.
        Is asserted to equal Token( String::s_empty.CStr() )
		String::s_empty default and turned into a Token will match this
        (i.e. for by-Token lookups that return SPtr(NULL) as a default).
	**/
	//static Token s_empty;
	static const Token & Empty();


	Token() : m_hashVal(0) { }
	
	enum EEmpty { eEmpty };
	explicit Token( const EEmpty e );
	explicit Token( const char * const pStr );
	explicit Token( const String & str );
	explicit Token( const uint32 hash );
	// default copy constructor is good

	uint32	GetHash() const { return m_hashVal; }
	void	SetHash(uint32 h); // avoid this if possible, it kills debug strings
	
	inline bool operator ==( const Token &tkn ) const { return m_hashVal == tkn.m_hashVal; }
	inline bool operator < ( const Token &tkn ) const { return m_hashVal <  tkn.m_hashVal; }

	// use this if you want an implicit conversion to a hashable value for hash_map
	//	personally, I don't like that - see the namespace std hash definition below
	//inline operator size_t () const { return (size_t) m_hash.GetValue(); }

	MAKE_COMPARISONS_FROM_LESS_AND_EQUALS(Token)

	//! convenience accessor to DEBUG_STRINGKEY_HOLD_STRING :
	const char * const DebugGetString( void ) const;

	void WriteBinary(FILE * fp, bool writeString = true) const;
	void ReadBinary(FILE * fp);
	
private:
	//FORBID_DEFAULT_CTOR(Token);

	uint32 m_hashVal;

	#ifdef DEBUG_STRINGKEY_HOLD_STRING
	String	m_string;
	#endif
};

END_CB


//=============================================================
/*

	Token was gStringKey

	Token is basically just a CRC on a string

	Token is case INsensitive !!

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
#include "cblib/CRC.h"

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

class Token
{
public:

	/*! Token::s_empty
  		An empty Token.  
        Is asserted to not clash with any other created Token.
        Is asserted to equal Token( String::s_empty.CStr() )
		String::s_empty default and turned into a Token will match this
        (i.e. for by-Token lookups that return gPtr(NULL) as a default).
	**/
	//static Token s_empty;
	static const Token & Empty();

	enum EEmpty { eEmpty };

	// no default constructor, use Token(eEmpty)
	//Token();
	explicit Token( const EEmpty e );
	explicit Token( const char * const pStr );
	explicit Token( const String & str );
	explicit Token( const ulong crc );
	// default copy constructor is good

	ulong	GetHash() const { return m_hash.GetHash(); }
	void	SetHash(ulong h); // avoid this if possible, it kills debug strings
	
	//void	SetULong(const ulong z) { m_hash = CRC(z); } // @@ should clear m_string

	inline bool operator ==( const Token &tkn ) const { return m_hash == tkn.m_hash; }
	inline bool operator < ( const Token &tkn ) const { return m_hash <  tkn.m_hash; }

	// use this if you want an implicit conversion to a hashable value for hash_map
	//	personally, I don't like that - see the namespace std hash definition below
	inline operator size_t () const { return (size_t) m_hash.GetHash(); }

	MAKE_COMPARISONS_FROM_LESS_AND_EQUALS(Token)

	//! convenience accessor to DEBUG_STRINGKEY_HOLD_STRING :
	const char * const DebugGetString( void ) const;

private:
	FORBID_DEFAULT_CTOR(Token);

	CRC m_hash;

	#ifdef DEBUG_STRINGKEY_HOLD_STRING
	String	m_string;
	#endif
};

END_CB

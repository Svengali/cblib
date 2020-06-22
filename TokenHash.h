#pragma once

#include "cblib/Base.h"
#include <hash_map>
#include "cblib/token.h"
#include "cblib/hash_function.h"

#pragma warning(disable : 4996) // deprecated hash_map in MSVC 2003

/*

To use Token in hash_map you must include hash_map *BEFORE* Token.h ! 
(ugly, I'm sorry)

(that's fine if all you include is TokenHash.h)

*/

//===============================================================================================

// specialize the std::hash<> function for Token :
// _STLP_HASH_FUN_H
#ifdef _STLP_BEGIN_NAMESPACE
_STLP_BEGIN_NAMESPACE

template <class _Key> struct hash;
//template< typename TKey > struct hash;

template<>
struct std::hash< cb::Token >
{
	inline size_t operator ()( const cb::Token &tkn ) const
	{
		return tkn.GetHash();
	}
};

template<>
struct std::hash< cb::String >
{
	inline size_t operator ()( const cb::String & str ) const
	{
		return cb::Token(str).GetHash();
	}
};

_STLP_END_NAMESPACE

#else // _STLP_BEGIN_NAMESPACE

//TODO @@@@ MH Change this to something actually meaningful
#define _HASH_SEED 4324233

// specialize hash_value in MSVC :
#ifdef _XHASH_
_STDEXT_BEGIN

//template<class _Kty> inline size_t hash_value(const _Kty& _Keyval);

inline size_t hash_value(const cb::Token & _Keyval)
{	// hash _Keyval to size_t value one-to-one
	return ((size_t)_Keyval.GetHash() ^ _HASH_SEED);
}
	
inline size_t hash_value(const cb::String & _Keyval)
{	// hash _Keyval to size_t value one-to-one
	return ((size_t)cb::Token(_Keyval).GetHash() ^ _HASH_SEED);
}

_STDEXT_END
#endif // _XHASH_

#endif // _STLP_BEGIN_NAMESPACE

//===========================================================================================

START_CB

// hash_function for cblib/hash_table :
// note - hashing on Token is totally retarded, because Token *is* a hash, don't it

template <>
inline hash_type hash_function<Token>(const Token & t)
{
	return t.GetHash();
}

struct hash_table_ops_Token : public hash_table_key_equal<Token>
{
	void make_empty(hash_type & hash,Token & key)
	{
		// use an impossible config : hash & Token don't match :
		hash = 0;
		key.SetHash(1);
	}

	bool is_empty(const hash_type & hash,const Token & key)
	{
		return ( hash == 0 && key.GetHash() == 1 );
	}

	void make_deleted(hash_type & hash,Token & key)
	{
		hash = 0;
		key.SetHash(2);
	}

	bool is_deleted(const hash_type & hash,const Token & key)
	{
		return ( hash == 0 && key.GetHash() == 2 );
	}
};

END_CB

//===============================================================================================

START_CB

// it's not easy to change a String to be an "empty" or "deleted" value,
//	so instead I reserve two spots in the hash value

template <>
inline hash_type hash_function<String>(const String & t)
{
	hash_type hash = FNVHashStr(t.CStr());
	if ( hash <= 1 ) hash = 2;
	return hash;
}

struct hash_table_ops_String
{
	hash_type hash_key(const String & key) { return hash_function(key); }

	bool key_equal(const String & lhs,const String & rhs)
	{
		return lhs == rhs;
	}
	bool key_equal(const String & lhs,const char *const rhs)
	{
		return lhs == rhs;
	}
	
	void make_empty(hash_type & hash,String & key)
	{
		hash = 0;
	}

	bool is_empty(const hash_type & hash,const String & key)
	{
		return ( hash == 0 );
	}

	void make_deleted(hash_type & hash,String & key)
	{
		hash = 1;
	}

	bool is_deleted(const hash_type & hash,const String & key)
	{
		return ( hash == 1 );
	}
};

END_CB

//===============================================================================================

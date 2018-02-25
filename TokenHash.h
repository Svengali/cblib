#pragma once

#include "cblib/Base.h"
#include <hash_map>
#include "cblib/token.h"

#pragma warning(disable : 4996) // deprecated hash_map in MSVC 2003

/*

To use Token in hash_map you must include hash_map *BEFORE* Token.h ! 
(ugly, I'm sorry)

(that's fine if all you include is TokenHash.h)

*/

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

_STLP_END_NAMESPACE

#else // _STLP_BEGIN_NAMESPACE

// specialize hash_value in MSVC :
#ifdef _XHASH_
_STDEXT_BEGIN

template<class _Kty> inline size_t hash_value(const _Kty& _Keyval);

inline size_t hash_value(const cb::Token & _Keyval)
{	// hash _Keyval to size_t value one-to-one
	return ((size_t)_Keyval.GetHash() ^ _HASH_SEED);
}
	
_STDEXT_END
#endif // _XHASH_

#endif // _STLP_BEGIN_NAMESPACE
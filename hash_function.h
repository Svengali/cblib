#pragma once

#include <cblib/Base.h>
#include <cblib/Hashes.h>

/****

hash_function

for use with hash_table , or elsewhere

no hash algorithms in this file - those are in Hashes.h

this is just a template adapter to reroute to the right algorithm for a given data type

if you want your data types to be automatically hashed nicely, implement hash_function<> for them
	(see TokenHash.h for examples on some weirder data types)

-----------

A common default implementation of hash_table_ops<> for hash_table is also here.

NOTE : do NOT template specialize on hash_table_ops - you do not need to overload it
just provide your own ops structure
hash_table_ops is a *convenience* that you can derive from so that you have to implement less

*****/

START_CB

typedef uint32 hash_type;

/**

generic hash_function<> :
implement your own

**/
template <typename t_key>
inline hash_type hash_function(const t_key & t)
{
	// works for any type that implicitly converts to uint32 :
	return Hash32( t );
}

//*****************************************************************

// hash for char_ptr :

// ** warning "char *" and "const char *" are different types
typedef const char * char_ptr;

template <>
inline hash_type hash_function<char_ptr>(const char_ptr & t)
{
	return FNVHashStr(t);
}

//=========================================================================

template <>
inline hash_type hash_function<intptr_t>(const intptr_t & t)
{
	#ifdef CB_64
	return Hash64( (uint64) t );
	#else
	return Hash32( (uint32) t );
	#endif
}

template <>
inline hash_type hash_function<uint64>(const uint64 & t)
{
	return Hash64( t );
}

//=========================================================================

/**
	{hash,key} pairs must have two special values
	
	EMPTY and DELETED
	
	note that these are not necessarilly just key values,
		you can reserve the special values in either the hash or the key
	
	in the defaults/examples provided here I am reserving the special values in the keys
		and letting the hash use all 32 bits
		
	for pointers 0 and 1 are good choices
	
**/

template <typename t_key>
struct hash_table_key_equal
{
	// to check key equality, I call key_equal
	//	by default it uses operator ==
	//	but you can override it to other things
	//	(using a helper function like this is handy when key is a basic type like char *)
	bool key_equal(const t_key & lhs,const t_key & rhs)
	{
		return lhs == rhs;
	}
	
	inline hash_type hash_key(const t_key & k)
	{
		return hash_function<t_key>( k );
	}
};

template <typename t_key,t_key empty_val,t_key deleted_val>
struct hash_table_ops_keyreserved : public hash_table_key_equal<t_key>
{

	inline hash_type hash_key(const t_key & k)
	{
		ASSERT( k != empty_val && k != deleted_val );
		return hash_function<t_key>( k );
	}
	
	void make_empty(hash_type & hash,t_key & key)
	{
		key = empty_val;
	}

	bool is_empty(const hash_type & hash,const t_key & key)
	{
		return ( key == empty_val );
	}

	void make_deleted(hash_type & hash,t_key & key)
	{
		key = deleted_val;
	}

	bool is_deleted(const hash_type & hash,const t_key & key)
	{
		return ( key == deleted_val );
	}

};

template <typename t_key>
struct hash_table_ops_hash31
{
	bool key_equal(const t_key & lhs,const t_key & rhs)
	{
		return lhs == rhs;
	}
	
	inline hash_type hash_key(const t_key & t)
	{
		// hash gets 31 bits
		hash_type h = hash_function<t_key>( t );
		return h & 0x7FFFFFFFU;
	}

	void make_empty(hash_type & hash,t_key & key)
	{
		hash = 0x80000000U;
	}

	bool is_empty(const hash_type & hash,const t_key & key)
	{
		return ( hash == 0x80000000U );
	}

	void make_deleted(hash_type & hash,t_key & key)
	{
		hash = 0x80000001U;
	}

	bool is_deleted(const hash_type & hash,const t_key & key)
	{
		return ( hash == 0x80000001U );
	}

};

//=========================================================================

// largest negative ints reserved as special key values :
//struct hash_table_ops_int : public hash_table_ops_keyreserved<int,int(0x80000000),int(0x80000001)>
struct hash_table_ops_int : public hash_table_ops_hash31<int>
{
};

struct hash_table_ops_uint32 : public hash_table_ops_hash31<uint32>
{
};

struct hash_table_ops_intptr_t : public hash_table_ops_keyreserved<intptr_t,intptr_t(0),intptr_t(1)>
{
};

struct hash_table_ops_charptr : public hash_table_ops_keyreserved<char_ptr,char_ptr(0),char_ptr(1)>
{
	bool key_equal(const char_ptr & lhs,const char_ptr & rhs)
	{
		return strcmp(lhs,rhs) == 0;
	}
};

END_CB

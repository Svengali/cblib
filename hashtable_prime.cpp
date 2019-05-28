#include "hashtable.h"

START_CB

// prime table stolen from khash which stole it from STLport hash_map

const uint32 c_prime_list[c_HASH_PRIME_SIZE] =
{
  0,          3,          11,         23,         53,
  97,         193,        389,        769,        1543,
  3079,       6151,       12289,      24593,      49157,
  98317,      196613,     393241,     786433,     1572869,
  3145739,    6291469,    12582917,   25165843,   50331653,
  100663319,  201326611,  402653189,  805306457,  1610612741
  //, 3221225473 , 4294967291
};

END_CB

USE_CB

#include "String.h"

static void test1()
{
	hashtable_prime<int,String,hash_table_ops_int >	test;
	
	test.insert(0,String("0"));
	test.insert(2,String("2"));
	
	test.erase( test.find(0) );
}
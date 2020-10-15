#include "hash_table.h"
#include "Log.h"
#include "String.h"

USE_CB


void hash_table_test1()
{
	typedef hash_table<int,String,hash_table_ops_int > t_hash;
	t_hash	test;
	
	test.insert(0,String("0"));
	test.insert(2,String("2"));
	
	test.insert_or_replace(0,String("1"));
	
	test.insert(0,String("0"));
	
	for( t_hash::walk_iterator it = test.begin(); it != test.end(); ++it )
	{
		lprintf("%d : %s\n",it->key(),it->data().CStr());
	}
	
	//test.erase( test.find(0) );
	hash_type h = hash_function(0);
	for( t_hash::find_iterator mf = test.find_first( h, 0 ); mf != test.end(); ++mf )
	{
		lprintf("%d : %s\n",mf->key(),mf->data().CStr());
		//test.find_next(&mf);
	}
	
}
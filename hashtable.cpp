#include "hashtable.h"

//=========================================================================

#ifdef HASH_HISTO_PROBE_COUNT
int g_histo_steps_found[HASH_HISTO_PROBE_COUNT] = { 0 };
int g_histo_steps_notfound[HASH_HISTO_PROBE_COUNT] = { 0 };
#endif

#include "Log.h"
#include "File.h"
#include "FileUtil.h"
#include "StrUtil.h"
#include "autoprintf.h"
#include "Token.h"
#include "TokenHash.h"

#include <stdlib.h>
#include <stdio.h>

USE_CB

static void hash_table_test1()
{
	typedef hashtableentry_hkd<int,String,hash_table_ops_int>  t_hash_entry;
	typedef hashtable<t_hash_entry> t_hash;
	t_hash	test;
	
	test.insert(0,String("0"));
	test.insert(2,String("2"));
	
	test.insert_or_replace(0,String("1"));
	
	test.insert(0,String("0"));
	
	lprintf("walk : \n");
	
	for( t_hash::walk_iterator it = test.begin(); it != test.end(); ++it )
	{
		lprintf("%d : %s\n",it->key(),it->data().CStr());
	}
	
	lprintf("find 0 : \n");

	//test.erase( test.find(0) );
	hash_type h = hash_function(0);
	for( t_hash::find_iterator mf = test.find_first( h, 0 ); mf != test.end(); ++mf )
	{
		lprintf("%d : %s\n",mf->key(),mf->data().CStr());
		//test.find_next(&mf);
	}
	
}


static void test1()
{
	typedef hashtableentry_hkd<int,String,hash_table_ops_int > t_int_hash_entry;
	typedef hashtable<t_int_hash_entry>	t_int_hash;
	t_int_hash	test;
	
	test.insert(0,String("0"));
	test.insert(2,String("2"));
	
	test.insert_or_replace(0,String("1"));
	
	test.insert(0,String("0"));
	test.insert(3,String("3"));
	
	//test.erase( test.find(0) );
	hash_type h = hash_function(0);
	t_int_hash::find_iterator mf = test.find_first( h, 0 );
	while( mf )
	{
		lprintf("%d : %s\n",mf->key(),mf->data().CStr());
		test.find_next(&mf); //,h,0);
	}
	
	//test.erase( test.find(0) );
	
	mf = test.find_first( h, 0 );
	while( mf )
	{
		lprintf("%d : %s\n",mf->key(),mf->data().CStr());
		test.erase( mf );
		test.find_next(&mf); //,h,0);
		//mf = test.find_next(mf,h,0);
	}
	
	for(int i=100;i<1000;i++)
	{
		char buffer[80];
		itoa(i,buffer,10);
		
		test.insert(i,String(buffer));
	}
	
	for(int i=100;i<1000;i++)
	{
		t_int_hash::entry_type const * te = test.find(i);
		te;
		ASSERT( te->key() == i );
	}
	
	t_int_hash::entry_ptrc te = test.find(110);
	test.erase( te );
	
	te = test.find(110);
	ASSERT( te == NULL );
	
	te = test.find(111);
	ASSERT( te->key() == 111);
	
	/*
	for( t_int_hash::iterator it = test.head(); it != test.tail(); ++it )
	{
		lprintf("%d : %s\n",it->key , it->data.CStr());
	}
	*/
	
}

static void test2()
{
	typedef hashtableentry_hkd<char_ptr,String,hash_table_ops_charptr> t_char_string_entry;
	hashtable<t_char_string_entry>	test;
	
	for(int i=100;i<1000;i++)
	{
		char buffer[80];
		itoa(i,buffer,10);
		
		String str(buffer);
		char * cs = (char *) str.CStr();
		
		test.insert(cs,str);
	}
	
	const char * tptr = "110";
	
	hashtable<t_char_string_entry>::entry_ptrc te = test.find(tptr);
	test.erase( te );
	
	te = test.find((const char *)"110");
	ASSERT( te == NULL );
	
	te = test.find((const char *)"777");
	//ASSERT( te->key == 111);
	int i = 1;
	i;
}

static void test3()
{
	typedef hashtableentry_hkd<Token,String,hash_table_ops_Token> t_token_string_entry;
	hashtable<t_token_string_entry>	test;
	
	for(int i=100;i<1000;i++)
	{
		char buffer[80];
		itoa(i,buffer,10);
		
		String str(buffer);
		char * cs = (char *) str.CStr();
		
		test.insert(Token(cs),str);
	}
}


static void test4()
{
	//typedef hashtableentry_hkd<int,int,hash_table_ops_int> t_entry_int;
	//typedef hashtableentry_hd<int,hash_table_ops_int> t_entry_int;
	typedef hashtableentry_d<int,hash_table_ops_int> t_entry_int;
	hashtable<t_entry_int>	test;
	
	int last = 1;
	int cur = 1;
	test.insert(1,1);
	for(int i=0;i<30;i++)
	{
		int next = cur+last;
		ASSERT( next > cur );
		last = cur;
		cur = next;
		
		test.insert(next,next);
	}
	
	int highest = cur;

	lprintf("highest : %d\n",highest);

	for(int i=0;i<=highest;i++)
	{
		const t_entry_int * e = test.find(i);
		
		if ( e )
		{
			lprintf("%d\n",i);
		}
	}
}


static void test5()
{
	typedef hashtableentry_d<int,hash_table_ops_int> t_int_hash_entry;
	typedef hashtable<t_int_hash_entry>	t_int_hash;
	t_int_hash	test;
	
	test.insert(0);
	test.insert(2);
	
	// should assert :
	//test.insert_or_replace(0,1);
	test.insert_or_replace(1);
	
	test.insert(0);
	//test.insert_or_replace(0);
	test.insert(3);
	
	lprintf("find 0:\n");
	//test.erase( test.find(0) );
	hash_type h = hash_function(0);
	t_int_hash::find_iterator mf = test.find_first( h, 0 );
	while( mf )
	{
		lprintf("%d : %d\n",mf->key(),mf->data());
		test.find_next(&mf); //,h,0);
	}
	
	//test.erase( test.find(0) );
	
	mf = test.find_first( h, 0 );
	while( mf )
	{
		//lprintf("%d : %d\n",mf->key(),mf->data());
		test.erase( mf );
		test.find_next(&mf); //,h,0);
		//mf = test.find_next(mf,h,0);
	}
	
	lprintf("walk : \n");
	
	for( t_int_hash::walk_iterator it = test.begin(); it != test.end(); ++it )
	{
		lprintf("%d : %d\n",it->key(),it->data());
	}
	
	for(int i=100;i<1000;i++)
	{
		test.insert(i,i);
	}
	
	for(int i=100;i<1000;i++)
	{
		t_int_hash::entry_type const * te = test.find(i);
		te;
		ASSERT( te->key() == i );
	}
	
	t_int_hash::entry_ptrc te = test.find(110);
	test.erase( te );
	
	te = test.find(110);
	ASSERT( te == NULL );
	
	te = test.find(111);
	ASSERT( te->key() == 111);
	
	/*
	for( t_int_hash::iterator it = test.head(); it != test.tail(); ++it )
	{
		lprintf("%d : %s\n",it->key , it->data.CStr());
	}
	*/
	
}

void hashtable_test()
{

	hash_table_test1();

	test1();
	
	test2();
	
	test3();

	test4();

	test5();

}
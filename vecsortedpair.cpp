#include "Base.h"
#include "vecsortedpair.h"
#include "vector.h"
#include "vector_s.h"

//! \todo - move this to Regression
//@@@@

START_CB

//=======================================================================================
// explicit template instantiation for testing :

template vecsortedpair< vector< std::pair<int,int> > >;
template multivecsortedpair< vector< std::pair<int,int> > >;

//=======================================================================================

END_CB

/*
void vecsortedpair_test()
{
	struct Local
	{
	static void printintpair(const std::pair<int,int> & p)
	{
		printf("%d:%d,",p.first,p.second);
	}
	};

	{
		vecsortedpair< vector< std::pair<int,int> > > vsp;

		vsp.insert( std::make_pair(1,-1) );
		vsp.insert( std::make_pair(7,-7) );
		vsp.insert( std::make_pair(3,-3) );
		vsp.insert( 5,-5 );
		vsp.insert( -2,2 );
		vsp.insert( 13,-13 );

		puts("X");

		std::for_each(vsp.begin(),vsp.end(),Local::printintpair);

		puts("X");

		Local::printintpair( *( vsp.find(5) ) );
		Local::printintpair( *( vsp.find(3) ) );
		
		puts("X");
	}

	{
		vecsortedpair< vector_s< std::pair<int,int> ,32> > vsp;

		vsp.insert( std::make_pair(1,-1) );
		vsp.insert( std::make_pair(7,-7) );
		vsp.insert( std::make_pair(3,-3) );
		vsp.insert( 5,-5 );
		vsp.insert( -2,2 );
		vsp.insert( 13,-13 );

		puts("X");

		std::for_each(vsp.begin(),vsp.end(),Local::printintpair);

		puts("X");

		Local::printintpair( *( vsp.find(5) ) );
		Local::printintpair( *( vsp.find(3) ) );
		
		puts("X");
	}
	
	{
	
		typedef vecsortedpair< vector_s< std::pair<int,int> ,32> > t_vsp32;
		typedef vecsortedpair< vector_s< std::pair<int,int> ,64> > t_vsp64;

		t_vsp32 vsp;

		vsp.insert( std::make_pair(1,-1) );
		vsp.insert( std::make_pair(7,-7) );
		vsp.insert( std::make_pair(3,-3) );
		vsp.insert( 5,-5 );
		vsp.insert( -2,2 );
		vsp.insert( 13,-13 );

		puts("X");

		std::for_each(vsp.begin(),vsp.end(),Local::printintpair);

		puts("X");

		Local::printintpair( *( vsp.find(5) ) );
		Local::printintpair( *( vsp.find(3) ) );
		
		puts("X");

		t_vsp64 vsp2(vsp.begin(),vsp.end(),vecsorted_construct::sorted);

	}
}
*/
	/**
	
	STLport 4.0 :
	8-1-01
	---------------------------------------

	num_add = 20000;
	range_add = 20000;
	range_find = 30000;

	vec Finds : 0.2427 seconds
	map Finds : 0.3032 seconds
	hash Finds : 0.2117 seconds

	num_add = 2000;
	range_add = 20000;
	range_find = 30000;

	vec Finds : 0.1688 seconds
	map Finds : 0.1500 seconds
	hash Finds : 0.1357 seconds

	num_add = 100;
	range_add = 20000;
	range_find = 30000;

	vec Finds : 0.1184 seconds
	map Finds : 0.1083 seconds
	hash Finds : 0.1218 seconds

	num_add = 1000;
	range_add = 2000;
	range_find = 2000;

	vec Finds : 0.1980 seconds
	map Finds : 0.1831 seconds
	hash Finds : 0.1375 seconds

	num_add    = 100;
	range_add  = 2000;
	range_find = 2000;

	vec Finds : 1.2547 seconds
	map Finds : 1.2118 seconds
	hash Finds : 1.2823 seconds

	**/

	/**

	STLport 4.0 :
	8-1-01
	---------------------------------------

	num_maps = 1000;
	num_map_finds = 500;
	num_add = 10;
	range_add = 20;
	range_find = 30;

	vec Finds : 0.5277 seconds
	map Finds : 0.7630 seconds
	hash Finds : 0.6697 seconds

	num_maps = 100;
	num_map_finds = 500;
	num_add = 10;
	range_add = 20;
	range_find = 30;

	vec Finds : 0.4990 seconds
	map Finds : 0.5496 seconds
	hash Finds : 0.5102 seconds

	num_maps = 100;
	num_map_finds = 50;
	num_add = 100;
	range_add = 200;
	range_find = 300;

	vec Finds : 0.8190 seconds
	map Finds : 1.0017 seconds
	hash Finds : 0.5914 seconds

	num_maps = 100;
	num_map_finds = 50;
	num_add = 30;
	range_add = 50;
	range_find = 50;

	vec Finds : 0.7281 seconds
	map Finds : 0.7850 seconds
	hash Finds : 0.5774 seconds

	**/

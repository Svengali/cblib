#include "Base.h"
#include "vector.h"
#include "vector_s.h"
#include "vector_t.h"
#include <algorithm>
#include <stdio.h>

START_CB

//! \todo - move this to Regression
//@@@@

static void printint(const int & i)
{
	printf("%d,",i);
};

void vector_test()
{	
	{
	vector<int>	test;
	test.resize(4);
	std::fill(test.begin(),test.end(),0);

	test.push_back(3);
	test.push_back(test.back());
	test.push_back(test.back());
	test.push_back(test.back());
	test.push_back(test.back());
	test.push_back(test.back());
	
	std::for_each(test.begin(),test.end(),printint);
	puts("X");

	test.insert( test.begin() + 2, 77 );
	test.insert( test.begin() + 5, 77 );
	test.insert( test.begin() + 1, test.end() - 3, test.end());
	
	std::for_each(test.begin(),test.end(),printint);
	puts("X");

	test.erase( test.begin() + 1, test.begin() + 4 );
	test.erase( test.begin() + 5 );
	test.erase( test.begin() + 2 );
	std::for_each(test.begin(),test.end(),printint);
	puts("X");

	vector<int>	test2;
	test.swap(test2);
	test = test2;
	
	std::for_each(test.begin(),test.end(),printint);
	puts("X");
	
	static const int stuff[5] = { 1 };
	test.insert( test.begin() + 1, stuff, stuff + 5);

	std::for_each(test.begin(),test.end(),printint);

	puts("X");

	test.insert( test.end() - 3, test.end() - 6, test.end() );

	std::for_each(test.begin(),test.end(),printint);

	puts("X");
	
	test.insert( test.begin() + 1, stuff, stuff + 5);

	std::for_each(test.begin(),test.end(),printint);

	puts("X");

	test.insert( test.end() - 3, test.end() - 6, test.end() );

	std::for_each(test.begin(),test.end(),printint);

	puts("X");
	}

	{
	vector_t<int>	test;
	test.resize(4);
	std::fill(test.begin(),test.end(),0);

	test.push_back(3);
	test.push_back(test.back());
	test.push_back(test.back());
	test.push_back(test.back());
	test.push_back(test.back());
	test.push_back(test.back());
	
	std::for_each(test.begin(),test.end(),printint);
	puts("X");

	test.insert( test.begin() + 2, 77 );
	test.insert( test.begin() + 5, 77 );
	test.insert( test.begin() + 1, test.end() - 3, test.end());
	
	std::for_each(test.begin(),test.end(),printint);
	puts("X");

	test.erase( test.begin() + 1, test.begin() + 4 );
	test.erase( test.begin() + 5 );
	test.erase( test.begin() + 2 );
	std::for_each(test.begin(),test.end(),printint);
	puts("X");

	vector_t<int>	test2;
	test.swap(test2);
	test = test2;
	
	std::for_each(test.begin(),test.end(),printint);
	puts("X");
	
	static const int stuff[5] = { 1 };
	test.insert( test.begin() + 1, stuff, stuff + 5);

	std::for_each(test.begin(),test.end(),printint);

	puts("X");

	test.insert( test.end() - 3, test.end() - 6, test.end() );

	std::for_each(test.begin(),test.end(),printint);

	puts("X");
	
	test.insert( test.begin() + 1, stuff, stuff + 5);

	std::for_each(test.begin(),test.end(),printint);

	puts("X");

	test.insert( test.end() - 3, test.end() - 6, test.end() );

	std::for_each(test.begin(),test.end(),printint);

	puts("X");
	}

	
	{
	vector_s<int,32>	test;
	test.resize(4);
	std::fill(test.begin(),test.end(),0);

	test.push_back(3);
	test.push_back(test.back());
	test.push_back(test.back());
	test.push_back(test.back());
	test.push_back(test.back());
	test.push_back(test.back());
	
	std::for_each(test.begin(),test.end(),printint);
	puts("X");

	test.insert( test.begin() + 2, 77 );
	test.insert( test.begin() + 5, 77 );
	test.insert( test.begin() + 1, test.end() - 3, test.end());
	
	std::for_each(test.begin(),test.end(),printint);
	puts("X");

	test.erase( test.begin() + 1, test.begin() + 4 );
	test.erase( test.begin() + 5 );
	test.erase( test.begin() + 2 );
	std::for_each(test.begin(),test.end(),printint);
	puts("X");

	vector_s<int,32>	test2;
	test.swap(test2);
	test = test2;
	
	std::for_each(test.begin(),test.end(),printint);
	puts("X");
	
	static const int stuff[5] = { 1 };
	test.insert( test.begin() + 1, stuff, stuff + 5);

	std::for_each(test.begin(),test.end(),printint);

	puts("X");

	test.insert( test.end() - 3, test.end() - 6, test.end() );

	std::for_each(test.begin(),test.end(),printint);

	puts("X");
	
	test.insert( test.begin() + 1, stuff, stuff + 5);

	std::for_each(test.begin(),test.end(),printint);

	puts("X");

	test.insert( test.end() - 3, test.end() - 6, test.end() );

	std::for_each(test.begin(),test.end(),printint);

	puts("X");
	}
}

END_CB

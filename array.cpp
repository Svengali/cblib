#include "array.h"

START_CB

//#define ARRAY(decl,data)	const int x[] = data; decl = x;

void array_test()
{
	array<int,3>	a;
	//const int x[] = { 1,2,3 };
	//array<int,3>	b(x);
	array<int,3>	b;
	//ARRAY( array<int,3> b , {1,2,3} );
	array<int,4>	c;
	array<int,3>	d(b);
	
	a[0] = 1;
	a = b;
	c.assign(b.begin(),b.end());
	d.set_all(2);
	//b.clear();
	a.assign(d);
	
// 	{
// 		const int x[] = { 1,2,3 };
// 		array<int,3>	b(x);
		
// 		//array<int,3>	c( (const int *)({ 1,2,3 }) );
// 		//array<int,3>	c( (const int y[] = { 1,2,3 }) );
// 	}
}

END_CB

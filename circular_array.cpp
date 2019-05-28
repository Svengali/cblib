#include "circular_array.h"

START_CB

//#define ARRAY(decl,data)	const int x[] = data; decl = x;

#if 0
void circular_array_test()
{
	circular_array<int,3>	a;
	//const int x[] = { 1,2,3 };
	//circular_array<int,3>	b(x);
	circular_array<int,3>	b;
	//circular_array( circular_array<int,3> b , {1,2,3} );
	circular_array<int,4>	c;
	circular_array<int,3>	d(b);
	
	a[0] = 1;
	a = b;
	c.assign(b.begin(),b.end());
	//d.set_all(2);
	//b.clear();
	//a.assign(d);
	
	circular_array<int,3>::iterator it = d.begin();
	
	++it;
	int x = *it;
	int y = *(it++);
	
	a = d;
	
// 	{
// 		const int x[] = { 1,2,3 };
// 		circular_array<int,3>	b(x);
		
// 		//circular_array<int,3>	c( (const int *)({ 1,2,3 }) );
// 		//circular_array<int,3>	c( (const int y[] = { 1,2,3 }) );
// 	}
}
#endif

END_CB

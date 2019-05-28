#include "cblib/Vec3i.h"
#include "cblib/File.h"
#include "cblib/Log.h"

START_CB

//-------------------------------------------------------------------
//statics 

const Vec3i Vec3i::zero(0,0,0);
const Vec3i Vec3i::unitX(1,0,0);
const Vec3i Vec3i::unitY(0,1,0);
const Vec3i Vec3i::unitZ(0,0,1);
const Vec3i Vec3i::unitXneg(-1,0,0);
const Vec3i Vec3i::unitYneg(0,-1,0);
const Vec3i Vec3i::unitZneg(0,0,-1);

//-------------------------------------------------------------------

//-------------------------------------------------------------------

void Vec3i::Log() const //!< writes xyz to Log; does NOT add a \n !
{
	lprintf("{%d,%d,%d}",x,y,z);
}

//-------------------------------------------------------------------

bool Vec3i::IsSafeToSquare64() const
{
	DURING_ASSERT( const int64 safe_range = 1073741824i64 );
	ASSERT( ABS(x) <= safe_range );
	ASSERT( ABS(y) <= safe_range );
	ASSERT( ABS(z) <= safe_range );
	return true;
}

bool Vec3i::ProductIsSafe64(const Vec3i & u,const Vec3i & v)
{
	#ifdef DO_ASSERTS
	const int64 u_max = MAX3( ABS(u.x), ABS(u.y), ABS(u.z) ); 
	const int64 v_max = MAX3( ABS(v.x), ABS(v.y), ABS(v.z) );
	const double max_product = double(u_max) * double(v_max); 
	const double safe_range = 8e+018; // about 2^63
	ASSERT( max_product <= safe_range );
	#endif
	return true;
}

bool Vec3i::ProductIsSafe32(const Vec3i & u,const Vec3i & v)
{
	#ifdef DO_ASSERTS
	const int64 u_max = MAX3( ABS(u.x), ABS(u.y), ABS(u.z) ); 
	const int64 v_max = MAX3( ABS(v.x), ABS(v.y), ABS(v.z) );
	const double max_product = double(u_max) * double(v_max); 
	ASSERT( max_product <= 2147483647 );
	#endif
	
	return true;
}

//-------------------------------------------------------------------

bool	Colinear( const Vec3i & a, const Vec3i & b, const Vec3i & c )
{
	// compare (b-a) and (c-a)
	// if they have the same ratios of deltas in all three 2d planar projections
	//	then the three points are colinear
	// (this is the same as making the 64-bit cross product and checking all components == 0)
   return 
         int64( b.y - a.y ) * int64( c.z - a.z ) ==
         int64( b.z - a.z ) * int64( c.y - a.y ) 
      && int64( b.z - a.z ) * int64( c.x - a.x ) ==
         int64( b.x - a.x ) * int64( c.z - a.z ) 
      && int64( b.x - a.x ) * int64( c.y - a.y ) ==
         int64( b.y - a.y ) * int64( c.x - a.x ) ;
}

/*
double  VolumeD( const Vec3i & a, const Vec3i & b, const Vec3i & c, const Vec3i & p )
{
	// the int64 version is actually much preferred; it's hard for me 
	const double ax = a.x - p.x;
	const double ay = a.y - p.y;
	const double az = a.z - p.z;
	const double bx = b.x - p.x;
	const double by = b.y - p.y;
	const double bz = b.z - p.z;
	const double cx = c.x - p.x;
	const double cy = c.y - p.y;
	const double cz = c.z - p.z;

	const double vol = 
		   ax * (by*cz - bz*cy)
		 + ay * (bz*cx - bx*cz)
		 + az * (bx*cy - by*cx);

	return vol;
}
*/

/*

p = {0,0,0}
a = {1,0,0}
b = {0,1,0}
c = {0,0,1}

has a volume of (1/6)

p = {0,0,1}
a = {0,0,0}
b = {1,0,0}
c = {0,1,0}

ap = {0,0,-1}
bp = {1,0,-1}
cp = {0,1,-1}

vol = (-1) * (- (1)(1)) = 1

*/

// Volume is *signed* ; this is actually SIX times the real volume
// if "abc" is CCW , then right-hand rule determines the normal, and if "p" is on the front
//	side of triangle "abc", then Volume is positive
int64  Volume( const Vec3i & a, const Vec3i & b, const Vec3i & c, const Vec3i & p )
{
	const int64 ax = a.x - p.x;
	const int64 ay = a.y - p.y;
	const int64 az = a.z - p.z;
	const int64 bx = b.x - p.x;
	const int64 by = b.y - p.y;
	const int64 bz = b.z - p.z;
	const int64 cx = c.x - p.x;
	const int64 cy = c.y - p.y;
	const int64 cz = c.z - p.z;

	#ifdef DO_ASSERTS
	{
		// check that none of the math is going to exceed precision
		static const double safe_range = 9223372036854775807.0 / 3.0;
		ASSERT( fabs(double(bx) * double(cz)) <= safe_range );
		ASSERT( fabs(double(bx) * double(cy)) <= safe_range );
		ASSERT( fabs(double(by) * double(cz)) <= safe_range );
		ASSERT( fabs(double(by) * double(cx)) <= safe_range );
		ASSERT( fabs(double(bz) * double(cy)) <= safe_range );
		ASSERT( fabs(double(bz) * double(cx)) <= safe_range );
		ASSERT( fabs(double(ax) * double(by*cz - bz*cy)) <= safe_range );
		ASSERT( fabs(double(ay) * double(bz*cx - bx*cz)) <= safe_range );
		ASSERT( fabs(double(az) * double(bx*cy - by*cx)) <= safe_range );
	}
	#endif

	const int64 vol = 
		   ax * (bz*cy - by*cz)
		 + ay * (bx*cz - bz*cx)
		 + az * (by*cx - bx*cy);

	return vol;
}

/*
int  VolumeSign( const Vec3i & a, const Vec3i & b, const Vec3i & c, const Vec3i & d )
{
   const int64 vol = Volume(a,b,c,d);

   if      ( vol >  0 )  return  1;
   else if ( vol < -0 )  return -1;
   else                  return  0;
}
*/

// AreaSqr return the square of (twice the area) of the triangle (a,b,c)
//  AreaSqr uses the *fourth* power of a coordinate, so it's not safe; use AreaSqrD
int64	AreaSqr( const Vec3i & a, const Vec3i & b, const Vec3i & c )
{
	// this will only work if the edges fit in 16 bits
	//	so they can be squared to 32
	//	and then squared again to 64
	const Vec3i e1 = b - a;
	const Vec3i e2 = c - a;
	
	// cross will call ProductIsSafe32 for us
	const Vec3i perp = e1 ^ e2;
	
	// area = length(perp)
	return perp.LengthSqr();
}

double	AreaSqrD( const Vec3i & a, const Vec3i & b, const Vec3i & c )
{
	const Vec3i e1 = b - a;
	const Vec3i e2 = c - a;

	// go to 64 bits to square :	
	const int64 inx = int64(e1.y) * int64(e2.z) - int64(e1.z) * int64(e2.y);
	const int64 iny = int64(e1.z) * int64(e2.x) - int64(e1.x) * int64(e2.z);
	const int64 inz = int64(e1.x) * int64(e2.y) - int64(e1.y) * int64(e2.x);

	// need to square again, switch to doubles :
	const double nx = double(inx);
	const double ny = double(iny);
	const double nz = double(inz);

	const double area_square = nx*nx + ny*ny + nz*nz;

	return area_square;
}

//-------------------------------------------------------------------

#if 0 //{

#include <stdio.h>

void Vec3i_Test()
{
	{
	Vec3i a(0,1,7),b(2,3,1);
	Vec3i c(7,0,3),d(14,19,23);

	bool col = Colinear(a,b,c);

	int64 v = Volume(a,b,c,d);
	
	printf(" vol = %I64\n",v);
	}

	{
	Vec3i a(0,1,7),b(2,3,1);
	Vec3i c(7,0,3),d(4,9,3);

	a *= 2642245/10;
	b *= 2642245/10;
	c *= 2642245/10;
	d *= 2642245/10;

	a.x ++;
	a.z ++;
	b.y ++;
	b.x ++;
	c.x ++;
	c.z ++;
	d.x ++;

	bool col = Colinear(a,b,c);

	int64 v = Volume(a,b,c,d);

	printf(" vol = %I64\n",v);
	}

	// this will trip the assert
	/*
	{
	Vec3i a(0,1,7),b(2,3,1);
	Vec3i c(7,0,3),d(4,9,3);

	a *= 214748364;
	b *= 214748364;
	c *= 214748364;
	d *= 214748364;

	bool col = Colinear(a,b,c);

	int64 v = Volume(a,b,c,d);
	}
	*/
}

#endif //}

END_CB

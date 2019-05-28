

//    Copright (C) 1999
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA,
//    or download the License terms from prep.ai.mit.edu/pub/gnu/COPYING-2.0.
//
//    Contact:
//    --------
//    Bernd Gaertner
//    Institut f. Informatik
//    ETH Zuerich
//    ETH-Zentrum
//    CH-8092 Zuerich, Switzerland
//    http://www.inf.ethz.ch/personal/gaertner
//

#include "cblib/Base.h"
#include "cblib/stl_basics.h"
#include "cblib/Vec3.h"
#include "cblib/Sphere.h"

USE_CB

#undef new
#undef delete

#pragma warning(push) //{ STL includes
#pragma warning(disable: 4702) //  unreachable code
#pragma warning(disable: 4245) //  signed/unsigned mismatch 

#include <list>

#pragma warning(pop) //} STL includes

namespace
{

class Miniball;
class Basis;

// Basis
// -----


class Basis 
{
    private:
	
		enum { d = 3 } eDimensions;

        // data members
        int				   m, s;   // size and number of support Vec3s
        
		Vec3            q0;

        float              z[d+1];
        float              f[d+1];
        Vec3            v[d+1];
        Vec3            a[d+1];
        Vec3            c[d+1];
        float              sqr_r[d+1];

        Vec3 *          current_c;      // Vec3s to some c[j]
        float              current_sqr_r;

        float              sqr (float r) const {return r*r;}

    public:
        Basis();

        // access
        const Vec3*     center() const;
        float              squared_radius() const;
        int                 size() const;
        int                 support_size() const;
        float              excess (const Vec3& p) const;

        // modification
        void                reset(); // generates empty Sphere with m=s=0
        bool                push (const Vec3& p);
        void                pop ();
};

// Miniball
// --------
    
    
class Miniball
{
    public:
        // types
		typedef std::list<Vec3>	VectorList;

        typedef VectorList::iterator         It;
        typedef VectorList::const_iterator   Cit;
		
    private:
        // data members
        VectorList	L;         // STL list keeping the Vec3s
        Basis	    B;              // basis keeping the current ball
        It          support_end;    // past-the-end iterator of support set

        // private methods
        void        mtf_mb (It k);
        void        pivot_mb (It k);
        void        move_to_front (It j);
        float      max_excess (It t, It i, It& pivot) const;
        float      abs (float r) const {return (r>0)? r: (-r);}
        float      sqr (float r) const {return r*r;}

    public:
        // construction
        Miniball() {}
        void        check_in (const Vec3& p);
        void        build ();

        // access
        Vec3       center() const;
        float		squared_radius () const;
        int         num_points () const;
        Cit         points_begin () const;
        Cit         points_end () const;
        int         nr_support_Vec3s () const;
        Cit         support_points_begin () const;
        Cit         support_points_end () const;
};
    
    
  
    

// Miniball
// --------


void Miniball::check_in (const Vec3& p)
{
   L.push_back(p);
}



void Miniball::build ()
{
	B.reset();
	support_end = L.begin();

	// @@ pivotting or not ?
	if ( 1 )
	   pivot_mb (L.end());
	else
	   mtf_mb (L.end());
}



void Miniball::mtf_mb (It i)
{
	support_end = L.begin();

	if ((B.size())== 4) 
		return;

	for (It k=L.begin(); k!=i;) 
	{
	   It j=k++;
	   if (B.excess(*j) > 0) 
	   {
		   if (B.push(*j)) 
		   {
			   mtf_mb (j);
			   B.pop();
			   move_to_front(j);
		   }
	   }
	}
}


void Miniball::move_to_front (It j)
{
	if (support_end == j)
	   support_end++;
	L.splice (L.begin(), L, j);
}



void Miniball::pivot_mb (It i)
{
	It t = ++L.begin();
	mtf_mb (t);
	float max_e =0.f, old_sqr_r = 0.f;
	do 
	{
	   It pivot = L.begin();
	   max_e = max_excess (t, i, pivot);
	   if (max_e > 0) 
	   {
		   t = support_end;
		   if (t==pivot) ++t;
		   old_sqr_r = B.squared_radius();
		   B.push (*pivot);
		   mtf_mb (support_end);
		   B.pop();
		   move_to_front (pivot);
	   }
	} while ((max_e > 0) && (B.squared_radius() > old_sqr_r));
}



float Miniball::max_excess (It t, It i, It& pivot) const
{
   const Vec3 * pCenter = B.center();
   float sqr_r = B.squared_radius();
   
   float e, max_e = 0;

   for (It k=t; k!=i; ++k)
   {
       const Vec3 & point = (*k);
       e = -sqr_r;
       
	   e += DistanceSqr(point,*pCenter);

       if (e > max_e)
	   {
           max_e = e;
           pivot = k;
       }
   }

   return max_e;
}




Vec3 Miniball::center () const
{
   return *((Vec3 *)B.center());
}


float Miniball::squared_radius () const
{
   return B.squared_radius();
}



int Miniball::num_points () const
{
   return (int) L.size();
}


Miniball::Cit Miniball::points_begin () const
{
   return L.begin();
}


Miniball::Cit Miniball::points_end () const
{
   return L.end();
}



int Miniball::nr_support_Vec3s () const
{
   return B.support_size();
}


Miniball::Cit Miniball::support_points_begin () const
{
   return L.begin();
}


Miniball::Cit Miniball::support_points_end () const
{
   return support_end;
}
   

//----------------------------------------------------------------------
// Basis
//---------------------------------------------------------------------

const Vec3* Basis::center () const
{
   return current_c;
}


float Basis::squared_radius() const
{
   return current_sqr_r;
}


int Basis::size() const
{
   return m;
}


int Basis::support_size() const
{
   return s;
}


float Basis::excess (const Vec3& p) const
{
	float e = -current_sqr_r;
	e += DistanceSqr(p,*current_c);
	return e;
}




void Basis::reset ()
{
	m = s = 0;
	// we misuse c[0] for the center of the empty Sphere
	c[0] = Vec3::zero;
	current_c = c;
	current_sqr_r = -1;
}



Basis::Basis ()
{
   reset();
}



void Basis::pop ()
{
   --m;
}
   
   
   
bool Basis::push (const Vec3& p)
{
	if (m==0)
	{
		q0 = p;
		c[0] = q0;
		sqr_r[0] = 0;
	}
	else
	{
		int i;
		const float eps = 1e-16f;

		// set v_m to Q_m
		v[m] = p - q0;

		// compute the a_{m,i}, i< m
		for (i=1; i<m; ++i)
		{
			a[m][i] = 0;

			a[m][i] += v[i] * v[m];

			a[m][i] *= (2.f/z[i]);
		}

		// update v_m to Q_m-\bar{Q}_m
		for (i=1; i<m; ++i)
		{
			v[m] -= a[m][i] * v[i];
		}

		// compute z_m
		z[m]=0;
		z[m] += v[m].LengthSqr();
		z[m]*=2;

		// reject push if z_m too small
		if (z[m]<eps*current_sqr_r)
		{
			return false;
		}

		// update c, sqr_r
		float e = -sqr_r[m-1];
		e += DistanceSqr(p,c[m-1]);

		f[m]=e/z[m];

		c[m] = c[m-1] + f[m] * v[m];

		sqr_r[m] = sqr_r[m-1] + e*f[m]/2;
	}

	current_c = c+m;
	current_sqr_r = sqr_r[m];
	s = ++m;
	return true;
}


}; // nameless namespace

//----------------------------------------------------------------------

namespace MiniBall
{
	//-----------------------------------------

	static bool SphereContainsAll(const Sphere & s,
								  const Vec3 * pVerts,
								  const int numVerts)
	{
		for(int i=0;i<numVerts;i++)
		{
			if (s.Contains(pVerts[i]) == false)
				return false;
		}
		return true;
	}

	static bool PatchUpSphere(Sphere &    s,
							  const Vec3* pVerts,
							  const int   numVerts)
	{
		float maxRadSq = 0.0f;
		for(int i=0;i<numVerts;i++)
		{
			maxRadSq = MAX(maxRadSq, DistanceSqr(s.GetCenter(), pVerts[i]));
		}

		const float maxRad = sqrtf(maxRadSq);
		s.SetRadius(maxRad * (1 + EPSILON));
		
		return true;
	}

	//-----------------------------------------

	void Make(Sphere * pS,
				const Vec3 * pVerts,
				const int numVerts)
	{
		ASSERT(pS);
		ASSERT(pVerts);
		ASSERT(numVerts>0);

		Miniball mb;

		for(int i=0;i<numVerts;i++)
		{
			mb.check_in(pVerts[i]);
		}

		mb.build();

		pS->SetCenter( mb.center() );
		pS->SetRadius( sqrtf( mb.squared_radius() ) );

		// dmoore: for some reason, this can sometimes fail quite unreasonably.  Sticking in
		//  a patch for right now, need to come back and diagnose this properly at some point
		if (SphereContainsAll(*pS, pVerts, numVerts) == false)
		{
			PatchUpSphere(*pS, pVerts, numVerts);
		}


		ASSERT( SphereContainsAll(*pS, pVerts, numVerts ) );
	}

	//-----------------------------------------
};

#include "MatNM.h"

// test :
template cb::MatNM<4,4>;

/**

http://www.euclideanspace.com/maths/algebra/matrix/code/index.htm#cpp

The following C++ matrix are more optimised than the java code above and can be used for any dimension matrix, It relies on templates, it would also be useful to overload the +, - ,* and / operators for matrix arithmetic. This code is written by sudeep das ( das_sudeep@hotmail.com ) who kindly allowed me to include it here.

template < int order>
         class Matx {protected: double mtx[ order][ order];public:
  double &element( int i, int j) { return mtx[ i][ j]; }
  const double &element( int i, int j) const { return mtx[ i][ j]; }
  ///////////////////////////////////////
  Matx<order> multiply( const Matx &mat) const  Matx< order> a;
  for ( int i = 0; i < order; ++i)
    for ( int l = 0; l < order; ++l) {
      double &x = a.element( i, l) = 0;
        for ( int j = 0; j < order; ++j)
         x += element( i, j) * mat.element( j, l);
    }
    return a;
   }
   
   //////////////////////////////////////////
   template <int order>
   Matx<order> Matx<order>::inverse( void) const {
     Matx<order> b;
     for ( int i = 0; i < order; ++i)
       for ( int j = 0; j < order; ++j) {
         int sgn = ( (i+j)%2) ? -1 : 1;
         b.element( i, j) = sgn * MatxCofactor< order -1>( *this, i,j).determinant();
       }
       b.transpose();
       b /= determinant();
       return b;
    } //////////////////////////////////////////
   template <int order>
   double Matx<order>::determinant( void) const {
     double d = 0;
     for ( int i = 0; i < order; ++i) {
       int sgn = ( i % 2) ? -1 : 1;
       Matx<order -1> cf = MatxCofactor< order -1>( *this, i, 0);
       d += sgn * element( i, 0) * cf.determinant();
     }
     return d;
   }
 //////////////////////////////////////////
   void transpose( void) {
     for ( int i = 0; i < order; ++i) {
       for ( int j = i +1; j < order; ++j)
         ::swap( &element( i, j), &element( j, i)); // need a swap function not provided here
     }
   }
};////////////////////////////////////////////////////
template <int corder>
class MatxCofactor : public Matx< corder> {
  public:
  MatxCofactor( const Matx< corder+1> &a, int aI, int aJ) {
    for ( int i = 0, k = 0; i < ( corder+1); ++i) {
      if ( i != aI) {
        for ( int j = 0, l = 0; j < ( corder+1); ++j) {
          if ( j != aJ) {
            element( k, l) = a.element( i, j);
            ++l;
          }
        }
        ++k;
      }
    }
  }
};

**/
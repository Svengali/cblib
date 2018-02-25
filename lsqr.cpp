/*
* lsqr.c
* This is a C version of LSQR, derived from the Fortran 77 implementation
* of C. C. Paige and M. A. Saunders.
*
* This file defines functions for data type allocation and deallocation,
* lsqr itself (the main algorithm),
* and functions that scale, copy, and compute the Euclidean norm of a vector.
*
* 08 Sep 1999: First version from James W. Howse <jhowse@lanl.gov>
*/

#include "lsqr.h"
#include "cblib/Base.h"
#include "cblib/Log.h"
#include "cblib/Util.h"
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <float.h>

/*------------------------*/
/* User-defined functions */
/*------------------------*/

#define sqr(a)		( (a) * (a) )
//#define max(a,b)	( (a) < (b) ? (b) : (a) )
//#define min(a,b)	( (a) < (b) ? (a) : (b) )
#define round(a)        ( (a) > 0.0 ? (int)((a) + 0.5) : (int)((a) - 0.5) )


/*----------------------*/
/* Default declarations */
/*----------------------*/

#define TRUE	(1)
#define FALSE	(0)
//#define PI      (4.0 * atan(1.0))


//#define DO_LOGGING

START_CB

namespace lsqr
{


/*
*------------------------------------------------------------------------------
*
*     Workspace Quantities
*     --------------------
*
*     bidiag_wrk_vec  workspace  This float vector is a workspace for the 
*                                current iteration of the
*                                Lanczos bidiagonalization.  
*                                This vector has length 'n' (i.e., 'num_cols').
*
*     srch_dir_vec    workspace  This float vector contains the search direction 
*                                at the current iteration.  This vector has a 
*                                length of 'n' (i.e., 'num_cols').
*
*------------------------------------------------------------------------------
*/

class LSQR_WORK
{
public:
  VectorDouble     bidiag_wrk_vec;
  VectorDouble     srch_dir_vec;

	LSQR_WORK() { }
	LSQR_WORK(int n)
	{
		bidiag_wrk_vec.resize(n);
		srch_dir_vec.resize(n);
	}  
};

     /*---------------------------------------------------------------*/
     /*                                                               */
     /*  Define the error handling function for LSQR and its          */
     /*  associated functions.                                        */ 
     /*                                                               */
     /*---------------------------------------------------------------*/

void lsqr_error( char  *msg,
                 int     )
{
  //fprintf(stderr, "\t%s\n", msg);
  //exit(code);
	cb::lprintf( "\t%s\n", msg);
	THROW;
}

/*-------------------------------------------------------------------------*/
/*                                                                         */
/*  Define the LSQR function.                                              */
/*                                                                         */
/*-------------------------------------------------------------------------*/

/*
*------------------------------------------------------------------------------
*
*     LSQR  finds a solution x to the following problems:
*
*     1. Unsymmetric equations --    solve  A*x = b
*
*     2. Linear least squares  --    solve  A*x = b
*                                    in the least-squares sense
*
*     3. Damped least squares  --    solve  (   A    )*x = ( b )
*                                           ( damp*I )     ( 0 )
*                                    in the least-squares sense
*
*     where 'A' is a matrix with 'm' rows and 'n' columns, 'b' is an
*     'm'-vector, and 'damp' is a scalar.  (All quantities are real.)
*     The matrix 'A' is intended to be large and sparse.  
*
*
*     Notation
*     --------
*
*     The following quantities are used in discussing the subroutine
*     parameters:
*
*     'Abar'   =  (   A    ),          'bbar'  =  ( b )
*                 ( damp*I )                      ( 0 )
*
*     'r'      =  b  -  A*x,           'rbar'  =  bbar  -  Abar*x
*
*     'rnorm'  =  sqrt( norm(r)**2  +  damp**2 * norm(x)**2 )
*              =  norm( rbar )
*
*     'rel_prec'  =  the relative precision of floating-point arithmetic
*                    on the machine being used.  Typically 2.22e-16
*                    with 64-bit arithmetic.
*
*     LSQR  minimizes the function 'rnorm' with respect to 'x'.
*
*
*     References
*     ----------
*
*     C.C. Paige and M.A. Saunders,  LSQR: An algorithm for sparse
*          linear equations and sparse least squares,
*          ACM Transactions on Mathematical Software 8, 1 (March 1982),
*          pp. 43-71.
*
*     C.C. Paige and M.A. Saunders,  Algorithm 583, LSQR: Sparse
*          linear equations and least-squares problems,
*          ACM Transactions on Mathematical Software 8, 2 (June 1982),
*          pp. 195-209.
*
*     C.L. Lawson, R.J. Hanson, D.R. Kincaid and F.T. Krogh,
*          Basic linear algebra subprograms for Fortran usage,
*          ACM Transactions on Mathematical Software 5, 3 (Sept 1979),
*          pp. 308-323 and 324-325.
*
*------------------------------------------------------------------------------
*/
void lsqr( lsqr_input *input, lsqr_output *output,
           lsqr_func *func,
	   void *prod )
{
  double  dvec_norm2( VectorDouble * );

  long    indx,
          //num_iter,
          term_iter,
          term_iter_max;
  
  double  alpha, 
          beta,
          rhobar,
          phibar,
          bnorm,
          bbnorm,
          cs1,
          sn1,
          psi,
          rho,
          cs,
          sn,
          theta,
          phi,
          tau,
          ddnorm,
          delta,
          gammabar,
          zetabar,
          gamma,
          cs2,
          sn2,
          zeta,
          xxnorm,
          res,
          resid_tol,
          cond_tol,
          resid_tol_mach,
          //temp,
          stop_crit_1,
          stop_crit_2,
          stop_crit_3;

  static char term_msg[8][80] = 
  {
    "The exact solution is x = x0",
    "The residual Ax - b is small enough, given ATOL and BTOL",
    "The least squares error is small enough, given ATOL",
    "The estimated condition number has exceeded CONLIM",
    "The residual Ax - b is small enough, given machine precision",
    "The least squares error is small enough, given machine precision",
    "The estimated condition number has exceeded machine precision",
    "The iteration limit has been reached"
  };

  //if( input->lsqr_fp_out != NULL )
    //fprintf( input->lsqr_fp_out,
	#ifdef DO_LOGGING
	Log( "  Least Squares Solution of A*x = b\n"
				"The matrix A has %7i rows and %7i columns\n"
				"The damping parameter is\tDAMP = %10.2e\n"
				"ATOL = %10.2e\t\tCONDLIM = %10.2e\n"
				"BTOL = %10.2e\t\tITERLIM = %10i\n\n",
				input->num_rows, input->num_cols, input->damp_val, input->rel_mat_err,
				input->cond_lim, input->rel_rhs_err, input->max_iter );
	#endif // DO_LOGGING

  output->term_flag = 0;
  term_iter = 0;

  output->num_iters = 0;

  output->frob_mat_norm = 0.0;
  output->mat_cond_num = 0.0;
  output->sol_norm = 0.0;

  LSQR_WORK workspace(input->num_cols);
 LSQR_WORK * work = &workspace; 
 
	//output->std_err_vec.clear();
	//output->std_err_vec.resize(input->num_cols,0.f);
 
  for(indx = 0; indx < input->num_cols; indx++)
    {
      work->bidiag_wrk_vec.data()[indx] = 0.0;
      work->srch_dir_vec.data()[indx] = 0.0;
      //output->std_err_vec.data()[indx] = 0.0;
    }

  bbnorm = 0.0;
  ddnorm = 0.0;
  xxnorm = 0.0;

  cs2 = -1.0;
  sn2 = 0.0;
  zeta = 0.0;
  res = 0.0;

  if( input->cond_lim > 0.0 )
    cond_tol = 1.0 / input->cond_lim;
  else
    cond_tol = DBL_EPSILON;

  alpha = 0.0;
  beta = 0.0;
/*
*  Set up the initial vectors u and v for bidiagonalization.  These satisfy 
*  the relations
*             BETA*u = b - A*x0 
*             ALPHA*v = A^T*u
*/
    /* Compute b - A*x0 and store in vector u which initially held vector b */
  dvec_scale( (-1.0), input->rhs_vec );
  func->mat_vec_prod( 0, input->sol_vec, input->rhs_vec, prod );
  dvec_scale( (-1.0), input->rhs_vec );
  
    /* compute Euclidean length of u and store as BETA */
  beta = dvec_norm2( input->rhs_vec );
  
  if( beta > 0.0 )
    {
        /* scale vector u by the inverse of BETA */
      dvec_scale( (1.0 / beta), input->rhs_vec );

        /* Compute matrix-vector product A^T*u and store it in vector v */
      func->mat_vec_prod( 1, &work->bidiag_wrk_vec, input->rhs_vec, prod );
      
        /* compute Euclidean length of v and store as ALPHA */
      alpha = dvec_norm2( &work->bidiag_wrk_vec );      
    }
  
  if( alpha > 0.0 )
    {
        /* scale vector v by the inverse of ALPHA */
      dvec_scale( (1.0 / alpha), &work->bidiag_wrk_vec );

        /* copy vector v to vector w */
      dvec_copy( &work->bidiag_wrk_vec, &work->srch_dir_vec );
    }    

  output->mat_resid_norm = alpha * beta;
  output->resid_norm = beta;
  bnorm = beta;
/*
*  If the norm || A^T r || is zero, then the initial guess is the exact
*  solution.  Exit and report this.
*/
  if( (output->mat_resid_norm == 0.0) ) // && (input->lsqr_fp_out != NULL) )
    {
      //fprintf( input->lsqr_fp_out,
	  #ifdef DO_LOGGING
	  Log( "\tISTOP = %3i\t\t\tITER = %9i\n"
			"|| A ||_F = %13.5e\tcond( A ) = %13.5e\n"
			"|| r ||_2 = %13.5e\t|| A^T r ||_2 = %13.5e\n"
			"|| b ||_2 = %13.5e\t|| x - x0 ||_2 = %13.5e\n\n", 
			output->term_flag, output->num_iters, output->frob_mat_norm, 
			output->mat_cond_num, output->resid_norm, output->mat_resid_norm,
			bnorm, output->sol_norm );

      //fprintf( input->lsqr_fp_out,
	  Log( "  %s\n", term_msg[output->term_flag]);
      #endif // DO_LOGGING
      
      return;
    }

  rhobar = alpha;
  phibar = beta;
/*
*  If statistics are printed at each iteration, print a header and the initial
*  values for each quantity.
*/
  //if( input->lsqr_fp_out != NULL )
    {
      //fprintf( input->lsqr_fp_out,
	  //Log("  ITER     || r ||    Compatible  ||A^T r|| / ||A|| ||r||  || A ||    cond( A )\n\n" );

      stop_crit_1 = 1.0;
      stop_crit_2 = alpha / beta;

      //fprintf( input->lsqr_fp_out,
	  /*Log(
		  "%6i %13.5e %10.2e \t%10.2e \t%10.2e  %10.2e\n",
			   output->num_iters, output->resid_norm, stop_crit_1, stop_crit_2,
			   output->frob_mat_norm, output->mat_cond_num);
		*/
	}
 
/*
*  The main iteration loop is continued as long as no stopping criteria
*  are satisfied and the number of total iterations is less than some upper
*  bound.
*/
  while( output->term_flag == 0 )
    {
      output->num_iters++;
/*      
*     Perform the next step of the bidiagonalization to obtain
*     the next vectors u and v, and the scalars ALPHA and BETA.
*     These satisfy the relations
*                BETA*u  =  A*v  -  ALPHA*u,
*                ALFA*v  =  A^T*u  -  BETA*v.
*/      
         /* scale vector u by the negative of ALPHA */
      dvec_scale( (-alpha), input->rhs_vec );

        /* compute A*v - ALPHA*u and store in vector u */
      func->mat_vec_prod( 0, &work->bidiag_wrk_vec, input->rhs_vec, prod );

        /* compute Euclidean length of u and store as BETA */
      beta = dvec_norm2( input->rhs_vec );

        /* accumulate this quantity to estimate Frobenius norm of matrix A */
      bbnorm += sqr(alpha) + sqr(beta) + sqr(input->damp_val);

      if( beta > 0.0 )
	{
	    /* scale vector u by the inverse of BETA */
	  dvec_scale( (1.0 / beta), input->rhs_vec );

	    /* scale vector v by the negative of BETA */
	  dvec_scale( (-beta), &work->bidiag_wrk_vec );

	    /* compute A^T*u - BETA*v and store in vector v */
	  func->mat_vec_prod( 1, &work->bidiag_wrk_vec, input->rhs_vec, prod );
      
	    /* compute Euclidean length of v and store as ALPHA */
	  alpha = dvec_norm2( &work->bidiag_wrk_vec );

	  if( alpha > 0.0 )
	  {
	      /* scale vector v by the inverse of ALPHA */
	    dvec_scale( (1.0 / alpha), &work->bidiag_wrk_vec );
		}
	}
/*
*     Use a plane rotation to eliminate the damping parameter.
*     This alters the diagonal (RHOBAR) of the lower-bidiagonal matrix.
*/
      cs1 = rhobar / sqrt( sqr(rhobar) + sqr(input->damp_val) );
      sn1 = input->damp_val / sqrt( sqr(rhobar) + sqr(input->damp_val) );
      
      psi = sn1 * phibar;
      phibar = cs1 * phibar;
/*      
*     Use a plane rotation to eliminate the subdiagonal element (BETA)
*     of the lower-bidiagonal matrix, giving an upper-bidiagonal matrix.
*/
      rho = sqrt( sqr(rhobar) + sqr(input->damp_val) + sqr(beta) );
      cs = sqrt( sqr(rhobar) + sqr(input->damp_val) ) / rho;
      sn = beta / rho;

      theta = sn * alpha;
      rhobar = -cs * alpha;
      phi = cs * phibar;
      phibar = sn * phibar;
      tau = sn * phi;
/*
*     Update the solution vector x, the search direction vector w, and the 
*     standard error estimates vector se.
*/     
      for(indx = 0; indx < input->num_cols; indx++)
	{
	    /* update the solution vector x */
	  output->sol_vec->data()[indx] += (phi / rho) * 
	    work->srch_dir_vec.data()[indx];

	    /* update the standard error estimates vector se */
	  //output->std_err_vec.data()[indx] += sqr( (1.0 / rho) * 
	//    work->srch_dir_vec.data()[indx] );

	    /* accumulate this quantity to estimate condition number of A 
*/
	  ddnorm += sqr( (1.0 / rho) * work->srch_dir_vec.data()[indx] );

	    /* update the search direction vector w */
	  work->srch_dir_vec.data()[indx] = 
          work->bidiag_wrk_vec.data()[indx] -
	    (theta / rho) * work->srch_dir_vec.data()[indx];
	}
/*
*     Use a plane rotation on the right to eliminate the super-diagonal element
*     (THETA) of the upper-bidiagonal matrix.  Then use the result to estimate 
*     the solution norm || x ||.
*/
      delta = sn2 * rho;
      gammabar = -cs2 * rho;
      zetabar = (phi - delta * zeta) / gammabar;

        /* compute an estimate of the solution norm || x || */
      output->sol_norm = sqrt( xxnorm + sqr(zetabar) );

      gamma = sqrt( sqr(gammabar) + sqr(theta) );
      cs2 = gammabar / gamma;
      sn2 = theta / gamma;
      zeta = (phi - delta * zeta) / gamma;

        /* accumulate this quantity to estimate solution norm || x || */
      xxnorm += sqr(zeta);
/*
*     Estimate the Frobenius norm and condition of the matrix A, and the 
*     Euclidean norms of the vectors r and A^T*r.
*/
      output->frob_mat_norm = sqrt( bbnorm );
      output->mat_cond_num = output->frob_mat_norm * sqrt( ddnorm );

      res += sqr(psi);
      output->resid_norm = sqrt( sqr(phibar) + res );

      output->mat_resid_norm = alpha * fabs( tau );
/*
*     Use these norms to estimate the values of the three stopping criteria.
*/
      stop_crit_1 = output->resid_norm / bnorm;

      stop_crit_2 = 0.0;
      if( output->resid_norm > 0.0 )
	stop_crit_2 = output->mat_resid_norm / ( output->frob_mat_norm * 
	  output->resid_norm );

      stop_crit_3 = 1.0 / output->mat_cond_num;

      resid_tol = input->rel_rhs_err + input->rel_mat_err * 
      output->mat_resid_norm * 
	output->sol_norm / bnorm;

      resid_tol_mach = DBL_EPSILON + DBL_EPSILON * output->mat_resid_norm * 
	output->sol_norm / bnorm;
/*
*     Check to see if any of the stopping criteria are satisfied.
*     First compare the computed criteria to the machine precision.
*     Second compare the computed criteria to the the user specified precision.
*/
        /* iteration limit reached */
      if( output->num_iters >= input->max_iter )
	output->term_flag = 7;

        /* condition number greater than machine precision */
      if( stop_crit_3 <= DBL_EPSILON )
	output->term_flag = 6;
        /* least squares error less than machine precision */
      if( stop_crit_2 <= DBL_EPSILON )
	output->term_flag = 5;
        /* residual less than a function of machine precision */
      if( stop_crit_1 <= resid_tol_mach )
	output->term_flag = 4;

        /* condition number greater than CONLIM */
      if( stop_crit_3 <= cond_tol )
	output->term_flag = 3;
        /* least squares error less than ATOL */
      if( stop_crit_2 <= input->rel_mat_err )
	output->term_flag = 2;
        /* residual less than a function of ATOL and BTOL */
      if( stop_crit_1 <= resid_tol )
	output->term_flag = 1;
/*
*  If statistics are printed at each iteration, print a header and the initial
*  values for each quantity.
*/
    //  if( input->lsqr_fp_out != NULL )
	/*
	{
	  //fprintf( input->lsqr_fp_out,
	  
	  Log(
          "%6i %13.5e %10.2e \t%10.2e \t%10.2e %10.2e\n",
		   output->num_iters, output->resid_norm, stop_crit_1, 
                   stop_crit_2,
		   output->frob_mat_norm, output->mat_cond_num);
	}
	*/
/*
*     The convergence criteria are required to be met on NCONV consecutive 
*     iterations, where NCONV is set below.  Suggested values are 1, 2, or 3.
*/
      if( output->term_flag == 0 )
	term_iter = -1;

      term_iter_max = 1;
      term_iter++;

      if( (term_iter < term_iter_max) &&
          (output->num_iters < input->max_iter) )
	output->term_flag = 0;
    } /* end while loop */

	
/*
*  Finish computing the standard error estimates vector se.
*/
	/*
  temp = 1.0;

  if( input->num_rows > input->num_cols )
    temp = ( double ) ( input->num_rows - input->num_cols );

  if( sqr(input->damp_val) > 0.0 )
    temp = ( double ) ( input->num_rows );
  
  temp = output->resid_norm / sqrt( temp );
  
  for(indx = 0; indx < input->num_cols; indx++)
	{
      // update the standard error estimates vector se 
    output->std_err_vec.data()[indx] = temp * sqrt( output->std_err_vec.data()[indx] );
	}
	*/

/*
*  If statistics are printed at each iteration, print the statistics for the
*  stopping condition.
*/
  //if( input->lsqr_fp_out != NULL )
    {
		#ifdef DO_LOGGING
	  Log(
      //fprintf( input->lsqr_fp_out, 
	  "\n\tISTOP = %3i\t\t\tITER = %9i\n"
		"|| A ||_F = %13.5e\tcond( A ) = %13.5e\n"
		"|| r ||_2 = %13.5e\t|| A^T r ||_2 = %13.5e\n"
		"|| b ||_2 = %13.5e\t|| x - x0 ||_2 = %13.5e\n\n", 
			output->term_flag, output->num_iters, output->frob_mat_norm, 
			output->mat_cond_num, output->resid_norm, output->mat_resid_norm,
			bnorm, output->sol_norm );

      //fprintf( input->lsqr_fp_out, "
	  
	  Log(  "%s\n", term_msg[output->term_flag]);
		#endif //DO_LOGGING
      
    }

  return;
}

/*-------------------------------------------------------------------------*/
/*                                                                         */
/*  Define the function 'dvec_norm2()'.  This function takes a vector      */
/*  arguement and computes the Euclidean or L2 norm of this vector.  Note  */
/*  that this is a version of the BLAS function 'dnrm2()' rewritten to     */
/*  use the current data structures.                                       */
/*                                                                         */
/*-------------------------------------------------------------------------*/

double dvec_norm2(VectorDouble *vec)
{
  long   indx;
  double norm;
  
  norm = 0.0;

	int n = vec->size();
  for(indx = 0; indx < n; indx++)
    norm += sqr(vec->at(indx));

  return sqrt(norm);
}


/*-------------------------------------------------------------------------*/
/*                                                                         */
/*  Define the function 'dvec_scale()'.  This function takes a vector      */
/*  and a scalar as arguments and multiplies each element of the vector    */
/*  by the scalar.  Note  that this is a version of the BLAS function      */
/*  'dscal()' rewritten to use the current data structures.                */
/*                                                                         */
/*-------------------------------------------------------------------------*/

void dvec_scale(double scal, VectorDouble *vec)
{
  long   indx;

  for(indx = 0; indx < vec->size(); indx++)
    vec->data()[indx] *= scal;

  return;
}

/*-------------------------------------------------------------------------*/
/*                                                                         */
/*  Define the function 'dvec_copy()'.  This function takes two vectors    */
/*  as arguements and copies the contents of the first into the second.    */
/*  Note  that this is a version of the BLAS function 'dcopy()' rewritten  */
/*  to use the current data structures.                                    */
/*                                                                         */
/*-------------------------------------------------------------------------*/

void dvec_copy(VectorDouble *orig, VectorDouble *copy)
{
  long   indx;

  for(indx = 0; indx < orig->size(); indx++)
    copy->data()[indx] = orig->data()[indx];

  return;
}


// y = m * x + b
double solve_simple_linear(const cb::vector<double> & y,const cb::vector<double> & x, double * pM, double * pB)
{
	const int N = x.size();

	ASSERT_RELEASE( x.size() == y.size() );
	ASSERT_RELEASE( N > 0 );
	
	/*
	// N == 1 will be handled fine later
	if ( N == 0 )
	{
		*pM = 0;
		*pB = 0;
		return 0.0;
	}
	else if ( N == 1 )
	{
		*pM = 0;
		*pB = y[0];
		return 0.0;
	}
	*/
	
	double _Y = 0, _X = 0, _XY = 0, _XX = 0;

	for(int i=0;i<N;i++)
	{
		_Y += y[i];
		_X += x[i];
		_XY += x[i] * y[i];
		_XX += x[i] * x[i];
	}
	double scale = 1.0/N;
	_Y *= scale;
	_X *= scale;
	_XY *= scale;
	_XX *= scale;

	double Xsdev = _XX - _X*_X;
	ASSERT( Xsdev >= 0.0 );
	
	if ( Xsdev < 1e-16F ) // @@ FLT_MIN is pretty arbitrary and tiny ??? or FLT_EPSILON ?? or 0.0 ???
	{
		// all X's are the same
		*pM = 0.0;
		*pB = _Y;
		return 0.0;
	}
		
	double M = ( _XY - _Y*_X )/Xsdev;

	*pM = M;
	
	double B = _Y - M * _X;

	*pB = B;

	double sqrErr = 0.0;
	
	for(int i=0;i<N;i++)
	{
		double yt = M * x[i] + B;
		double err = y[i] - yt;
		sqrErr +=  err*err;
	}
	
	return sqrErr;
}

// returns sqrError
double solve_lsqr_std(const cb::vector<double> & rhs,SimpleMatrixBase * matrix,t_mat_vec_product mvp,VectorDouble * solution,double damping)
{
	int lsqr_rows = matrix->GetNumRows();
	int lsqr_columns = matrix->GetNumColumns();

	ASSERT_RELEASE( rhs.size() == lsqr_rows );
	ASSERT_RELEASE( solution->size() == lsqr_columns );

	LSQR_INPUTS lsqr_inputs;

	lsqr_inputs.num_rows = lsqr_rows;
	lsqr_inputs.num_cols = lsqr_columns;
	lsqr_inputs.damp_val = damping;
	lsqr_inputs.rel_mat_err = FLT_EPSILON;
	lsqr_inputs.rel_rhs_err = FLT_EPSILON;
	lsqr_inputs.cond_lim = 0.0;
	lsqr_inputs.max_iter = lsqr_columns*4 + 16;
	//lsqr_inputs.lsqr_fp_out = NULL;
	//lsqr_inputs.lsqr_fp_out = stdout;
	
	// have to copy "rhs" cuz LSQR mutates it
	VectorDouble lsqr_rhs(rhs);

	lsqr_inputs.rhs_vec = &lsqr_rhs;
	lsqr_inputs.sol_vec = solution;

	LSQR_FUNC lsqr_func;
	lsqr_func.mat_vec_prod = mvp;

	LSQR_OUTPUTS lsqr_outputs;
	//memset(&lsqr_outputs,0,sizeof(lsqr_outputs));
	//lsqr_outputs.std_err_vec.resize(lsqr_columns);
	lsqr_outputs.sol_vec = lsqr_inputs.sol_vec;

	// lsqr will throw a gException if it fails
	try
	{
		lsqr(&lsqr_inputs,&lsqr_outputs,&lsqr_func,(void *)matrix);
	}
	catch( ... )
	{
		cb::lprintf("lsqr failed !?!?!?\n");
		return FLT_MAX;
	}

	if ( lsqr_outputs.term_flag == 3 || lsqr_outputs.term_flag == 6 )
	{
		cb::lprintf("LSQR ** BAD FAILURE **\n");
		return FLT_MAX; // and bail out
	}
	else if ( lsqr_outputs.term_flag == 7 )
	{
		cb::lprintf("LSQR ** HIT MAX ITERATIONS **\n");
	}
	else
	{
		//Log("LSQR ** SUCCESS ! **\n");
	}

	/*
	Log("LSQR result : %d , condition : %1.3g , residual : %1.3g, mat residual : %1.3g\n",
					lsqr_outputs.term_flag,
					lsqr_outputs.mat_cond_num,
					lsqr_outputs.resid_norm,
					lsqr_outputs.mat_resid_norm);
	*/

	//double lsqr_sqrError = 0;	
	return cb::fsquare(lsqr_outputs.resid_norm);
}

}
		
END_CB

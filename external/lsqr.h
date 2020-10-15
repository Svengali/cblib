/*
* lsqr.h
* Contains auxiliary functions, data type definitions, and function 
* prototypes for the iterative linear solver LSQR. 
*
* 08 Sep 1999: First version from James W. Howse <jhowse@lanl.gov>
*/

/*

cbloom modification : these function throw a gException on error

NOTE : 

weighted LSQR can be done just by multiplying SQRT(W) onto both A and b before solving

*/

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
*                    on the machine being used.  For example, on the IBM 370,
*                    'rel_prec' is about 1.0E-6 and 1.0D-16 in single and double
*                    precision respectively.
*
*     LSQR  minimizes the function 'rnorm' with respect to 'x'.
*
*------------------------------------------------------------------------------
*/

/*---------------*/
/* Include files */
/*---------------*/

#include "../Base.h"
#include "../array.h"
#include "../vector.h"
#include <stdio.h>

START_CB

// wrap external code in "lsqr" namespace
namespace lsqr
{

/*------------------*/
/* Type definitions */
/*------------------*/

typedef vector<double> VectorDouble;

class lsqrMatrixBase
{
public:
	virtual int GetNumRows() const = 0;
	virtual int GetNumColumns() const = 0;
	//virtual void SetNumRows(int num) = 0;
	// GetPtr() returns an array that's just like a rows x cols array
	//virtual float * GetPtr() = 0;
	//virtual const float * GetPtr() const = 0;
	
	//float * GetRowPtr(int row) { return GetPtr() + GetNumColumns() * row; }
	//const float * GetRowPtr(int row) const { return GetPtr() + GetNumColumns() * row; }
};

template <int t_columns> class SimpleMatrix : public lsqrMatrixBase
{
public:
	SimpleMatrix() { }
	~SimpleMatrix() { }

	virtual int GetNumRows() const { return matrix.size(); }
	virtual int GetNumColumns() const { return t_columns; }

	//virtual 
	void SetNumRows(int num) { matrix.resize(num); }
		
	// GetPtr() returns an array that's just like a rows x cols array
	float * GetPtr() { return &matrix[0][0]; }
	const float * GetPtr() const { return &matrix[0][0]; }
	
	typedef cb::array<float,t_columns> t_rowType;
	
	t_rowType & Row(int n) { return matrix[n]; }
	const t_rowType & GetRow(int n) const { return matrix[n]; }
	
	cb::vector< t_rowType > matrix; //matrix[row][col]
};

class SparseMatrix : public lsqrMatrixBase
{
public:
	SparseMatrix(int rows,int cols) : m_cols(cols) { matrix.resize(rows); }
	~SparseMatrix() { }

	virtual int GetNumRows() const { return matrix.size(); }
	virtual int GetNumColumns() const { return m_cols; }
	
	struct SparseEntry
	{
		int col;
		float val;
		
		SparseEntry() { }
		SparseEntry(int i,float f) : col(i), val(f) { }
	};
	
	int m_cols;
	typedef cb::vector< SparseEntry > t_row;
	cb::vector< t_row > matrix; //matrix[row][col]
};

// (void *) data should be a SimpleMatrix of t_columns
template <int t_columns>
void SimpleMatrixLsqrMatVecProd(long mode, VectorDouble * x, VectorDouble * y, void * _data);

void SparseMatrixLsqrMatVecProd(long mode, VectorDouble * x, VectorDouble * y, void * _data);

// NormalizeColumn puts the column in [-1,1] with an avg of 0
template <int t_columns>
void NormalizeColumn(SimpleMatrix<t_columns> & mat,int col,bool log);

template <int t_columns>
void GetColumn(const SimpleMatrix<t_columns> & mat,int col,cb::vector<double> * pvec);

typedef void (*t_mat_vec_product) (long, VectorDouble *, VectorDouble *, void *);

// solve_lsqr_std returns sqrError
double solve_lsqr_std(const cb::vector<double> & rhs,lsqrMatrixBase * matrix,t_mat_vec_product mvp,VectorDouble * solution,double damping = 0.0);

// y = m * x + b
double solve_simple_linear(const cb::vector<double> & y,const cb::vector<double> & x, double * pM, double * pB);

/*
*------------------------------------------------------------------------------
*
*     Input Quantities
*     ----------------
*
*     num_rows     input  The number of rows (e.g., 'm') in the matrix A.
*
*     num_cols     input  The number of columns (e.g., 'n') in the matrix A.
*
*     damp_val     input  The damping parameter for problem 3 above.
*                         ('damp_val' should be 0.0 for problems 1 and 2.)
*                         If the system A*x = b is incompatible, values
*                         of 'damp_val' in the range
*                            0 to sqrt('rel_prec')*norm(A)
*                         will probably have a negligible effect.
*                         Larger values of 'damp_val' will tend to decrease
*                         the norm of x and reduce the number of
*                         iterations required by LSQR.
*
*                         The work per iteration and the storage needed
*                         by LSQR are the same for all values of 'damp_val'.
*
*     rel_mat_err  input  An estimate of the relative error in the data
*                         defining the matrix 'A'.  For example,
*                         if 'A' is accurate to about 6 digits, set
*                         rel_mat_err = 1.0e-6 .
*
*     rel_rhs_err  input  An extimate of the relative error in the data
*                         defining the right hand side (rhs) vector 'b'.  For 
*                         example, if 'b' is accurate to about 6 digits, set
*                         rel_rhs_err = 1.0e-6 .
*
*     cond_lim     input  An upper limit on cond('Abar'), the apparent
*                         condition number of the matrix 'Abar'.
*                         Iterations will be terminated if a computed
*                         estimate of cond('Abar') exceeds 'cond_lim'.
*                         This is intended to prevent certain small or
*                         zero singular values of 'A' or 'Abar' from
*                         coming into effect and causing unwanted growth
*                         in the computed solution.
*
*                         'cond_lim' and 'damp_val' may be used separately or
*                         together to regularize ill-conditioned systems.
*
*                         Normally, 'cond_lim' should be in the range
*                         1000 to 1/rel_prec.
*                         Suggested value:
*                         cond_lim = 1/(100*rel_prec)  for compatible systems,
*                         cond_lim = 1/(10*sqrt(rel_prec)) for least squares.
*
*             Note:  If the user is not concerned about the parameters
*             'rel_mat_err', 'rel_rhs_err' and 'cond_lim', any or all of them 
*             may be set to zero.  The effect will be the same as the values
*             'rel_prec', 'rel_prec' and 1/rel_prec respectively.
*
*     max_iter     input  An upper limit on the number of iterations.
*                         Suggested value:
*                         max_iter = n/2   for well-conditioned systems
*                                          with clustered singular values,
*                         max_iter = 4*n   otherwise.
*
*     lsqr_fp_out  input  Pointer to the file for sending printed output.  If  
*                         not NULL, a summary will be printed to the file that 
*                         'lsqr_fp_out' points to.
*
*     rhs_vec      input  The right hand side (rhs) vector 'b'.  This vector
*                         has a length of 'm' (i.e., 'num_rows').  The routine 
*                         LSQR is designed to over-write 'rhs_vec'.
*
*     sol_vec      input  The initial guess for the solution vector 'x'.  This 
*                         vector has a length of 'n' (i.e., 'num_cols').  The  
*                         routine LSQR is designed to over-write 'sol_vec'.
*
*------------------------------------------------------------------------------
*/

typedef struct LSQR_INPUTS {
  long     num_rows;
  long     num_cols;
  double   damp_val;
  double   rel_mat_err;
  double   rel_rhs_err;
  double   cond_lim;
  long     max_iter;
  //FILE     *lsqr_fp_out;
  VectorDouble     *rhs_vec;
  VectorDouble     *sol_vec;
  
  LSQR_INPUTS()
  {
	ZERO_PTR(this);
  }
  
} lsqr_input;

/*
*------------------------------------------------------------------------------
*
*     Output Quantities
*     -----------------
*
*     term_flag       output  An integer giving the reason for termination:
*
*                     0       x = x0  is the exact solution.
*                             No iterations were performed.
*
*                     1       The equations A*x = b are probably compatible.  
*                             Norm(A*x - b) is sufficiently small, given the 
*                             values of 'rel_mat_err' and 'rel_rhs_err'.
*
*                     2       The system A*x = b is probably not
*                             compatible.  A least-squares solution has
*                             been obtained that is sufficiently accurate,
*                             given the value of 'rel_mat_err'.
*
*                     3       An estimate of cond('Abar') has exceeded
*                             'cond_lim'.  The system A*x = b appears to be
*                             ill-conditioned.  Otherwise, there could be an
*                             error in subroutine APROD.
*
*                     4       The equations A*x = b are probably
*                             compatible.  Norm(A*x - b) is as small as
*                             seems reasonable on this machine.
*
*                     5       The system A*x = b is probably not
*                             compatible.  A least-squares solution has
*                             been obtained that is as accurate as seems
*                             reasonable on this machine.
*
*                     6       Cond('Abar') seems to be so large that there is
*                             no point in doing further iterations,
*                             given the precision of this machine.
*                             There could be an error in subroutine APROD.
*
*                     7       The iteration limit 'max_iter' was reached.
*  
*     num_iters       output  The number of iterations performed.
*
*     frob_mat_norm   output  An estimate of the Frobenius norm of 'Abar'.
*                             This is the square-root of the sum of squares
*                             of the elements of 'Abar'.
*                             If 'damp_val' is small and if the columns of 'A'
*                             have all been scaled to have length 1.0,
*                             'frob_mat_norm' should increase to roughly
*                             sqrt('n').
*                             A radically different value for 'frob_mat_norm' 
*                             may indicate an error in subroutine APROD (there
*                             may be an inconsistency between modes 1 and 2).
*
*     mat_cond_num    output  An estimate of cond('Abar'), the condition
*                             number of 'Abar'.  A very high value of 
*                             'mat_cond_num'
*                             may again indicate an error in APROD.
*
*     resid_norm      output  An estimate of the final value of norm('rbar'),
*                             the function being minimized (see notation
*                             above).  This will be small if A*x = b has
*                             a solution.
*
*     mat_resid_norm  output  An estimate of the final value of
*                             norm( Abar(transpose)*rbar ), the norm of
*                             the residual for the usual normal equations.
*                             This should be small in all cases.  
*                             ('mat_resid_norm'
*                             will often be smaller than the true value
*                             computed from the output vector 'x'.)
*
*     sol_norm        output  An estimate of the norm of the final
*                             solution vector 'x'.
*
*     sol_vec         output  The vector which returns the computed solution 
*                             'x'.
*                             This vector has a length of 'n' (i.e., 
*                             'num_cols').
*
*     std_err_vec     output  The vector which returns the standard error 
*                             estimates  for the components of 'x'.
*                             This vector has a length of 'n'
*                             (i.e., 'num_cols').  For each i, std_err_vec(i) 
*                             is set to the value
*                             'resid_norm' * sqrt( sigma(i,i) / T ),
*                             where sigma(i,i) is an estimate of the i-th
*                             diagonal of the inverse of Abar(transpose)*Abar
*                             and  T = 1      if  m <= n,
*                                  T = m - n  if  m > n  and  damp_val = 0,
*                                  T = m      if  damp_val != 0.
*
*------------------------------------------------------------------------------
*/

typedef struct LSQR_OUTPUTS 
{
  long     term_flag;
  long     num_iters;
  double   frob_mat_norm;
  double   mat_cond_num;
  double   resid_norm;
  double   mat_resid_norm;
  double   sol_norm;
  VectorDouble *sol_vec;
  //VectorDouble std_err_vec;
  
  LSQR_OUTPUTS()
  {
	ZERO_PTR(this);
  }
} lsqr_output;

/*
*------------------------------------------------------------------------------
*
*     Functions Called
*     ----------------
*
*     mat_vec_prod       functions  A pointer to a function that performs the 
*                                   matrix-vector multiplication.  The function
*                                   has the calling parameters:
*
*                                       APROD ( mode, x, y, prod_data ),
*
*                                   and it must perform the following functions:
*
*                                       If MODE = 0, compute  y = y + A*x,
*                                       If MODE = 1, compute  x = x + A^T*y.
*
*                                   The vectors x and y are input parameters in 
*                                   both cases.
*                                   If mode = 0, y should be altered without
*                                   changing x.
*                                   If mode = 2, x should be altered without
*                                   changing y.
*                                   The argument prod_data is a pointer to a
*                                   user-defined structure that contains
*                                   any data need by the function APROD that is
*                                   not used by LSQR.  If no additional data is 
*                                   needed by APROD, then prod_data should be
*                                   the NULL pointer.
*------------------------------------------------------------------------------
*/

typedef struct LSQR_FUNC {
  void     (*mat_vec_prod) (long, VectorDouble *, VectorDouble *, void *);
} lsqr_func;

/*---------------------*/
/* Function prototypes */
/*---------------------*/

void lsqr_error( char *, int );

void lsqr( lsqr_input *, lsqr_output *, lsqr_func *, void * );

double dvec_norm2( VectorDouble * );
void dvec_scale( double, VectorDouble * );
void dvec_copy( VectorDouble *, VectorDouble * );


//===========================================================================

template <int t_columns>
void SimpleMatrixLsqrMatVecProd(long mode, VectorDouble * x, VectorDouble * y, void * _data)
{
	const SimpleMatrix<t_columns> * matrix = (const SimpleMatrix<t_columns> *) _data;
	ASSERT( mode == 0 || mode == 1 );
	
	//  If MODE = 0, compute  y = y + A*x,
	//  If MODE = 1, compute  x = x + A^T*y.

	//int columns = x->length;
	int rows = y->size();
	ASSERT( x->size() == t_columns );
		
	if ( mode == 0 )
	{
		// y += A * x
		for(int r=0;r<rows;r++)
		{
			for(int c=0;c<t_columns;c++)
			{
				y->at(r) += matrix->matrix[r][c] * x->at(c);
			}		
		}
	}
	else
	{
		// x += X^T * y
		
		for(int c=0;c<t_columns;c++)
		{
			for(int r=0;r<rows;r++)
			{
				x->at(c) += matrix->matrix[r][c] * y->at(r);
			}
		}
	}
}

void SparseMatrixLsqrMatVecProd(long mode, VectorDouble * x, VectorDouble * y, void * _data);


// NormalizeColumn puts the column in [-1,1] with an avg of 0
template <int t_columns>
void NormalizeColumn(SimpleMatrix<t_columns> & mat,int col,bool log)
{
	int rows = mat.GetNumRows();
	if ( rows < 2 )
		return;
		
	double lo = mat.matrix[0][col];
	double hi = mat.matrix[0][col];
	double avg = 0;
	
	for(int r=0;r<rows;r++)
	{
		double val = mat.matrix[r][col];
		lo = MIN(lo,val);
		hi = MAX(hi,val);
		avg += val;
	}
	
	avg /= rows;
	
	if ( log )
	{
		lprintf("Column %d ; avg = %f, lo = %f, hi = %f\n",col,avg,lo,hi);
	}
	
	lo -= avg;
	hi -= avg;
	ASSERT( lo <= 0 && hi >= 0 );
	double extreme = MAX(-lo,hi);
	double scale;
	if ( extreme == 0 )
		scale = 1;
	else
		scale = 1/extreme;
	
	for(int r=0;r<rows;r++)
	{
		float & val = mat.matrix[r][col];
		val -= avg;
		val *= scale;
		ASSERT( val >= -1.00001 && val <= 1.00001 );
	}
}

template <int t_columns>
void GetColumn(const SimpleMatrix<t_columns> & mat,int col,cb::vector<double> * pvec)
{
	int rows = mat.GetNumRows();
	pvec->clear();
	pvec->resize(rows);
	for(int r=0;r<rows;r++)
	{
		pvec->at(r) = mat.matrix[r][col];
	}
}


//=======================

// solve_lsqr_exact :
//	solve A x = B
//	 (A^T A) x = (A^T B)
//	 x = (A^T A)^-1 * (A^T B)

template <int terms>
void solve_lsqr_exact( const VectorDouble & B, const SimpleMatrix<terms> * A, VectorDouble * solution)
{
	// just make A^T * A :
	
	MatN<terms> AT_A;
	
	int rows = B.size();
	ASSERT( rows == A->matrix.size() );
	
	for(int r=0;r<terms;r++)
	{
		// AT_A is symmetric, so we could skip half of these -
		for(int c=0;c<=r;c++)
		{
			double sum = 0;
			for(int t=0;t<rows;t++)
			{
				sum += A->matrix[t][r] * A->matrix[t][c];
			}
			AT_A.Element(r,c) = (float) sum;
		}		
	}
	for(int r=0;r<terms;r++)
	{
		for(int c=r+1;c<terms;c++)
		{
			AT_A.Element(r,c) = AT_A.Element(c,r);
		}
	}
	
	// make A^T * B :
	
	VecN<terms> AT_B;
		
	for(int c=0;c<terms;c++)
	{
		double sum = 0;
		for(int t=0;t<rows;t++)
		{
			sum += A->matrix[t][c] * B[t];
		}
		AT_B[c] = (float) sum;
	}	
	
	// find (A^T A)^-1 :
	
	MatN<terms> AT_A_Inverse;

	AT_A.GetInverse(&AT_A_Inverse);
	
	//	 x = (A^T A)^-1 * (A^T B)
	
	VecN<terms> solve;
	AT_A_Inverse.TransformVector(&solve,AT_B);
	
	for(int t=0;t<terms;t++)
	{
		(*solution)[t] = solve[t];
	}
}

//-----------------------
} // namespace lsqr

//===========================================================================

END_CB

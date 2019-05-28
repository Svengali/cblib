#pragma once

// MatNM template NxM matrix
//	acts on a VecN generic vector

#include "cblib/Base.h"
#include "cblib/VecN.h"
#include <string.h> // for memset

//}{=======================================================================

START_CB
					   
template <int t_rows,int t_cols> class MatNM
{
public:
	enum { c_rows = t_rows };
	enum { c_cols = t_cols };

	typedef MatNM<t_rows,t_cols> this_type;

	//-----------------------------------------

	MatNM()
	{
		SetZero();
	}
	
	~MatNM() { }

	//-----------------------------------------
	// simple Set's

	void SetZero()
	{
		memset(m_data,0,sizeof(float)*t_rows*t_cols);
	}

	void SetIdentity()
	{
		// Identity matrix of non-square is not well-defined
		SetZero();
		const int nMin = MIN(t_rows,t_cols);
		for(int i=0;i<nMin;i++)
		{
			m_data[i][i] = 1.f;
		}
	}

	// += matrix
	void Set(const this_type & m)
	{
		for(int r=0;r<t_rows;r++)
			for(int c=0;c<t_cols;c++)
				m_data[r][c] = m.m_data[r][c];
	}
	
	//-----------------------------------------

	float & Element(const int row,const int col)
	{
		ASSERT( row >= 0 && row < t_rows );
		ASSERT( col >= 0 && col < t_cols );
		return m_data[row][col];
	}
	float GetElement(const int row,const int col) const
	{
		ASSERT( row >= 0 && row < t_rows );
		ASSERT( col >= 0 && col < t_cols );
		return m_data[row][col];
	}
	VecN<t_cols> & operator [] (const int row)
	{
		return *( (VecN<t_cols> *) m_data[row] );
	}
	const VecN<t_cols> & operator [](const int row) const
	{
		return *(( const VecN<t_cols> *) m_data[row] );
	}

	// get a row as a VecN :
	//	(use a nasty cast:)
	const VecN<t_cols> & GetRow(const int i) const
	{
		ASSERT( i >= 0 && i < t_rows );
		return *((const VecN<t_cols> *)m_data[i]);
	}
	VecN<t_cols> & MutableRow(const int i)
	{
		ASSERT( i >= 0 && i < t_rows );
		return *((VecN<t_cols> *)m_data[i]);
	}

	void GetColumn(const int c,VecN<t_rows> * pColumn) const
	{
		ASSERT( c >= 0 && c < t_cols );
		for(int r=0;r<t_rows;r++)
		{
			(*pColumn)[r] = m_data[r][c];
		}
	}
	
	void SetRow(const int r,const VecN<t_cols> & row)
	{
		ASSERT( r >= 0 && r < t_rows );
		for(int c=0;c<t_cols;c++)
		{
			m_data[r][c] = row[c];
		}
	}
	void SetColumn(const int c,const VecN<t_rows> & column)
	{
		ASSERT( c >= 0 && c < t_cols );
		for(int r=0;r<t_rows;r++)
		{
			m_data[r][c] = column[r];
		}
	}

	//-----------------------------------------
	// simple math operators on matrices

	// *= scalar
	void Scale(const float scale) // scale
	{
		for(int r=0;r<t_rows;r++)
			for(int c=0;c<t_cols;c++)
				m_data[r][c] *= scale;
	}

	// += matrix
	void Add(const this_type & m)
	{
		for(int r=0;r<t_rows;r++)
			for(int c=0;c<t_cols;c++)
				m_data[r][c] += m.m_data[r][c];
	}

	// += matrix * scale
	void AddScaled(const this_type & m, const float scale)
	{
		for(int r=0;r<t_rows;r++)
			for(int c=0;c<t_cols;c++)
				m_data[r][c] += m.m_data[r][c] * scale;
	}

	// -= matrix
	void Subtract(const this_type & m)
	{
		for(int r=0;r<t_rows;r++)
			for(int c=0;c<t_cols;c++)
				m_data[r][c] -= m.m_data[r][c];
	}

	//-----------------------------------------

	// to = matrix * from
	void TransformVector(VecN<t_rows> * pTo,const VecN<t_cols> & from) const
	{
		for(int r=0;r<t_rows;r++)
		{
			(*pTo)[r] = (float)( GetRow(r) * from );
		}
	}

	// float = rowVec * Matrix * colVec
	double InnerProduct(const VecN<t_rows> & left,const VecN<t_cols> & right) const
	{
		double sum = 0.0;
		for(int r=0;r<t_rows;r++)
		{
			for(int c=0;c<t_cols;c++)
			{
				sum += m_data[r][c] * left[r] * right[c];
			}
		}
		return sum;
	}

	//-----------------------------------------

	// matrix = rowVec /\ colVec
	void SetOuterProduct(const VecN<t_rows> & rowVec,const VecN<t_cols> & colVec)
	{
		for(int r=0;r<t_rows;r++)
			for(int c=0;c<t_cols;c++)
				m_data[r][c] = rowVec[r] * colVec[c];
	}

	// matrix += rowVec /\ colVec
	void AddOuterProduct(const VecN<t_rows> & rowVec,const VecN<t_cols> & colVec,const float scale = 1.f)
	{
		for(int r=0;r<t_rows;r++)
			for(int c=0;c<t_cols;c++)
				m_data[r][c] += rowVec[r] * colVec[c] * scale;
	}

	// matrix = left * right
	template <int t_mid> void SetProduct(const MatNM<t_rows,t_mid> & left,
										 const MatNM<t_mid,t_cols> & right)
	{
		for(int r=0;r<t_rows;r++)
		{
			for(int c=0;c<t_cols;c++)
			{
				double accum = 0.0;

				for(int t=0;t<t_mid;t++)
				{
					accum += left.m_data[r][t] * right.m_data[t][c];
				}
				
				m_data[r][c] = (float)accum;
			}
		}
	}

	//-----------------------------------------

	template <int t_rows_new,int t_cols_new> void Reinterpret(const MatNM<t_rows_new,t_cols_new> * pNew) const
	{
		pNew->SetZero();
		const int r_min = MIN(t_rows,t_rows_new);
		const int c_min = MIN(t_cols,t_cols_new);
		for(int r=0;r<r_min;r++)
		{
			for(int c=0;c<c_min;c++)
			{
				pNew->Element(r,c) = m_data[r][c];
			}
		}
	}

	void GetTranspose(MatNM<t_cols,t_rows> * pNew) const
	{
		for(int r=0;r<t_rows;r++)
		{
			for(int c=0;c<t_cols;c++)
			{
				pNew->Element(c,r) = m_data[r][c];
			}
		}
	}
	
	//-----------------------------------------

//protected:
	// nah, just public it

	float m_data[t_rows][t_cols];
};
		   
template <int t_size> class MatN : public MatNM<t_size,t_size>
{
public:

	//-----------------------------------------
	
	double Determinant() const;
	
	void GetInverse(MatN<t_size> * pNew) const;

	void GetInverseOneRow(int r,VecN<t_size> * pNew) const;
	
	template <int t_rows_new>
	void GetCofactor(MatN<t_rows_new> * pNew,int skip_r,int skip_c) const
	{
		for(int r=0;r<t_rows_new;r++)
		{
			int from_r = r;
			if ( from_r >= skip_r ) from_r ++;
			for(int c=0;c<t_rows_new;c++)
			{
				int from_c = c;
				if ( from_c >= skip_c ) from_c ++;
				pNew->Element(r,c) = m_data[from_r][from_c];
			}
		}
	}
	
};

//}{=======================================================================

/**
	Determinant is O(N^3)
**/
template <int t_size>
inline double MatN<t_size>::Determinant() const
{
	double det = 0;
	
	for(int c=0;c<t_size;c++)
	{
		MatN<t_size-1> cofactor;
			
		GetCofactor(&cofactor,0,c);
		double subdet = GetElement(0,c) * cofactor.Determinant();
			
		det += ( c & 1 ) ? -subdet : subdet;
	}

	return det;
}
	
template < >
inline double MatN<1>::Determinant() const
{
	return m_data[0][0];
}

template < >
inline double MatN<2>::Determinant() const
{
	return m_data[0][0] * m_data[1][1] - m_data[0][1] * m_data[1][0];
}

// could certainly special case Mat<3> and Mat<4> for speed, but whatever

/**

	just do the dumb Determinant way of making the inverse
	this can use a lot of stack for large N
	
	this is O(N^4) !!
	LU decomposition for example is N^3
**/
template <int t_size>
inline void MatN<t_size>::GetInverse(MatN<t_size> * pNew) const
{
	double det = Determinant();
	if ( det == 0.0 )
		return;
	
	for(int r=0;r<t_size;r++)
	{
		for(int c=0;c<t_size;c++)
		{
			MatN<t_size-1> cofactor;
				
			GetCofactor(&cofactor,r,c);
			double subdet = cofactor.Determinant();

			double sign = ( (r+c) & 1 ) ? -1 : 1;
			
			// writing output transposed :
			pNew->Element(c,r) = (float) (sign * subdet / det);
		}
	}
}

template <int t_size>
inline void MatN<t_size>::GetInverseOneRow(int c,VecN<t_size> * pNew) const
{
	double det = Determinant();
	if ( det == 0.0 )
		return;
	
	ASSERT( c >= 0 && c < t_size );
	
	for(int r=0;r<t_size;r++)
	{
		MatN<t_size-1> cofactor;
			
		GetCofactor(&cofactor,r,c);
		double subdet = cofactor.Determinant();

		double sign = ( (r+c) & 1 ) ? -1 : 1;
		
		// writing output transposed :
		(*pNew)[r] = (float) (sign * subdet / det);
	}
}
	
template <>
inline void MatN<1>::GetInverse(MatN<1> * pNew) const
{
	double det = Determinant();
	if ( det == 0.0 )
		return;
	
	pNew->Element(0,0) = (float)(1.f / det);
}

//}{=======================================================================

END_CB
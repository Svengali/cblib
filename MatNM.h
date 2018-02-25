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
			(*pTo)[r] = GetRow(r) * from;
		}
	}

	// float = rowVec * Matrix * colVec
	float InnerProduct(const VecN<t_rows> & left,const VecN<t_cols> & right) const
	{
		float sum = 0.f;
		for(int r=0;r<t_rows;r++)
			for(int c=0;c<t_cols;c++)
				sum += m_data[r][c] * left[r] * right[c];
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
				m_data[r][c] = 0.f;

				for(int t=0;t<t_mid;t++)
				{
					m_data[r][c] += left.m_data[r][t] * right.m_data[t][c];
				}
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

	//-----------------------------------------

private:

	float m_data[t_rows][t_cols];
};

//}{=======================================================================

END_CB
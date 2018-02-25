#pragma once

#include "cblib/RefCounted.h"

/**

Buffer<type> is a naked buffer that can be refcounted & frees itself

NOTE : the convention is Size is a # of elements, not a # of bytes!

Memory was allocated with new [] and delete [] !!

**/

START_CB

template <typename t_type>
class BufferT : public RefCounted
{
public:
	typedef BufferT<t_type>	this_type;
	typedef SPtr< this_type >	sptr_type;
	
	BufferT(t_type * ptr,int size) // takes ownership!!
		: m_data(ptr), m_size(size)
	{
	}
	
	~BufferT()
	{
		Free();
	}
	
	void Give(t_type * ptr,int size) // Buffer will free ptr !!
	{
		Free();
		m_data = ptr;
		m_size = size;
	}
	
	t_type * TakeAndClear() // takes away ownership of the ptr !
	{
		t_type * ret = m_data;
		m_data = NULL;
		m_size = 0;
		return ret;
	}
	
	bool Alloc(int size)
	{
		ASSERT_RELEASE( m_data == NULL );
		
		m_data = new t_type [size];
		
		if ( m_data )
		{
			m_size = size;
			return true;
		}
		else
		{
			return false;
		}
	}
	
	void Free()
	{
		if ( m_data != NULL )
		{
			delete [] m_data;
			m_data = NULL;
		}
		m_size = 0;
	}
	
	t_type * GetData() { return m_data; }
	int GetSize() { return m_size; }

	static sptr_type Create()
	{
		return sptr_type( new this_type );
	}
	static sptr_type Create(int size)
	{
		sptr_type ret( new this_type );
		ret->Alloc(size);
		return ret;
	}
	static sptr_type Create(t_type * ptr,int size) // takes ownership!!
	{
		return sptr_type( new this_type(ptr,size) );
	}

public:
	t_type * m_data;
	int	m_size;

private:

	FORBID_CLASS_STANDARDS(BufferT);

	BufferT() : m_data(NULL), m_size(0)
	{
	}
};

typedef cb::BufferT<char> CharBuffer;
SPtrDef(CharBuffer); // CharBufferPtr

typedef cb::BufferT<char> Buffer;
SPtrDef(Buffer); // BufferPtr

END_CB

#pragma once

#include "cblibCpp0x.h"
using namespace CBLIB_LF_NS;

#define handle_is_64bit	0

//=================================================================	

#if handle_is_64bit
#define handle_bits			64
typedef lf_os::uint64 handle_type;
#else
#define handle_bits			32
typedef lf_os::uint32 handle_type;
#endif

//=================================================================	

struct Referable
{
	Referable();
	virtual ~Referable() { }
	
	handle_type m_self;

	LF_OS_FORBID_CLASS_STANDARDS(Referable);
};

//=================================================================	

namespace ReferenceTable
{

	// Init is not thread-safe, must be done at cinit or from main before threads are started
	void Init();
	
	
	// IncRef looks up the weak reference; returns null if lost
	Referable * IncRef( handle_type h );
	
	// IncRefRelaxed can be used when you know a ref is held
	//	so there's no chance of the object being gone
	void IncRefRelaxed( handle_type h );
	
	// DecRef
	void DecRef( handle_type h );
	
	
};

//=================================================================	

typedef handle_type	WeakRef;

struct OwningRef
{
private:
	Referable * m_ptr;
	handle_type m_handle;
public:

	~OwningRef() { Release(); }
	
	OwningRef() : m_handle(0), m_ptr(0) { }
	OwningRef(handle_type handle) : m_handle(handle) { m_ptr = ReferenceTable::IncRef(handle); }
	OwningRef(Referable * ptr) : m_ptr(ptr) , m_handle(0)
	{
		if ( ptr )
		{
			m_handle = ptr->m_self;
			ReferenceTable::IncRefRelaxed(m_handle); 
		}
	}
	OwningRef(const OwningRef & rhs) : m_ptr(rhs.m_ptr), m_handle(rhs.m_handle)
	{
		if ( m_ptr ) ReferenceTable::IncRefRelaxed(m_handle);
	}
	
	void Release()
	{
		if ( m_ptr )
		{
			ReferenceTable::DecRef(m_handle);
			m_ptr = NULL;
		}
	}
	
	void operator = (const OwningRef & rhs)
	{
		if ( m_handle == rhs.m_handle ) return;
		Release();
		m_ptr = rhs.m_ptr;
		m_handle = rhs.m_handle;
		if ( m_ptr ) ReferenceTable::IncRefRelaxed(m_handle);
	}
	
	void operator = (Referable * ptr)
	{
		if ( m_ptr == ptr ) return;
		Release();
		m_ptr = ptr;
		if ( m_ptr )
		{
			m_handle = ptr->m_self;
			ReferenceTable::IncRefRelaxed(m_handle);
		}
	}
	
	WeakRef GetWeakRef() const { return WeakRef(m_handle); }
	
	Referable * GetPtr() const { return m_ptr; }
	
	bool IsNull() const { return m_ptr == NULL; }
	
	bool operator == (const OwningRef & rhs) const
	{
		return m_ptr == rhs.m_ptr;
	}	
	
	// I'm intentionally not providing operator ->
	//	to encourage you to GetPtr before working with the object
	
	#if 0
	// if you want == NULL :
	bool operator == (const void * rhs) const
	{
		return (void *)m_ptr == rhs;
	}	
	bool operator == (const __int64 rhs) const
	{
		return (__int64)m_ptr == rhs;
	}
	bool operator == (const int rhs) const
	{
		LF_OS_ASSERT( rhs == 0 );
		return (int)m_ptr == rhs;
	}
	#endif	
};

template <typename t_type>
struct WeakRefT
{
	handle_type	m_handle;

	typedef t_type	value_type;
	typedef WeakRefT<t_type>	this_type;

	WeakRefT() : m_handle(0) { }
	WeakRefT(handle_type h) : m_handle(h) { }
	WeakRefT(const this_type & rhs) : m_handle(rhs.m_handle) { }
	void operator = (const this_type & rhs)
	{
		m_handle = rhs.m_handle;
	}
};

template <typename t_type>
struct OwningRefT : public OwningRef
{
	typedef t_type				value_type;
	typedef OwningRef			parent_type;
	typedef WeakRefT<t_type>	weakref_type;
	typedef OwningRefT<t_type>	this_type;

	~OwningRefT() { }
	
	OwningRefT() { }
	OwningRefT(handle_type handle) : OwningRef(handle) { }
	OwningRefT(const weakref_type & wr) : OwningRef(wr.m_handle) { }
	OwningRefT(Referable * ptr) : OwningRef(ptr)
	{
	}
	OwningRefT(const this_type & rhs) : OwningRef(rhs)
	{
	}
	
	//void Release()
	
	void operator = (const this_type & rhs)
	{
		OwningRef::operator =(rhs);
	}
	void operator = (value_type rhs)
	{	
		OwningRef::operator =(rhs);
	}
	
	weakref_type GetWeakRefT() const { return weakref_type(GetWeakRef()); }
	
	value_type * GetPtr() const { return (value_type *)m_ptr; }
	
	//bool IsNull() const;
	
	bool operator == (const OwningRef & rhs) const
	{
		return m_ptr == rhs.m_ptr;
	}	
	
	// I'm intentionally not providing operator ->
	//	to encourage you to GetPtr before working with the object
};

template <typename t_type>
static OwningRefT<t_type> Factory()
{
	t_type * ptr = new t_type;
	return OwningRefT<t_type>(ptr);
}
	

//=================================================================	

#pragma once

// see RefCounted.* for more notes

#include "cblib/RefCounted.h"
#include "cblib/SPtr.h"
#include "cblib/Util.h"

// DEBUG_WEAKPTR_CACHE stores an m_ptr member in WeakPtr
//	just for inspection in the debugger windows
//	toggle here ; on by default in _DEBUG
// CB 8-20-04 : WARNING !!!  This is a DEBUG INSPECTION CACHE !!
//	If the SPtr() has gone away, THIS POINTER MAY POINT AT CRAP !!!
//	You need to check s_pointerTable[] and check the GUIDs to really make sure this WeakPtr is valid !!	
#ifdef _DEBUG
#define DEBUG_WEAKPTR_CACHE
#endif

START_CB

//{= WeakPtr =======================================================

/*! WeakPtr : smart pointer to a *const* RefCounted class
   automagically refs things it points at,
	 and de-refs when it's set to null or goes out of scope

	WeakPtr holds a non-owning pointer to a RefCounted thing

	when the RefCounted thing goes to NULL, all the WeakPtr will
	now point at NULL !

	That means you must check weak pointers for null at all times!!

*/

class WeakPtrBase
{
public:
	#pragma warning(disable : 4355) // using "this" as initializer
	WeakPtrBase() : m_index(0), m_guid(0)
	{
		#ifndef FINAL
		s_numberExisting++;
		#endif
	}
	#pragma warning(default : 4355)

	~WeakPtrBase()
	{
		#ifndef FINAL
		s_numberExisting--;
		#endif
	}
	
	//RefCounted * GetPtr() const { return m_ptr; }

	#ifdef DO_REFCOUNTED_REF_TRACKING //{
	void LogRefHolder() const
	{
		m_trace.Log(Log::eNaked);
	}
	
	void RecordRefHolder()
	{
		m_trace.Record(1); // skip 1 levels		
	}
	#endif // DO_REFCOUNTED_REF_TRACKING }
	
	static int s_numberExisting;
		
	// don't call it IsValid() to avoid mistakes by the client
	bool IsValidWeakPtrBase() const
	{
		RefCounted * ptr = GetRefCounted();
		if ( ptr == NULL )
		{
			//ASSERT( m_link.IsEmpty() );
		}
		else
		{
			//ASSERT( ! m_link.IsEmpty() );
			// @@ could check that we're actually linked to ptr
			
			// @@ this is dup'ed from SPtr ; should share it
			/*
			// Check to make sure the actual pointer is ok.
			ASSERT( reinterpret_cast<uint32>( ptr ) != 0xcccccccc );
			ASSERT( reinterpret_cast<uint32>( ptr ) != 0xcdcdcdcd );
			ASSERT( reinterpret_cast<uint32>( ptr ) != 0xdddddddd );
			ASSERT( reinterpret_cast<uint32>( ptr ) != 0xffffffff );

			// Check to make sure the underlying object is fine.  This uses the
			// assumption that the vtable is the first 32bits in an object.
			// If we do not check here, by the time the XBox knows the pointer
			// is bad we have already jumped to one of the bad values.
			ASSERT( *reinterpret_cast<uint32 const * const>( ptr ) != 0xcccccccc );
			ASSERT( *reinterpret_cast<uint32 const * const>( ptr ) != 0xcdcdcdcd );
			ASSERT( *reinterpret_cast<uint32 const * const>( ptr ) != 0xdddddddd );
			ASSERT( *reinterpret_cast<uint32 const * const>( ptr ) != 0xffffffff );
			*/
			
			// ML note: This check means that in some cases we need to include
			// the definition of the class which is the argument for this
			// template. For example, if we call ConstWeakPtrToConstWeakPtrCCast
			// in the header file.
			//ASSERT( m_ptr->GetRefCount() > 0 );
		}
		return true;
	}
	
protected:
	
	RefCounted * GetRefCounted() const
	{
		return RefCounted::LookupPointer(m_index,m_guid);
	}
	void SetRefCounted(const RefCounted *ptr)
	{
		if ( ptr == NULL )
		{
			m_index = 0;
			m_guid = 0;
		}
		else
		{
			m_index = ptr->GetPointerTableIndex();
			m_guid  = ptr->GetPointerTableGuid();
		}
	}
	
	RefCounted::index_type	m_index;
	RefCounted::guid_type	m_guid;
	
	#ifdef DO_REFCOUNTED_REF_TRACKING //{
	StackTrace		m_trace;
	#endif // DO_REFCOUNTED_REF_TRACKING }
};

template <class BaseClass> class WeakPtr : private WeakPtrBase
{
public:
	// BaseClass must be derived from RefCounted
	typedef WeakPtr<BaseClass>	this_type;
	typedef BaseClass			base_class;

	//-------------------------------------------------------------
	// Constructors :

	//! default constructor makes a NULL smart pointer
	inline WeakPtr() : WeakPtrBase()
	{
		//ASSERT( sizeof(BaseClass::m_refCount) == sizeof(int) );
		#ifdef DEBUG_WEAKPTR_CACHE
		m_ptr = NULL;
		#endif
	}

	// !! important explicit !!
	// NULL is valid as an argument
	explicit inline WeakPtr(BaseClass * bc) : WeakPtrBase()
	{
		Set(bc);
	}

	// copy constructor
	inline WeakPtr(const this_type & bc) : WeakPtrBase()
	{
		Set(bc.GetPtr());
	}

	//! copy constructor from derived class Smart pointer
	//! NOT explicit !!
	template <class OtherClass>
	inline WeakPtr(const WeakPtr<OtherClass>& tc) : WeakPtrBase()
	{
		Set(tc.GetPtr());
	}

	template <class OtherClass>
	inline WeakPtr(const SPtr<OtherClass>& tc) : WeakPtrBase()
	{
		Set(tc.GetPtr());
	}

	/*
	inline ~WeakPtr()
	{
		// ~WeakPtrBase cleans me up
	}
	*/

	//-------------------------------------------------------------
	// Accessors :

	BaseClass * GetPtr() const
	{
		ASSERT( IsValidWeakPtr() );
		RefCounted * rc = GetRefCounted();
		BaseClass * ptr = static_cast<BaseClass *>(rc);

		#ifdef DEBUG_WEAKPTR_CACHE
		// update the cache in case we went NULL
		m_ptr = ptr;
		#endif
		
		return ptr;
	}
	
	BaseClass * operator -> () const
	{
		return GetPtr();
	}

	BaseClass & GetRef() const
	{
		return *GetPtr();
	}

	SPtr<BaseClass> GetSPtr() const
	{
		return SPtr<BaseClass>( GetPtr() );
	}

	bool IsValidWeakPtr() const
	{
		return IsValidWeakPtrBase();
	}
	
	//-------------------------------------------------------------
	// Mutators :

	template <class OtherBase>
	void operator = (const WeakPtr<OtherBase> & tc)
	{
		Set( tc.GetPtr() );
	}

	template <class OtherBase>
	void operator = (const SPtr<OtherBase> & tc)
	{
		Set( tc.GetPtr() );
	}

	// specialization to this type :
	void operator = (const this_type & bc)
	{
		Set(bc.GetPtr());
	}

	void operator = (BaseClass * bc)
	{
		Set(bc);
	}

	//-------------------------------------------------------------
	// Compares :

	// compare to an arbitrary other WeakPtr :
	template <class OtherBase>
	bool operator == (const WeakPtr<OtherBase> & other) const
	{
		return ( GetRefCounted() == other.GetPtr() );
	}

	// compare to an arbitrary other SPtr :
	template <class OtherBase>
	bool operator == (const SPtr<OtherBase> & other) const
	{
		return ( GetRefCounted() == other.GetPtr() );
	}

	// specialization to this type :
	bool operator == (const this_type & bc) const
	{
		return ( GetRefCounted() == bc.GetRefCounted() );
	}

	//! Important : anything that can implicitly convert to
	//!		BaseClass will also use this compare!
	// for comparison against "this", both const are needed
	bool operator == (const BaseClass * const bc) const
	{
		return ( GetRefCounted() == bc );
	}

	bool operator != (const this_type & bc) const
	{
		return ( GetRefCounted() != bc.GetRefCounted() );
	}

	//! Important : anything that can implicitly convert to
	//!		BaseClass will also use this compare!
	// for comparison against "this", both const are needed
	bool operator != (const BaseClass * const bc) const
	{
		return ( GetRefCounted() != bc );
	}

	//! This operator is necessary so we can use WeakPtr's as keys in a std::map.
	//	nein, use a functor
	/*
	bool operator < (const this_type& p) const
	{
		return ( m_ptr < p.m_ptr );
	}
	*/

	//-------------------------------------------------------------

	/**
	When an object stores a smart pointer to a non-const object and contains
	a const query function to get the smart pointer, this function is provided
	to avoid some of the casting ugliness. Usage example:

	\code
	class XXX
	{
	public:
		const ObjPtr &GetObj()			{ return m_spObj; }
		const ObjPtrC &GetObj() const	{ return m_spObj.ConstWeakPtrToConstWeakPtrCCast(); }

	private:
		ObjPtr m_spObj;
	};
	\endcode
	*/
	const WeakPtr<const BaseClass> &ConstWeakPtrToConstWeakPtrCCast() const
	{
		ASSERT( IsValidWeakPtr() );
		return reinterpret_cast<const WeakPtr<const BaseClass> &>(*this);
	}

	//-------------------------------------------------------------
private:

	inline void Set(BaseClass * ptr)
	{
		//ASSERT( IsValidWeakPtr() );
		//if ( m_ptr != NULL )
		//{
		//	// de-link from m_ptr
		//	m_link.Cut();
		//	ASSERT( m_link.IsEmpty() );
		//}
		SetRefCounted(ptr);
		
		#ifdef DEBUG_WEAKPTR_CACHE
		// data :
		m_ptr = ptr;
		#endif // DEBUG_WEAKPTR_CACHE
		
		if ( ptr != NULL )
		{
			//((RefCounted *)(m_ptr))->AddToWeakLink((WeakPtrLink *)&m_link);
			//m_ptr->AddToWeakLink((WeakPtrLink *)&m_link);
			//ASSERT( ! m_link.IsEmpty() );
			
			#ifdef DO_REFCOUNTED_REF_TRACKING
			RecordRefHolder();
			#endif
		}
		ASSERT( IsValidWeakPtr() );
	}
	
	#ifdef DEBUG_WEAKPTR_CACHE
	// data :
	// CB 8-20-04 : WARNING !!!  This is a DEBUG INSPECTION CACHE !!
	//	If the SPtr() has gone away, THIS POINTER MAY POINT AT CRAP !!!
	//	You need to check s_pointerTable[] and check the GUIDs to really make sure this WeakPtr is valid !!	
	mutable BaseClass * m_ptr;
	#endif // DEBUG_WEAKPTR_CACHE
	
}; // end class WeakPtr

//}{=================================================================================================

/*! SPtrCast
	safe cast between SPtr types using dynamic_cast
	use like :
	SPtr<derived> to = SPtrCast<derived>( from_ptr );
	 you need to tell it the first template class type, no
	 need for the second
	notez : dynamic_cast is pretty smart; if you cast up
	in inheritance, no CPU work is needed; if you cast down,
	then it does the expensive rot
*/
template <class ToPtr,class From> ToPtr SPtrCast(const WeakPtr<From> & from)
{
	From * const pfrom = from.GetPtr();

	if (pfrom == NULL)
	{
		return ToPtr(NULL);
	}

	ASSERT( from.IsValidWeakPtr() );

	ToPtr::base_class * const to = dynamic_cast<ToPtr::base_class * const>(pfrom);
	return ToPtr(to);
}

//========================================================================================

// GetPtr
//	base in SPtr.h

template < typename T_Class>
T_Class * GetPtr(const WeakPtr<T_Class> & ptr)
{
	return ptr.GetPtr();
}

// operator == for pointer on the left :

template <typename T1,typename T2>
inline bool operator ==(T1 * ptr1,const WeakPtr<T2> & ptr2)
{
	return ptr1 == ptr2.GetPtr();
}
  
template <typename T1,typename T2>
inline bool operator !=(T1 * ptr1,const WeakPtr<T2> & ptr2)
{
	return ptr1 != ptr2.GetPtr();
}
 
//}{=================================================================================================

END_CB

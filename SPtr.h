#pragma once

// see RefCounted.* for more notes

#include "cblib/RefCounted.h"

#ifdef DO_REFCOUNTED_REF_TRACKING //{
#include "cblib/StackTrace.h"
#include "cblib/LogEnum.h"
#endif // DO_REFCOUNTED_REF_TRACKING //}

START_CB

//{= SPtr =======================================================

/*! SPtr : smart pointer to a RefCounted class
   automagically refs things it points at,
	 and de-refs when it's set to null or goes out of scope
*/

template <class BaseClass> class SPtr
{
public:
	// BaseClass must be derived from RefCounted
	typedef SPtr<BaseClass> this_type;
	typedef BaseClass		base_class;

	//-------------------------------------------------------------
	// Constructors :

	//! default constructor makes a NULL smart pointer
	inline SPtr() : m_ptr(NULL)
	{
		#ifdef DO_REFCOUNTED_REF_TRACKING //{
		m_link.SetData(this);
		#endif //DO_REFCOUNTED_REF_TRACKING }
	}

	// !! important explicit !!
	// NULL is valid as an argument
	explicit inline SPtr(BaseClass * bc)
	{
		#ifdef DO_REFCOUNTED_REF_TRACKING //{
		m_link.SetData(this);
		#endif //DO_REFCOUNTED_REF_TRACKING }
		m_ptr = bc;
		if ( m_ptr )
		{
			ASSERT( IsValidSPtr() );
			TakeRef(m_ptr);
		}
	}

	// copy constructor
	inline SPtr(const this_type & bc)
	{
		#ifdef DO_REFCOUNTED_REF_TRACKING //{
		m_link.SetData(this);
		#endif //DO_REFCOUNTED_REF_TRACKING }
		m_ptr = bc.m_ptr;
		if ( m_ptr )
		{
			ASSERT( IsValidSPtr() );
			TakeRef(m_ptr);
		}
	}

	//! copy constructor from derived class Smart pointer
	//! NOT explicit !!
	template <class OtherClass>
	inline SPtr(const SPtr<OtherClass>& tc)
	{
		#ifdef DO_REFCOUNTED_REF_TRACKING //{
		m_link.SetData(this);
		#endif //DO_REFCOUNTED_REF_TRACKING }
		//! I am intentionally using "implicit cast" here, not static_cast,
		//!	for compatibility with the other functions
		m_ptr = /*static_cast<BaseClass*>*/ (tc.GetPtr());
		if ( m_ptr )
		{
			ASSERT( IsValidSPtr() );
			TakeRef(m_ptr);
		}
	}

	template <class OtherClass>
	inline SPtr(const WeakPtr<OtherClass>& tc)
	{
		#ifdef DO_REFCOUNTED_REF_TRACKING //{
		m_link.SetData(this);
		#endif //DO_REFCOUNTED_REF_TRACKING }
		//! I am intentionally using "implicit cast" here, not static_cast,
		//!	for compatibility with the other functions
		m_ptr = /*static_cast<BaseClass*>*/ (tc.GetPtr());
		if ( m_ptr )
		{
			ASSERT( IsValidSPtr() );
			TakeRef(m_ptr);
		}
	}

	inline ~SPtr()
	{
		Set(NULL);
		#ifdef DO_REFCOUNTED_REF_TRACKING //{
		m_link.SetData(NULL);
		#endif //DO_REFCOUNTED_REF_TRACKING }
	}

	//-------------------------------------------------------------
	// Accessors :

	BaseClass * operator -> () const
	{
		ASSERT( IsValidSPtr() );
		ASSERT(m_ptr);
		return m_ptr;
	}

	BaseClass * GetPtr() const
	{
		return m_ptr;
	}

	BaseClass & GetRef() const
	{
		ASSERT( IsValidSPtr() );
		ASSERT(m_ptr != NULL);
		return *m_ptr;
	}

	// cbloom : do not do this, because it hands out an
	//	unsafe naked pointer in a very non-obvious way
	/*
	BaseClass& operator*() const
	{
		ASSERT( IsValidSPtr() );
		ASSERT(m_ptr);
		return *m_ptr;
	}
	*/

	//-------------------------------------------------------------
	// Mutators :

	/*
	template <class OtherBase>
	void operator = (OtherBase * tc)
	{
		BaseClass * bc = static_cast<BaseClass * >(tc);
		Set( bc );
	}
	*/

	template <class OtherBase>
	void operator = (const SPtr<OtherBase> & tc)
	{
		//BaseClass * bc = /*static_cast<BaseClass * >*/ (tc.GetPtr());
		//Set( bc );
		Set( tc.GetPtr() );
	}

	template <class OtherBase>
	void operator = (const WeakPtr<OtherBase> & tc)
	{
		//BaseClass * bc = /*static_cast<BaseClass * >*/ (tc.GetPtr());
		//Set( bc );
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

	// compare to an arbitrary other SPtr :
	template <class OtherBase>
	bool operator == (const SPtr<OtherBase> & other) const
	{
		//BaseClass * bc = /*static_cast<BaseClass *>*/ (other.GetPtr());
		//return ( m_ptr == bc );
		return ( m_ptr == other.GetPtr() );
	}

	template <class OtherBase>
	bool operator == (const WeakPtr<OtherBase> & other) const
	{
		//BaseClass * bc = /*static_cast<BaseClass *>*/ (other.GetPtr());
		//return ( m_ptr == bc );
		return ( m_ptr == other.GetPtr() );
	}

	/*
	// this breaks comparison to NULL because it creates an ambiguity
	// fortuantely, this is not needed because of implicit conversion!
	//	see below
	// for comparison against "this", both const would be needed
	template <class OtherBase>
	bool operator == (const OtherBase * const other) const
	{
		BaseClass * bc = static_cast<BaseClass *>(other);
		return ( m_ptr == bc )
	}
	*/

	// specialization to this type :
	bool operator == (const this_type & bc) const
	{
		return ( m_ptr == bc.m_ptr );
	}

	//! Important : anything that can implicitly convert to
	//!		BaseClass will also use this compare!
	// for comparison against "this", both const are needed
	bool operator == (const BaseClass * const bc) const
	{
		return ( m_ptr == bc );
	}

	bool operator != (const this_type & bc) const
	{
		return ( m_ptr != bc.m_ptr );
	}

	//! Important : anything that can implicitly convert to
	//!		BaseClass will also use this compare!
	// for comparison against "this", both const are needed
	bool operator != (const BaseClass * const bc) const
	{
		return ( m_ptr != bc );
	}

	//! This operator is necessary so we can use SPtr's as keys in a std::map.
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
		const ObjPtrC &GetObj() const	{ return m_spObj.ConstSPtrToConstSPtrCCast(); }

	private:
		ObjPtr m_spObj;
	};
	\endcode
	*/
	const SPtr<const BaseClass> &ConstSPtrToConstSPtrCCast() const
	{
		ASSERT( m_ptr == NULL || IsValidSPtr() );
		return reinterpret_cast<const SPtr<const BaseClass> &>(*this);
	}

	//-------------------------------------------------------------

	bool IsValidSPtr( void ) const
	{
		// Check to make sure the actual pointer is ok.
		ASSERT( *(uint32*)( &m_ptr ) != 0xcccccccc );
		ASSERT( *(uint32*)( &m_ptr ) != 0xcdcdcdcd );
		ASSERT( *(uint32*)( &m_ptr ) != 0xdddddddd );
		ASSERT( *(uint32*)( &m_ptr ) != 0xffffffff );

		#ifdef DO_REFCOUNTED_REF_TRACKING //{
		ASSERT( m_link.IsValid() );
		#endif //DO_REFCOUNTED_REF_TRACKING }

		// Check to make sure the underlying object is fine.  This uses the
		// assumption that the vtable is the first 32bits in an object.
		// If we do not check here, by the time the XBox knows the pointer
		// is bad we have already jumped to one of the bad values.
		if ( m_ptr != NULL )
		{
			ASSERT( *(uint32 *)( m_ptr ) != 0xcccccccc );
			ASSERT( *(uint32 *)( m_ptr ) != 0xcdcdcdcd );
			ASSERT( *(uint32 *)( m_ptr ) != 0xdddddddd );
			ASSERT( *(uint32 *)( m_ptr ) != 0xffffffff );
			
			// can't do this, causes a circle/infinite loop
			//ASSERT( m_ptr->SPtrLinkIsValid() );
		}

		return true;
	}

	#ifdef DO_REFCOUNTED_REF_TRACKING //{
	void LogRefHolder() const
	{
		if ( m_ptr != NULL )
		{
			m_trace.Log(Log::eNaked);
		}
	}
	#endif // DO_REFCOUNTED_REF_TRACKING }

private:

	//friend void SPtrRelease(this_type & ptr);

	inline void Set(BaseClass * bc)
	{
		//if ( m_ptr != bc )
		// works fine even when they're equal
		enum { If_You_Hit_This_You_Are_Forward_Declaring_A_Class = sizeof ( BaseClass ) };

		#ifdef DO_REFCOUNTED_REF_TRACKING //{
		m_link.Cut();
		#endif //DO_REFCOUNTED_REF_TRACKING }

		if ( bc )
		{
			TakeRef(bc);
		}
		if ( m_ptr )
		{
			#ifdef DO_REFCOUNTED_REF_TRACKING //{
			ASSERT( m_ptr->SPtrLinkIsValid() );
			#endif //DO_REFCOUNTED_REF_TRACKING }
			m_ptr->RefCounted_FreeRef();
		}

		m_ptr = bc;

		ASSERT( m_ptr == NULL || IsValidSPtr() );
	}

	// TakeRef tiny helper :
	#ifndef DO_REFCOUNTED_REF_TRACKING //{
	
	inline void TakeRef(BaseClass * bc)
	{
		bc->RefCounted_TakeRef();
	}

	#else //DO_REFCOUNTED_REF_TRACKING }{
	
	inline void TakeRef(BaseClass * bc)
	{
		m_link.Cut();
		m_trace.Record(1); // skip 1 levels
		bc->RefCounted_TakeRef();
		bc->AddToSPtrLink(&m_link);
	}

	#endif // DO_REFCOUNTED_REF_TRACKING }

	// data :
	BaseClass * m_ptr;

	#ifdef DO_REFCOUNTED_REF_TRACKING //{
	SptrVoidLink	m_link;
	StackTrace		m_trace;
	#endif // DO_REFCOUNTED_REF_TRACKING }

}; // end class SPtr

//}{=================================================================================================

/*! SPtrRelease
	same as setting = NULL
	but neater for those who want to be really explicit
*/
template <class BaseClass>
inline void SPtrRelease(SPtr<BaseClass> & ptr)
{
	//ptr.Set(NULL);
	ptr = NULL;
}


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
template <class ToPtr,class From> ToPtr SPtrDynamicCast(const SPtr<From> & from)
{
	From * const pfrom = from.GetPtr();

	if (pfrom == NULL)
	{
		return ToPtr(NULL);
	}

	ASSERT( from.IsValidSPtr() );

	ToPtr::base_class * const to = dynamic_cast<ToPtr::base_class * const>(pfrom);
	return ToPtr(to);
}

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
template <class ToPtr,class From> ToPtr SPtrCast(const SPtr<From> & from)
{
	return SPtrDynamicCast<ToPtr>(from);
}

/*! SPtrCastStatic
  Analogous to static_cast<Derived*>(Base*).  Unsafe unless you
  know what you're doing...
*/
template <class ToPtr, class From> ToPtr SPtrStaticCast(const SPtr<From>& from)
{
	ASSERT(from == NULL || SPtrDynamicCast<ToPtr>(from) != NULL);

	return ToPtr(static_cast<ToPtr::base_class*>(from.GetPtr()));
}

/*
	GetPtr() with overrides, works on all pointer types
	(WeakPtr version in WeakPtr.h)
*/

template < typename T_Class >
T_Class * GetPtr(T_Class * ptr)
{
	return ptr;
}

template < typename T_Class>
T_Class * GetPtr(const SPtr<T_Class> & ptr)
{
	return ptr.GetPtr();
}

// operator == for pointer on the left :

template <typename T1,typename T2>
inline bool operator ==(T1 * ptr1,const SPtr<T2> & ptr2)
{
	return ptr1 == ptr2.GetPtr();
}
  
template <typename T1,typename T2>
inline bool operator !=(T1 * ptr1,const SPtr<T2> & ptr2)
{
	return ptr1 != ptr2.GetPtr();
}
  
 END_CB
  
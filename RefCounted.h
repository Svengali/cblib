#pragma once

#include "cblib/Base.h"
#include "cblib/Link.h"

START_CB

//= Forward Decl's =======================================================

/*! RefCounted :
	just has a reference count and a virtual destructor
	for use with SPtr<> , a smart template pointer

	Smart Pointers should be used with the typedefs created by SPtrDef,
	like MyClassPtr and MyClassPtrC

	the PtrC typedef is for holding const data
	you can convert from SPtr to SPtrC , but not vice-versa
	
	See http://www.cbloom.com/3d/techdocs/smart_pointers.txt
	
*/

class RefCounted;
class WeakPtrBase;
template <class BaseClass> class SPtr;
template <class BaseClass> class WeakPtr;

//! forward declaration and smart pointer def with Name-Ptr convention :
//!	 PtrC holds a const version of the same class
#define SPtrDef(BaseClass)	typedef cb::SPtr<BaseClass> BaseClass##Ptr; \
							typedef cb::SPtr<const BaseClass> BaseClass##PtrC; \
							typedef cb::WeakPtr<BaseClass> BaseClass##WeakPtr; \
							typedef cb::WeakPtr<const BaseClass> BaseClass##WeakPtrC

#define SPtrFwd(BaseClass)	class BaseClass;	\
							SPtrDef(BaseClass);


// base class : RefCountedPtr, RefCountedPtrC
SPtrDef(RefCounted);

// @@ TOGGLE HERE
//	on by default in debug
// -> broken w/o stack traces
//#ifdef _DEBUG
//#define DO_REFCOUNTED_REF_TRACKING
//#endif

#ifdef DO_REFCOUNTED_REF_TRACKING
typedef Link<void *>		SptrVoidLink;
#endif // DO_REFCOUNTED_REF_TRACKING

// 16-bit WeakPtr reference
//	makes WeakPtr 4 bytes instead of 8
//	limits you to 65535 objects total
//#define DO_16BIT_INDEX

//= RefCounted =======================================================

// CLASS_ABSTRACT_BASE
class RefCounted
{
public:

	#ifdef DO_16BIT_INDEX
	typedef uint16	guid_type;
	typedef uint16	index_type;
	typedef short	refc_type;
	#else
	typedef uint32	guid_type;
	typedef uint32	index_type;	
	typedef int		refc_type;
	#endif

	///////////////////////////

	RefCounted();

		// RefCounted destructor is virtual so that we'll call
		//	to our derived destructors first, even if you
		//	destruct from a RefCounted pointer
		// that is :
		//   TestClass * stuff = new TestClass();
		//   RefCounted * ptr = (RefCounted *) stuff;
		//   delete ptr;
		// this code segment will call the TestClass destructor
		
	virtual ~RefCounted();

	virtual bool IsValid() const;

	///////////////////////////
	// internal stuff ; you should never be calling these :

	int GetRefCount() const { return m_refCount; }

	index_type	GetPointerTableIndex() const;
	guid_type	GetPointerTableGuid() const;

	static RefCounted * LookupPointer(const index_type index, const guid_type guid);

	///////////////////////////
	// debug/tracking stuff :

	static int GetTotalRefCount();
	static int GetTotalObjectCount();

	// no-op unless DO_REFCOUNTED_REF_TRACKING
	void LogRefHolders() const;

	void LogWeakPointers() const;

	///////////////////////////
	// ClearWeakPtrs makes all the WeakPtrs to an object go NULL without releasing the object
	//	this can be useful to make an object "virtually" recycled, but it is very dangerous
	//	DO NOT USE UNLESS YOU ARE A WIZARD OR NINJA
	void ClearWeakPtrs();

private:

	// Ref count mutiliation can only be done by the smart pointer classes :
	template <class BaseClass> friend class WeakPtr;
	template <class BaseClass> friend class SPtr;

	//	refCount is const_cast'ed so that the class can act like its const
	//	even when its being passed around in smart pointers, which
	//	will necessarily modify the refCount

	void TakeRef() const;
	void FreeRef() const;

	FORBID_ASSIGNMENT(RefCounted);
	FORBID_COPY_CTOR(RefCounted);

	//-----------------------------------------------------------------
	// data :
	refc_type		m_refCount;
	index_type		m_index;

	// class statics :
	static int	s_totalRefCount;
	static int	s_totalObjectCount;
	
	//-----------------------------------------------------------------

	#ifdef DO_REFCOUNTED_REF_TRACKING
	mutable SptrVoidLink m_sptrLink;

	public:
	bool SPtrLinkIsValid() const;
	private:

	void AddToSPtrLink(SptrVoidLink * pLink) const;
	#endif // DO_REFCOUNTED_REF_TRACKING

}; // end class RefCounted

END_CB

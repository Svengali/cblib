#include "Base.h"
#include "RefCounted.h"
#include "WeakPtr.h"
#include "SPtr.h"
#include "vector.h"
#include <limits.h>

START_CB

// RefCounted class statics :
int RefCounted::s_totalRefCount = 0;
int RefCounted::s_totalObjectCount = 0;

//=====================================================================================================

#pragma pack(push)
#pragma pack(2)
struct WeakTableEntry
{
	// when I'm alive, "ptr" points at my goods
	// when I'm free nextFree points at the next is the free list
	//	(the head of the free list is at s_nextFree)
	union
	{
		RefCounted *	m_ptr;
		int				m_nextFree;
	};
	RefCounted::guid_type		m_guid;
};
#pragma pack(pop)

//------------------------------------------------------------------------
#ifdef DO_16BIT_INDEX

COMPILER_ASSERT( sizeof(WeakTableEntry) == 6 );

static const int c_maxObjects = 1<<16;

// just always have 64k :
// index 0 always means the NULL pointer
//	@@@@ CB ? does that = 0 make this take up XBE size?
static WeakTableEntry	s_pointerTable[c_maxObjects] = { 0 };

#else // DO_16BIT_INDEX

// just have a growing vector
//vector<WeakTableEntry> s_pointerTable;
// need a proper singleton cuz this can be accessed during cinit
static const int c_tableGrowCount = 4096/sizeof(WeakTableEntry);

static vector<WeakTableEntry> * s_ptrPointerTable = NULL;

static vector<WeakTableEntry> & GetPointerTable()
{
	if ( s_ptrPointerTable == NULL )
	{
		s_ptrPointerTable = new vector<WeakTableEntry>;
		s_ptrPointerTable->resize(c_tableGrowCount);
		ZERO(&s_ptrPointerTable->at(0));
	}
	return *s_ptrPointerTable;
}

#define s_pointerTable	GetPointerTable()

void DestroyPointerTable()
{
	delete s_ptrPointerTable;
}


#endif // DO_16BIT_INDEX
//------------------------------------------------------------------------

static int		s_numPointers = 1; // next available index
static int		s_nextFree = -1; // linked list of frees

static RefCounted::index_type AddToTable(RefCounted * ptr)
{
	if ( s_nextFree > 0 )
	{
		// pop from the freed list
		RefCounted::index_type index = (RefCounted::index_type)s_nextFree;
		WeakTableEntry & entry = s_pointerTable[index];
		s_nextFree = entry.m_nextFree;
		entry.m_ptr = ptr;
		// guid was already incremented by RemoveFromTable
		return index;
	}
	else
	{
		#ifdef DO_16BIT_INDEX
		ASSERT( s_numPointers < c_maxObjects );
		if ( s_numPointers >= c_maxObjects )
		{
			FAIL("Too many RefCounted objects!\n");
			exit(20);
		}
		#else
		// do a growing vector :
		if ( s_numPointers >= s_pointerTable.size() )
		{
			s_pointerTable.resize(s_numPointers+c_tableGrowCount);
		}
		#endif // DO_16BIT_INDEX
		
		RefCounted::index_type index = (RefCounted::index_type) s_numPointers;
		s_numPointers++;
		WeakTableEntry & entry = s_pointerTable[index];
		entry.m_ptr = ptr;
		entry.m_guid = 0;
				
		return index;
	}
}

static void RemoveFromTable(RefCounted *ptr,RefCounted::index_type index)
{
	ASSERT( s_pointerTable[index].m_ptr == ptr );

	// step the guid so it doesnt match and we'll return NULL
	s_pointerTable[index].m_guid++;

	// add to the free link :
	s_pointerTable[index].m_nextFree = s_nextFree;
	s_nextFree = index;
}

//=====================================================================================================

RefCounted::RefCounted() : m_refCount(0)
{
	// start with no ref;
	// the first smart pointer assignment takes a ref
	// so you come out of the factory with one ref
	
	m_index = AddToTable(this);
	
	// debugging only :
	s_totalObjectCount++;

#ifdef DO_REFCOUNTED_REF_TRACKING
	m_sptrLink.SetData(NULL);
#endif
}

RefCounted::~RefCounted()
{
	// RefCounted destructor is virtual so that we'll call
	//	to our derived destructors first, even if you
	//	destruct from a RefCounted pointer
	// that is :
	//   TestClass * stuff = new TestClass();
	//   RefCounted * ptr = (RefCounted *) stuff;
	//   delete ptr;
	// this code segment will call the TestClass destructor
	
	// you should have one ref when you destruct
	ASSERT( m_refCount == 0 );
	
	// FreeRef just invalidates my GUID but doesn't release my table index
	RemoveFromTable(this,m_index);
	
	#ifndef FINAL
	ASSERT( s_totalObjectCount > 0 );
	s_totalObjectCount--;
	#endif
}

RefCounted::index_type	RefCounted::GetPointerTableIndex() const
{
	ASSERT( s_pointerTable[m_index].m_ptr == this );
	return m_index;
}

RefCounted::guid_type	RefCounted::GetPointerTableGuid() const
{
	const WeakTableEntry & entry = s_pointerTable[m_index];
	ASSERT( entry.m_ptr == this );
	return entry.m_guid;
}
	
bool RefCounted::IsValid() const
{
	// We make this > 0, since we should wind up
	//  deleted if our refcount drops to 0...
	ASSERT(m_refCount > 0);

	return true;
}

/*static*/ RefCounted * RefCounted::LookupPointer(const index_type index, const guid_type guid)
{
	const WeakTableEntry & entry = s_pointerTable[index];
	if ( entry.m_guid != guid )
	{
		// it was freed
		return NULL;
	}
	else
	{
		// guids match, yeehaw
		return entry.m_ptr;
	}
}

void RefCounted::RefCounted_TakeRef() const
{
	const_cast<RefCounted *>(this)->m_refCount ++;
	
	s_totalRefCount++;
}

void RefCounted::RefCounted_FreeRef() const
{
	ASSERT( m_refCount > 0 );
	
	s_totalRefCount--;
	
	const_cast<RefCounted *>(this)->m_refCount --;
	
	if ( m_refCount == 0 )
	{
		// null out all weak pointers before deleting
		const_cast<RefCounted *>(this)->ClearWeakPtrs();
		// this is the last ref
		delete this; // calls virtual destructor
	}
}
	
///////////////////////////

/*static*/ int RefCounted::GetTotalRefCount()
{
	#ifndef FINAL
	return s_totalRefCount;
	#else
	return 0;
	#endif
}
/*static*/  int RefCounted::GetTotalObjectCount()
{
	#ifndef FINAL
	return s_totalObjectCount;
	#else
	return 0;
	#endif
}
	
void RefCounted::ClearWeakPtrs()
{
	//	this makes all weak pointers fail to point to me :
	s_pointerTable[m_index].m_guid++;
}

#ifdef DO_REFCOUNTED_REF_TRACKING //{

void RefCounted::AddToSPtrLink(SptrVoidLink * pLink) const
{
	ASSERT( pLink != NULL );
	ASSERT( pLink->IsEmpty() );
	ASSERT( SPtrLinkIsValid() );
	//ASSERT( pLink->GetData()->GetPtr() == this );
	m_sptrLink.AddAfter(pLink);
}
	
bool RefCounted::SPtrLinkIsValid() const
{
#ifdef DO_ASSERTS
	ASSERT( m_sptrLink.IsValid() );

	int count = 0;
	for(const SptrVoidLink * pLink = m_sptrLink.GetNext(); pLink != &(m_sptrLink);  )
	{
		ASSERT( pLink->IsValid() );
		const SptrVoidLink * pNext = pLink->GetNext(); // cuz Set(NULL) may cut
		void * pData = pLink->GetData();
		ASSERT( pData != NULL );

		// C-Style cast here is Ok because we're casting the SPtr , not the class itself!
		const RefCountedPtr * pSP = (const RefCountedPtr *) pData;
		ASSERT( pSP->IsValidSPtr() );
		pLink = pNext;

		count++;
		ASSERT( count < 9999 );
	}
#endif // DO_ASSERTS
	return true;
}

void RefCounted::LogRefHolders() const
{
	for(const SptrVoidLink * pLink = m_sptrLink.GetNext(); pLink != &(m_sptrLink);  )
	{
		const SptrVoidLink * pNext = pLink->GetNext(); // cuz Set(NULL) may cut
		void * pData = pLink->GetData();
		ASSERT( pData != NULL );
		
		// C-Style cast here is Ok because we're casting the SPtr , not the class itself!
		const RefCountedPtr * pSP = (const RefCountedPtr *) pData;
		pSP->LogRefHolder();
		pLink = pNext;
	}	
}

void RefCounted::LogWeakPointers() const
{
	// walk m_weakLink
	// @@@@ put this back as a debug feature !!
	/*
	for(const WeakPtrLink * pLink = m_weakLink.GetNext(); pLink != &m_weakLink; pLink = pLink->GetNext())
	{
		const WeakPtrBase * pPtr = WeakPtrBase::WeakPtrLinkData(pLink);
		ASSERT( pPtr != NULL );
		pPtr->LogRefHolder();
	}
	*/
}

#else // DO_REFCOUNTED_REF_TRACKING

void RefCounted::LogRefHolders() const
{
	// silent no-op
	//LOGERROR("DO_REFCOUNTED_REF_TRACKING is not enabled!\n");
}

void RefCounted::LogWeakPointers() const
{
	// silent no-op
	//LOGERROR("DO_REFCOUNTED_REF_TRACKING is not enabled!\n");
}

#endif // DO_REFCOUNTED_REF_TRACKING }

/**************************

NOTES :

----------------------------------------------------------------

The tricky thing in this is mixing stack instantiation
and new () instantiation.  The problem is this :
in stack instantiation the stack owns a ref; in new
instantation, the only ref is given to a smart pointer.

One solution is to get rid of new() and force it to
return a smart pointer.  This is a bit onerous for the
derived classes, but not too bad *if you can force it*.

Examples :

Stack :

	{
	gBase b; // ref = 1

	gPtr p = b; // ref = 2
	p = NULL; // ref = 1;

	// if b had not had a ref of 1 to start with,
	//	it would've been destructed by the above line !

	} //end of scope, ~b called

Non-Stack :

	{

	gPtr p = new gBase(); // ref = 2
	p = NULL; // ref = 1;

	// gBase is leaked !

	}

----------------------------------------------------------------

One of the scary exception spots is the smart-pointer factory;
in fact, we could define that this is the only thing that can
throw.  Then, it must be defined like this :

{
Smart.m_ptr = NULL
temp = Factory(); // may throw !!
Smart.m_ptr = temp; // no throw
}

The thing that scares me is that every class that the throwing
object uses must also be exception-safe.  That means that any
place where you can throw, you cannot be in the middle of something
that invalidates the class.

----------------------------------------------------------------

Thatcher :

> * Re factories.  I think constructor/factory args are a requirement.  It
> looks to me that the DECLARE_FACTORY() macro plumbing is only required
> in order to prevent unauthorized use of the class constructor to get a
> raw pointer.

Cbloom :

So all "full classes" are pure-virtual in the header, and you only get
them from a Factory() (with args) which returns a smart pointer.  I like
it.  It's better, it has the other advantages of forcing the headers to be
clean, allows args, etc.  "simple data types" (SDT/structs) won't be
ref-counted.  I start running into the problem of what whould be a full
class vs. an SDT, however, and I find that many things are in gray area.  For
example, String and Array - are they FC or SDT ?  I think library-level stuff is
kind of a special case.

Here's a question : should Factory() catch exceptions and
return NULL, or should it pass out exceptions ?

Thatcher :

Heh, this is unfamiliar ground for me.  But I'll throw out some
top-of-the-head opinions:

* I agree that you don't want to be trapping memory alloc errors all
over the place.  If malloc() goes dry, then the game goes kaput.  So no
reason to throw or catch or explicitly check for that.

* However there's often the case of a factory failing due to a missing
resource or something, e.g. Texture::Factory("some filename that doesn't
exist.bmp").  So you either do:

        TexturePtr t = Texture::Factory( "filename.bmp" );
        if ( t.IsNull() ) {
                // fallback
                t = Texture::Factory( "default.bmp" );
        }

or

        TexturePtr t;
        try {
                t = Texture::Factory( "filename.bmp" );
        }
        catch ( TextureFilenameMissing e ) {
                // fallback
                t = Texture::Factory( "default.bmp" );
        }

or just:

        // might throw; somebody else can catch or check.
        t = TextureFactor( "filename.bmp" );

features of exceptions:

1. pro: you more easily get more info about the specific error
2. pro: can defer error processing to higher level
3. pro: avoids goto in case of initializing a bunch of stuff in series
4. con: try/catch is gunky; don't want it all over the code

I think that 1 is in practice not usually very useful.  You really don't
want to be writing complex exception handlers in client code.  The
primary benefit is diagnostic, which you get from a log message anyway.

I think 2 is probably the best argument for exceptions; an unhandled
error condition is automatically a run-time error message.  However,
without exceptions, if you're using the debugger you can usually trace
these types of errors without too much trouble, because somebody
dereferences a NULL pointer.  Also, it can make coders more blase about
dealing with error conditions.  And I suspect a real higher-level error
handler that can actually recover and continue successfully is a rare
bird that does not survive long in the wild.  It takes some serious
thought and understanding of called code to code up such a thing, so I
would predict there'd be about two of them in the whole project, and at
least one of those would be buggy.

So, I think if you're serious about catching error conditions, you
actually do want to have explicit checks around everything that can
fail.  Exception people like to point out 3.  My response to that is, so
what?  goto is usually perfectly clear and justified in those cases.

So if you are going to have explicit checks everywhere, I think 4
prevails.  "if ( p.IsNull() ) {}" is far preferable to try/catch
blocks.  Clearer, shorter, harder to screw up, everybody gets it, no
deep thought required, no need to declare error types, etc.

***************************/

END_CB
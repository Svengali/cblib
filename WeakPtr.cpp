#include "Base.h"
#include "WeakPtr.h"
#include "SPtr.h"

START_CB

// test :
typedef WeakPtr<RefCounted> RefWeakPtr;

#ifdef _XBOX
#ifndef DO_REFCOUNTED_REF_TRACKING
#ifndef DEBUG_WEAKPTR_CACHE
COMPILER_ASSERT( sizeof(WeakPtrBase) == 4 );
COMPILER_ASSERT( sizeof(RefWeakPtr) == 4 );
#endif
#endif
#endif

/*static*/ int WeakPtrBase::s_numberExisting = 0;

void WeakPtr_Test()
{
	RefCounted * pRC = new RefCounted();
	RefCountedPtr spRC( pRC );
	RefWeakPtr weak;
	ASSERT( weak == NULL );
	weak = spRC;
	ASSERT( weak == pRC );
	ASSERT( weak == spRC );
	ASSERT( spRC == weak );
	spRC = NULL;
	ASSERT( weak == NULL );
}

END_CB
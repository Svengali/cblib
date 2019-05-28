#include "Win32Hooks.h"
#include "Win32Util.h"
#include "External/HookImportedFunctionByName.h"
#include "stl_basics.h"
#include "Log.h"
#include <algorithm>


START_CB

//=============================================================================================

static BOOL IsHookableDLL(HMODULE hm)
{
    if ( TRUE == IsBadReadPtr ( hm , sizeof ( IMAGE_DOS_HEADER ) ) )
    {
        return ( FALSE ) ;
    }
    
	return TRUE;
}

static int DoHookFuncs ( HookFuncDesc * pHooks, int numHooks,
						const char ** dllsToHook, int numDLLs)
{
    // Get the loaded modules.

	// only works on self :
	// this routine treats an hModule as just a memory address
	//	which is not true unless you are in the addressing space of the target process
	DWORD pid = GetCurrentProcessId();

	HMODULE modArray[256];
	UINT uiModCount = 0;
	
	//BOOL b = NT4GetLoadedModules ( 
	BOOL b = TLHELPGetLoadedModules(
									pid,
                                   ARRAY_SIZE(modArray),
                                   modArray,
                                   &uiModCount);

	if ( ! b )
	{
		return FALSE;
	}

	UINT origModCount = uiModCount;

	// modules can be in the list multiple times
	//	make sure we only do each one once :
	std::sort(modArray,modArray+uiModCount);
	HMODULE * pUniqueEnd = std::unique(modArray,modArray+uiModCount);
	uiModCount = (UINT) (pUniqueEnd - modArray);

	lprintf("Modules: %d Unique : %d\n",origModCount,uiModCount);

	STACK_ARRAY(stHooks, HOOKFUNCDESC , numHooks);
	memset(stHooks,0,numHooks*sizeof(HOOKFUNCDESC));
	
	// stupid copy from HookFuncDesc to HOOKFUNCDESC
	for(int i=0;i<numHooks;i++)
	{
		stHooks[i].szFunc = pHooks[i].pFuncName;
		stHooks[i].pProc = (PROC) pHooks[i].pProc;
		stHooks[i].pOrig = (PROC) pHooks[i].pOrig;
		stHooks[i].bHooked = pHooks[i].didHook;
	}
	
	UINT totalHooked = 0;

    // Loop through each module and hook the functions.
    for ( UINT i = 0 ; i < uiModCount ; i++ )
    {
        ASSERT( modArray[ i ] != NULL );

        // Is this a module that is hookable?
        if ( IsHookableDLL( modArray [ i ] ) )
        {
			for(int j=0;j<numDLLs;j++)
			{
	            UINT uiHooked = 0 ;

				

				HookImportedFunctionsByName( modArray [ i ]   ,
											dllsToHook[ j  ] ,
											numHooks           ,
											stHooks          ,
											&uiHooked          ) ;
			
			
				totalHooked += uiHooked;
			}
        }
    }
    
    int numHooked = 0;
    
	for(int i=0;i<numHooks;i++)
	{
		pHooks[i].pFuncName = stHooks[i].szFunc;
		pHooks[i].pProc = stHooks[i].pProc;
		pHooks[i].pOrig = stHooks[i].pOrig;
		pHooks[i].didHook = !! stHooks[i].bHooked;
		if ( stHooks[i].bHooked )
			numHooked ++;
	}
	
    lprintf("totalHooked : %d , numHooked : %d\n",totalHooked,numHooked);

    return numHooked;
}

//=============================================================================================

int InstallFuncHooks( HookFuncDesc * pHooks, int numHooks,
						const char ** dllsToHook, int numDLLs)
{

	/*		
	char ProcessName[_MAX_PATH];
	GetModuleFileName(GetModuleHandle( NULL ),ProcessName,sizeof(ProcessName) );
	char * p1 = strrchr(ProcessName,'\\');
	char * p2 = strrchr(ProcessName,'/');
	char * pName = max( p1 , p2 );
	if ( pName )
		pName++;
	else
		pName = "";
	*/
	
	// put the Orig pointer in so it can be found by pointer cuz some people don't give me names !
	
	for(int j=0;j<numDLLs;j++)
	{
		// LoadLib shouldn't actually load it, it's just to get a handle :
		HINSTANCE hDLL = LoadLibrary(dllsToHook[j]);
		
		if ( hDLL )
		{
			for(int i=0;i<numHooks;i++)
			{
				if ( pHooks[i].pFuncName != NULL && pHooks[i].pOrig == NULL )
				{
					pHooks[i].pOrig = GetProcAddress(hDLL,pHooks[i].pFuncName);
				}
			}
		
			// FreeLib just decs the ref count :	
			FreeLibrary(hDLL);
		}
	}
	
	return DoHookFuncs(pHooks,numHooks,dllsToHook,numDLLs);
}

int RemoveFuncHooks(HookFuncDesc * pHooks, int numHooks,
					 const char ** dllsToHook, int numDLLs)
{
	STACK_ARRAY(stUnHook, HookFuncDesc , numHooks);
	memset(stUnHook,0,numHooks*sizeof(HookFuncDesc));

    // Put in all the hooks.
    for ( int i = 0 ; i < numHooks ; i++ )
    {
		if ( pHooks[ i ].pOrig )
		{
			stUnHook[ i ].pFuncName = pHooks[i].pFuncName;
			stUnHook[ i ].pOrig =  pHooks[i].pProc;
			stUnHook[ i ].pProc =  pHooks[i].pOrig;
		}
    }
    
	int numDone = DoHookFuncs(stUnHook,numHooks,dllsToHook,numDLLs);

    for ( int i = 0 ; i < numHooks ; i++ )
    {
		if ( pHooks[ i ].pOrig )
		{
			//pHooks[ i ].pProc = pHooks[ i ].pOrig;
			pHooks[ i ].pOrig = NULL;
		}
    }
                                          
	return numDone;
}

END_CB

//=====================================================
#if 0

USE_CB

typedef int (CALLBACK * t_StretchDIBits) (IN HDC, IN int, IN int, IN int, IN int, IN int, IN int, IN int, IN int, IN CONST VOID *, IN CONST BITMAPINFO *, IN UINT, IN DWORD);

int CALLBACK my_StretchDIBits(
  HDC hdc,                      // handle to DC
  int XDest,                    // x-coord of destination upper-left corner
  int YDest,                    // y-coord of destination upper-left corner
  int nDestWidth,               // width of destination rectangle
  int nDestHeight,              // height of destination rectangle
  int XSrc,                     // x-coord of source upper-left corner
  int YSrc,                     // y-coord of source upper-left corner
  int nSrcWidth,                // width of source rectangle
  int nSrcHeight,               // height of source rectangle
  CONST VOID *lpBits,           // bitmap bits
  CONST BITMAPINFO *lpBitsInfo, // bitmap data
  UINT iUsage,                  // usage options
  DWORD dwRop                   // raster operation code
 );
 
static HookFuncDesc g_test_hookFuncs[ ] =
{
	{ "StretchDIBits", (void *)my_StretchDIBits, NULL },
};
const int c_test_numHooks = ARRAY_SIZE(g_test_hookFuncs);

int CALLBACK my_StretchDIBits(
  HDC hdc,                      // handle to DC
  int XDest,                    // x-coord of destination upper-left corner
  int YDest,                    // y-coord of destination upper-left corner
  int nDestWidth,               // width of destination rectangle
  int nDestHeight,              // height of destination rectangle
  int XSrc,                     // x-coord of source upper-left corner
  int YSrc,                     // y-coord of source upper-left corner
  int nSrcWidth,                // width of source rectangle
  int nSrcHeight,               // height of source rectangle
  CONST VOID *lpBits,           // bitmap bits
  CONST BITMAPINFO *lpBitsInfo, // bitmap data
  UINT iUsage,                  // usage options
  DWORD dwRop                   // raster operation code
 )
{
	lprintf("my StretchDIBits : %d %d\n",nSrcWidth,nSrcHeight);
			
	t_StretchDIBits orig_func;
	
	orig_func = (t_StretchDIBits) g_test_hookFuncs[ 0 ].pOrig;

	if ( orig_func )
	{
		int ret = (*orig_func) (hdc,XDest,YDest,nDestWidth,nDestHeight,XSrc,YSrc,nSrcWidth,nSrcHeight,lpBits,lpBitsInfo,iUsage,dwRop);
		
		return ret;
	}
	else
	{
		return 0;
	}
}


static const char * g_whereToHook[] =
{
    "gdi32.dll"
} ;
const int c_test_numHookPlaces = ARRAY_SIZE(g_whereToHook);


void TestHooks()
{
	InstallFuncHooks(g_test_hookFuncs,c_test_numHooks,g_whereToHook,c_test_numHookPlaces);
	
	StretchDIBits(0,0,0,0,0,0,0,0,0,0,0,0,0);
	
	RemoveFuncHooks(g_test_hookFuncs,c_test_numHooks,g_whereToHook,c_test_numHookPlaces);
}

#endif
//===========================================================

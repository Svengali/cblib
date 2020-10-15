
/*----------------------------------------------------------------------
       John Robbins - Microsoft Systems Journal Bugslayer Column
----------------------------------------------------------------------*/

/*//////////////////////////////////////////////////////////////////////
                                Includes
//////////////////////////////////////////////////////////////////////*/
#define WINVER 0x0501
#include "HookImportedFunctionByName.h"
//       "HookImportedFunctionsByName.h"
#include "../Base.h"
#include "../Log.h"

/*//////////////////////////////////////////////////////////////////////
                         File Specific Defines
//////////////////////////////////////////////////////////////////////*/
#define MakePtr( cast , ptr , AddValue ) \
                                 (cast)( (char *)(ptr)+(DWORD)(AddValue))

/*//////////////////////////////////////////////////////////////////////
                        File Specific Prototypes
//////////////////////////////////////////////////////////////////////*/
static PIMAGE_IMPORT_DESCRIPTOR
                     GetNamedImportDescriptor ( HMODULE hModule     ,
                                                LPCSTR  szImportMod  ) ;

static void HookImportedFunctionsByNameSub (  HMODULE         hModule     ,
                                       LPCSTR          szImportMod ,
                                       PIMAGE_IMPORT_DESCRIPTOR pImportDesc,
                                       UINT            uiCount     ,
                                       HOOKFUNCDESCA * paHookArray ,
                                       LPUINT          puiHooked    );

static void HookImportedFunctionsByPointerSub (  HMODULE         hModule     ,
                                       LPCSTR          szImportMod ,
                                       PIMAGE_IMPORT_DESCRIPTOR pImportDesc,
                                       UINT            uiCount     ,
                                       HOOKFUNCDESCA * paHookArray ,
                                       LPUINT          puiHooked    );

#if 0
// this is bogus junk you don't need once the image is loaded into memory :

PVOID  WINAPI ImageDirectoryOffset (
		PIMAGE_DOS_HEADER pDOSHeader,
		PIMAGE_NT_HEADERS pNTHeader,
        DWORD     dwIMAGE_DIRECTORY)
{
    PIMAGE_OPTIONAL_HEADER   poh;
    PIMAGE_SECTION_HEADER    psh;
    int                      nSections = pNTHeader->FileHeader.NumberOfSections;
    int                      i = 0;
    LPVOID                   VAImageDir;

    /* Must be 0 thru (NumberOfRvaAndSizes-1). */
    if (dwIMAGE_DIRECTORY >= pNTHeader->OptionalHeader.NumberOfRvaAndSizes)
        return NULL;

    /* Retrieve offsets to optional and section headers. */
    poh = (PIMAGE_OPTIONAL_HEADER) &(pNTHeader->OptionalHeader);
    psh = IMAGE_FIRST_SECTION(pNTHeader);

    /* Locate image directory's relative virtual address. */
    VAImageDir = (LPVOID)poh->DataDirectory
                       [dwIMAGE_DIRECTORY].VirtualAddress;

    /* Locate section containing image directory. */
    while (i++<nSections)
        {
        if (psh->VirtualAddress <= (DWORD)VAImageDir &&
            psh->VirtualAddress + 
                 psh->SizeOfRawData > (DWORD)VAImageDir)
            break;
        psh++;
        }

    if (i > nSections)
        return NULL;

	int offset = (int)psh->PointerToRawData - (int)psh->VirtualAddress;

    /* Return image import directory offset. */
    return (LPVOID)((int)pDOSHeader + 
                     (int)VAImageDir +
                     offset);
}
           
PIMAGE_SECTION_HEADER WINAPI GetSectionHdrByName (
		PIMAGE_NT_HEADERS pNTHeader,
	    char *szSection)
{
	int nSections = pNTHeader->FileHeader.NumberOfSections;
	PIMAGE_SECTION_HEADER psh = IMAGE_FIRST_SECTION(pNTHeader);

	/* find the section by name */
	for (int i=0; i<nSections; i++, psh++)
	{
		if ( stricmp((char *)psh->Name, szSection) == 0 )
		{
			return psh;
		}
	}

	return NULL;
}
#endif
                            
/*//////////////////////////////////////////////////////////////////////
                             Implementation
//////////////////////////////////////////////////////////////////////*/

BOOL BUGSUTIL_DLLINTERFACE
        HookImportedFunctionsByName ( HMODULE         hModule     ,
                                       LPCSTR          szImportMod ,
                                       UINT            uiCount     ,
                                       HOOKFUNCDESCA  * paHookArray ,
                                       LPUINT          puiHooked    )
{
    // Double check the parameters.
    ASSERT ( NULL != szImportMod ) ;
    ASSERT ( 0 != uiCount ) ;
    ASSERT ( FALSE == IsBadReadPtr ( paHookArray ,
                                     sizeof (HOOKFUNCDESC) * uiCount ));
    ASSERT ( FALSE == IsBadWritePtr ( puiHooked , sizeof ( UINT ) ) ) ;
#ifdef _DEBUG
    // Check each function name in the hook array.
    {
        for ( UINT i = 0 ; i < uiCount ; i++ )
        {
            // If the proc is not NULL, then it is checked.
            if ( NULL != paHookArray[ i ].pProc )
            {
                ASSERT ( FALSE == IsBadCodePtr ( paHookArray[i].pProc));
            }
        }
    }
#endif
    // Do the parameter validation for real.
    if ( ( 0    == uiCount      )                                  ||
         ( NULL == szImportMod  )                                  ||
         ( TRUE == IsBadWritePtr ( puiHooked , sizeof ( UINT ) ) ) ||
         ( TRUE == IsBadReadPtr ( paHookArray ,
                                  sizeof (HOOKFUNCDESC) * uiCount ) ) )
    {
        SetLastErrorEx ( ERROR_INVALID_PARAMETER , SLE_ERROR ) ;
        return ( FALSE ) ;
    }

    // TODO TODO
    //  Should each item in the hook array be checked in release builds?

    // Set the number of functions hooked to zero.
    *puiHooked = 0 ;

    // Get the specific import descriptor.
    /*
    PIMAGE_IMPORT_DESCRIPTOR pImportDesc =
                     GetNamedImportDescriptor ( hModule , szImportMod );
    if ( NULL == pImportDesc )
    {
        // The requested module was not imported.
        return ( FALSE ) ;
    }
    */

	// Go over ALL import headers

    PIMAGE_DOS_HEADER pDOSHeader = (PIMAGE_DOS_HEADER)hModule ;

    // Is this the MZ header?
    if ( ( TRUE == IsBadReadPtr ( pDOSHeader                  ,
                                 sizeof ( IMAGE_DOS_HEADER )  ) ) ||
         ( IMAGE_DOS_SIGNATURE != pDOSHeader->e_magic           )   )
    {
        ASSERT ( FALSE ) ;
        SetLastErrorEx ( ERROR_INVALID_PARAMETER , SLE_ERROR ) ;
        return ( FALSE ) ;
    }

    // Get the PE header.
    PIMAGE_NT_HEADERS pNTHeader = MakePtr ( PIMAGE_NT_HEADERS       ,
                                            pDOSHeader              ,
                                            pDOSHeader->e_lfanew     ) ;

    // Is this a real PE image?
    if ( ( TRUE == IsBadReadPtr ( pNTHeader ,
                                  sizeof ( IMAGE_NT_HEADERS ) ) ) ||
         ( IMAGE_NT_SIGNATURE != pNTHeader->Signature           )   )
    {
        ASSERT ( FALSE ) ;
        SetLastErrorEx ( ERROR_INVALID_PARAMETER , SLE_ERROR ) ;
        return ( FALSE ) ;
    }

    // Get the pointer to the imports section.
    //*
    PIMAGE_IMPORT_DESCRIPTOR pImportDesc
     = MakePtr ( PIMAGE_IMPORT_DESCRIPTOR ,
                 pDOSHeader               ,
                 pNTHeader->OptionalHeader.
                         DataDirectory[ IMAGE_DIRECTORY_ENTRY_IMPORT ].
                                                      VirtualAddress ) ;
	/**/
	//PIMAGE_IMPORT_DESCRIPTOR pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR) ImageDirectoryOffset(pDOSHeader,pNTHeader,IMAGE_DIRECTORY_ENTRY_IMPORT);
	
    // Does it have an imports section?
    if ( 0 == pImportDesc )
    {
        return ( FALSE ) ;
    }
    
    // Loop through the import module descriptors looking for the
    //  module whose name matches szImportMod.
    for ( ; NULL != pImportDesc->Name ;  pImportDesc++ )
    {
		if ( pImportDesc->Name != 65535 )
		{
			PSTR szCurrMod = MakePtr ( PSTR              ,
									pDOSHeader        ,
									pImportDesc->Name  ) ;
			if ( ! IsBadStringPtr(szCurrMod,strlen(szImportMod)) )
			{
				if ( 0 == stricmp ( szCurrMod , szImportMod ) )
				{
					if ( pImportDesc->OriginalFirstThunk == 0 )
					{
						HookImportedFunctionsByPointerSub(hModule,szImportMod,pImportDesc,uiCount,paHookArray,puiHooked);
					}
					else
					{
						HookImportedFunctionsByNameSub(hModule,szImportMod,pImportDesc,uiCount,paHookArray,puiHooked);
					}
				}
			}
		}       
    }

    // All OK, JumpMaster!
    SetLastError ( ERROR_SUCCESS ) ;
    return ( TRUE ) ;
}          

/*
DWORD GetImportDataBase( HMODULE hModule, PIMAGE_IMPORT_DESCRIPTOR pImportDesc )
{

    PIMAGE_DOS_HEADER pDOSHeader = (PIMAGE_DOS_HEADER)hModule ;

    // Get the PE header.
    PIMAGE_NT_HEADERS pNTHeader = MakePtr ( PIMAGE_NT_HEADERS       ,
                                            pDOSHeader              ,
                                            pDOSHeader->e_lfanew     ) ;

    // Get the pointer to the imports section.

	DWORD VA = pNTHeader->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_IMPORT ].VirtualAddress;

    PIMAGE_IMPORT_DESCRIPTOR pImportDesc1 = MakePtr ( PIMAGE_IMPORT_DESCRIPTOR ,
                 pDOSHeader               ,
                 VA ) ;

	//PIMAGE_IMPORT_DESCRIPTOR pImportDesc2 = (PIMAGE_IMPORT_DESCRIPTOR) ImageDirectoryOffset(pDOSHeader,pNTHeader,IMAGE_DIRECTORY_ENTRY_IMPORT);
	//int offset = (int)pImportDesc1 - (int)pImportDesc2;
	
	PIMAGE_SECTION_HEADER pIData = GetSectionHdrByName(pNTHeader,".idata");

	int offset = 0;
	if ( pIData )
	{
		offset = VA - pIData->VirtualAddress;
	}
	
	//---------------------------------------------------------------

    PIMAGE_THUNK_DATA pOrigThunk =
                        MakePtr ( PIMAGE_THUNK_DATA       ,
                                  hModule                 ,
                                  pImportDesc->OriginalFirstThunk  ) ;

    // Look get the name of this imported function.
    PIMAGE_IMPORT_BY_NAME pFirstByName = MakePtr ( PIMAGE_IMPORT_BY_NAME    ,
                        hModule                  ,
                        pOrigThunk->u1.AddressOfData  ) ;
                        
	//---------------------------------------------------------------

	return (DWORD)hModule;
}
*/

/*

c:\src\GoldBullion\GBHookDLL\HookImportedFunctionByName.cpp(180) : warning C4311: 'type cast' : pointer truncation from 'PROC' to 'DWORD'
c:\src\GoldBullion\GBHookDLL\HookImportedFunctionByName.cpp(187) : warning C4312: 'type cast' : conversion from 'DWORD' to 'PROC' of greater size
c:\src\GoldBullion\GBHookDLL\HookImportedFunctionByName.cpp(189) : warning C4311: 'type cast' : pointer truncation from 'PROC' to 'DWORD'
c:\src\GoldBullion\GBHookDLL\HookImportedFunctionByName.cpp(190) : warning C4311: 'type cast' : pointer truncation from 'PROC' to 'DWORD'
c:\src\GoldBullion\GBHookDLL\HookImportedFunctionByName.cpp(192) : warning C4312: 'type cast' : conversion from 'DWORD' to 'PROC' of greater size
c:\src\GoldBullion\GBHookDLL\HookImportedFunctionByName.cpp(193) : warning C4311: 'type cast' : pointer truncation from 'PROC' to 'DWORD'

*/
#pragma warning(disable : 4311)
#pragma warning(disable : 4312)

BOOL myIsBadReadPtr( CONST VOID *lp, UINT_PTR ucb )
{
	__try
	{
		if ( IsBadReadPtr(lp,ucb) )
		{
			return true;
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		return true;
	}
	return false;
}
			
void HookImportedFunctionsByNameSub (	HMODULE         hModule     ,
                                       LPCSTR          szImportMod ,
                                       PIMAGE_IMPORT_DESCRIPTOR pImportDesc,
                                       UINT            uiCount     ,
                                       HOOKFUNCDESCA * paHookArray ,
                                       LPUINT          puiHooked    )
{
	DWORD dwBase = (DWORD) hModule;
	//DWORD dwBase = GetImportDataBase(hModule,pImportDesc);

    // Get the original thunk information for this DLL.  I cannot use
    //  the thunk information stored in the pImportDesc->FirstThunk
    //  because the that is the array that the loader has already
    //  bashed to fix up all the imports.  This pointer gives us acess
    //  to the function names.
    PIMAGE_THUNK_DATA pOrigThunk =
                        MakePtr ( PIMAGE_THUNK_DATA       ,
                                  dwBase                 ,
                                  pImportDesc->OriginalFirstThunk  ) ;
    // Get the array pointed to by the pImportDesc->FirstThunk.  This is
    //  where I will do the actual bash.
    PIMAGE_THUNK_DATA pRealThunk = MakePtr ( PIMAGE_THUNK_DATA       ,
                                             hModule                 ,
                                             pImportDesc->FirstThunk  );

    // Look get the name of this imported function.
    /*
    PIMAGE_IMPORT_BY_NAME pFirstByName ;

    pFirstByName = MakePtr ( PIMAGE_IMPORT_BY_NAME    ,
                        dwBase                  ,
                        pOrigThunk->u1.AddressOfData  ) ;

	MEMORY_BASIC_INFORMATION orig_mbi;
	VirtualQuery((LPCVOID)pFirstByName,&orig_mbi,sizeof(MEMORY_BASIC_INFORMATION));
	
    DWORD dwOldProtect ;
    VERIFY ( VirtualProtect ( orig_mbi.BaseAddress ,
                                orig_mbi.RegionSize  ,
                                //orig_mbi.AllocationProtect ,
                                PAGE_EXECUTE_READ,
                                &dwOldProtect) ) ;
	*/
	
    // Loop through and look for the one that matches the name.
    for ( ;
		NULL != pOrigThunk->u1.Function ;    
        pOrigThunk++ ,
        pRealThunk++ )
    {
        // Only look at those that are imported by name, not ordinal.
        if (  IMAGE_ORDINAL_FLAG !=
                        ( pOrigThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG ))
        {
            // Look get the name of this imported function.
            PIMAGE_IMPORT_BY_NAME pByName ;

            pByName = MakePtr ( PIMAGE_IMPORT_BY_NAME    ,
                                dwBase                  ,
                                pOrigThunk->u1.AddressOfData  ) ;

			//MEMORY_BASIC_INFORMATION mbi;
			//VirtualQuery((LPCVOID)pByName,&mbi,sizeof(MEMORY_BASIC_INFORMATION));
	
			if ( myIsBadReadPtr(pByName,sizeof(IMAGE_IMPORT_BY_NAME)) )
			{
				continue ;
			}

            // If the name starts with NULL, then just skip out now.
            if ( 0 == pByName->Name[ 0 ] )
            {
                continue ;
            }

            // Determines if we do the hook.
            BOOL bDoHook = FALSE ;
            UINT iHook = 0;

            // TODO TODO
            //  Might want to consider bsearch here.

            // See if the particular function name is in the import
            //  list.  It might be good to consider requiring the
            //  paHookArray to be in sorted order so bsearch could be
            //  used so the lookup will be faster.  However, the size of
            //  uiCount coming into this function should be rather
            //  small but it is called for each function imported by
            //  szImportMod.
            for ( iHook = 0 ; iHook < uiCount ; iHook++ )
            {
                // If the proc is NULL, kick out
				if ( paHookArray[iHook].szFunc == NULL ||
					 paHookArray[iHook].pProc == NULL )
				{
					continue;
				}
				
                if ( ( toupper(paHookArray[iHook].szFunc[0]) ==
                       toupper(pByName->Name[0]) ) &&
                     ( 0 == stricmp ( paHookArray[iHook].szFunc ,
                                      (char*)pByName->Name   )   )    )
                {
                    bDoHook = TRUE ;
                    break ;
                }
            }
            
            //HOOKFUNCDESCA & hookDesc = paHookArray[ iHook ];

            if ( bDoHook )
            {            
                // I found it.  Now I need to change the protection to
                //  writable before I do the blast.  Note that I am now
                //  blasting into the real thunk area!
                MEMORY_BASIC_INFORMATION mbi_thunk ;

                VirtualQuery ( pRealThunk                          ,
                               &mbi_thunk                          ,
                               sizeof ( MEMORY_BASIC_INFORMATION )  ) ;

                //VERIFY ( 
                VirtualProtect ( mbi_thunk.BaseAddress ,
                                          mbi_thunk.RegionSize  ,
                                          PAGE_READWRITE        ,
                                          &mbi_thunk.Protect     );

                // Do the actual hook saving the original address to
                //  return it.
                if ( pRealThunk->u1.Function == (DWORD)paHookArray[iHook].pProc )
                {
					// it's already been done ! shouldn't really get here, but w/e
					lprintf("Func In Thunk already hooked : %08X\n", (DWORD)pRealThunk->u1.Function );
                }
                else
                {                
					ASSERT( paHookArray[iHook].pOrig == NULL || paHookArray[iHook].pOrig == (PROC)pRealThunk->u1.Function );

					lprintf("%s : ", paHookArray[iHook].szFunc );
					lprintf("Thunk: %08X", (DWORD)pRealThunk->u1.Function );
					lprintf(" New Func: %08X", (DWORD)paHookArray[iHook].pProc );
					lprintf(" Orig Was: %08X\n", (DWORD)paHookArray[iHook].pOrig );
					
					paHookArray[iHook].bHooked = TRUE;
					paHookArray[iHook].pOrig = (PROC)pRealThunk->u1.Function ;
					pRealThunk->u1.Function = (DWORD)paHookArray[iHook].pProc ;
				}
				
                DWORD dwOldProtect ;

                // Change the protection back to what it was before I
                //  blasted.
                VirtualProtect ( mbi_thunk.BaseAddress ,
                                          mbi_thunk.RegionSize  ,
                                          mbi_thunk.Protect     ,
                                          &dwOldProtect          );

                // Increment the total number hooked.
                *puiHooked += 1 ;
            }
        }
        else
        {
			/*
			// ordinal import ?
			DWORD ord = pOrigThunk->u1.Ordinal;
			if ( stricmp(szImportMod,"gdi32.dll") == 0 )
			{
				int i = 1;
			}
			*/			
        }
    }
    
    // Change the protection back to what it was before I
    //  blasted.
    /*
    VERIFY ( VirtualProtect ( orig_mbi.BaseAddress ,
                                orig_mbi.RegionSize  ,
                                orig_mbi.Protect     ,
                                &dwOldProtect          ) ) ;
	*/
}         

/**

6-27-07 : HookImportedFunctionsByPointerSub

Cake.exe doesn't have the Orig Thunk at all, it's set to zero

So we can't find the names (they are in there and maybe I can find them but this is easier)

Instead just look in the Bound IAT (Import Address Table) for the function by pointer and change it to mine

In fact this is a general technique that I could juse use all the time if I wanted and fuck the names

**/

void HookImportedFunctionsByPointerSub (HMODULE         hModule     ,
                                       LPCSTR          szImportMod ,
                                       PIMAGE_IMPORT_DESCRIPTOR pImportDesc,
                                       UINT            uiCount     ,
                                       HOOKFUNCDESCA * paHookArray ,
                                       LPUINT          puiHooked    )
{
	//DWORD dwBase = (DWORD) hModule;

    // Get the array pointed to by the pImportDesc->FirstThunk.  This is
    //  where I will do the actual bash.
    PIMAGE_THUNK_DATA pRealThunk = MakePtr ( PIMAGE_THUNK_DATA       ,
                                             hModule                 ,
                                             pImportDesc->FirstThunk  );

    // Loop through and look for the one that matches the name.
    for ( ;
		NULL != pRealThunk->u1.Function ;  
        pRealThunk++ )
    {
        // Only look at those that are imported by name, not ordinal.

        // Determines if we do the hook.
        BOOL bDoHook = FALSE ;
        UINT iHook = 0;

        for ( iHook = 0 ; iHook < uiCount ; iHook++ )
        {
            // If the proc is NULL, kick out
			if ( pRealThunk->u1.Function == (DWORD) paHookArray[iHook].pOrig )
            {
                bDoHook = TRUE ;
                break ;
            }
        }
            
        if ( bDoHook )
        {            
            // I found it.  Now I need to change the protection to
            //  writable before I do the blast.  Note that I am now
            //  blasting into the real thunk area!
            MEMORY_BASIC_INFORMATION mbi_thunk ;

            VirtualQuery ( pRealThunk                          ,
                            &mbi_thunk                          ,
                            sizeof ( MEMORY_BASIC_INFORMATION )  ) ;

            VirtualProtect ( mbi_thunk.BaseAddress ,
                                        mbi_thunk.RegionSize  ,
                                        PAGE_READWRITE        ,
                                        &mbi_thunk.Protect     );

            // Do the actual hook saving the original address to
            //  return it.

			// Hooking by pointer is not as safe as hooking by name,
			//	I can't do the same nice checks :

			lprintf("%s: \n\t", paHookArray[iHook].szFunc );
			lprintf("Pointer In Thunk : %08X", (DWORD)pRealThunk->u1.Function );
			lprintf("  New Func : %08X \n", (DWORD)paHookArray[iHook].pProc );
				
			// pOrig should be already saved, that's how I found this !
			//paOrigFuncs[iHook] = (PROC)pRealThunk->u1.Function ;
			
			pRealThunk->u1.Function = (DWORD)paHookArray[iHook].pProc ;

			paHookArray[iHook].bHooked = TRUE;
					
            // Change the protection back to what it was before I
            //  blasted.
            DWORD dwOldProtect ;
            VirtualProtect ( mbi_thunk.BaseAddress ,
                                        mbi_thunk.RegionSize  ,
                                        mbi_thunk.Protect     ,
                                        &dwOldProtect          );

            // Increment the total number hooked.
            *puiHooked += 1 ;
        }
    }
}         

/*----------------------------------------------------------------------
FUNCTION        :   GetNamedImportDescriptor
DISCUSSION      :
    Gets the import descriptor for the requested module.  If the module
is not imported in hModule, NULL is returned.
    This is a potential useful function in the future.
PARAMETERS      :
    hModule      - The module to hook in.
    szImportMod  - The module name to get the import descriptor for.
RETURNS         :
    NULL  - The module was not imported or hModule is invalid.
    !NULL - The import descriptor.
----------------------------------------------------------------------*/
static PIMAGE_IMPORT_DESCRIPTOR
                     GetNamedImportDescriptor ( HMODULE hModule     ,
                                                LPCSTR  szImportMod  )
{
    // Always check parameters.
    ASSERT ( NULL != szImportMod ) ;
    ASSERT ( NULL != hModule     ) ;
    if ( ( NULL == szImportMod ) || ( NULL == hModule ) )
    {
        SetLastErrorEx ( ERROR_INVALID_PARAMETER , SLE_ERROR ) ;
        return ( NULL ) ;
    }

    PIMAGE_DOS_HEADER pDOSHeader = (PIMAGE_DOS_HEADER)hModule ;

    // Is this the MZ header?
    if ( ( TRUE == IsBadReadPtr ( pDOSHeader                  ,
                                 sizeof ( IMAGE_DOS_HEADER )  ) ) ||
         ( IMAGE_DOS_SIGNATURE != pDOSHeader->e_magic           )   )
    {
        ASSERT ( FALSE ) ;
        SetLastErrorEx ( ERROR_INVALID_PARAMETER , SLE_ERROR ) ;
        return ( NULL ) ;
    }

    // Get the PE header.
    PIMAGE_NT_HEADERS pNTHeader = MakePtr ( PIMAGE_NT_HEADERS       ,
                                            pDOSHeader              ,
                                            pDOSHeader->e_lfanew     ) ;

    // Is this a real PE image?
    if ( ( TRUE == IsBadReadPtr ( pNTHeader ,
                                  sizeof ( IMAGE_NT_HEADERS ) ) ) ||
         ( IMAGE_NT_SIGNATURE != pNTHeader->Signature           )   )
    {
        ASSERT ( FALSE ) ;
        SetLastErrorEx ( ERROR_INVALID_PARAMETER , SLE_ERROR ) ;
        return ( NULL ) ;
    }

    // Get the pointer to the imports section.
    PIMAGE_IMPORT_DESCRIPTOR pImportDesc
     = MakePtr ( PIMAGE_IMPORT_DESCRIPTOR ,
                 pDOSHeader               ,
                 pNTHeader->OptionalHeader.
                         DataDirectory[ IMAGE_DIRECTORY_ENTRY_IMPORT ].
                                                      VirtualAddress ) ;
    // Does it have an imports section?
    if ( 0 == pImportDesc )
    {
        return ( NULL ) ;
    }
    // Loop through the import module descriptors looking for the
    //  module whose name matches szImportMod.
    while ( NULL != pImportDesc->Name )
    {
		if ( pImportDesc->Name != 65535 )
		{
			PSTR szCurrMod = MakePtr ( PSTR              ,
									pDOSHeader        ,
									pImportDesc->Name  ) ;
			if ( ! IsBadStringPtr(szCurrMod,strlen(szImportMod)) )
			{
				if ( 0 == stricmp ( szCurrMod , szImportMod ) )
				{
					// Found it.
					break ;
				}
			}
		}
		
        // Look at the next one.
        pImportDesc++ ;
    }

    // If the name is NULL, then the module is not imported.
    if ( NULL == pImportDesc->Name )
    {
        return ( NULL ) ;
    }

    // All OK, Jumpmaster!
    return ( pImportDesc ) ;

}



#include "TLHELP32.h"

/*//////////////////////////////////////////////////////////////////////
                                Typedefs
//////////////////////////////////////////////////////////////////////*/
// The typedefs for the TOOLHELP32.DLL functions used by this module.
// Type definitions for pointers to call tool help functions.
typedef BOOL (WINAPI *MODULEWALK) ( HANDLE          hSnapshot ,
                                    LPMODULEENTRY32 lpme       ) ;
typedef BOOL (WINAPI *THREADWALK) ( HANDLE          hSnapshot ,
                                    LPTHREADENTRY32 lpte       ) ;
typedef BOOL (WINAPI *PROCESSWALK) ( HANDLE           hSnapshot ,
                                     LPPROCESSENTRY32 lppe       ) ;
typedef HANDLE (WINAPI *CREATESNAPSHOT) ( DWORD dwFlags       ,
                                          DWORD th32ProcessID  ) ;


/*//////////////////////////////////////////////////////////////////////
                            File Static Data
//////////////////////////////////////////////////////////////////////*/
// Has the function stuff here been initialized?  This is only to be
//  used by the InitPSAPI function and nothing else.
static BOOL           g_bInitialized              = FALSE ;
static CREATESNAPSHOT g_pCreateToolhelp32Snapshot = NULL  ;
static MODULEWALK     g_pModule32First            = NULL  ;
static MODULEWALK     g_pModule32Next             = NULL  ;
static PROCESSWALK    g_pProcess32First           = NULL  ;
static PROCESSWALK    g_pProcess32Next            = NULL  ;
static THREADWALK     g_pThread32First            = NULL  ;
static THREADWALK     g_pThread32Next             = NULL  ;

/*----------------------------------------------------------------------
FUNCTION        :   InitTOOLHELP32
DISCUSSION      :
    Retrieves the function pointers needed to access the tool help
routines.  Since this code is supposed to work on NT4, it cannot link
to the non-existant addresses in KERNEL32.DLL.
    This is pretty much lifter right from the MSDN.
PARAMETERS      :
    None.
RETURNS         :
    TRUE  - Everything initialized properly.
    FALSE - There was a problem.
----------------------------------------------------------------------*/
static BOOL InitTOOLHELP32 ( void )
{
    if ( TRUE == g_bInitialized )
    {
        return ( TRUE ) ;
    }

    BOOL      bRet    = FALSE ;
    HINSTANCE hKernel = NULL  ;

    // Obtain the module handle of the kernel to retrieve addresses of
    //  the tool helper functions.
    hKernel = GetModuleHandleA ( "KERNEL32.DLL" ) ;
    ASSERT ( NULL != hKernel ) ;

    if ( NULL != hKernel )
    {
        g_pCreateToolhelp32Snapshot =
           (CREATESNAPSHOT)GetProcAddress ( hKernel ,
                                            "CreateToolhelp32Snapshot");
        ASSERT ( NULL != g_pCreateToolhelp32Snapshot ) ;

        g_pModule32First = (MODULEWALK)GetProcAddress (hKernel ,
                                                       "Module32First");
        ASSERT ( NULL != g_pModule32First ) ;

        g_pModule32Next = (MODULEWALK)GetProcAddress (hKernel        ,
                                                      "Module32Next"  );
        ASSERT ( NULL != g_pModule32Next ) ;

        g_pProcess32First =
                (PROCESSWALK)GetProcAddress ( hKernel          ,
                                              "Process32First"  ) ;
        ASSERT ( NULL != g_pProcess32First ) ;

        g_pProcess32Next =
                (PROCESSWALK)GetProcAddress ( hKernel         ,
                                              "Process32Next" ) ;
        ASSERT ( NULL != g_pProcess32Next ) ;

        g_pThread32First =
                (THREADWALK)GetProcAddress ( hKernel         ,
                                             "Thread32First"  ) ;
        ASSERT ( NULL != g_pThread32First ) ;

        g_pThread32Next =
                (THREADWALK)GetProcAddress ( hKernel        ,
                                             "Thread32Next"  ) ;
        ASSERT ( NULL != g_pThread32Next ) ;

        // All addresses must be non-NULL to be successful.  If one of
        //  these addresses is NULL, one of the needed lists cannot be
        //  walked.

        bRet =  g_pModule32First            &&
                g_pModule32Next             &&
                g_pProcess32First           &&
                g_pProcess32Next            &&
                g_pThread32First            &&
                g_pThread32Next             &&
                g_pCreateToolhelp32Snapshot    ;
    }
    else
    {
        // Could not get the module handle of kernel.
        SetLastErrorEx ( ERROR_DLL_INIT_FAILED , SLE_ERROR ) ;
        bRet = FALSE ;
    }

    ASSERT ( TRUE == bRet ) ;

    if ( TRUE == bRet )
    {
        // All OK, Jumpmaster!
        g_bInitialized = TRUE ;
    }
    return ( bRet ) ;
}

/*----------------------------------------------------------------------
FUNCTION        :   TLHELPGetLoadedModules
DISCUSSION      :
    The TOOLHELP32 specific version of GetLoadedModules.  This function
assumes that GetLoadedModules does the work to validate the parameters.
PARAMETERS      :
    dwPID        - The process ID to look into.
    uiCount      - The number of slots in the paModArray buffer.  If
                   this value is 0, then the return value will be TRUE
                   and puiRealCount will hold the number of items
                   needed.
    paModArray   - The array to place the HMODULES into.  If this buffer
                   is too small to hold the result and uiCount is not
                   zero, then FALSE is returned, but puiRealCount will
                   be the real number of items needed.
    puiRealCount - The count of items needed in paModArray, if uiCount
                   is zero, or the real number of items in paModArray.
RETURNS         :
    FALSE - There was a problem, check GetLastError.
    TRUE  - The function succeeded.  See the parameter discussion for
            the output parameters.
----------------------------------------------------------------------*/
BOOL BUGSUTIL_DLLINTERFACE TLHELPGetLoadedModules ( DWORD     dwPID        ,
                              UINT      uiCount      ,
                              HMODULE * paModArray   ,
                              LPUINT    puiRealCount   )
{

    // Always set puiRealCount to a know value before anything else.
    *puiRealCount = 0 ;

    if ( FALSE == InitTOOLHELP32 ( ) )
    {
        ASSERT ( FALSE ) ;
        //SetLastErrorEx ( ERROR_DLL_INIT_FAILED , SLE_ERROR ) ;
        return ( FALSE ) ;
    }

    // The snapshot handle.
    HANDLE        hModSnap     = NULL ;
    // The module structure.
    MODULEENTRY32 stME32              ;
    // The return value for the function.
    BOOL          bRet         = TRUE ;
    // A flag kept to report if the buffer was too small.
    BOOL          bBuffToSmall = FALSE ;


    // Get the snapshot for the specified process.
    hModSnap = g_pCreateToolhelp32Snapshot ( TH32CS_SNAPMODULE ,
                                             dwPID              ) ;
    ASSERT ( INVALID_HANDLE_VALUE != hModSnap ) ;
    if ( INVALID_HANDLE_VALUE == hModSnap )
    {
    //    TRACE1 ( "Unable to get module snapshot for %08X\n" , dwPID ) ;
        return ( FALSE ) ;
    }

    memset ( &stME32 , NULL , sizeof ( MODULEENTRY32 ) ) ;
    stME32.dwSize = sizeof ( MODULEENTRY32 ) ;

    // Start getting the module values.
    if ( TRUE == g_pModule32First ( hModSnap , &stME32 ) )
    {
        do
        {
            // If uiCount is not zero, copy values.
            if ( 0 != uiCount )
            {
                // If the passed in buffer is to small, set the flag.
                //  This is so we match the functionality of the NT4
                //  version of this function which will return the
                //  correct total needed.
                if ( ( TRUE == bBuffToSmall     ) ||
                     ( *puiRealCount == uiCount )   )
                {
                    bBuffToSmall = TRUE ;
                    break ;
                }
                else
                {
                    // Copy this value in.
                    paModArray[ *puiRealCount ] =
                                         (HINSTANCE)stME32.modBaseAddr ;
                }
            }
            // Bump up the real total count.
            *puiRealCount += 1 ;
        }
        while ( ( TRUE == g_pModule32Next ( hModSnap , &stME32 ) ) ) ;
    }
    else
    {
        ASSERT ( FALSE ) ;
   //     TRACE0 ( "Failed to get first module!\n" ) ;
        bRet = FALSE ;
    }

    // Close the snapshot handle.
    CloseHandle ( hModSnap ) ;

    // Check if the buffer was too small.
    if ( TRUE == bBuffToSmall )
    {
        ASSERT ( FALSE ) ;
   //     TRACE0 ( "Buffer to small in TLHELPGetLoadedModules\n" ) ;
        //SetLastErrorEx ( ERROR_INSUFFICIENT_BUFFER , SLE_ERROR ) ;
        bRet = FALSE ;
    }

    // All OK, Jumpmaster!
    //SetLastError ( ERROR_SUCCESS ) ;
    return ( bRet ) ;
}


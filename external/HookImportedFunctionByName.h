#pragma once

/*----------------------------------------------------------------------
       John Robbins - Microsoft Systems Journal Bugslayer Column
------------------------------------------------------------------------

The BugslayerUtil.DLL file contains various routines that are useful
across various projects, not just those for a single column.  I will
be adding to the DLL in future columns.

HISTORY     :

FEB '98     - Initial version.  This also incorporated the October '97
              memory dumper and validator library.

----------------------------------------------------------------------*/

#ifndef _BUGSLAYERUTIL_H
#define _BUGSLAYERUTIL_H

/*//////////////////////////////////////////////////////////////////////
                                Includes
//////////////////////////////////////////////////////////////////////*/
// If windows.h has not yet been included, include it now.
#ifndef _INC_WINDOWS
#include <windows.h>
#endif

/*//////////////////////////////////////////////////////////////////////
                            Special Defines
//////////////////////////////////////////////////////////////////////*/
// The defines that set up how the functions or classes are exported or
//  imported.
#define BUGSUTIL_DLLINTERFACE 

/*//////////////////////////////////////////////////////////////////////
                            Special Includes
//////////////////////////////////////////////////////////////////////*/
// Include the headers that do the memory dumping and validation
//  routines.
//#include "MemDumperValidator.h"


/*//////////////////////////////////////////////////////////////////////
                      C Function Declaration Area
                                 START
//////////////////////////////////////////////////////////////////////*/
#ifdef __cplusplus
extern "C" {
#endif  // _cplusplus

/*----------------------------------------------------------------------
FUNCTION        :   GetLoadedModules
DISCUSSION      :
    For the specified process id, this function returns the HMODULES for
all modules loaded into that process address space.  This function works
for both NT and 95.
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
BOOL BUGSUTIL_DLLINTERFACE NT4GetLoadedModules ( DWORD     dwPID        ,
                           UINT      uiCount      ,
                           HMODULE * paModArray   ,
                           LPUINT    puiRealCount   );
                           
BOOL BUGSUTIL_DLLINTERFACE TLHELPGetLoadedModules ( DWORD     dwPID        ,
                                              UINT      uiCount      ,
                                              HMODULE * paModArray   ,
                                              LPUINT    puiRealCount  );


typedef struct tag_HOOKFUNCDESCA
{
    // The name of the function to hook.
    LPCSTR szFunc  ;
    // The procedure to blast in.
    PROC   pProc   ;
    // the orig : (set this to NULL, I will fill it )
    PROC   pOrig   ;
    BOOL	bHooked;
} HOOKFUNCDESCA , * LPHOOKFUNCDESCA ;

typedef struct tag_HOOKFUNCDESCW
{
    // The name of the function to hook.
    LPCWSTR szFunc ;
    // The procedure to blast in.
    PROC    pProc  ;
    // the orig : (set this to NULL, I will fill it )
    PROC   pOrig   ;
    BOOL	bHooked;
} HOOKFUNCDESCW , * LPHOOKFUNCDESCW ;

#ifdef UNICODE
#define HOOKFUNCDESC   HOOKFUNCDESCW
#define LPHOOKFUNCDESC LPHOOKFUNCDESCW
#else
#define HOOKFUNCDESC   HOOKFUNCDESCA
#define LPHOOKFUNCDESC LPHOOKFUNCDESCA
#endif  // UNICODE

/*----------------------------------------------------------------------
FUNCTION        :   HookImportedFunctionsByName
DISCUSSION      :
    Hooks the specified functions imported into hModule by the module
indicated by szImportMod.  This function can be used to hook from one
to 'n' of the functions imported.
    The techniques used in the function are slightly different than
that shown by Matt Pietrek in his book, "Windows 95 System Programming
Secrets."  He uses the address of the function to hook as returned by
GetProcAddress.  Unfortunately, while this works in almost all cases, it
does not work when the program being hooked is running under a debugger
on Windows95 (an presumably, Windows98).  The problem is that
GetProcAddress under a debugger returns a "debug thunk," not the address
that is stored in the Import Address Table (IAT).
    This function gets around that by using the real thunk list in the
PE file, the one not bashed by the loader when the module is loaded and
fixed up, to find where the named import is located.  Once the named
import is found, then the original table is blasted to make the hook.
As the name implies, this function will only hook functions imported by
name.
PARAMETERS      :
    hModule     - The module where the imports will be hooked.
    szImportMod - The name of the module whose functions will be
                  imported.
    uiCount     - The number of functions to hook.  This is the size of
                  the paHookArray and paOrigFuncs arrays.
    paHookArray - The array of function descriptors that list which
                  functions to hook.  At this point, the array does not
                  have to be in szFunc name order.  Also, if a
                  particular pProc is NULL, then that item will just be
                  skipped.  This makes it much easier for debugging.
    paOrigFuncs - The array of original addresses that were hooked.  If
                  a function was not hooked, then that item will be
                  NULL.
    puiHooked   - Returns the number of functions hooked out of
                  paHookArray.
RETURNS         :
    FALSE - There was a problem, check GetLastError.
    TRUE  - The function succeeded.  See the parameter discussion for
            the output parameters.
----------------------------------------------------------------------*/
BOOL BUGSUTIL_DLLINTERFACE
        HookImportedFunctionsByName ( HMODULE         hModule     ,
                                       LPCSTR          szImportMod ,
                                       UINT            uiCount     ,
                                       HOOKFUNCDESCA * paHookArray ,
                                       LPUINT          puiHooked    ) ;


#ifdef __cplusplus
}
#endif  // _cplusplus
/*//////////////////////////////////////////////////////////////////////
                                  END
                      C Function Declaration Area
//////////////////////////////////////////////////////////////////////*/

/*//////////////////////////////////////////////////////////////////////
                         C++ Only Declarations
                                 START
//////////////////////////////////////////////////////////////////////*/
#ifdef __cplusplus

#endif  // __cplusplus
/*//////////////////////////////////////////////////////////////////////
                                  END
                         C++ Only Declarations
//////////////////////////////////////////////////////////////////////*/

#endif  // _BUGSLAYERUTIL_H



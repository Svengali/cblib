#pragma once

#include "Base.h"

START_CB

struct HookFuncDesc
{
    // The name of the function to hook.
    const char * pFuncName;
    
    // The procedure to blast in.
    void *   pProc   ;
    
    // the orig : (set this to NULL, I will fill it )
    void *   pOrig;
    bool	didHook; // I fill this too 
};

// return number hooked :
//  InstallFuncHooks will fill out pOrig

int InstallFuncHooks(HookFuncDesc * pHooks, int numHooks,const char ** dllsToHook, int numDLLs);
int RemoveFuncHooks( HookFuncDesc * pHooks, int numHooks,const char ** dllsToHook, int numDLLs);

END_CB

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#include "StackTrace.h"
#include "Log.h"

START_CB

#ifdef CB_DO_STACK_TRACE

//#define CB_DO_USE_STACKWALK64
// -> StackWalk64 works with frame pointer omission
//  RtlCaptureStackBackTrace does not

/*******************************************************

DONE :
StackWalk64 is kind of slow ;
you could benefit from the undocumented (at least not for winxp) ntdll!RtlCaptureStackBackTrace
RtlCaptureStackBackTrace
http://undocumented.ntinternals.net/UserMode/Undocumented%20Functions/Debug/RtlCaptureStackBackTrace.html
http://msdn.microsoft.com/en-us/library/bb204633(VS.85).aspx

 

---------------------------------------------------------------

CB : should work on all Win32 and Win64 platforms
very similar code should work on Xenon

The Grab should be as cheap as possible, the Log can be slow

---------------------------------------------------------------

To get the latest version of dbghelp.lib :
Microsoft Debugging tools : version 6.9.3.113 - April 29, 2008

http://www.microsoft.com/whdc/devtools/debugging/installx86.Mspx

Download the big symbol packages :

http://www.microsoft.com/whdc/DevTools/Debugging/symbolpkg.mspx

(For example :)    
* Windows XP with Service Pack 3 x86 retail symbols, all languages (File size: 209 MB - Most customers want this package.)

Keep the default symbol install path : c:\windows\symbols

Then set _NT_SYMBOL_PATH=c:\windows\symbols

---------------------------------------------------------------

About the MS symbol server and _NT_SYMBOL_PATH :

http://support.microsoft.com/kb/311503
http://msdn.microsoft.com/en-us/library/ms681416(VS.85).aspx
http://blogs.msdn.com/ms_joc/archive/2008/02/28/nt-symbol-path-and-vs.aspx

Some decent sample code :

http://www.gamedev.net/community/forums/topic.asp?topic_id=364861
http://www.codeproject.com/KB/debug/extendedtrace.aspx

**************************************************************************/

#include <windows.h>
#include <dbghelp.h>
	
#pragma comment(lib,"dbghelp.lib")

static bool s_init = false;

typedef VOID (WINAPI t_RtlCaptureContext)(PCONTEXT);
typedef USHORT (WINAPI t_RtlCaptureStackBackTrace)(ULONG, ULONG, PVOID*, PULONG);

static t_RtlCaptureContext * s_pRtlCaptureContext = NULL;
static t_RtlCaptureStackBackTrace * s_pRtlCaptureStackBackTrace = NULL;
	
/**************************************************************************/

// Let's figure out the path for the symbol files
// Search path= ".;%_NT_SYMBOL_PATH%;%_NT_ALTERNATE_SYMBOL_PATH%;%SYSTEMROOT%;%SYSTEMROOT%\System32;" + lpszIniPath
// Note: There is no size check for lpszSymbolPath!
static void SetSymbolSearchPath()
{
	char symbolPath[4096];
	char curPath[1024];

	// Creating the default path
	// ".;%_NT_SYMBOL_PATH%;%_NT_ALTERNATE_SYMBOL_PATH%;%SYSTEMROOT%;%SYSTEMROOT%\System32;"
	strcpy( symbolPath, "." );

	// environment variable _NT_SYMBOL_PATH
	if ( GetEnvironmentVariableA( "_NT_SYMBOL_PATH", curPath, sizeof(curPath) ) )
	{
		strcat( symbolPath, ";" );
		strcat( symbolPath, curPath );
	}

	// environment variable _NT_ALTERNATE_SYMBOL_PATH
	if ( GetEnvironmentVariableA( "_NT_ALTERNATE_SYMBOL_PATH", curPath, sizeof(curPath) ) )
	{
		strcat( symbolPath, ";" );
		strcat( symbolPath, curPath );
	}

	// environment variable SYSTEMROOT
	if ( GetEnvironmentVariableA( "SYSTEMROOT", curPath, sizeof(curPath) ) )
	{
		strcat( symbolPath, ";" );
		strcat( symbolPath, curPath );
		strcat( symbolPath, ";" );

		// SYSTEMROOT\System32
		strcat( symbolPath, curPath );
		strcat( symbolPath, "\\System32" );
	}

	/*
	// Add user defined path
	if ( lpszIniPath != NULL )
		if ( lpszIniPath[0] != '\0' )
		{
			strcat( symbolPath, ";" );
			strcat( symbolPath, lpszIniPath );
		}
	*/
	
	/*
	// Kinda cool but maybe really slow cuz it goes over the net :
	strcat( symbolPath, ";" );
	//strcat( symbolPath, "SRV*c:\\symbols*http://msdl.microsoft.com/download/symbols" );
	strcat( symbolPath, "symsrv*symsrv.dll*c:\\symbols*http://msdl.microsoft.com/download/symbols" );
	*/
	 
	SymSetSearchPath( GetCurrentProcess(), symbolPath );
}

static void InitRtlFuncs()
{
	if ( s_pRtlCaptureContext == NULL )
	{
		HMODULE hKernel32Dll = GetModuleHandle("kernel32.dll");
		
		s_pRtlCaptureContext = (t_RtlCaptureContext *) GetProcAddress(hKernel32Dll,"RtlCaptureContext");
		ASSERT(s_pRtlCaptureContext );
		
		HMODULE hNtDll = GetModuleHandle("ntdll.dll");
		s_pRtlCaptureStackBackTrace = (t_RtlCaptureStackBackTrace *) GetProcAddress(hNtDll,"RtlCaptureStackBackTrace");
		ASSERT(s_pRtlCaptureStackBackTrace );
	}
}

void StackTrace_Init()
{
	if ( s_init )
		return;
		
	// not super necessary it seems :
	//SetSymbolSearchPath();

	// SYMOPT_DEFERRED_LOADS make us not take a ton of time unless we actual log traces
	SymSetOptions(SYMOPT_DEFERRED_LOADS|SYMOPT_FAIL_CRITICAL_ERRORS|SYMOPT_LOAD_LINES|SYMOPT_UNDNAME);
		//SYMOPT_NO_PROMPTS|

	SymInitialize(GetCurrentProcess(), NULL, TRUE);
	
	InitRtlFuncs();
	
	s_init = true;
}

void StackTrace_Shutdown()
{
	if ( s_init )
	{
		SymCleanup(GetCurrentProcess());
	
		s_init = false;
	}
}

/**

StackTrace_Grab should be as lean as possible
it can be done in tight places like Malloc

**/

#ifdef CB_DO_USE_STACKWALK64 // StackWalk64 way

void StackTrace_Grab(StackTrace * trace,int skips)
{
	// zero right away in case we error out
	ZERO(trace);

	InitRtlFuncs();
	if ( ! s_pRtlCaptureContext )
		return;

	// Capture context
	//	this is just grabbing your process registers
	CONTEXT ctx = { 0 };
	//RtlCaptureContext(&ctx);
	s_pRtlCaptureContext(&ctx);

	/*
	if ( ! s_pRtlCaptureContext )
	{
		// Grap the current context (state of EBP,EIP,ESP registers)
		ctx.ContextFlags = CONTEXT_ALL;

		_asm 
		{
				call x
			x: pop eax
				mov ctx.Eip, eax
				mov ctx.Ebp, ebp
				mov ctx.Esp, esp
		}
	}
	*/
	
	// Init the stack frame for this function
	STACKFRAME64 stackFrame = { 0 };
	
	#ifdef _M_IX86
	DWORD dwMachineType = IMAGE_FILE_MACHINE_I386;
	stackFrame.AddrPC.Offset = ctx.Eip;
	stackFrame.AddrPC.Mode = AddrModeFlat;
	stackFrame.AddrFrame.Offset = ctx.Ebp;
	stackFrame.AddrFrame.Mode = AddrModeFlat;
	stackFrame.AddrStack.Offset = ctx.Esp;
	stackFrame.AddrStack.Mode = AddrModeFlat;
	#elif _M_X64
	DWORD dwMachineType = IMAGE_FILE_MACHINE_AMD64;
	stackFrame.AddrPC.Offset = ctx.Rip;
	stackFrame.AddrPC.Mode = AddrModeFlat;
	stackFrame.AddrFrame.Offset = ctx.Rsp;
	stackFrame.AddrFrame.Mode = AddrModeFlat;
	stackFrame.AddrStack.Offset = ctx.Rsp;
	stackFrame.AddrStack.Mode = AddrModeFlat;
	#elif _M_IA64
	DWORD dwMachineType = IMAGE_FILE_MACHINE_IA64;
	stackFrame.AddrPC.Offset = ctx.StIIP;
	stackFrame.AddrPC.Mode = AddrModeFlat;
	stackFrame.AddrFrame.Offset = ctx.IntSp;
	stackFrame.AddrFrame.Mode = AddrModeFlat;
	stackFrame.AddrBStore.Offset = ctx.RsBSP;
	stackFrame.AddrBStore.Mode = AddrModeFlat;
	stackFrame.AddrStack.Offset = ctx.IntSp;
	stackFrame.AddrStack.Mode = AddrModeFlat;
	#else
	#error "Platform not supported!"
	#endif

	// Walk up the stack
	HANDLE hThread = GetCurrentThread();
	HANDLE hProcess = GetCurrentProcess();
	for(int i=0; i< (CB_STACK_TRACE_DEPTH + skips) ; ++i)
	{
		// walking once first makes us skip self
		if(!StackWalk64(dwMachineType, hProcess, hThread, &stackFrame,
			&ctx, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL))
		{
			break;
		}
		
		if (stackFrame.AddrPC.Offset == stackFrame.AddrReturn.Offset ||
			stackFrame.AddrPC.Offset == 0 )
		{
			break;
		}
		
		if ( i >= skips )
		{
			// Offset is a DWORD64
			trace->ips[i - skips] = (instructionaddress_t) stackFrame.AddrPC.Offset;
		}
	}
}

#else // s_pRtlCaptureStackBackTrace way

// RtlCaptureStackBackTrace is faster :

void StackTrace_Grab(StackTrace * trace,int skips)
{
	// zero right away in case we error out
	ZERO_PTR(trace);

	InitRtlFuncs();
	if ( ! s_pRtlCaptureStackBackTrace )
		return;

	// always skip self :
	skips ++;

	//#if ( sizeof(PVOID) == sizeof(instructionaddress_t) )
	#if 0
	
	COMPILER_ASSERT( sizeof(PVOID) == sizeof(instructionaddress_t) );
	
	(*s_pRtlCaptureStackBackTrace)(skips,CB_STACK_TRACE_DEPTH,(PVOID *)trace->ips,NULL);

	#else // not equal sizes

	PVOID pointers[CB_STACK_TRACE_DEPTH];
	int count = (*s_pRtlCaptureStackBackTrace)(skips,CB_STACK_TRACE_DEPTH,pointers,NULL);
	
	ASSERT( count <= CB_STACK_TRACE_DEPTH );
	
	for(int i=0;i<count;i++)
	{
		trace->ips[i] = (instructionaddress_t) pointers[i];
	}
	
	#endif
}

#endif


void StackTrace_Log(const StackTrace * trace)
{
	//HANDLE hThread = GetCurrentThread();
	HANDLE hProcess = GetCurrentProcess();
	
	// Resolve PC to function names
	for(int i=0; i< CB_STACK_TRACE_DEPTH; i++)
	{
		// Check for end of stack walk
		DWORD64 ip = trace->ips[i];
		if( ip == 0 )
			break;

		// Get function name
		#define MAX_STRING_LEN	(512)
		unsigned char byBuffer[sizeof(IMAGEHLP_SYMBOL64) + MAX_STRING_LEN] = { 0 };
		IMAGEHLP_SYMBOL64* pSymbol = (IMAGEHLP_SYMBOL64*)byBuffer;
		pSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
		pSymbol->MaxNameLength = MAX_STRING_LEN;

		DWORD64 dwDisplacement;
		
		if ( SymGetSymFromAddr64(hProcess, ip, &dwDisplacement, pSymbol) )
		{
			pSymbol->Name[MAX_STRING_LEN-1] = 0;
			
			/*
			// Make the symbol readable for humans
			UnDecorateSymbolName( pSym->Name, lpszNonUnicodeUnDSymbol, BUFFERSIZE, 
				UNDNAME_COMPLETE | 
				UNDNAME_NO_THISTYPE |
				UNDNAME_NO_SPECIAL_SYMS |
				UNDNAME_NO_MEMBER_TYPE |
				UNDNAME_NO_MS_KEYWORDS |
				UNDNAME_NO_ACCESS_SPECIFIERS );
			*/
			
			// pSymbol->Name
			const char * pFunc = pSymbol->Name;
			
			// Get file/line number
			IMAGEHLP_LINE64 theLine = { 0 };
			theLine.SizeOfStruct = sizeof(theLine);

			DWORD dwDisplacement;
			if ( ! SymGetLineFromAddr64(hProcess, ip, &dwDisplacement, &theLine))
			{
				lprintf("unknown(%08X) : %s\n",(uint32)ip,pFunc);
			}
			else
			{
				/*
				const char* pFile = strrchr(theLine.FileName, '\\');
				if ( pFile == NULL ) pFile = theLine.FileName;
				else pFile++;
				*/
				const char * pFile = theLine.FileName;
				
				int line = theLine.LineNumber;
				
				lprintf("%s(%d) : %s\n",pFile,line,pFunc);
			}
		}
	}
}



#endif // CB_DO_STACK_TRACE

END_CB

#include "cblib/inc.h"
#include "Win32Util.h"
#include <windows.h>
#include <tlhelp32.h>
#include <shellapi.h>

START_CB

bool NewKeyDown(const int vk)
{
	if (GetAsyncKeyState(vk) & 0x1)
		return true;

	return false;
}

bool IsKeyDown(const int vk)
{
	if (GetAsyncKeyState(vk) & 0x8000)
		return true;

	return false;
}

void SendInputKey(int vk)
{
	INPUT inputs[2] = { 0 };
	inputs[0].type = INPUT_KEYBOARD;
	inputs[0].ki.wVk = vk;
	inputs[1] = inputs[0];
	inputs[1].ki.dwFlags |= KEYEVENTF_KEYUP;
	SendInput(2,inputs,sizeof(INPUT));
}

void SendMessageKeyUpDown(HWND w,int vk,bool async)
{
	const LPARAM lpKU = (1UL<<31) | (1UL<<30);

	if ( async )
	{
		PostMessage(w,WM_KEYDOWN,vk,0);
		PostMessage(w,WM_KEYUP,vk,lpKU);	
	}
	else
	{
		SendMessageSafe(w,WM_KEYDOWN,vk,0);
		SendMessageSafe(w,WM_KEYUP,vk,lpKU);	
	}
}

/***

WARNING :

@@ Find can crash if you enumerate a bunch of
	windows and then one goes away
	better to just not build that vector and do proper enum func

Better to quickly find the windows you want and act on them

Filtering INSIDE the Enum() is the safest way

**/

static BOOL CALLBACK EnumWindowsToVector(HWND hwnd, LPARAM lParam)
{
	cb::vector<HWND>	*enumVector = (cb::vector<HWND> *)lParam;
	if (enumVector != NULL)
	{
		enumVector->push_back(hwnd);
		return TRUE; //continue
	}
	
	return FALSE; //stop
}

/*
	EnumWindowsOrRoot enums all kids if hwnd is a window, only top-levels if window is NULL
*/
static void EnumWindowsOrRoot(HWND hwnd, WNDENUMPROC proc, LPARAM v)
{
	if (hwnd == NULL)
	{
		//hwnd = GetDesktopWindow();
		//EnumChildWindows(hwnd, EnumChildProc, (LPARAM)v);
		// EnumWindows only enumerates top level windows !
		EnumWindows(proc,v);
	}
	else
	{
		// EnumChildWindows recurses to all children
		EnumChildWindows(hwnd,proc,v);
	}
}

void EnumChildren(HWND hwnd, cb::vector<HWND> *v)
{
	EnumWindowsOrRoot(hwnd,EnumWindowsToVector,(LPARAM)v);
}

struct FindEquivWindowData
{
	const char * typeName;
	const char * searchForTitle;
	bool exactMatch;
	bool exactMatchType;
	bool mustBeVisible;
	HWND parent;

	HWND found;
	cb::vector<HWND> *vec;
};

static BOOL CALLBACK EnumWindowsFindEquivWindowData(HWND hwnd, LPARAM lParam)
{
	FindEquivWindowData * fewd = (FindEquivWindowData *)lParam;

	if ( fewd->mustBeVisible && ! IsWindowVisible(hwnd) )
		return TRUE; //continue enum

	if ( ! IsWindow(hwnd) ) // should no happen	
		return TRUE; //continue enum

	char buffer[1024];
	buffer[0] = 0;
	
	__try
	{

		//First-chance exception at 0x7e42f450 in GoldBullion.exe: 0xC0000005: Access violation reading location 0x442b0054.
		// when I get this except "fewd" is fine
		//	the hwnd seems to point at a window that's semi-destroyed
		//	and its class is unregistered or the dll it was in is unloaded
		// WTF

		if ( GetClassNameA(hwnd, buffer, 1024) == 0 )
			return TRUE; //continue enum

	}
	__except( EXCEPTION_EXECUTE_HANDLER  )
	{
		return FALSE;
	}

	bool match;
	if ( fewd->typeName == NULL )
		match = true;
	else if ( fewd->exactMatchType )
		match = ( strcmp(buffer,fewd->typeName) == 0 );
	else
		match = stripresame(buffer, fewd->typeName);

	if ( match )
	{
		// this is similar to GetWindowText()
		//	the difference is that GetWindowText() doesn't work on controls in other apps,
		//	only top-level other app windows
		buffer[0] = 0;
		if ( fewd->parent == NULL )
		{
			GetWindowTextA(hwnd,buffer,sizeof(buffer));
		}
		else
		{
			//SendMessageSafe(hwnd, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
			DWORD_PTR dwResult;
			SendMessageTimeout(hwnd,WM_GETTEXT,sizeof(buffer), (LPARAM)buffer,SMTO_ABORTIFHUNG,500,&dwResult);
		}
		
		bool match;
		if ( fewd->searchForTitle == NULL )
			match = true;
		else if ( fewd->exactMatch )
			match = ( stricmp(buffer,fewd->searchForTitle) == 0 );
		else
			match = stripresame(buffer, fewd->searchForTitle);

		if ( match )
		{
			if ( fewd->vec )
			{
				fewd->vec->push_back(hwnd);
				return TRUE;
			}
			else
			{
				fewd->found = hwnd;
				return FALSE; // stop enum
			}
		}
	}

	return TRUE; //continue enum
}

HWND FindEquivWindow(const char * typeName, const char * searchForTitle, const bool exactMatch, HWND parent, const bool mustBeVisible, const bool exactMatchType)
{
	FindEquivWindowData fewd = { 0 };
	fewd.typeName = typeName;
	fewd.searchForTitle = searchForTitle;
	fewd.exactMatch = exactMatch;
	fewd.exactMatchType = exactMatchType;
	fewd.mustBeVisible = mustBeVisible;
	fewd.parent = parent;
	fewd.found = NULL;
	fewd.vec = NULL;
	
	EnumWindowsOrRoot(parent,EnumWindowsFindEquivWindowData,(LPARAM)&fewd);
	
	return fewd.found;
}

void EnumEquivWindows(cb::vector<HWND> *v,const char * typeName, const char * searchForTitle, const bool exactMatch, HWND parent, const bool mustBeVisible, const bool exactMatchType)
{
	FindEquivWindowData fewd = { 0 };
	fewd.typeName = typeName;
	fewd.searchForTitle = searchForTitle;
	fewd.exactMatch = exactMatch;
	fewd.exactMatchType = exactMatchType;
	fewd.mustBeVisible = mustBeVisible;
	fewd.parent = parent;
	fewd.found = NULL;
	fewd.vec = v;
	
	EnumWindowsOrRoot(parent,EnumWindowsFindEquivWindowData,(LPARAM)&fewd);
	
	return;
}

struct FindWindowAtRectData
{
	RECT rc;
	bool visibleCheck;
	int tolerance;

	HWND found;
	cb::vector<HWND> *vec;
};

static BOOL CALLBACK EnumWindowsFindWindowAtRect(HWND hwnd, LPARAM lParam)
{
	FindWindowAtRectData * fwar = (FindWindowAtRectData *)lParam;
	
	if ( fwar->visibleCheck && !IsWindowVisible(hwnd) )
	{
		return TRUE; // continue enum
	}
	
	// get this childs window rect
	RECT rect;
	GetWindowRect(hwnd, &rect);

	// see if this is the one
	if (
		abs(rect.left - fwar->rc.left) <= fwar->tolerance &&
		abs(rect.top - fwar->rc.top) <= fwar->tolerance &&
		abs(rect.right - fwar->rc.right) <= fwar->tolerance &&
		abs(rect.bottom - fwar->rc.bottom) <= fwar->tolerance )
	{
		fwar->found = hwnd;
		return FALSE; // done, stop enum
	}

	return TRUE;
}

HWND FindWindowAtRect(const RECT *rc, HWND parent, bool visibleCheck, int tolerance)
{
	ASSERT_RELEASE( rc );
	ASSERT_RELEASE( parent );

	FindWindowAtRectData fwar = { 0 };
	fwar.rc = *rc;
	fwar.visibleCheck = visibleCheck;
	fwar.tolerance = tolerance;
	fwar.found = NULL;
	fwar.vec = NULL;
	
	// get the parent rect
	RECT parentRect;
	GetWindowRect(parent, &parentRect);

	// add on the parent rect
	fwar.rc.left += parentRect.left;
	fwar.rc.top += parentRect.top;
	fwar.rc.right += parentRect.left;
	fwar.rc.bottom += parentRect.top;

	EnumWindowsOrRoot(parent,EnumWindowsFindWindowAtRect,(LPARAM)&fwar);
	
	return fwar.found;
}

BOOL GetProcessModule (DWORD dwPID, DWORD dwModuleID, 
     LPMODULEENTRY32 lpMe32, DWORD cbMe32) 
{ 
    BOOL          bRet        = FALSE; 
    BOOL          bFound      = FALSE; 
    HANDLE        hModuleSnap = NULL; 
    MODULEENTRY32 me32        = {0}; 
 
    // Take a snapshot of all modules in the specified process. 

    hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPID); 
    if (hModuleSnap == INVALID_HANDLE_VALUE) 
        return (FALSE); 
 
    // Fill the size of the structure before using it. 

    me32.dwSize = sizeof(MODULEENTRY32); 
 
    // Walk the module list of the process, and find the module of 
    // interest. Then copy the information to the buffer pointed 
    // to by lpMe32 so that it can be returned to the caller. 

    if (Module32First(hModuleSnap, &me32)) 
    { 
        do 
        { 
            if (me32.th32ModuleID == dwModuleID) 
            { 
                CopyMemory (lpMe32, &me32, cbMe32); 
                bFound = TRUE; 
            } 
        } 
        while (!bFound && Module32Next(hModuleSnap, &me32)); 
 
        bRet = bFound;   // if this sets bRet to FALSE, dwModuleID 
                         // no longer exists in specified process 
    } 
    else 
        bRet = FALSE;           // could not walk module list 
 
    // Do not forget to clean up the snapshot object. 

    CloseHandle (hModuleSnap); 
 
    return (bRet); 
}

BOOL CALLBACK MyWindowKiller(HWND hwnd, LPARAM lParam)
{
	DWORD seekPID = (DWORD)lParam;
	DWORD pid;
	//DWORD thread = 
	GetWindowThreadProcessId(hwnd,&pid);
	if ( pid == seekPID )
	{
		lprintf("Posting WM_QUIT\n");
		PostMessage(hwnd,WM_QUIT,0,0);
	}
	return TRUE;
}

bool TryWindowKill(HANDLE hProcess,DWORD pid)
{
	/*            
	Post a WM_CLOSE to all Top-Level windows owned by the process that you want to shut down. Many Windows applications respond to this message by shutting down.

	NOTE: A console application's response to WM_CLOSE depends on whether or not it has installed a control handler. 

	Use EnumWindows() to find the handles to your target windows. In your callback function, 
	check to see if the windows' process ID matches the process you want to shut down. 
	You can do this by calling GetWindowThreadProcessId(). Once you have established a match, 
	use PostMessage() or SendMessageTimeout() to post the WM_CLOSE message to the window.

	Use WaitForSingleObject() to wait for the handle of the process. Make sure you wait with a 
	timeout value, because there are many situations in which the WM_CLOSE will not shut down 
	the application. Remember to make the timeout long enough (either with WaitForSingleObject(),
	or with SendMessageTimeout()) so that a user can respond to any dialog boxes that were created 
	in response to the WM_CLOSE message.
	 
	If the return value is WAIT_OBJECT_0, then the application closed itself down cleanly. If the 
	return value is WAIT_TIMEOUT, then you must use TerminateProcess() to shutdown the application.

	NOTE: If you are getting3 a return value from WaitForSingleObject() other then WAIT_OBJECT_0 or WAIT_TIMEOUT, use GetLastError() to determine the cause. 
	By 
	*/

	EnumWindows(MyWindowKiller,(LPARAM)pid);
	
	// wait up to one second :
	DWORD ret = WaitForSingleObject(hProcess,1000);
	if ( ret == WAIT_TIMEOUT )
		return false;
	else
		return true;
}
			
void MyKillProcess (const char * szMyExeName) 
{
    HANDLE         hProcessSnap = NULL; 
    BOOL           bRet      = FALSE; 
    PROCESSENTRY32 pe32      = {0}; 
 
    //  Take a snapshot of all processes in the system. 

	DWORD myPID = GetCurrentProcessId();

    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 

    if (hProcessSnap == INVALID_HANDLE_VALUE) 
    {
		lprintf("CreateToolhelp32Snapshot failed!\n");
        return;
	}
	
    //  Fill in the size of the structure before using it. 

    pe32.dwSize = sizeof(PROCESSENTRY32); 
 
    //  Walk the snapshot of the processes, and for each process, 
    //  display information. 

    if (Process32First(hProcessSnap, &pe32)) 
    { 
        //DWORD         dwPri8orityClass; 
        //BOOL          bGotModule = FALSE; 
        //MODULEENTRY32 me32       = {0}; 
 
        do 
        {
			if ( pe32.th32ProcessID == myPID )
			{
				// hey, that's me
				continue;
			}
			
            //bGotModule = GetProcessModule(pe32.th32ProcessID, 
            //    pe32.th32ModuleID, &me32, sizeof(MODULEENTRY32)); 

			if ( ! strisame(pe32.szExeFile,szMyExeName) )
			{
				continue;
			}

            lprintf( "Found existing instance\n");
            lprintf( "EXE\t\t\t%s\n", pe32.szExeFile);
            lprintf( "PID\t\t\t%d\n", pe32.th32ProcessID);
   
			HANDLE hProcess; 

			// Get the actual priority class. 
			hProcess = OpenProcess (PROCESS_ALL_ACCESS, 
				FALSE, pe32.th32ProcessID); 
			if ( hProcess == INVALID_HANDLE_VALUE )
			{
				LogLastError("Couldn't open process!");
				continue;
			}
				
			if ( ! TryWindowKill(hProcess,pe32.th32ProcessID) )
			{
				// it failed, do a hard kill
				lprintf("Doing TerminateProcess\n");
						                        
				TerminateProcess(hProcess,0);
				// GenerateConsoleCtrlEvent  ?				
			}
			
			CloseHandle (hProcess);
        } 
        while (Process32Next(hProcessSnap, &pe32)); 
        bRet = TRUE; 
    } 
    else 
        bRet = FALSE;    // could not walk the list of processes 
 
    // Do not forget to clean up the snapshot object. 

    CloseHandle (hProcessSnap); 
    return;
}


bool IsProcessAlreadyRunning(const char * szMyExeName) 
{
	// @@ lots of code dupe with MyKillProcess

    HANDLE         hProcessSnap = NULL; 
    PROCESSENTRY32 pe32      = {0}; 
 
    //  Take a snapshot of all processes in the system. 

	DWORD myPID = GetCurrentProcessId();

    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 

    if (hProcessSnap == INVALID_HANDLE_VALUE) 
    {
		lprintf("CreateToolhelp32Snapshot failed!\n");
        return false;
	}
	
    //  Fill in the size of the structure before using it. 

    pe32.dwSize = sizeof(PROCESSENTRY32); 
 
    //  Walk the snapshot of the processes, and for each process, 
    //  display information. 

    if (Process32First(hProcessSnap, &pe32)) 
    { 
        //DWORD         dwPri8orityClass; 
        //BOOL          bGotModule = FALSE; 
        //MODULEENTRY32 me32       = {0}; 
 
        do 
        {
			if ( pe32.th32ProcessID == myPID )
			{
				// hey, that's me
				continue;
			}
			
            //bGotModule = GetProcessModule(pe32.th32ProcessID, 
            //    pe32.th32ModuleID, &me32, sizeof(MODULEENTRY32)); 

			if ( ! strisame(pe32.szExeFile,szMyExeName) )
			{
				continue;
			}

            lprintf( "Found existing instance\n");
            lprintf( "EXE\t\t\t%s\n", pe32.szExeFile);
            lprintf( "PID\t\t\t%d\n", pe32.th32ProcessID);
   
		    CloseHandle (hProcessSnap); 
			return true;	
        } 
        while (Process32Next(hProcessSnap, &pe32)); 
    } 
 
    // Do not forget to clean up the snapshot object. 

    CloseHandle (hProcessSnap); 
    return false;
}


void LogLastError(const char * message)
{
	char buf[4096];
	GetLogLastErrorString(buf,message);
	lprintf(buf);
}

void GetLogLastErrorString(char * pInto,const char * message /*= NULL*/)
{
	DWORD err = GetLastError();

	if ( err == S_OK )
	{
		strcpy(pInto,"GetLastError returned OK!\n");
		return;
	}

	LPVOID lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);

	if ( message )
	{
		sprintf(pInto,"%s : %s",message,(char *)lpMsgBuf);
	}
	else
	{
		strcpy(pInto,(char *)lpMsgBuf);
	}
	
	// Free the buffer.
	LocalFree( lpMsgBuf );
}


void ClickWindowUntilGone(const HWND hwnd)
{	
	// keep clicking until it goes away
	for(int i=0;i<4;i++)
	{
		if ( ! IsWindow(hwnd) || ! IsWindowVisible(hwnd) )
			return;
			
		ClickWindowOnce(hwnd);
		Sleep(10);
	}
}
	
void ClickClientXY(const HWND hwnd,int x,int y,bool async /*= false*/)
{
	// the x,y pos for WM_LBUTTON is in the client rect space
	
	if ( ! IsWindowEnabled(hwnd) )
		return;
	
	LPARAM lParam = MAKELONG(x,y);
	
	if ( async )
	{
		PostMessage(hwnd,WM_LBUTTONDOWN,MK_LBUTTON,lParam);
		PostMessage(hwnd,WM_LBUTTONUP,0,lParam);
	}
	else
	{
		//SendMessage(hwnd,WM_LBUTTONDOWN,MK_LBUTTON,lParam);
		//SendMessage(hwnd,WM_LBUTTONUP,0,lParam);
        DWORD_PTR dwResult;
		SendMessageTimeout(hwnd,WM_LBUTTONDOWN,MK_LBUTTON,lParam,SMTO_ABORTIFHUNG,1000,&dwResult);
		SendMessageTimeout(hwnd,WM_LBUTTONUP,0,lParam,SMTO_ABORTIFHUNG,1000,&dwResult);
	}
}

void ClickWindowOnce(const HWND hwnd,bool async /*= false*/)
{
	// the x,y pos for WM_LBUTTON is in the client rect space
	
	if ( ! IsWindowEnabled(hwnd) )
		return;
		
	RECT rect;
	
	GetClientRect(hwnd,&rect);
	
	int w = rect.right - rect.left;
	int h = rect.bottom - rect.top;

	// randomize a bit, safely :
	int x = ftoi( 0.5f + w*(0.5f + (frandunit()-0.5f)*0.2f) );
	int y = ftoi( 0.5f + h*(0.5f + (frandunit()-0.5f)*0.2f) );

	//lprintf("posting messages at (%d,%d)\n",x,y);
	
	LPARAM lParam = MAKELONG(x,y);
	
	if ( async )
	{
		PostMessage(hwnd,WM_LBUTTONDOWN,MK_LBUTTON,lParam);
		PostMessage(hwnd,WM_LBUTTONUP,0,lParam);
	}
	else
	{
		//SendMessage(hwnd,WM_LBUTTONDOWN,MK_LBUTTON,lParam);
		//SendMessage(hwnd,WM_LBUTTONUP,0,lParam);
        DWORD_PTR dwResult;
		SendMessageTimeout(hwnd,WM_LBUTTONDOWN,MK_LBUTTON,lParam,SMTO_ABORTIFHUNG,1000,&dwResult);
		SendMessageTimeout(hwnd,WM_LBUTTONUP,0,lParam,SMTO_ABORTIFHUNG,1000,&dwResult);
	}
}

LRESULT SendMessageSafe(   
	HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam)
{
    DWORD_PTR dwResult;
	if ( SendMessageTimeout(hWnd,Msg,wParam,lParam,SMTO_ABORTIFHUNG,5000,&dwResult) )
	{
		return dwResult;
	}
	else
	{
		// timed out !
		// (not sure what a good error value is here, return values for SendMessage() seem very nonstandardized)
		return (-1) ;
	}
}

void ClickRect(const RECT & rect)
{
	// randomize a tiny bit in the rect :
	//	(this is in screen coords)
	int w = rect.right - rect.left;
	int h = rect.bottom - rect.top;
//	int rx = (int)( MIN(w,16)*(frandunit()-0.5f)/2 );
//	int ry = (int)( MIN(h,16)*(frandunit()-0.5f)/2 );
	int rx=0,ry=0;
	int x = rect.left + w/2 + rx;
	int y = rect.top  + h/2 + ry;
	
	//lprintf("clicking (%d,%d) in {(%d,%d) to (%d,%d)}\n",x,y,rect.left,rect.top,rect.right,rect.bottom);

	POINT oldPos;
	GetCursorPos(&oldPos);
		
	static const int inputCount= 2;
	INPUT input[inputCount];

	memset(&input, 0, sizeof(input));
	
	input[0].type = INPUT_MOUSE;
	input[0].mi.dx = x;
	input[0].mi.dy = y;
	input[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

	input[1].type = INPUT_MOUSE;
	input[1].mi.dx = x;
	input[1].mi.dy = y;
	input[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

	SetCursorPos(x,y);
	SendInput(inputCount, input, sizeof(INPUT)); 
	
	SetCursorPos(oldPos.x,oldPos.y);
}

bool ToggleMinimized(HWND wnd)
{
	//WINDOWPLACEMENT wpl = { 0 };
	//wpl.length = sizeof(WINDOWPLACEMENT);
	//GetWindowPlacement(wnd,&wpl);
	//if ( wpl.showCmd == SW_SHOWMINIMIZED )
	if ( IsIconic(wnd) )
	{
		//ShowWindow(wnd,SW_SHOWNORMAL);
		ShowWindow(wnd,SW_RESTORE);
		return true;
	}
	else
	{
		//ShowWindow(wnd,SW_SHOWMINIMIZED);
		ShowWindow(wnd,SW_SHOWMINNOACTIVE);
		return false;
	}
}

void CopyStringToClipboard(const char * str)
{

	// test to see if we can open the clipboard first before
	// wasting any cycles with the memory allocation
	if ( ! OpenClipboard(NULL))
		return;
		
	// Empty the Clipboard. This also has the effect
	// of allowing Windows to free the memory associated
	// with any data that is in the Clipboard
	EmptyClipboard();

	// Ok. We have the Clipboard locked and it's empty. 
	// Now let's allocate the global memory for our data.

	// Here I'm simply using the GlobalAlloc function to 
	// allocate a block of data equal to the text in the
	// "to clipboard" edit control plus one character for the
	// terminating null character required when sending
	// ANSI text to the Clipboard.
	HGLOBAL hClipboardData;
	hClipboardData = GlobalAlloc(GMEM_DDESHARE, 
								strlen(str)+1);

	// Calling GlobalLock returns to me a pointer to the 
	// data associated with the handle returned from 
	// GlobalAlloc
	char * pchData;
	pchData = (char*)GlobalLock(hClipboardData);
			
	// At this point, all I need to do is use the standard 
	// C/C++ strcpy function to copy the data from the local 
	// variable to the global memory.
	strcpy(pchData, str);
			
	// Once done, I unlock the memory - remember you 
	// don't call GlobalFree because Windows will free the 
	// memory automatically when EmptyClipboard is next 
	// called. 
	GlobalUnlock(hClipboardData);
			
	// Now, set the Clipboard data by specifying that 
	// ANSI text is being used and passing the handle to
	// the global memory.
	SetClipboardData(CF_TEXT,hClipboardData);
			
	// Finally, when finished I simply close the Clipboard
	// which has the effect of unlocking it so that other
	// applications can examine or modify its contents.
	CloseClipboard();
}


/**

// interesting snippet from
http://www.codeproject.com/csharp/windowhider.asp?df=100&forumid=3881&exp=0&select=1103320

        /// <summary>
        /// Sets focus to this Window Object
        /// </summary>
        public void Activate()
        {
            if(m_hWnd == GetForegroundWindow())
                return;

            IntPtr ThreadID1 = GetWindowThreadProcessId(GetForegroundWindow(),
                                                        IntPtr.Zero);
            IntPtr ThreadID2 = GetWindowThreadProcessId(m_hWnd,IntPtr.Zero);
            
            if (ThreadID1 != ThreadID2)
            {
                AttachThreadInput(ThreadID1,ThreadID2,1);
                SetForegroundWindow(m_hWnd);
                AttachThreadInput(ThreadID1,ThreadID2,0);
            }
            else
            {
                SetForegroundWindow(m_hWnd);
            }

            if (IsIconic(m_hWnd))
            {
                ShowWindowAsync(m_hWnd,SW_RESTORE);
            }
            else
            {
                ShowWindowAsync(m_hWnd,SW_SHOWNORMAL);
            }
        }

// also :SwitchToThisWindow

SetFocus() only works on windows within the thread that calls
it. If you want to use it on windows in other threads, read up
on the AttachThreadInput() API.

**/

void MyShowWindow(HWND w,bool async)
{
	if ( async )
	{
		if (IsIconic(w))
		{
			ShowWindowAsync(w,SW_RESTORE);
		}
		else
		{
			ShowWindowAsync(w,SW_SHOWNORMAL);
		}
	}
	else
	{
		if (IsIconic(w))
		{
			ShowWindow(w,SW_RESTORE);
		}
		else
		{
			ShowWindow(w,SW_SHOWNORMAL);
		}
	}
}

void ReallyActivateWindow(HWND w)
{
	// LockSetForegroundWindow basically doesn't work because of the locks in Win2k
	LockSetForegroundWindow(LSFW_UNLOCK);
	
	// ShowWin just undoes minimized windows :
	if ( IsIconic(w) )
	{
		ShowWindowAsync(w,SW_RESTORE);
	}
	else
	{
		ShowWindowAsync(w,SW_SHOWNORMAL);
	}
	
	if ( GetForegroundWindow() != w )
	{
		// verbose activation info :
		/*
		if ( 1 )
		{
			char buffer[256];
			GetWindowText(w,buffer,sizeof(buffer));
			buffer[32] = 0;
			lprintf("RAW : %s\n",buffer);
		}
		*/
	
		// @@ ?
		DWORD ThreadID1 = GetCurrentThreadId();
        //DWORD ThreadID1 = GetWindowThreadProcessId(GetForegroundWindow(),NULL);
        DWORD ThreadID2 = GetWindowThreadProcessId(w,NULL);
        
        if (ThreadID1 != ThreadID2)
        {
            MyAttachThreadInput(ThreadID1,ThreadID2,TRUE);
		}
		
		SetForegroundWindow(w);
		
		// so far as I can tell these two do nothing :
		//	cuz SetForegroundWindow did it
		//SetActiveWindow(w);
		
		//BringWindowToTop(w);
		
		//SetWindowPos(w,HWND_TOP,0,0,0,0,SWP_ASYNCWINDOWPOS|SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
          
        if (ThreadID1 != ThreadID2)
        {
            MyAttachThreadInput(ThreadID1,ThreadID2,FALSE);
        }
	}
}
		
CriticalSection::CriticalSection()
{
	ZERO(&m_CritSec);
	InitializeCriticalSection ( &m_CritSec ) ;
}

CriticalSection::~CriticalSection ( )
{
	DeleteCriticalSection ( &m_CritSec ) ;
	ZERO(&m_CritSec);
}

UseCriticalSection::UseCriticalSection ( CriticalSection & cs )
{
	m_pCS = &cs.m_CritSec ;
	EnterCriticalSection ( m_pCS );
}
UseCriticalSection::UseCriticalSection ( CRITICAL_SECTION & cs )
{
	m_pCS = &cs ;
	EnterCriticalSection ( m_pCS );
}
UseCriticalSection::~UseCriticalSection ( )
{
	LeaveCriticalSection ( m_pCS );
	m_pCS = NULL ;
}


void RespawnDetachedIfConsole()
{
	if ( IsDebuggerPresent() )
	{
		// don't do this if being debugged cuz I want to stay attached

		lprintf("FYI : RespawnDetachedIfConsole skipped cuz of debugger\n");
		return;
	}
	
	if ( GetConsoleWindow() )
	{
		// I'm consoled and I don't want to be
	   
	    STARTUPINFO si = { 0 };
		PROCESS_INFORMATION pi = { 0 };
		si.cb = sizeof(si);

		LPSTR cmdLine = GetCommandLine();

		lprintf("Respawning detached : %s\n",cmdLine);
		
		LogFlush();
		
		BOOL ok = CreateProcess(
			NULL, //__argv[0],
			cmdLine,
			NULL,
			NULL,
			FALSE, // 	bInheritHandles
			CREATE_NO_WINDOW | DETACHED_PROCESS,
			NULL, // = inherit environment
			NULL, // = inherit curdir
			&si,
			&pi);

		if ( ! ok )
		{
			lprintf("ERROR : CreateProcess failed !\n");
		}

		exit(0);
	}
}
		
bool MakeConsoleIfNone(const char * title)
{
	if ( AllocConsole() )
	{
		freopen("CONIN$","rb",stdin);   // reopen stdin handle as console window input
		freopen("CONOUT$","wb",stdout);  // reopen stout handle as console window output
		freopen("CONOUT$","wb",stderr); // reopen stderr handle as console window output
	
		// we made a new one
		HWND w = GetConsoleWindow();
	
		if( title )
		{
			SetWindowText(w,title);
		}
			
		//SetWindowLong(w, GWL_STYLE, WS_DLGFRAME | WS_SIZEBOX | WS_VSCROLL | WS_VISIBLE );
		//SetWindowPos(w,0,0,0,0,0,SWP_NOMOVE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_FRAMECHANGED);

		//GBHookConsoleWndProc(w);

	    /*
	       // Subclass the edit control. 
        g_wpOrig = (WNDPROC) SetWindowLongPtr(w, GWL_WNDPROC, (LONG_PTR) MyProc); 

		if ( g_wpOrig == 0 )
		{
			LogLastError("Change WndProc failed");
		}
		*/

		HANDLE conOut = GetStdHandle(STD_OUTPUT_HANDLE);
		
		DWORD attr = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED; // | BACKGROUND_INTENSITY;
		SetConsoleTextAttribute(conOut , attr);
	
		COORD zerozero = { 0 };
		DWORD dw;
		FillConsoleOutputAttribute(conOut, attr, 100*1000, zerozero, &dw);
	
		/*
		for(int i=0;i<100*100;i++)
		{
			putc(' ',stdout);
		}
		*/
	
		/*
		#ifdef _DEBUG
		puts("MakeConsoleIfNone : Hello Console.");
		#endif
		*/
		
		return true;
	}
	else
	{
		return false;
	}
}

bool ShellOpenFile(HWND parent,const char * file)
{
	TRY
	{
		//system("iexplore.exe gbhelp.html");
		int ret = (int) ShellExecute(
			parent,
			"open",
			file,
			NULL,	//params,
			NULL,
			SW_SHOWNORMAL);
	    
		if ( ret <= 32 ) 
		{
			lprintf("ShellExecute failed, error = %d\n",ret);
			return false;
		}
		else
		{
			return true;
		}
	}
	CATCH
	{
		lprintf("ShellExecute threw\n");
		return false;
	}
}

WindowFocusPusher::WindowFocusPusher(bool autoRestore) :
	m_doRestore(autoRestore)
{
	m_foreground = GetForegroundWindow();
	
	m_focus = NULL;
	
	/*
	DWORD ThreadID1 = GetCurrentThreadId();
	DWORD ThreadID2 = GetWindowThreadProcessId(m_foreground,NULL);
        
    if (ThreadID1 != ThreadID2) AttachThreadInput(ThreadID1,ThreadID2,TRUE);

	m_focus = GetFocus();

    if (ThreadID1 != ThreadID2) AttachThreadInput(ThreadID1,ThreadID2,FALSE);
	*/
}

WindowFocusPusher::~WindowFocusPusher()
{
	if ( m_doRestore )
	{
		Restore();
	}
}

void WindowFocusPusher::Restore()
{
	HWND curFore = GetForegroundWindow();
	if ( m_foreground != curFore )
	{
		/*
		DWORD ThreadID1 = GetCurrentThreadId();
		DWORD ThreadID2 = GetWindowThreadProcessId(m_foreground,NULL);
        
		if (ThreadID1 != ThreadID2) AttachThreadInput(ThreadID1,ThreadID2,TRUE);

		SetForegroundWindow(m_foreground);
		//SetFocus(m_focus);

		if (ThreadID1 != ThreadID2) AttachThreadInput(ThreadID1,ThreadID2,FALSE);
		*/
		
		ReallyActivateWindow(m_foreground);
	}
}

	// get the primary desktop rect minus the taskbar : sexy!
void GetDesktopWorkArea(RECT * pRect)
{
	// get the primary desktop rect minus the taskbar : sexy!
	SystemParametersInfo(SPI_GETWORKAREA,0,pRect,0);
}

bool IsForegroundBeingManipulated()
{
	GUITHREADINFO gti;
	gti.cbSize = sizeof(gti);
	if ( ! GetGUIThreadInfo(0,&gti) )
		return false;
	
	const DWORD c_guiActiveFlags = GUI_INMENUMODE | GUI_INMOVESIZE | GUI_POPUPMENUMODE | GUI_SYSTEMMENUMODE;
	
	if ( gti.flags & c_guiActiveFlags )
	{
		/*
		if ( gti.hwndActive )
		{
			char buffer[80];
			GetWindowText(gti.hwndActive,buffer,sizeof(buffer));
			lprintf("ForegroundBeingManipulated : %s\n",buffer);
		}
		*/
	
		return true;
	}
	else
	{
		return false;
	}
}

BOOL MyAttachThreadInput(DWORD tid1,DWORD tid2,BOOL attach)
{
	return AttachThreadInput(tid1,tid2,attach);
}

typedef BOOL (PASCAL *t_IsHungAppWindow) ( HWND hwnd);
			
BOOL MyIsHungAppWindow(HWND hwnd)
{
	static t_IsHungAppWindow s_func = NULL;
	if ( ! s_func )
	{
		HMODULE u32 = GetModuleHandle("user32.dll");
		if ( u32 == NULL )
		{
			return FALSE;
		}
		s_func = (t_IsHungAppWindow) GetProcAddress(u32,"IsHungAppWindow");
		ASSERT( s_func );
	}
	return (*s_func)(hwnd);
}

END_CB

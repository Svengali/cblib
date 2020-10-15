#pragma once

#include "Base.h"
#include "stl_basics.h"
#include "RefCounted.h"
#include "vector.h"

#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 
#endif
#include <windows.h>
#include <winuser.h>
#include <tlhelp32.h>

START_CB

	// GetWinArgcArgv : get argc,argv in WinMain :
	char ** GetWinArgcArgv(int * pargc);
	void FreeWinArgv(int argc,char ** argv);

	void EnumChildren(HWND hwnd, cb::vector<HWND> *v);
	// if not exactMatch, then we just search for prefixes
	HWND FindEquivWindow(const char * windowType, const char * searchTitle, const bool exactMatchTitle, HWND parent, const bool mustBeVisible, const bool exactMatchType = true);
	void EnumEquivWindows(cb::vector<HWND> *v,const char * typeName, const char * searchForTitle, const bool exactMatch, HWND parent, const bool mustBeVisible, const bool exactMatchType = true);
	
	HWND FindWindowAtRect(const RECT *rc, HWND parent, bool visibleCheck = false, int tolerance = 0);

	void LogChildWindows(HWND hwnd);

	bool IsProcessAlreadyRunning(const char * szMyExeName);
	bool MyKillProcess(const char * szMyExeName);
	// kill my own process :
	void ProcessSuicide();

	void GetProcessName(HANDLE hproc,char * into,int intosize);
	
	DWORD FindPIDByName(const char * szMyExeName, DWORD ignoreId = 0);
	//bool FindAllPIDsByName(cb::vector<DWORD> * pProcesses,const char * szMyExeName, DWORD ignoreId = 0);

	bool FindAllProcessesByName(cb::vector<PROCESSENTRY32> * pProcesses,const char * szMyExeName, DWORD ignoreId );

	//! message should not have a "\n" in it;
	//	it can be NULL for none
	void LogLastError(const char * message = NULL);

	void GetLogLastErrorString(char * pInto,const char * message = NULL);
	
	// send clicks to windows :
	void ClickWindowOnce(HWND w,bool async = false);
	void ClickWindowUntilGone(HWND w);
	void ClickRect(const RECT & in_rect);
	void ClickClientXY(const HWND hwnd,int x,int y,bool async = false);

	// note : this is a really shitty way to do keyboard input, you shouldn't use this :
	bool IsKeyDown( const int vk);
	bool NewKeyDown(const int vk);
	void EatKeyDowns();
	
	// better keyboard input : 
	//	call Tick() before checking keys
	//	call Discard() when your window is activated after being inactive
	void StaticKeys_Tick();
	void StaticKeys_Discard(); // key presses before the Discard call will be ignored
	bool StaticKeys_IsKeyDown(const int vk);
	bool StaticKeys_NewKeyDown(const int vk);
	
	void SendInputKey(int vk); // uses SendInput not Message
	void SendMessageKeyUpDown(HWND w,int vk,bool async = false);

	bool ToggleMinimized(HWND wnd); // returns whether window is visible after the op

	void MyShowWindow(HWND w,bool async);
	void ReallyActivateWindow(HWND w);

	void CopyStringToClipboard(const char * str);

	// SendMessageSafe looks like SendMessage() but will timeout if target app is hung
	LRESULT SendMessageSafe(   
		HWND hWnd,
		UINT Msg,
		WPARAM wParam,
		LPARAM lParam);

	// ShellOpenFile : invokes shell "open" command on file
	bool ShellOpenFile(HWND parent,const char * file);   

	// MakeConsoleIfNone : if I don't have a Console, makes one.  then just use stdout as usual.
	bool MakeConsoleIfNone(const char * title = NULL);
	// RespawnDetachedIfConsole : if I was started as a console app, kill me and respawn detached from a console (DOES NOT RETURN)
	void RespawnDetachedIfConsole();
	// ShowConsole : show/hide console window (if you have one)
	void ShowConsole( bool show );

	inline int Width(const RECT & r) { return r.right - r.left; }
	inline int Height(const RECT & r) { return r.bottom - r.top; }

	inline bool RectsIntersect(RECT *a,RECT *b)
	{
		RECT t;
		return !!IntersectRect(&t,a,b);
	}
	
	// InvalidateWindow: make it redraw
	inline void InvalidateWindow(HWND w) { InvalidateRect(w,NULL,FALSE); }

	// get the primary desktop rect minus the taskbar : sexy!
	void GetDesktopWorkArea(RECT * pRect);

	// you should not steal focus if IsForegroundBeingManipulated
	bool IsForegroundBeingManipulated();

	BOOL MyIsHungAppWindow(HWND hwnd);

	BOOL MyAttachThreadInput(DWORD tid1,DWORD tid2,BOOL attach);

	//////////////////////////////////////////////////////////////////////
	
    class UseCriticalSection ;

	class CriticalSection
	{
	public      :

		CriticalSection();
		~CriticalSection();

		void Lock();
		void Unlock();

		friend UseCriticalSection ;
	private     :
		CRITICAL_SECTION m_CritSec ;
	} ;

	// UseCriticalSection will Enter/Leave in scope
	class UseCriticalSection
	{
	public      :
		explicit UseCriticalSection ( CriticalSection & cs );
		explicit UseCriticalSection ( CRITICAL_SECTION & cs );

		~UseCriticalSection ( );

	private     :
		UseCriticalSection ( void )
		{
			m_pCS = NULL ;
		}
		CRITICAL_SECTION * m_pCS ;
	} ;

	// CB_SCOPE_CRITICAL_SECTION : hold crit in scope
	#define CB_SCOPE_CRITICAL_SECTION(crit)	cb::UseCriticalSection NUMBERNAME(critscoper) (crit)

	// WindowFocusPusher : saves and restores focus/foregound in case you mess with it :
	class WindowFocusPusher
    {
    public:
		explicit WindowFocusPusher(bool autoRestore);
		~WindowFocusPusher();
		void Restore();
	private:
		bool	m_doRestore;
		HWND	m_foreground;
		HWND	m_focus;
    };

	class SetCursorInScope
	{
	public :
		HCURSOR m_old;
		 SetCursorInScope(HCURSOR newCursor) { m_old = SetCursor( newCursor ); }
		~SetCursorInScope() { SetCursor( m_old ); }
	};
	
	// little scoper you can toss in any time you do something slow : CB_SCOPE_BUSY_CURSOR();
	#define CB_SCOPE_BUSY_CURSOR()	SetCursorInScope NUMBERNAME(scoper) ( LoadCursor(NULL,IDC_WAIT) )
		
	// BmpImage/HBITMAP utils :
	SPtrFwd(BmpImage);

	// returns a BmpImage in the same format as the HBITMAP was :
	BmpImagePtr ConvertBitmap(HBITMAP hBitmap);
	BmpImagePtr GrabDesktop();

	// useful for stepping through a sequence with persistence :
	//	 can be used for or instead of random seeding as well
	int GetAndAdvanceRegistryCounter();

	// GetCursorInClientRect returns a frac in [0,1] in the client rect
	//	returns true/false for in rect
	class Vec2;
	bool GetCursorInClientRect(HWND hwnd, Vec2 * pFracPos);
    
    //=================================

    void TrashTheCache();

	// if num_repeats <= 0 , GetMinFuncTicks just runs func once
	// returns ticks
	typedef void (fp_void_void)(void);
	uint64 GetMinFuncTicks( fp_void_void * pfunc ,int min_num_repeats, double min_time, bool trashTheCache, bool logdots);
    
END_CB

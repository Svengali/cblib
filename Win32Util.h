#pragma once

#include "cblib/Base.h"
#include "cblib/stl_basics.h"
#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 
#endif
#include <windows.h>
#include <winuser.h>
#include "cblib/vector.h"

START_CB

	void EnumChildren(HWND hwnd, cb::vector<HWND> *v);
	// if not exactMatch, then we just search for prefixes
	HWND FindEquivWindow(const char * windowType, const char * searchTitle, const bool exactMatchTitle, HWND parent, const bool mustBeVisible, const bool exactMatchType = true);
	void EnumEquivWindows(cb::vector<HWND> *v,const char * typeName, const char * searchForTitle, const bool exactMatch, HWND parent, const bool mustBeVisible, const bool exactMatchType = true);
	
	HWND FindWindowAtRect(const RECT *rc, HWND parent, bool visibleCheck = false, int tolerance = 0);

	void MyKillProcess(const char * szMyExeName);

	bool IsProcessAlreadyRunning(const char * szMyExeName);

	//! message should not have a "\n" in it;
	//	it can be NULL for none
	void LogLastError(const char * message = NULL);

	void GetLogLastErrorString(char * pInto,const char * message = NULL);
	
	void ClickWindowOnce(HWND w,bool async = false);
	void ClickWindowUntilGone(HWND w);
	void ClickRect(const RECT & in_rect);
	
	void ClickClientXY(const HWND hwnd,int x,int y,bool async = false);

	bool IsKeyDown(const int vk);
	bool NewKeyDown(const int vk);
	
	void SendInputKey(int vk); // uses SendInput not Message
	void SendMessageKeyUpDown(HWND w,int vk,bool async = false);

	bool ToggleMinimized(HWND wnd); // returns whether window is now visible or not

	void MyShowWindow(HWND w,bool async);
	void ReallyActivateWindow(HWND w);

	void CopyStringToClipboard(const char * str);

	// SendMessageSafe looks like SendMessage() but will timeout if target app is hung
	LRESULT SendMessageSafe(   
		HWND hWnd,
		UINT Msg,
		WPARAM wParam,
		LPARAM lParam);

	bool ShellOpenFile(HWND parent,const char * file);   

	// MakeConsoleIfNone : if I don't have a Console, makes one.  then just use stdout as usual.
	bool MakeConsoleIfNone(const char * title = NULL);
	// RespawnDetachedIfConsole : if I was started as a console app, kill me and respawn detached from a console (DOES NOT RETURN)
	void RespawnDetachedIfConsole();

	inline int Width(const RECT & r) { return r.right - r.left; }
	inline int Height(const RECT & r) { return r.bottom - r.top; }

	// get the primary desktop rect minus the taskbar : sexy!
	void GetDesktopWorkArea(RECT * pRect);

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
    
END_CB

#pragma once

#include "cblib/Base.h"
#include "cblib/Win32Util.h"
#include "cblib/SPtr.h"
//#include <windows.h>

START_CB

SPtrFwd(Wnd);
class Wnd : public RefCounted
{
public:
	   				Wnd();
	virtual			~Wnd();
	HWND			   CreateWnd(HWND parent, DWORD style, DWORD exstyle, int x, int y, int w, int h);
	HWND			   GetHwnd(void) const { return m_hwnd; }
	static void		RegisterWindowClass(void);
	
	// message maps
	virtual void	WmCommand(WPARAM wParam, LPARAM lParam) { }
	virtual void	WmCreate(void) { }
	virtual void	WmDestroy(void) { }
	virtual void	WmKeydown(WPARAM wParam, LPARAM lParam) { }
	virtual void	WmKeyup(WPARAM wParam, LPARAM lParam) { }
	virtual void	WmLButtonDown(int x, int y) { }
	virtual void	WmLButtonDoubleClick(int x, int y) { }
	virtual void	WmLButtonUp(int x, int y) { }
	virtual void	WmMButtonDown(int x, int y) { }
	virtual void	WmMButtonUp(int x, int y) { }
	virtual void	WmRButtonDown(int x, int y) { }
	virtual void	WmRButtonUp(int x, int y) { }
	virtual void	WmMouseMove(int x, int y) { }
	virtual void	WmMouseWheel(int wheelDelta, int x, int y) { }
	virtual void	WmNotify(WPARAM wParam, LPARAM lParam) { }
	virtual void	WmPaint(HDC hdc) { }
	virtual void	WmSize(int x, int y);
	virtual void	WmSysKeydown(WPARAM wParam, LPARAM lParam) { }
	virtual void	WmSysKeyup(WPARAM wParam, LPARAM lParam) { }
	virtual void	WmTimer(void) { }
	virtual void	WmVScroll(WPARAM wParam, HWND lParam) { }
	virtual void	WmHScroll(WPARAM wParam, HWND lParam) { }
	
	// window procedure
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
	
	void UpdateRects();
	
	HWND	m_hwnd;			// the main hwnd for the widget
	RECT	m_windowRect;  // the window rect for this window
	RECT	m_clientRect;  // the client rect for this window
	//RECT	m_ssClientRect;// the screen space client rect for this window
	int		m_width;
	int		m_height;

};

END_CB

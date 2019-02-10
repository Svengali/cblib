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
	virtual void	WmCommand(WPARAM, LPARAM) { }
	virtual void	WmCreate(void) { }
	virtual void	WmDestroy(void) { }
	virtual void	WmKeydown(WPARAM, LPARAM) { }
	virtual void	WmKeyup(WPARAM, LPARAM) { }
	virtual void	WmLButtonDown(int , int ) { }
	virtual void	WmLButtonDoubleClick(int , int ) { }
	virtual void	WmLButtonUp(int , int ) { }
	virtual void	WmMButtonDown(int, int ) { }
	virtual void	WmMButtonUp(int, int ) { }
	virtual void	WmRButtonDown(int, int ) { }
	virtual void	WmRButtonUp(int, int ) { }
	virtual void	WmMouseMove(int, int ) { }
	virtual void	WmMouseWheel(int , int, int ) { }
	virtual void	WmNotify(WPARAM , LPARAM ) { }
	virtual void	WmPaint(HDC ) { }
	virtual void	WmSize(int, int );
	virtual void	WmSysKeydown(WPARAM , LPARAM ) { }
	virtual void	WmSysKeyup(WPARAM , LPARAM ) { }
	virtual void	WmTimer(void) { }
	virtual void	WmVScroll(WPARAM , HWND ) { }
	virtual void	WmHScroll(WPARAM , HWND ) { }
	
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

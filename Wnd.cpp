#include "Wnd.h"
//#include <zmouse.h>

START_CB

static const char * c_WndClassName = "myWnd";

Wnd::Wnd() :
	m_hwnd(NULL)
{
}

Wnd::~Wnd()
{
	if (m_hwnd != NULL)
	{
		DestroyWindow(m_hwnd);
	}
}

// static 
HWND Wnd::CreateWnd(HWND parent,
					DWORD style,
					DWORD exstyle,
					int x, int y,
					int width, int height)
{
	RegisterWindowClass();

	m_hwnd = CreateWindowEx(	
		exstyle,
		c_WndClassName,
		NULL,
		style,
		x,
		y,
		width,
		height,
		parent,
		NULL,
		GetModuleHandle(NULL),
		(LPVOID)this);

// crashes on Windows ME
//	SendMessage(m_hwnd, WM_SETFONT, (WPARAM)GetStockObject(ANSI_VAR_FONT), 1);

	return m_hwnd;
}

void Wnd::RegisterWindowClass(void)
{
	WNDCLASS wndclass;

	if (!GetClassInfo(GetModuleHandle(NULL), c_WndClassName, &wndclass))
	{
		memset(&wndclass, 0, sizeof(WNDCLASS));
		// CS_OWNDC
		// | CS_PARENTDC);
		wndclass.style = (CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_CLASSDC); 
		wndclass.lpfnWndProc  = Wnd::WndProc;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = sizeof(Wnd*);
		wndclass.hInstance = GetModuleHandle(NULL);
		wndclass.hIcon = LoadIcon(GetModuleHandle(NULL), "IDI_ICON1");//MAKEINTRESOURCE(IDI_ICON1));
		wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
		//wndclass.hbrBackground = (HBRUSH) (COLOR_BTNFACE+1);
		//wndclass.hbrBackground = (HBRUSH) COLOR_BACKGROUND;
		wndclass.hbrBackground = CreateSolidBrush(RGB(225, 225, 225));
		wndclass.lpszMenuName = NULL;
		wndclass.lpszClassName = (char*)c_WndClassName;
		RegisterClass(&wndclass);
	}
}

void Wnd::UpdateRects()
{
	GetClientRect(m_hwnd,&m_clientRect);
	GetWindowRect(m_hwnd,&m_windowRect);
}
	
void Wnd::WmSize(int width, int height)
{
	m_width = width;
	m_height = height;
	UpdateRects();
}

LRESULT CALLBACK Wnd::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Wnd* wnd = NULL;

	if (msg == WM_CREATE)
	{
		if ((lParam) && ((CREATESTRUCT*)lParam)->lpCreateParams)
		{
			wnd = (Wnd*)((CREATESTRUCT*)lParam)->lpCreateParams;
		}

		wnd->m_hwnd = hwnd;
		wnd->WmCreate();

 		SetWindowLongPtr(hwnd, 0, (LONG_PTR)wnd);
		return 0;
	}

	wnd = (Wnd*)GetWindowLongPtr(hwnd, 0);

	if (wnd != NULL)
	{
		switch (msg)
		{
			//*
		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				wnd->WmPaint((HDC)BeginPaint(hwnd, &ps));
				EndPaint(hwnd, &ps);
			}
			return 0;
			//*/

		case WM_PRINTCLIENT:
			if (lParam & PRF_CLIENT)
			{
				wnd->WmPaint((HDC)wParam);
				lParam = lParam & ~PRF_CLIENT;
			}
			break;

		case WM_LBUTTONDBLCLK:
			wnd->WmLButtonDoubleClick((short)LOWORD(lParam), (short)HIWORD(lParam));
			break;

		case WM_LBUTTONDOWN:
			wnd->WmLButtonDown((short)LOWORD(lParam), (short)HIWORD(lParam));
			break;

		case WM_LBUTTONUP:
			wnd->WmLButtonUp((short)LOWORD(lParam), (short)HIWORD(lParam));
			break;

		case WM_MBUTTONDOWN:
			wnd->WmMButtonDown((short)LOWORD(lParam), (short)HIWORD(lParam));
			break;

		case WM_MBUTTONUP:
			wnd->WmMButtonUp((short)LOWORD(lParam), (short)HIWORD(lParam));
			break;

		case WM_RBUTTONDOWN:
			wnd->WmRButtonDown((short)LOWORD(lParam), (short)HIWORD(lParam));
			break;

		case WM_RBUTTONUP:
			wnd->WmRButtonUp((short)LOWORD(lParam), (short)HIWORD(lParam));
			break;

		case WM_MOUSEMOVE:
			wnd->WmMouseMove((short)LOWORD(lParam), (short)HIWORD(lParam));
			return 0;

		case WM_MOUSEWHEEL:
			wnd->WmMouseWheel(((short)HIWORD(wParam)), (short)LOWORD(lParam), (short)HIWORD(lParam));
			break;

		case WM_NOTIFY:
			wnd->WmNotify(wParam, lParam);
			break;

		case WM_SIZE:
			wnd->WmSize((short)LOWORD(lParam), (short)HIWORD(lParam));
			break;

		case WM_HSCROLL:
			wnd->WmHScroll(wParam, (HWND)lParam);
			break;

		case WM_VSCROLL:
			wnd->WmVScroll(wParam, (HWND)lParam);
			break;

		case WM_COMMAND:
			wnd->WmCommand(wParam, lParam);
			break;

		case WM_KEYDOWN:
			wnd->WmKeydown(wParam, lParam);
			break;

		case WM_KEYUP:
			wnd->WmKeyup(wParam, lParam);
			break;

		case WM_SYSKEYDOWN:
			wnd->WmSysKeydown(wParam, lParam);
			break;

		case WM_SYSKEYUP:
			wnd->WmSysKeyup(wParam, lParam);
			break;

		case WM_TIMER:
			wnd->WmTimer();
			break;

		case WM_DESTROY:
			wnd->WmDestroy();
			break;
		}
	}

	// call the default window proc
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

END_CB

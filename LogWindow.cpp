
//#define _WIN32_WINDOWS	0x0500
#define _WIN32_WINNT	0x0501

#define SELECTED_ALLOCATOR	New_Allocator

#include "logwindow.h"
#include "Base.h"
#include "Log.h"
#include "Threading.h"
#include "Threading.h"
#include "Win32Util.h"
#include "StrUtil.h"
//#include "LF/NodeAllocator.h"
//#include "LF/LFSPSC2.h"
#include <stdio.h>

START_CB

#define DO_USE_TIMER

#define TICK_FPS	(30)

namespace LogWindowInternal
{

#define WM_LOG_UPDATE		(WM_USER + 0x30)
#define WM_LOG_PUTS			(WM_USER + 0x31)
#define WM_LOG_SETCOLOR		(WM_USER + 0x32)
#define WM_LOG_SCROLLTOEND	(WM_USER + 0x33)

char * MyStrDup(const char * str)
{
	int len = strlen32(str) + 1;
	char * ret = (char *) CBALLOC( len );
	memcpy(ret,str,len);
	return ret;
}

void MyStrFree(char * ptr)
{
	CBFREE(ptr);
}


void ErrorExit(const char * str)
{
	//__asm int 3;
	ASSERT_BREAK();
	exit(10);
}

#define BIGBUFFER_COUNT		(4096)
#define BIGBUFFER_STRLEN	(256)
// 4096 * 256 = 1 MB

#define mainClass		"LogWindowMainClass"

#define SCROLLBAR_SIZE		20

#define LIST_TOP_SPACE		0	// header
#define LIST_LEFT_SPACE		(10)
#define LIST_RIGHT_SPACE 	(5)

#define LINE_HEIGHT(dp)		( dp->textSize.cy + 1 )
#define LIST_VISIBLE(rect,dp)	( ( Height(rect) - LIST_TOP_SPACE ) / LINE_HEIGHT(dp) )

//-----------------------------
// these variables are owned by the LogWindow thread :

typedef struct 
{
	bool eatClick,inPopup;
	bool isAtEnd;
	int topEntry,numEntries;
	HWND window,scrollbar;
	RECT clientRect;
	SIZE textSize;
	bool middleDown;
	int middleDownY;
	bool needsRedraw;
} windowData;

static RECT cfg_lastWindowPos = { -1,-1,-1,-1 };

static HBRUSH backgroundBrush = NULL;
static HFONT listFont = NULL;

static const DWORD c_defaultColorText = 0x00000000;
static const DWORD c_defaultColorBack = 0x00FFFFFF;

static DWORD g_colorText = c_defaultColorText;
static DWORD g_colorBack = c_defaultColorBack;

struct Line
{
	DWORD	color;
	char	str[BIGBUFFER_STRLEN];
};

static Line bigbuffer[BIGBUFFER_COUNT];
static int bigbufferNext = 0;

static int g_cmdShow = 0;

#ifdef DO_USE_TIMER
static UINT_PTR g_timer = 0;
#else
static HANDLE g_event = 0;
#endif

static windowData * g_windowData = NULL;

static int g_screenSizeX = 0;
static int g_screenSizeY = 0;

/*
static SPSC_FIFO2 g_fifo;
static New_Allocator<SPSC_FIFO2_Node>	g_allocator;
*/

//-----------------------------
// these variables are atomic and shared :

static char g_pad1[LF_CACHE_LINE_SIZE];

static LF_ALIGN_TO_CACHE_LINE HWND g_window = 0;

static char g_pad3[LF_CACHE_LINE_SIZE];

//-----------------------------

static void newScrollWindow( windowData * dp )
{
	RECT rect;
	HWND windowH;

	GetClientRect(dp->window,&rect);

	#ifdef _WIN64
	HINSTANCE inst = (HINSTANCE) GetWindowLongPtr(dp->window, GWLP_HINSTANCE);
	#else
	HINSTANCE inst = (HINSTANCE) GetWindowLong(dp->window,GWL_HINSTANCE);
	#endif
	
	windowH = CreateWindow(
		"scrollbar",			/* class */
		NULL,					/* caption */
		WS_CHILD | WS_BORDER | SBS_VERT | SBS_RIGHTALIGN,	/* style */
		rect.right - SCROLLBAR_SIZE, rect.top + LIST_TOP_SPACE,
		SCROLLBAR_SIZE, Height(rect) - LIST_TOP_SPACE,
		dp->window,             /* parent window */
		NULL,                   /* menu handle */
		inst,					/* program handle */
		NULL                    /* create parms */
		);

	if ( ! windowH )
		ErrorExit("couldn't open ScrollBar child window!");

	dp->scrollbar = windowH;

	ShowWindow( windowH, SW_SHOW );
	UpdateWindow( windowH );
}

static void newWindow(HINSTANCE inst,int cmdShow)
{
	HWND windowH;
	windowData *dp;

	g_screenSizeY = GetSystemMetrics(SM_CYFULLSCREEN);
	g_screenSizeX = GetSystemMetrics(SM_CXFULLSCREEN);
		
	// make window in same place as last time :
	if ( cfg_lastWindowPos.bottom == -1 )
	{
		// defaults here :
		cfg_lastWindowPos.left = g_screenSizeX/4;
		cfg_lastWindowPos.right = g_screenSizeX*3/4;
		cfg_lastWindowPos.top = 10;
		cfg_lastWindowPos.bottom = g_screenSizeY - 20;
	}

	windowH = CreateWindow(
		mainClass,			/* class */
		"LogWindow",		/* caption */
		WS_OVERLAPPEDWINDOW | WS_TABSTOP, /* style */
		cfg_lastWindowPos.left,
		cfg_lastWindowPos.top,
		cfg_lastWindowPos.right-cfg_lastWindowPos.left,
		cfg_lastWindowPos.bottom-cfg_lastWindowPos.top,
		NULL,                   /* parent window */
		NULL,                   /* menu handle */
		inst,              		/* program handle */
		NULL                    /* create parms */
		);

	if ( ! windowH )
		ErrorExit("open main window failed!");


	#ifdef DO_USE_TIMER
	// tick me at 10 fps :
	g_timer = SetTimer( windowH, 1, (1000/TICK_FPS), NULL );
	#endif
	
	if ( (dp = new windowData) == NULL )
		{ CloseWindow(windowH); ErrorExit("malloc failed!"); }

	g_windowData = dp;

	/**** init windowData values ****/

	memset(dp,0,sizeof(windowData));

	dp->isAtEnd = true;

	dp->numEntries = 0;
	bigbufferNext = 0;

	dp->window = windowH;

	SetWindowLongPtr( windowH, 0, (LONG_PTR) dp );

	{
	HDC hdc;

	hdc = GetDC(windowH);
	SelectObject( hdc, listFont );
	GetTextExtentPoint32(hdc,"W",1,&(dp->textSize));
	
	ReleaseDC(windowH,hdc);
	}

	newScrollWindow( dp );

	GetClientRect(dp->window,&(dp->clientRect));

	ShowWindow( windowH, cmdShow );
	UpdateWindow( windowH );

	StoreRelease(&g_window,windowH);
}


static void entriesChanged(windowData *dp,int from,int to);
static void moveListTop(windowData *dp,int from,int to,bool callChange);
static LRESULT PASCAL mainWindowProc( HWND, unsigned, WPARAM, LPARAM );

// callback bool tells you whether to echo the log to other outputs; a false supresses all output
//typedef bool (t_rtlLogCallback) (const char * buffer);
static bool MyLogCallback(const char * buffer)
{
	LogWindow::Puts(buffer);
	return true;
}

static void Internal_Open(int cmdShow)
{
    HINSTANCE instance = (HINSTANCE)GetModuleHandle(NULL);

	//winUtilInit();

	/*
	 * set up and register window classes
	 */

	{
		WNDCLASS    wc;

		wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wc.lpfnWndProc = (WNDPROC) mainWindowProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = sizeof( DWORD );
		wc.hInstance = instance;
		wc.hIcon = 0;
		wc.hCursor = LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground = backgroundBrush;
		wc.lpszMenuName  = NULL;
		wc.lpszClassName = mainClass;

		if ( !RegisterClass( &wc ) )
			ErrorExit("RegisterClass mainWindow failed");
	}

	{
		LOGFONT lf;

		lf.lfHeight = 16;
		lf.lfWidth = 8;
		lf.lfEscapement = 0;
		lf.lfOrientation = 0;
		lf.lfWeight = 400;
		lf.lfItalic = 0;
		lf.lfUnderline =0;
		lf.lfStrikeOut =0;
		lf.lfCharSet= ANSI_CHARSET;
		lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
		lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
		lf.lfQuality= DEFAULT_QUALITY;
		lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
		lf.lfFaceName[0] = 0;

		listFont = CreateFontIndirect(&lf);
	}

	//selectPen = CreatePen(PS_SOLID,3,SELECT_COLOR); //yellow
	//unselectPen = CreatePen(PS_SOLID,3,g_colorBack); //black

	//HEADER_GRAY = GetSysColor(COLOR_ACTIVEBORDER);
	//headerGrayBrush = CreateSolidBrush( HEADER_GRAY );
	//headerGrayBrush = GetSysColorBrush(COLOR_ACTIVEBORDER);
	
	backgroundBrush = CreateSolidBrush(g_colorBack);

	newWindow(instance,cmdShow);

	// test :
	/*
	for(int i=0;i<100;i++)
	{
		Printf("killme\r");
		Printf("test %d test %d test %d test %d test %d test %d test %d test %d test %d\n",i,i,i,i,i,i,i,i,i,i,i,i,i);
	}
	{for(int i=0;i<100;i++)
	{
		Printf("wrap %d \t %d test %d \t %d test %d test %d \t %d test %d test\t%d\t",i,i,i,i,i,i,i,i,i,i,i,i,i);
	}}
	*/
		
	ShowConsole(false);
}

static void Internal_SetColor(DWORD text,DWORD background)
{
	g_colorText = text;
	g_colorBack = background;

	bigbuffer[bigbufferNext].color = text;

	if ( backgroundBrush != NULL )
	{
		DeleteObject(backgroundBrush);
	}
	
	backgroundBrush = CreateSolidBrush(background);
}
	
static void Internal_Close()
{	
	HWND hwnd = g_window;
	StoreRelease(&g_window,(HWND)0);
	
	if ( hwnd )
	{
		// DestroyWindow is a SendMessage(WM_DESTROY)
		//	it must be called by the owning thread
		DestroyWindow( hwnd );
	}
	
	DeleteObject(listFont);
	listFont = NULL;
	DeleteObject(backgroundBrush);
	backgroundBrush = NULL;

	//winUtilDispose();
}

int g_entryToBuffer = 0;

static int EntryToBB(const int i)
{
	int b = i + g_entryToBuffer;
	while ( b < 0 )
	{
		b += BIGBUFFER_COUNT;
	}
	return b;
}

static const char* GetEntryString(const int i)
{
	return bigbuffer[EntryToBB(i)].str;
}

static void Internal_ScrollToEnd(bool callChange)
{
	moveListTop(g_windowData,g_windowData->topEntry,g_windowData->numEntries,callChange);
	g_windowData->isAtEnd = true;
}
	
static void Advance()
{
	g_windowData->numEntries++;

	bigbufferNext++;
	if ( bigbufferNext >= BIGBUFFER_COUNT )
	{
		bigbufferNext = 0;
	}
	bigbuffer[bigbufferNext].str[0] = 0;
	bigbuffer[bigbufferNext].color = g_colorText;

	g_entryToBuffer = bigbufferNext - g_windowData->numEntries;

	// if last entry was visible, scroll topEntry

	RECT winR = g_windowData->clientRect;
	//GetClientRect(g_window,&winR);
	//int lineHeight = g_windowData->textSize.cy + 1;
	int visibleEntries = LIST_VISIBLE(winR,g_windowData);

	if ( g_windowData->topEntry + visibleEntries >= g_windowData->numEntries )
	{
		Internal_ScrollToEnd(false);
		g_windowData->needsRedraw = true;
	}
	int low = g_windowData->numEntries - BIGBUFFER_COUNT;
	if ( g_windowData->topEntry < low )
	{
		g_windowData->topEntry = low;
	}
}

static void Internal_Puts(const char* str)
{
	// parse str for \t , \r, \n

	char * pTo = bigbuffer[bigbufferNext].str;
	int toLen = strlen32(pTo);

	#define ADVANCE()	\
	pTo[toLen] = 0; Advance(); pTo = bigbuffer[bigbufferNext].str; toLen = strlen32(pTo);

	//RECT win;
	//GetWindowRect(g_windowData->window,&win);
	int sx = g_screenSizeX;
	int wrapLen = (sx - 10 - SCROLLBAR_SIZE - LIST_LEFT_SPACE - LIST_RIGHT_SPACE) / g_windowData->textSize.cx;
	if (wrapLen < 10) wrapLen = 10;
	else if ( wrapLen > BIGBUFFER_STRLEN ) wrapLen = BIGBUFFER_STRLEN;

	while(*str)
	{
		if ( toLen == wrapLen-2 )
		{
			pTo[toLen++] = '\\';
			ADVANCE();
		}
		else if ( *str == '\n' )
		{
			str++;
			ADVANCE();
		}
		else if ( *str == '\r' )
		{
			str++;
			pTo[0] = 0;
		}
		else if ( *str == '\t' )
		{
			if ( toLen >= wrapLen-5 )
			{
				pTo[toLen++] = '\\';
				ADVANCE();
			}
			else
			{
				str++;
				pTo[toLen++] = ' ';
				pTo[toLen++] = ' ';
				pTo[toLen++] = ' ';
				pTo[toLen++] = ' ';
			}
		}
		else
		{
			pTo[toLen++] = *str++;			
		}
	}
	pTo[toLen] = 0;

	g_windowData->needsRedraw = true;
}


static void freeWindowData(windowData * dp)
{
	if ( ! dp ) return;

	RECT win;
	GetWindowRect(dp->window,&win);
	cfg_lastWindowPos = win;

	delete dp;
}

static void moveListTop(windowData *dp,int from,int to,bool callChange)
{
	int visible;
	RECT rect;

	rect = dp->clientRect;
	//GetClientRect(dp->window,&rect);

	visible = LIST_VISIBLE(rect,dp);

	int low = max(0, dp->numEntries - BIGBUFFER_COUNT );

	if ( to < low )
	{
		to = low;
	}
	else if ( (to + visible) > dp->numEntries )
	{
		to = max( dp->numEntries - visible + 1 , 0 );
	}

	dp->topEntry = to;

	if ( callChange )
	{
		if ( from == to ) 
		{
			entriesChanged(dp,0,0);
		}
		else 
		{
			entriesChanged(dp,dp->topEntry,dp->topEntry + visible);
		}
	}
}

static void entriesChanged(windowData *dp,int from,int to)
{
RECT rect,winR;
int visible;

	rect = dp->clientRect;
	//GetClientRect(dp->window,&rect);

	winR = rect;
	//copyRect(winR,rect);

	visible = LIST_VISIBLE(rect,dp);

	int low = max(0, dp->numEntries - BIGBUFFER_COUNT );

	SetScrollRange( dp->scrollbar, SB_CTL, low,
		max( 0, (dp->numEntries+1) - visible ), FALSE);

	SetScrollPos( dp->scrollbar, SB_CTL, dp->topEntry, TRUE );

	if ( dp->topEntry + visible >= dp->numEntries )
		dp->isAtEnd = true;
	else
		dp->isAtEnd = false;

	if ( to >= from ) 
	{
		int h = LINE_HEIGHT(dp);

		rect.top += ((from - dp->topEntry) * h + LIST_TOP_SPACE);
		rect.bottom = rect.top + (to - from + 1) * h;

		if ( RectsIntersect(&winR,&rect) ) 
		{
			InvalidateRect( dp->window, &rect, FALSE );
			SendMessage( dp->window, WM_PAINT, 0, 0);
		}
	}

}

void Internal_PushFIFO(char * str)
{
	//void * data = (void *) str;
	//SPSC_FIFO2_PushData(&g_fifo,&g_allocator,data);
}

void Internal_PopFIFO()
{
    /*
	void * data;
	while( (data = SPSC_FIFO2_PopData(&g_fifo)) != NULL )
	{
		char * str = (char *)data;
		
		Internal_Puts(str);
		
		MyStrFree(str);
	}
	*/
}		

LRESULT FAR PASCAL mainWindowProc( HWND windowH, unsigned msg,
                                     WPARAM wParam, LPARAM lParam )
{
	// lazy :
	windowData *dp;

	dp = (windowData *) GetWindowLongPtr( windowH,0);

	switch( msg ) 
	{

	/*
	case WM_LOG_PUTS:
		{
		char * str = (char *)lParam;
		
		Internal_Puts(str);
		
		MyStrFree(str);
		}
		break;
	*/

	case WM_LOG_SETCOLOR:
		{
		DWORD text = (DWORD) wParam;
		DWORD back = (DWORD) lParam;
		
		Internal_SetColor(text,back);
		}
		break;

	case WM_LOG_SCROLLTOEND:
		{
		Internal_ScrollToEnd(true);
		}
		break;


	/*
	case WM_MOUSEACTIVATE:
		dp->eatClick = TRUE;
		break;
		*/
	// don't want to eat clicks on change to/from my children
	case WM_ACTIVATE:
		if ( LOWORD(wParam) == WA_CLICKACTIVE ) 
		{
			if ( ! IsChild(dp->window,(HWND)lParam) )
				dp->eatClick = TRUE;
		}
		break;

	case WM_LBUTTONDOWN:
		if ( dp->eatClick )
		{
			dp->eatClick = FALSE;
		}
		else 
		{
		}
		break;
	case WM_MBUTTONDOWN:
		{
			SetCapture(windowH);
			dp->middleDown = true;
			POINT mouse;
			GetCursorPos(&mouse);
			dp->middleDownY = mouse.y;

			#pragma PRAGMA_MESSAGE("@@@@ need to draw that funny widget at the middle-down pause")
			//	during the whole time the middle is done
		}
		break;
	case WM_MBUTTONUP:
		{
			ReleaseCapture();
			dp->middleDown = false;
		}
		break;

	case WM_TIMER:
		
	//	break;
	//case WM_LOG_UPDATE:
		{		
		//Internal_PopFIFO();
		
		bool middleDown = !!(GetAsyncKeyState(VK_MBUTTON) & 0x8000);
		if ( ! middleDown && dp->middleDown )
		{
			ReleaseCapture();
			dp->middleDown = false;
		}
		if ( middleDown )
		{
			static float s_scroll = 0;
			POINT mouse;
			GetCursorPos(&mouse);
			int dy = mouse.y - dp->middleDownY;
			int screenSize = g_screenSizeY;
			float t = dy / (float)screenSize;
			float a = 100.f * t * t;
			if ( dy > 0 ) s_scroll -= a;
			if ( dy < 0 ) s_scroll += a;
			while(s_scroll >= 1.f )
			{
				s_scroll -= 1.f;
				moveListTop(dp,dp->topEntry,dp->topEntry - 1,true);
			}
			while(s_scroll <= -1.f )
			{
				s_scroll += 1.f;
				moveListTop(dp,dp->topEntry,dp->topEntry + 1,true);
			}
		}
		if ( dp->needsRedraw )
		{
			dp->needsRedraw = false;
			entriesChanged(g_windowData,0,0);
			InvalidateWindow(g_window);
		}
		}
		break;

	case WM_MOUSEWHEEL:
		{
			static int s_zDelta = 0;
			//int fwKeys = LOWORD(wParam);    // key flags
			int zDelta = (short) HIWORD(wParam);    // wheel rotation
			
			s_zDelta += zDelta;
			while(s_zDelta >= WHEEL_DELTA )
			{
				s_zDelta -= WHEEL_DELTA;
				moveListTop(dp,dp->topEntry,dp->topEntry - 1,true);
			}
			while(s_zDelta <= -WHEEL_DELTA )
			{
				s_zDelta += WHEEL_DELTA;
				moveListTop(dp,dp->topEntry,dp->topEntry + 1,true);
			}
		}
		break;
 

	case WM_CHAR:
		break;

	case WM_KEYDOWN:
		{
			// do PgUp/PgDn/Home/End
			RECT rect;
			GetClientRect(windowH,&rect);
			int vk = (int) wParam;
			switch(vk)
			{
				case VK_HOME:
					moveListTop(dp,dp->topEntry,0,true);
					break;
				case VK_END:
					moveListTop(dp,dp->topEntry,dp->numEntries,true);
					break;
				case VK_PRIOR:
					moveListTop(dp,dp->topEntry,dp->topEntry - LIST_VISIBLE(rect,dp),true);
					break;
				case VK_NEXT:
					moveListTop(dp,dp->topEntry,dp->topEntry + LIST_VISIBLE(rect,dp),true);
					break;
			}
		}
		break;

	case WM_VSCROLL:
		if ( dp ) 
		{
			switch( LOWORD(wParam) ) 
			{
			case SB_TOP:
				moveListTop(dp,dp->topEntry,0,true);
				break;
			case SB_BOTTOM:
				moveListTop(dp,dp->topEntry,dp->numEntries,true);
				break;
			case SB_PAGEDOWN: {
				RECT rect;
				GetClientRect(windowH,&rect);
				moveListTop(dp,dp->topEntry,dp->topEntry + LIST_VISIBLE(rect,dp),true);
				break;
			}
			case SB_PAGEUP:{
				RECT rect;
				GetClientRect(windowH,&rect);
				moveListTop(dp,dp->topEntry,dp->topEntry - LIST_VISIBLE(rect,dp),true);
				break;
			}
			case SB_LINEDOWN:
				moveListTop(dp,dp->topEntry,dp->topEntry + 1,true);
				break;
			case SB_LINEUP:
				moveListTop(dp,dp->topEntry,dp->topEntry - 1,true);
				break;
			case SB_THUMBTRACK:
			case SB_THUMBPOSITION:
				moveListTop(dp,dp->topEntry,HIWORD(wParam),true);
				break;
			}
		}
		break;

	case WM_PAINT:
		if ( dp ) 
		{

		RECT	winR;
		HDC		hdc;
		PAINTSTRUCT ps;
		int visibleEntries,entryNum,i;

		hdc = BeginPaint(windowH,&ps);

		SelectObject( hdc, listFont );

		GetClientRect(windowH,&winR);

		SetBkColor(   hdc, g_colorBack );

		int lineHeight = dp->textSize.cy + 1;

		visibleEntries = LIST_VISIBLE(winR,dp);

		for(i=0;i<=visibleEntries;i++) 
		{
			RECT curR;
			//copyRect(curR,winR);
			curR = winR;
			curR.top	+= LIST_TOP_SPACE + (lineHeight * i);
			curR.bottom = curR.top + lineHeight;
			curR.right	-= SCROLLBAR_SIZE;
			
			if ( RectsIntersect(&curR,&ps.rcPaint) ) 
			{
				FillRect(hdc,&curR, backgroundBrush );
				
				curR.left   += LIST_LEFT_SPACE;
				curR.right	-= LIST_RIGHT_SPACE;

				entryNum = i + dp->topEntry;
				if ( entryNum < dp->numEntries ) 
				{		
					int bb = EntryToBB(entryNum);
					const char * str = bigbuffer[bb].str;
					
					SetTextColor( hdc, bigbuffer[bb].color );

					DrawText(hdc,
						str,strlen32(str),
						&curR,
						DT_SINGLELINE | DT_VCENTER | DT_LEFT);
				}			
			}
		}

        EndPaint(windowH, &ps);
		break;
		}

	case WM_CREATE:
		break;

	case WM_WINDOWPOSCHANGING: 
		{
		//WINDOWPOS * wp;
		//wp = (WINDOWPOS *) lParam;
		break;
		}

	case WM_SIZE: 
		/*
		// hide on min :
		if ( wParam == SIZE_MINIMIZED )
		{
			ShowWindow(windowH,SW_HIDE);
			break;
		}
		*/
		if ( dp ) 
		{
			RECT rect;
			GetClientRect(windowH,&rect);
			dp->clientRect = rect;

			bool wasAtEnd = dp->isAtEnd;

			MoveWindow(dp->scrollbar,
				rect.right - SCROLLBAR_SIZE, rect.top + LIST_TOP_SPACE,
				SCROLLBAR_SIZE, Height(rect) - LIST_TOP_SPACE,
				FALSE );

			//#pragma PRAGMA_MESSAGE("should preserve *bottom* entry visible, not *top*")
			// easy enough, except that I don't keep track of bottomEntry

			if ( (dp->topEntry + LIST_VISIBLE(rect,dp)) > dp->numEntries ) 
			{
				dp->topEntry = max(0, (dp->numEntries - LIST_VISIBLE(rect,dp)) );
			}
			
			if ( wasAtEnd )
				Internal_ScrollToEnd(true);
			else
				entriesChanged(dp,0,0);
		}
		break;

	case WM_CLOSE:
		// hide on close :
		//ShowWindow(windowH,SW_HIDE);
		DestroyWindow(dp->window);	
		return 0;

	case WM_DESTROY:
		{
		StoreRelease(&g_window,(HWND)0);
	
		if ( dp ) 
		{
			#ifdef DO_USE_TIMER
			if ( g_timer )
			{
				KillTimer(dp->window,g_timer);
				g_timer = NULL;
			}
			#endif
		
			freeWindowData(dp);
			SetWindowLong( windowH,0, 0);
		}
		
		break;
		}

	default:
		return DefWindowProc( windowH, msg, wParam, lParam );
	}

return 0;
}

static void DoPeekMessages()
{
	MSG msg = { 0 };
	while ( PeekMessage(&msg,g_window,0,0,PM_REMOVE) )
	{				
	    TranslateMessage(&msg); 
		DispatchMessage(&msg);
	}
}

DWORD WINAPI LogWindowThreadRoutine(LPVOID lpThreadParameter)
{
	Internal_Open(g_cmdShow);

	for(;;)
	{
		// my WM_DESTROY sets g_window to null
		if ( ! g_window )
			break;
			
		#ifdef DO_USE_TIMER

		WaitMessage();

		Internal_PopFIFO();
		
		#else

		MsgWaitForMultipleObjects
		(
			1,
			&g_event,
			FALSE,
			INFINITE,
			QS_ALLINPUT
		);
    
		Internal_PopFIFO();
		
		DoPeekMessages();
    
		if ( g_windowData->needsRedraw )
		{
			g_windowData->needsRedraw = false;
			entriesChanged(g_windowData,0,0);
			InvalidateWindow(g_window);
		}
		
		#endif // ifdef DO_USE_TIMER
		
		/*
		MSG msg = { 0 };
		if ( GetMessage(&msg,g_window,0,0) <= 0 )
			break;
		
		Internal_PopFIFO();
        TranslateMessage(&msg); 
		DispatchMessage(&msg);
		*/
		
		DoPeekMessages();
	}
	
	Internal_Close();
	
	return 0;
}
	
}; // LogWindowInternal

//=====================================================================

using namespace LogWindowInternal;

static HANDLE g_threadHandle = 0;
static uint32 g_prevLogState = 0;

void LogWindow::Open(int cmdShow)
{
	if ( g_threadHandle )
		return;
	
	#ifndef DO_USE_TIMER
	g_event = CreateEvent(NULL,FALSE,FALSE,NULL);
	#endif
	
	//SPSC_FIFO2_Open(&g_fifo,&g_allocator);
	
	LogWindowInternal::g_cmdShow = cmdShow;
	
	SequentialFence();
	
	g_threadHandle = CreateThread(NULL,0,LogWindowInternal::LogWindowThreadRoutine,NULL,0,NULL);
	
	//SetThreadPriority(g_threadHandle,THREAD_PRIORITY_ABOVE_NORMAL);
	
	//SetEvent(g_event);
	
	SpinBackOff bo;
	while( LogWindow::GetWindow() == NULL )
	{
		bo.BackOffYield();
	}
	
	// now log can point at me :
	g_prevLogState = LogGetState();
	LogSetState( (g_prevLogState & (~CB_LOG_ECHO)) | CB_LOG_CALLBACK );
	LogSetCallback( LogWindowInternal::MyLogCallback );
}	

void LogWindow::Close()
{
	if ( ! g_threadHandle )
		return;
	
	// make Log stop pointing at me :
	LogSetState( g_prevLogState );
	LogSetCallback( NULL );

	// request thread kill :
	HWND wnd = LogWindow::GetWindow();
	PostMessage(wnd,WM_CLOSE,0,0);
	
	// I don't care about the text color - this is just to send the thread a Message so it
	//	does its message loop and sees g_threadKill
	LogWindow::SetColor(LogWindowInternal::c_defaultColorText,LogWindowInternal::c_defaultColorBack);
	
	//SetEvent(g_event);
	
	WaitForSingleObject(g_threadHandle,INFINITE);
	CloseHandle(g_threadHandle);
	g_threadHandle = 0;
	
	#ifndef DO_USE_TIMER
	CloseHandle(g_event);
	g_event = 0;
	#endif
	
	//SPSC_FIFO2_Flush(&g_fifo,&g_allocator);
	//SPSC_FIFO2_Close(&g_fifo,&g_allocator);
	//g_allocator.Finalize();
}

void LogWindow::Printf(const char* pMessage, ...)
{
	// no CS here, Puts() has one :

	// Bind into one big string
	char msg[1024];		
	va_list args;
	va_start(args,pMessage);
	_vsnprintf(msg,1024,pMessage,args);
	va_end(args);
	msg[1023]=0;

	LogWindow::Puts(msg);
}

HWND LogWindow::GetWindow()
{
	// g_window is shared
	HWND w = LoadAcquire(&(LogWindowInternal::g_window));

	return w;
}

// WARNING : this Puts is like "fputs",not "puts" - it does NOT add '\n' for you
void LogWindow::Puts(const char* str)
{
	if ( ! g_threadHandle )
	{
		FAIL("LogWindow not open");
		return;
	}
	// queue it even if window doesn't exist yet :
	/*
	HWND wnd = GetWindow();
	if ( ! wnd )
		return;
	*/
	
	// must dupe string to send it :
	char * dup = LogWindowInternal::MyStrDup(str);
	//PostMessage(wnd,WM_LOG_PUTS,0,(LPARAM)dup);

	Internal_PushFIFO(dup);

	#ifndef DO_USE_TIMER	
	SetEvent(g_event);
	#endif
}
	
void LogWindow::SetColor(DWORD text,DWORD background)
{
	HWND wnd = GetWindow();
	if ( ! wnd )
		return;
		
	PostMessage(wnd,WM_LOG_SETCOLOR,(WPARAM)text,(LPARAM)background);
}

void LogWindow::ScrollToEnd()
{
	HWND wnd = GetWindow();
	if ( ! wnd )
		return;
		
	PostMessage(wnd,WM_LOG_SCROLLTOEND,0,0);
}


END_CB

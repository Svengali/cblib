#pragma once

#include "Base.h"
#include "Win32Util.h"

/*********

LogWindow

runs a simple log window async on a thread

much faster than console stdio

LogWindow::Open() automatically redirects lprintf to the LogWindow
(you can of course undo it)

LogWindow::Printf and Puts are *thread safe*
-> actually not really true right now, they use an SPSC
	so you must only call from the *same* thread all the time
	or use a Mutex lock for the Producer side

they do not necessarilly immediate print the string
(it is queued and sent to the LogWindow thread)

-------------------------------------

NOTEZ : if you really want this to be smooth & fast you need to set the
	LogState to be only CB_LOG_CALLBACK , no other flags
	because CB_LOG_TO_DEBUGGER for example will destroy your performance

***********/

START_CB

namespace LogWindow
{
	void Open(int cmdShow);
	void Close();

	void SetColor(DWORD text,DWORD background);

	void ScrollToEnd();

	HWND GetWindow();

	void Printf(const char* pMessage, ...);

	// WARNING : this Puts is like "fputs",not "puts" - it does NOT add '\n' for you
	void Puts(const char* str);

};

END_CB

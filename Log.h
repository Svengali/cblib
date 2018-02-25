#pragma once

#include "cblib/Base.h"
#include "cblib/safeprintf.h"

START_CB

//-----------------------------------------------------------

void LogClose();

// SetLog() is not necessary; it changes the defaults if you like
//	eg. SetLog(".\\log\\log.txt",".\\log\\log_prev.txt");
void SetLog(const char * name,const char *prevname);

// Log also goes to stdio if echo is on
void rawLog(const char * fmt,...);
void LogSetEcho(const bool echo); //default is off

// "CloseAfterEachWrite" is slower but safer
//	turn it on when you need logs near a hard crash
void LogSetCloseAfterEachWrite(bool b);
//void LogError(const char * fmt,...);

bool LogGetEnabled();
void LogSetEnabled(const bool able);

void LogPushTab();
void LogPopTab();

void LogFlush();

// lprintf = Log & printf to stdio
void rawlprintf(const char * fmt,...);
void lprintfSetEcho(const bool echo); //default is on
bool lprintfGetEcho();
void lprintfSetEnabled(const bool able);
bool lprintfGetEnabled();

inline void lputs(const char * str)
{
	rawlprintf("%s\n",str);
}

inline void eatargs(const char * fmt,...)
{
	UNUSED_PARAMETER(fmt);
}

class LogEnablePusher
{
public:
	LogEnablePusher(const bool able)
	{
		m_able = LogGetEnabled();
		LogSetEnabled(able);
	}
	~LogEnablePusher()
	{
		LogSetEnabled(m_able);
	}
private:
	bool m_able;
};

class lprintfEnablePusher
{
public:
	lprintfEnablePusher(const bool able)
	{
		m_able = lprintfGetEnabled();
		lprintfSetEnabled(able);
	}
	~lprintfEnablePusher()
	{
		lprintfSetEnabled(m_able);
	}
private:
	bool m_able;
};

class LogTabber
{
public:
	LogTabber() { LogPushTab(); }
	~LogTabber() { LogPopTab(); }
};

//-----------------------------------------------------------

#define SPI_SAFEDECL void lprintf
#define SPI_CALLRAW rawlprintf
#define SPI_PREARG
#define SPI_CALLARG fmt
#define SPI_BADRETURN
#include "safeprintf.inc"
#undef SPI_SAFEDECL
#undef SPI_CALLRAW
#undef SPI_PREARG
#undef SPI_CALLARG
#undef SPI_BADRETURN

#define SPI_SAFEDECL void Log
#define SPI_CALLRAW rawLog
#define SPI_PREARG 
#define SPI_CALLARG fmt
#define SPI_BADRETURN
#include "safeprintf.inc"
#undef SPI_SAFEDECL
#undef SPI_CALLRAW
#undef SPI_PREARG
#undef SPI_CALLARG
#undef SPI_BADRETURN

END_CB

#pragma once

#include "Base.h"
//#include "safeprintf.h"
#include "autoprintf.h"

/*

CB : this is a debug / dev log, not a production log

The design goal is to make it extremely simple to use.  You can just call "printf" and not woy about
anything else in simple apps.

The main interface is "Printf" that looks just like "printf" but goes to the log.

Printf messages may or may not :
    write to log file
    echo to a console
    echo to VC debugger window
    log file & line
    call user callback

BTW if you turn on the option file & line with VC debugger window, you can click on log lines
    to jump to the source code.

You can push & pop tabbing to indent sections for nice use with tabview

In C++ you can push & pop the whole log state to temporarily turn off or on different features

the user callback can tell lines not to output, so you can use it as a filter

the log opens automatically when you first call it! this may do allocations and is generally slow!
    if you don't want the log to open when you first call printf,
    you should manually open it with LogOpen
   
the log is flushed after every write if you turn on LogSetCloseAfterEachWrite
    (off by default)
    that makes it slow but makes sure you get printfs that happen right before a crash
    warning : this can fragment the hell out of your disk
    
-----------------

cuently I'm not supporting any "verbose level" or different categories of Log
I've always found that stuff messy and hard to maintain in the past, but maybe it's needed

CB : WARNING : these are not thread safe !!

*/

typedef struct _iobuf FILE;

START_CB

// compile option toggles :
//#define DO_CB_LPRINTF_SIMPLE
//#define DO_CB_KILL_LPRINTF

//-----------------------------------
//

// log function bits :
#define CB_LOG_TO_FILE     (1<<0)
#define CB_LOG_ECHO        (1<<1)
#define CB_LOG_TO_DEBUGGER (1<<2)
#define CB_LOG_FILE_LINE   (1<<3)
#define CB_LOG_CALLBACK    (1<<4)

#define CB_LOG_DEFAULT_STATE     (CB_LOG_TO_FILE|CB_LOG_ECHO|CB_LOG_TO_DEBUGGER)
#define CB_LOG_DEFAULT_VERBOSITY	(1)

// LogOpen will be done automatically with the default names,
//  but you can do it yourself to set the names and control when it's opened
void LogOpen(const char * file,const char * prevFile, bool doHeader = true);
	// if you don't want to set name, you can still do
	//	LogOpen(NULL,NULL,false);

// set the state bits to enable/disable various functions
void   LogSetState(uint32 options); // SetState(0) disables all
uint32 LogGetState();

// LogSetVerboseLevel returns old level
//	default verboselvel is 1 , set to 0 for less, 2 for more
int  LogSetVerboseLevel(int verbosity);
int  LogGetVerboseLevel();

// set where the echo goes (usually stdout,stderr, or NULL)
// note : you must also turn on CB_LOG_ECHO in state to actually get logs to the echo file
void LogSetEcho(FILE * echo); // set to NULL to disable echo
FILE * LogGetEcho();

// callback bool tells you whether to echo the log to other outputs; a false supresses all output
typedef bool (t_rtlLogCallback) (const char * buffer);

// note : you must also turn on the callback enable bit in State to actually get logs to your callback
void LogSetCallback(t_rtlLogCallback * cb);
t_rtlLogCallback * LogGetCallback();

// increase & decrease tab
//  returns the # of tabs in effect after your requested change
int LogPushTab();
int LogPopTab();

// "Flush" is actually a close & reopen to ensure the data is written
void LogFlush();
// toggle CloseAfterEachWrite , good when you're debugging crashes
void LogSetCloseAfterEachWrite(bool b);

// internal use only :
void rawlprintf(const char * fmt,...);
void lprintf_file_line(const char * file,const int line);

inline void eatargs(const char * fmt,...)
{
	UNUSED_PARAMETER(fmt);
}

inline void lputs(const char * str)
{
	rawlprintf("%s\n",str);
}

//-----------------------------------
// C++ poppers :

class LogTabber
{
public:
    LogTabber() { LogPushTab(); }
    ~LogTabber() { LogPopTab(); }
};

class LogBracketTabber
{
public:
    explicit LogBracketTabber(const char * str) { rawlprintf("%s :\n {\n",str); LogPushTab(); }
    ~LogBracketTabber() { LogPopTab(); rawlprintf("};\n"); }
};

#define LOG_TABBER()	LogTabber NUMBERNAME(logger) // ()
#define LOG_BRACKET_TABBER(name)	LogBracketTabber NUMBERNAME(logger) (name)

class LogVerboseScoper
{
public:
    LogVerboseScoper(int newLevel) { m_oldLevel = LogSetVerboseLevel(newLevel); }
    ~LogVerboseScoper() { LogSetVerboseLevel(m_oldLevel); }
    
    int m_oldLevel;
};

#define LOG_SCOPE_VERBOSE(level)	LogVerboseScoper NUMBERNAME(logger) (level)

// LogStateSaver lets you easily set a state temporarily in scope and restore it on exit
//  for example :
//      LogStateSaver saver(0,NULL);
//  will disable all logging in cuent scope
//      LogStateSaver saver( LogGetState()|CB_LOG_FILE_LINE ,NULL);
//  will make sure file & line logging is enabled for cuent scope
class LogStateSaver
{
public:
	// save all :
    LogStateSaver() : m_state( LogGetState() ), m_echo( LogGetEcho() ), m_verboseLevel( LogGetVerboseLevel() ) { }

    // restore :
    ~LogStateSaver() { LogSetState(m_state); LogSetEcho( m_echo ); LogSetVerboseLevel(m_verboseLevel); }
    
    LogStateSaver(int newState,FILE * newEcho,int newVerboseLevel) : 
		m_state( LogGetState() ), m_echo( LogGetEcho() ), m_verboseLevel( LogGetVerboseLevel() )
    {
        LogSetState(newState);
        LogSetEcho(newEcho);
        LogSetVerboseLevel(newVerboseLevel);
    }
    
    int m_state;
	int m_verboseLevel;
    FILE * m_echo;
};

#define LOG_SCOPE_DISABLE()	LogStateSaver NUMBERNAME(lss) (0,NULL,0)

//-------------------------------------------------
#ifdef DO_CB_KILL_LPRINTF

// to kill printfs entirely :
#define lprintf    NS_CB::eatargs

#else // DO_CB_KILL_LPRINTF

#ifdef DO_CB_LPRINTF_SIMPLE

// fast version :
#define lprintf		NS_CB::rawlprintf

#else // DO_CB_LPRINTF_SIMPLE

// prefix file & line then log :
// @@ ICKY: using a macro means I'm not in CB namespace ! (there is no cb::lprintf)
//#define lprintf		NS_CB::lprintf_file_line(__FILE__,__LINE__), NS_CB::safelprintf
#define lprintf		NS_CB::lprintf_file_line(__FILE__,__LINE__), autolprintf

#endif // DO_CB_LPRINTF_SIMPLE
#endif // DO_CB_KILL_LPRINTF
//-------------------------------------------

#define lprintfvar(var) lprintf(STRINGIZE(var) " = ",var,"\n")

// @@ err print : for now just call lprintf
//	should do some other stuff here
//	todo : optional int 3 on err print
#define lprinterr	lprintf

// use lverbose like : lverbose(2) lprintf("blah blah");
#define lverbose(n)	if ( LogGetVerboseLevel() < (n) ) ; else

// by convention I usually use three verbose levels
//	v0 = "quiet"
//	v1 = "default"
//	v2 = "verbose"
// so use lprintf_v0() or lprintf() for stuff you always want
// use lprintf_v1() for stuff that should go away in quiet mode
// use lprintf_v2() for stuff that should only show up in verbose mode

#define lprintf_v0	lverbose(0) lprintf
#define lprintf_v1	lverbose(1) lprintf
#define lprintf_v2	lverbose(2) lprintf

//-----------------------------------------------------------

/*
#define SPI_SAFEDECL void safelprintf
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

// @@ OLD CB "Log" meant with no echo
#define SPI_SAFEDECL void Log
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
*/

class autoprintf_LogString
{
public:
  void operator << ( const String &rhs );
};


#define autolprintf NS_CB::autoprintf_LogString() << NS_CB::autoToString

//-----------------------------------------------------------

void vsnprintfdynamic(cb::vector<char> * pBuf,const char * fmt,va_list args);

//-----------------------------------------------------------

// danger! use with care
FILE * GetLogFile();

END_CB

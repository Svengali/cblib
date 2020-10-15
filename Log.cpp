#include "Util.h"
#include "File.h"
#include "FileUtil.h"
#include "Log.h"
#include "Win32Util.h"

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
    
/***********

Default name is now "c:\logs\exename.log"

*************/

START_CB

//--------------------------------------------------
// Log state in static vars :

static const char * c_defaultLogDir = "c:\\logs";

static int s_logState = CB_LOG_DEFAULT_STATE;
static int s_verbosity = CB_LOG_DEFAULT_VERBOSITY;

static const int s_maxLogPrevSize = 1 * 1024 * 1024 ; // 1 MB

static bool s_logCloseAfterEachWrite = false;

static bool s_logDoHeader = true;

// default names :
//  TODO : put in some global dir ?
//  TODO : use the process name by default?  like process_log.txt ?
static bool s_logNameSet = false;
static const char * s_logName = NULL;
static const char * s_logPrevName = NULL;

static FILE * s_logFile = NULL;
static FILE * s_logEchoFile = stdout;
static t_rtlLogCallback * s_callback = NULL;

static int s_logTabs = 0;

static bool s_logOpen = false;

// protect from recursion in case we Assert in log, which calls log :
static bool s_inLog = false;

static bool s_logIsAtEOL = true;

//--------------------------------------------------

// LOG_PROTECTOR forbids recursions
#define LOG_PROTECTOR() if ( s_inLog ) return; ScopedSet<bool> s(&s_inLog,true); LogOpen()


/******************

SetupLogName :

if a log name was not given and they tell us to log to file, then make a default log name :

    c:\logs\[exename].log

*********************/
static void SetupLogName()
{
    s_logNameSet = true;

    if ( s_logName != NULL )
        return;
    
    //PsGetProcessImageFileName(        
    //ZwQueryInformationProcess(
    
    char fileName[32] = { 0 };
    
    GetProcessName( GetCurrentProcess(), fileName, sizeof(fileName) );
    
    // take off the ".exe"
    char * pDot = strrchr(fileName,'.');
    if ( pDot )
        *pDot = 0;
    
    CreateDirectory( c_defaultLogDir , NULL );
    
    static char s_logNameBuffer[256];
    static char s_prevNameBuffer[256];
    sprintf(s_logNameBuffer,"%s\\%s.log",c_defaultLogDir,fileName);
    sprintf(s_prevNameBuffer,"%s\\%s.prev",c_defaultLogDir,fileName);
    
    s_logName = s_logNameBuffer;
    s_logPrevName = s_prevNameBuffer;
    
}
    
/***********************

MoveLogToPrev :

at startup we don't just overwrite the exiting log
we move it to the "prev" file
we append onto the prev, but we have some limit so it doesn't keep growing forever

s_maxLogPrevSize    

*************************/
static void MoveLogToPrev()
{
    if ( s_logName == NULL )
        return;

    int64 logSize = GetFileLength(s_logName);
    
    if ( logSize == CB_FILE_LENGTH_INVALID )
        return; // no log
    
    int64 logPrevSize = GetFileLength(s_logPrevName);
    
    if ( logPrevSize == CB_FILE_LENGTH_INVALID )
		logPrevSize = 0;
    
    int64 amountOfPrevToKeep = MIN( (s_maxLogPrevSize - (int)logSize) , (int)logPrevSize );

    if ( amountOfPrevToKeep <= 0 )
    {
        // just move it
        
        DeleteFile(s_logPrevName);
        bool success = !! MoveFile(s_logName,s_logPrevName);
        if ( ! success )
        {
            OutputDebugString("MoveFile failed to open log file\n");
            fprintf(stderr,"MoveFile failed to open log file %s\n",s_logName);      
        }
    }
    else
    {
        // make the last [amountOfPrevToKeep] of prev + log

        char tempName[_MAX_PATH];
        MakeTempFileName(tempName,sizeof(tempName));
        
        FILE * tempFile = fopen(tempName,"wb");
        if ( ! tempFile )
        {
            OutputDebugString("fopen failed to open log file\n");
            fprintf(stderr,"fopen failed to open log file %s\n",tempName);      
			return;
        }
        
        char * logBuf = ReadWholeFile(s_logName);
        
        if ( ! logBuf )
        {
            OutputDebugString("ReadWholeFile failed to open log file\n");
            fprintf(stderr,"ReadWholeFile failed to open log file %s\n",s_logName);      
			return;
        }
        
        char * logPrevBuf = ReadWholeFile(s_logPrevName);

        if ( ! logPrevBuf )
        {
            OutputDebugString("ReadWholeFile failed to open log file\n");
            fprintf(stderr,"ReadWholeFile failed to open log file %s\n",s_logPrevName);      
			return;
        }
        
        int64 logPrevStart = logPrevSize - amountOfPrevToKeep;

		/*
        fwrite(logPrevBuf + logPrevStart,amountOfPrevToKeep,1,tempFile);
        fwrite(logBuf,logSize,1,tempFile);
        */
        
        FWrite(tempFile,logPrevBuf + logPrevStart,amountOfPrevToKeep);
        FWrite(tempFile,logBuf,logSize);
        
        fclose(tempFile);
        tempFile = NULL;
        
        CBFREE(logBuf);
        CBFREE(logPrevBuf);
        
        DeleteFile(s_logPrevName);
        bool success = !! MoveFile(tempName,s_logPrevName);
        if ( ! success )
        {
            OutputDebugString("MoveFile failed to open log file\n");
            fprintf(stderr,"MoveFile failed to open log file %s\n",tempName);
            
            MoveFile(s_logName,s_logPrevName);
        }
        DeleteFile(s_logName);
    }
}

/**


LogBeginWrite / EndWrite

wraps each logging to file

normally just makes sure the file is open & flushes

if s_logCloseAfterEachWrite is set, this will open & close the file each time

s_logCloseAfterEachWrite is useful when you're crashing and want to make sure the log is flushed

**/
static bool LogBeginWrite()
{
    // open file if it's not open :
    
    if ( s_logFile )
        return true;
    if ( ! s_logName )
        return false;
        
    if ( (s_logFile = fopen(s_logName,"ab")) == NULL )
    {
        fprintf(stderr,"ERROR : Failed to open log file %s\n",s_logName);
            
        return false;
    }
    
    return true;
}

static void LogEndWrite()
{
    if ( s_logFile )
    {
        if ( s_logCloseAfterEachWrite )
        {
            fclose(s_logFile); s_logFile = NULL;
        }
        else
        {
            fflush(s_logFile);
        }
    }
}

static void LogOpen()
{
    if ( s_logOpen || ! (s_logState & CB_LOG_TO_FILE) )
        return;
    
    if ( ! s_logNameSet && s_logName == NULL )
    {
        SetupLogName();
    }
    
    if ( s_logName == NULL )
        return;
        
    s_logOpen = true;

    if ( s_logPrevName )
    {
        MoveLogToPrev();
    }
    
    if ( ! LogBeginWrite() )
    {
        return;
    }

	if ( s_logDoHeader )
	{
		__time64_t long_time;
		_time64( &long_time );                /* Get time as long integer. */
		struct tm *newtime = _localtime64( &long_time ); /* Convert to local time. */
	    
		// asctime has a \n in it already but that's okay, add another
		fprintf(s_logFile,"-----------------------------------------------------------------\n");
		fprintf(s_logFile,"Log opened : %s\n",asctime(newtime));
    }
    
    s_logIsAtEOL = true;
    
    LogEndWrite();
}

static void LogClose()
{
    LogFlush();
    s_logOpen = false;
}

FILE * GetLogFile()
{
	if ( ! LogBeginWrite() )
		return NULL;
	return s_logFile;
}

void LogFlush()
{
    if ( s_logFile )
    {
        fclose(s_logFile);
        s_logFile = NULL;
    }
}

void LogOpen(const char * name,const char *prevname, bool doHeader)
{
    LogClose();
    
    s_logDoHeader = doHeader;
    
    if ( name )
    {    
		s_logName = name;
		s_logPrevName = prevname;
		s_logNameSet = true;
    }
    
    // mmm .. no wait, cuz state TO_FILE might be off now
    //  and they might set it after doing this
    //LogOpen();
}

void LogSetCloseAfterEachWrite(bool b)
{
    s_logCloseAfterEachWrite = b;
}

void LogSetState(uint32 newState) // SetState(0) disables all
{
    const uint32 oldState = s_logState;
    if ( oldState != newState )
    {
        s_logState = newState;
    
        if ( (newState & CB_LOG_TO_FILE) != (oldState & CB_LOG_TO_FILE) )
        {
			LogFlush();
			
			// NO NO , this causes the log file to get moved to prev when you use scope disabler
            //LogClose();
            
            /*
            // mmm.. no wait for first call to lprintf
            // cuz user could change the file name after setting state
                LogOpen();
            */
        }
    }
}

uint32 LogGetState()
{
    return s_logState;
}

int LogSetVerboseLevel(int verbosity)
{
	int old = s_verbosity;
	s_verbosity = verbosity;
	return old;
}

int  LogGetVerboseLevel()
{
	return s_verbosity;
}

void LogSetEcho(FILE * echo) // set to NULL to disable echo
{
    s_logEchoFile = echo;
}

FILE * LogGetEcho()
{
    return s_logEchoFile;
}

int LogPushTab()
{
    s_logTabs++;
    return s_logTabs;
}

int LogPopTab()
{
    s_logTabs--;
    ASSERT( s_logTabs >= 0 );
    s_logTabs = MAX(s_logTabs,0);
    return s_logTabs;
}

void LogSetCallback(t_rtlLogCallback cb)
{
    s_callback = cb;
}

t_rtlLogCallback * LogGetCallback()
{
    return s_callback;
}

//-----------------------------------------------------------

void lprintf_file_line(const char * file,const int line)
{
    // don't log file & line in the middle of a couple of prints without EOLs
    if ( (s_logState & CB_LOG_FILE_LINE) && s_logIsAtEOL )
    {
        rawlprintf("%s(%d) : ",file,line);
        s_logIsAtEOL = false;
    }
}

void vsnprintfdynamic(cb::vector<char> * pBuf,const char * fmt,va_list args)
{
	pBuf->resize( 1024 + strlen32(fmt) );
	
	for(;;)
	{
		errno = 0;
		int wroteLen = _vsnprintf(pBuf->data(),pBuf->size()-1,fmt,args);
		if ( wroteLen < 0 || errno != 0 )
		{
			// overflow
			pBuf->resize( 4096 + pBuf->size()*2 );
		}
		else
		{
			return;
		}
    }
}

void rawlprintf(const char * fmt,...)
{
    if ( ! s_logState ) // all turned off
        return;
    LOG_PROTECTOR();

    // use a static so we don't blow the stack :
    static char s_buffer[1024];
    int bufsize = sizeof(s_buffer);
    char * buffer = s_buffer;
    cb::vector<char> overflow;
    
    // fill buffer :
    {
        va_list arg;
        va_start(arg,fmt);

        // put tabs in :
        retry:
        buffer[0] = 0;
        char * ptr = buffer;
        if ( s_logIsAtEOL )
        {
			for(int t=0;t<s_logTabs;t++)
			{
				*ptr++ = '\t';
			}
		}
		errno = 0;
        int wroteLen = _vsnprintf(ptr,bufsize-1-s_logTabs,fmt,arg);
        if ( wroteLen < 0 || errno )
        {
			// overflow
			overflow.resize( 4096 + bufsize*2 );
			bufsize = overflow.size();
			buffer = overflow.data();
			goto retry;
        }
        //buffer[sizeof(buffer)-1] = 0;
        
        int len = (int)strlen(buffer);
        if ( len == 0 )
            return;
        
        va_end(arg);
    
        // set s_logIsAtEOL so that file & line works right with incomplete lines
        char last = buffer[ len - 1];
        if ( last == '\n' || last == '\r' )
            s_logIsAtEOL = true;
        else
            s_logIsAtEOL = false; 
    }
         
        
    if ( (s_logState & CB_LOG_CALLBACK) && s_callback )
    {
        if ( ! s_callback(buffer) )
            return; // bool return from callback means no other output
    }

    if ( s_logState & CB_LOG_TO_FILE )
    {   
        // normal writing to the log file
        if ( ! LogBeginWrite() )
        {
            s_logName = NULL; // don't try again
        }
        else
        {
            fputs(buffer,s_logFile);
            //fflush(s_logEchoFile); LogEndWrite does this
        }
    }
    
    if ( (s_logState & CB_LOG_ECHO) && s_logEchoFile )
    {
        fputs(buffer,s_logEchoFile);
        fflush(s_logEchoFile);
    }
    
    if ( s_logState & CB_LOG_TO_DEBUGGER )
    {
        OutputDebugString(buffer);
    }
    
    LogEndWrite();
}



//=========================================================================================

END_CB

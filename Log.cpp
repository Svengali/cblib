#include "cblib/Util.h"
#include "cblib/File.h"
#include "cblib/FileUtil.h"
#include "cblib/Log.h"

#include <string.h>
#include <stdio.h>
#include <time.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h> // for MoveFile

START_CB

//-----------------------------------------------------------

static const int s_maxLogPrevSize = 2 * 1024 * 1024 ; // 2 MB

static bool s_logOpen = false;
static bool s_logEcho = false;
static bool s_logEnabled = true;
static bool s_logCloseAfterEachWrite = false;

// can't do this cuz we're a lib :
//#ifdef _CONSOLE
static bool s_lprintfEcho = true;
//#else
//static bool s_lprintfEcho = false;
//#endif

static bool s_lprintfEnabled = true;
//static const char * s_logName = ".\\log\\log.txt";
//static const char * s_logPrevName = ".\\log\\log_prev.txt";

static const char * s_logName = "log.txt";
static const char * s_logPrevName = "log_prev.txt";

static File *s_file = NULL;

static int s_logTabs = 0;

// protect from recursion in case we Assert in log, which calls log :
static bool s_inLog = false;

class BoolInScope
{
public:
	BoolInScope(bool * pb) : m_p(pb)
	{
		*m_p = true;
	}
	~BoolInScope()
	{
		*m_p = false;
	}
	bool * m_p;
};

// LOG_PROTECTOR forbids recursions
#define LOG_PROTECTOR() if ( s_inLog ) return; BoolInScope s(&s_inLog); LogOpen()

static void MoveLogToPrev()
{
	const size_t logSize = GetFileLength(s_logName);
	const size_t logPrevSize = GetFileLength(s_logPrevName);
	
	const size_t amountOfPrevToKeep = MIN( (s_maxLogPrevSize - logSize) , logPrevSize );

	if ( amountOfPrevToKeep <= 0 )
	{
		// just move it
		
		DeleteFileA(s_logPrevName);
		BOOL success = MoveFileA(s_logName,s_logPrevName);
		if ( ! success )
		{
			OutputDebugStringA("MoveFile failed to open log file\n");
			fprintf(stderr,"MoveFile failed to open log file %s\n",s_logName);		
		}
	}
	else
	{
		// make the last [amountOfPrevToKeep] of prev + log
		//char tempName[_MAX_PATH];
		//tmpnam(tempName);
		const char * tempName = _tempnam(NULL,"gblog");
		
		File tempFile(tempName,"wb");
		
		char * logBuf = ReadWholeFile(s_logName);
		char * logPrevBuf = ReadWholeFile(s_logPrevName);

		const size_t logPrevStart = logPrevSize - amountOfPrevToKeep;

		fwrite(logPrevBuf + logPrevStart,amountOfPrevToKeep,1,tempFile.Get());
		fwrite(logBuf,logSize,1,tempFile.Get());
		
		tempFile.Close();
		
		CBFREE(logBuf);
		CBFREE(logPrevBuf);
		
		DeleteFileA(s_logPrevName);
		BOOL success = MoveFileA(tempName,s_logPrevName);
		if ( ! success )
		{
			OutputDebugStringA("MoveFile failed to open log file\n");
			fprintf(stderr,"MoveFile failed to open log file %s\n",tempName);
			
			MoveFileA(s_logName,s_logPrevName);
		}
		DeleteFileA(s_logName);
		
		free( (void *)tempName );
	}
}

static void LogOpen()
{
	if ( s_logOpen )
		return;		
	s_logOpen = true;

	if ( s_logPrevName )
	{
		MoveLogToPrev();
	}
	
	if( !s_file )
	{
		s_file = new File();
	}
	
	if ( ! s_file->Open(s_logName,"ab") )
	{
		fprintf(stderr,"Failed to open log file %s\n",s_logName);
		return;
	}
	  
	__time64_t long_time;
	_time64( &long_time );                /* Get time as long integer. */
	struct tm *newtime = _localtime64( &long_time ); /* Convert to local time. */
	
	// asctime has a \n in it already but that's okay, add another
	safefprintf(s_file->Get(),"-----------------------------------------------------------------\n");
	safefprintf(s_file->Get(),"cblib Log opened : %s\n",asctime(newtime));
	
	if ( s_logCloseAfterEachWrite )
		s_file->Close();
	else
		s_file->Flush();
}

void LogClose()
{
	LogFlush();
	s_logOpen = false;

	delete s_file;
	s_file = NULL;	
	
}

void LogFlush()
{
	if ( s_file->IsOpen() )
	{
		s_file->Close();
	}
}

void SetLog(const char * name,const char *prevname)
{
	LogClose();
	s_logName = name;
	s_logPrevName = prevname;
	LogOpen();
}

void LogSetCloseAfterEachWrite(bool b)
{
	s_logCloseAfterEachWrite = b;
}

bool LogGetEnabled()
{
	return s_logEnabled;
}

void LogSetEnabled(const bool able)
{
	s_logEnabled = able;
}

void lprintfSetEnabled(const bool able)
{
	s_lprintfEnabled = able;
}
bool lprintfGetEnabled()
{
	return s_lprintfEnabled;
}

void LogSetEcho(const bool echo)
{
	s_logEcho = echo;
}

void lprintfSetEcho(const bool echo)
{
	s_lprintfEcho = echo;
}

bool lprintfGetEcho()
{
	return s_lprintfEcho;
}

void LogPushTab()
{
	s_logTabs++;
}

void LogPopTab()
{
	s_logTabs--;
	ASSERT( s_logTabs >= 0 );
	s_logTabs = MAX(s_logTabs,0);
}

//-----------------------------------------------------------

// internal helper :
void logtabs(FILE * to)
{
	for(int i=0;i<s_logTabs;i++)
	{
		// tab = 2 spaces
		fputc(' ',to);
		fputc(' ',to);
	}
}

void rawlprintf(const char * fmt,...)
{
	// lprintf identical to main Log() but always echos to stdout
	// always do lprintf even if not enabled
	//if ( ! s_logEnabled )
	//	return;
	if ( ! s_lprintfEnabled )
		return;
	LOG_PROTECTOR();

	if ( ! s_file->IsOpen() )
	{
		if ( ! s_file->Open(s_logName,"ab") )
		{
			fprintf(stderr,"Failed to open log file %s\n",s_logName);
			return;
		}
	}

	va_list arg;
	va_start(arg,fmt);

	logtabs(s_file->Get());
	vfprintf(s_file->Get(),fmt,arg);
	if ( s_lprintfEcho )
	{
		logtabs(stdout);
		vprintf(fmt,arg);
	}
	
	#ifdef _DEBUG
	{
		char buffer[1024];
		vsprintf(buffer,fmt,arg);
		OutputDebugStringA(buffer);
	}
	#endif
	
	va_end(arg);
	
	if ( s_logCloseAfterEachWrite )
		s_file->Close();
	else
		s_file->Flush();
}

void rawLog(const char * fmt,...)
{
	if ( ! s_logEnabled )
		return;
	LOG_PROTECTOR();

	if ( ! s_file->IsOpen() )
	{
		if ( ! s_file->Open(s_logName,"ab") )
		{
			fprintf(stderr,"Failed to open log file %s\n",s_logName);
			return;
		}
	}

	va_list arg;
	va_start(arg,fmt);

	logtabs(s_file->Get());
	vfprintf(s_file->Get(),fmt,arg);

	if ( s_logEcho )
	{
		logtabs(stdout);
		vprintf(fmt,arg);
	}

	#ifdef _DEBUG
	{
		char buffer[1024];
		vsprintf(buffer,fmt,arg);
		OutputDebugStringA(buffer);
	}
	#endif
	
	va_end(arg);

	if ( s_logCloseAfterEachWrite )
		s_file->Close();
	else
		s_file->Flush();
}

//-----------------------------------------------------------

END_CB

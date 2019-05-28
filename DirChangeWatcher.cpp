#include <cblib/inc.h>
#include <cblib/Win32Util.h>
#include <cblib/File.h>
#include <cblib/vector.h>
#include <conio.h>
#include <stdlib.h>
#include <direct.h>
#include <stdio.h>
#include <time.h>
#include "cblib/DirChangeWatcher.h"

START_CB

/*

FILE_NOTIFY_CHANGE_FILE_NAME Any file name change in the watched directory or subtree causes a change notification wait operation to return. Changes include renaming, creating, or deleting a file. 
FILE_NOTIFY_CHANGE_DIR_NAME Any directory-name change in the watched directory or subtree causes a change notification wait operation to return. Changes include creating or deleting a directory. 
FILE_NOTIFY_CHANGE_ATTRIBUTES Any attribute change in the watched directory or subtree causes a change notification wait operation to return. 
FILE_NOTIFY_CHANGE_SIZE Any file-size change in the watched directory or subtree causes a change notification wait operation to return. The operating system detects a change in file size only when the file is written to the disk. For operating systems that use extensive caching, detection occurs only when the cache is sufficiently flushed. 
FILE_NOTIFY_CHANGE_LAST_WRITE Any change to the last write-time of files in the watched directory or subtree causes a change notification wait operation to return. The operating system detects a change to the last write-time only when the file is written to the disk. For operating systems that use extensive caching, detection occurs only when the cache is sufficiently flushed. 
FILE_NOTIFY_CHANGE_LAST_ACCESS Any change to the last access time of files in the watched directory or subtree causes a change notification wait operation to return. 
FILE_NOTIFY_CHANGE_CREATION Any change to the creation time of files in the watched directory or subtree causes a change notification wait operation to return. 
FILE_NOTIFY_CHANGE_SECURITY 

*/

const uint32 c_dirChangeDefaultNotifyFlags = 
	FILE_NOTIFY_CHANGE_FILE_NAME|
	FILE_NOTIFY_CHANGE_DIR_NAME|
//	FILE_NOTIFY_CHANGE_ATTRIBUTES|
	FILE_NOTIFY_CHANGE_SIZE|
	FILE_NOTIFY_CHANGE_LAST_WRITE|
//	FILE_NOTIFY_CHANGE_LAST_ACCESS|
	FILE_NOTIFY_CHANGE_CREATION|
//	FILE_NOTIFY_CHANGE_SECURITY|
	0;

//=====================================================================================================================================

#define MAX_BUFFER		(32*1024)

#define MY_MAX_PATH		4096

// this is the all purpose structure that contains  
// the interesting directory information and provides
// the input buffer that is filled with file change data
struct DirInfo 
{
	HANDLE      hDir;
	DWORD       dwBufLength;
	OVERLAPPED  Overlapped;
	char        lpszDirName[MY_MAX_PATH];
	CHAR        lpBuffer[MAX_BUFFER];
};

/*
#define FILE_ACTION_ADDED                   0x00000001   
#define FILE_ACTION_REMOVED                 0x00000002   
#define FILE_ACTION_MODIFIED                0x00000003   
#define FILE_ACTION_RENAMED_OLD_NAME        0x00000004   
#define FILE_ACTION_RENAMED_NEW_NAME        0x00000005   
*/

const char * c_dirChangeActionStrings[] = 
{
	"null   ",
	"add    ",
	"remove ",
	"modify ",
	"oldname",
	"newname"
};

/*
FILE_ACTION_ADDED The file was added to the directory. 
FILE_ACTION_REMOVED The file was removed from the directory. 
FILE_ACTION_MODIFIED The file was modified. This can be a change in the time stamp or attributes. 
FILE_ACTION_RENAMED_OLD_NAME The file was renamed and this is the old name. 
FILE_ACTION_RENAMED_NEW_NAME The file was renamed and this is the new name. 
*/

//---------------------------------------------------------------
// globals :

class DirChangeWatcherImpl : public DirChangeWatcher
{
public:

	DirChangeWatcherImpl() :
		m_threadKillRequested(false),
		m_hCompPort(0),
		m_hThread(0),
		m_notifyFlags(0)
	{
	}
	~DirChangeWatcherImpl()
	{
		if ( m_hThread )
		{
			Stop();
		}
	}

	static DWORD WINAPI ThreadProc(LPVOID lpParameter);

	bool GetDirChanges(cb::vector<DirChangeRecord> * pInto );
	bool GetDirChangesDiscard();

	bool StartWatchingDirs(const char ** dirsToWatch,const int numDirs,DWORD notifyFlags);
	void Stop();

private:
	
	void AddCommand(const char * path, DWORD action);
	void CheckChangedFile( DirInfo *lpdi, PFILE_NOTIFY_INFORMATION lpfni);

	cb::vector<DirInfo>		m_dirs;
	DWORD					m_notifyFlags;
    HANDLE					m_hCompPort;
    HANDLE					m_hThread;
	CriticalSection			m_pendingCS;
	vector<DirChangeRecord>	m_pending;
	bool m_threadKillRequested;
	
	FORBID_CLASS_STANDARDS(DirChangeWatcherImpl);
};

//---------------------------------------------------------------

void DirChangeWatcherImpl::AddCommand(const char * path, DWORD action)
{
	m_pending.push_back();
	cb::strncpy(m_pending.back().path,path,sizeof(m_pending.back().path));
	m_pending.back().action = action;
	m_pending.back().time = time(NULL);
}

/**********************************************************************
   CheckChangedFile()

	!!!! This function is called from the Dir-Watcher thread !!!!
	It cannot call global functions that touch shared thread stuff.

********************************************************************/
void DirChangeWatcherImpl::CheckChangedFile( DirInfo *lpdi, PFILE_NOTIFY_INFORMATION lpfni)
{
    char	szFullPathName[MY_MAX_PATH*2];
	char	name[MY_MAX_PATH];
	WCHAR	wname[MY_MAX_PATH];
	
	COMPILER_ASSERT( sizeof(WCHAR) == 2 );

	// FileNameLength is in BYTES !!??
	int len = lpfni->FileNameLength/2;
	ASSERT( ( lpfni->FileNameLength & 1 ) == 0 );
		
	len = MIN(len,MY_MAX_PATH-1);
	
	memcpy(wname,lpfni->FileName,sizeof(WCHAR)*len);
	wname[len] = 0;		 
		
	wcstombs(name, wname, sizeof(name) );
	name[MY_MAX_PATH-1] = 0;
	name[len] = 0;
	
	strcpy( szFullPathName, lpdi->lpszDirName);
	
	int dirNameLen = strlen32(lpdi->lpszDirName);
	if ( dirNameLen > 0 )
	{
		char last = lpdi->lpszDirName[ dirNameLen - 1];
		if ( last != '\\' && last != '/' )
		{
			strcat( szFullPathName, "\\" );
		}
	}
	
	strcat( szFullPathName, name );
	
	UseCriticalSection usecs(m_pendingCS);

	AddCommand(szFullPathName,lpfni->Action);
}

bool DirChangeWatcherImpl::GetDirChanges(cb::vector<DirChangeRecord> * pInto )
{
	UseCriticalSection usecs(m_pendingCS);
	
	if ( m_pending.empty() )
	{
		return false;
	}

	/*
	File myLog;
	
	if ( myLog.Open(g_myLogPath,"ab") )
	{	
		for(int i=0;i<g_pending.size();i++)
		{
			// asctime puts a \n on it
			char * timestr = asctime( localtime( &m_pending[i].time ) );
			fprintf(myLog.Get(),"%s : %s : %s",
				c_dirChangeActionStrings[ m_pending[i].action ],
				m_pending[i].path,
				timestr);
		}
	
		myLog.Close();
	}
	*/

	pInto->appendv(m_pending);

	m_pending.clear();	

	return true;
}

bool DirChangeWatcherImpl::GetDirChangesDiscard()
{
	UseCriticalSection usecs(m_pendingCS);
	
	if ( m_pending.empty() )
	{
		return false;
	}

	m_pending.clear();	

	return true;
}

/**********************************************************************
   HandleDirectoryChanges()

   Purpose:
      This function receives notification of directory changes and
      calls CheckChangedFile() to display the actual changes. After
      notification and processing, this function calls
      ReadDirectoryChangesW to reestablish the watch.

   Parameters:

      HANDLE hCompPort - Handle for completion port


   Return Value:
      None

   Comments:

********************************************************************/

/*static*/ DWORD WINAPI DirChangeWatcherImpl::ThreadProc(LPVOID lpParameter)
{
	DirChangeWatcherImpl * watcher = (DirChangeWatcherImpl *) lpParameter;

	HANDLE hComPort = 0;
	
	// copy out hComPort
	{
		UseCriticalSection usecs(watcher->m_pendingCS);
		hComPort = watcher->m_hCompPort;
	}
	
	for(;;)
    {
		DWORD numBytes = 0;
		DirInfo *di = NULL ;
		LPOVERLAPPED lpOverlapped = NULL;
    
        // Retrieve the directory info for this directory
        // through the completion key
        // GetQueuedCompletionStatus will stall until something is available
        BOOL ret = 
        GetQueuedCompletionStatus( hComPort,
                                   &numBytes,
                                   (PULONG_PTR) &di,
                                   &lpOverlapped,
                                   INFINITE);

		// check for the main thread telling us to die :
		{
			UseCriticalSection usecs(watcher->m_pendingCS);
			if ( watcher->m_threadKillRequested )
			{
				break;
			}
		}
			
		if ( ret == 0 )
		{
			continue;
		}

		// apparently numBytes can come back 0
        if ( di && numBytes > 0 )
        {
			PFILE_NOTIFY_INFORMATION fni;
            fni = (PFILE_NOTIFY_INFORMATION)di->lpBuffer;

			DWORD cbOffset;
            do
            {
				watcher->CheckChangedFile( di, fni );

				cbOffset = fni->NextEntryOffset;
				fni = (PFILE_NOTIFY_INFORMATION)((LPBYTE) fni + cbOffset);

            } while( cbOffset );
            
            // Reissue the watch command
            if ( ! ReadDirectoryChangesW( di->hDir,di->lpBuffer,
                                   MAX_BUFFER,
                                   TRUE,
                                   watcher->m_notifyFlags,
                                   &di->dwBufLength,
                                   &di->Overlapped,
                                   NULL) )
            {
				// can't do this, we're on a thread !!
				//ZLOGLASTERROR("ReadDirectoryChangesW failed!");
            }
        }
        else
        {
			// @@ ? maybe an error, not a completion
        }
	}

	// thread exit
	return 0;
}


void DirChangeWatcherImpl::Stop()
{
    // The user has quit - clean up
    
    // tell the thread to die :
    {
    UseCriticalSection usecs(m_pendingCS);
    m_threadKillRequested = true;
    }

    PostQueuedCompletionStatus( m_hCompPort, 0, 0, NULL );

    // Wait for the Directory thread to finish before exiting

    if ( WaitForSingleObject( m_hThread, 5000 ) != WAIT_OBJECT_0 )
    {
		lprintf("WARNING : DirChangeWatcherImpl thread didn't exit nicely!!\n");
    }

    CloseHandle( m_hThread );
    m_hThread = 0;
	
    CloseHandle( m_hCompPort );
    m_hCompPort = 0;
    
    for(int i=0;i<m_dirs.size32();i++)
    {
        CloseHandle( m_dirs[i].hDir );
	}	
	m_dirs.clear();
}


bool DirChangeWatcherImpl::StartWatchingDirs(const char ** dirsToWatch,const int numDirs,DWORD notifyFlags)
{
	// this is just an optimization (theoretically)
	m_pending.reserve(256);
	
	// I use &back() in the loop so make sure the vector never relocates :
	m_dirs.reserve(numDirs);

	m_notifyFlags = notifyFlags;

    // First, walk through the raw list and count items, creating
    // an array of handles for each directory
    for (int i=0;i<numDirs;i++)
    {
		m_dirs.push_back();
    
        // Get a handle to the directory
        m_dirs.back().hDir = CreateFile( dirsToWatch[i],
                                            FILE_LIST_DIRECTORY,
                                            FILE_SHARE_READ |
												FILE_SHARE_WRITE |
		                                        FILE_SHARE_DELETE,
                                            NULL,
                                            OPEN_EXISTING,
                                            FILE_FLAG_BACKUP_SEMANTICS |
	                                            FILE_FLAG_OVERLAPPED,
                                            NULL);

        if( m_dirs.back().hDir == INVALID_HANDLE_VALUE )
        {
			lprintf("CreateFile failed on : %s\n",dirsToWatch[i]);
			m_dirs.pop_back();
            continue;
        }

        strcpy( m_dirs.back().lpszDirName, dirsToWatch[i] );

        // Set up a key(directory info) for each directory
        m_hCompPort = CreateIoCompletionPort( m_dirs.back().hDir,
                                          m_hCompPort,
                                          (ULONG_PTR) &m_dirs.back(),
                                          0);
                   
		if ( m_hCompPort == INVALID_HANDLE_VALUE )
		{
			lprintf("CreateIoCompletionPort failed\n");
			return false;
		}
	}

	if ( m_dirs.empty() )
	{
		lprintf("StartWatchingDirs : no valid dirs\n");

		// happens if no valid dirs are given
	    CloseHandle( m_hCompPort );
	    m_hCompPort = 0;
	    
		return false;
	}

    // Start watching each of the directories of interest

    for (int i=0;i<m_dirs.size32();i++)
    {
        if ( ! ReadDirectoryChangesW( m_dirs[i].hDir,
                               m_dirs[i].lpBuffer,
                               MAX_BUFFER,
                               TRUE,
                               m_notifyFlags,
                               &m_dirs[i].dwBufLength,
                               &m_dirs[i].Overlapped,
                               NULL) )
		{
			//ZLOGLASTERROR("ReadDirectoryChangesW failed");
		}
    }

    // Create a thread to sit on the directory changes

    DWORD   tid;
    m_hThread = CreateThread( NULL,
                            0,
                            ThreadProc,
                            (LPVOID)this,
                            0,
                            &tid);

	if ( m_hThread == INVALID_HANDLE_VALUE )
	{
		lprintf("CreateThread failed\n");
		return false;
	}
	
	return true;
}

//=========================================================================================================
// public exposure :

DirChangeWatcherPtr StartWatchingDirs(const char ** dirs,const int numDirs,uint32 notifyFlags)
{
	DirChangeWatcherImpl * watcher = new DirChangeWatcherImpl;
	
	if ( ! watcher->StartWatchingDirs(dirs,numDirs,notifyFlags) )
	{
		delete watcher;
		return DirChangeWatcherPtr(NULL);
	}
	
	return DirChangeWatcherPtr(watcher);
}

DirChangeWatcherPtr StartWatchingDirs(const vector<String> & dirs,uint32 notifyFlags)
{
	DirChangeWatcherImpl * watcher = new DirChangeWatcherImpl;

	int numDirs = dirs.size32();
	vector<const char *> cptrDirs;
	cptrDirs.resize(numDirs);
	for(int i=0;i<numDirs;i++)
	{
		cptrDirs[i] = dirs[i].CStr();
	}
	
	if ( ! watcher->StartWatchingDirs(cptrDirs.data(),numDirs,notifyFlags) )
	{
		delete watcher;
		return DirChangeWatcherPtr(NULL);
	}
	
	return DirChangeWatcherPtr(watcher);
}


//=========================================================================================================


END_CB

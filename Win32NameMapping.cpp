#include "Win32NameMapping.h"

#include <cblib/inc.h>
#include <cblib/vector.h>
#include <cblib/Log.h>
#include <cblib/LogUtil.h>
#include <cblib/FileUtil.h>
#include <cblib/MoreUtil.h>
#include <cblib/Win32Util.h>

#include <windows.h>
#include <direct.h>
#include <Psapi.h>
#include <winioctl.h>

USE_CB

#define BUFSIZE 2048
	
//=====================================================

template <typename t_func_type>
t_func_type GetWindowsImport( t_func_type * pFunc , const char * funcName, const char * libName , bool dothrow)
{
    if ( *pFunc == 0 )
    {
        HMODULE m = GetModuleHandle(libName);
        if ( m == 0 ) m = LoadLibrary(libName); // adds extension for you
        ASSERT_RELEASE( m != 0 );
        t_func_type f = (t_func_type) GetProcAddress( m, funcName );
        if ( f == 0 && dothrow )
        {
			throw funcName;
        }
        *pFunc = f;
    }
    return (*pFunc); 
}

// GET_IMPORT can return NULL
#define GET_IMPORT(lib,name) (GetWindowsImport(&STRING_JOIN(fp_,name),STRINGIZE(name),lib,false))

// CALL_IMPORT throws if not found
#define CALL_IMPORT(lib,name) (*GetWindowsImport(&STRING_JOIN(fp_,name),STRINGIZE(name),lib,true))
#define CALL_KERNEL32(name) CALL_IMPORT("kernel32",name)
#define CALL_NT(name) CALL_IMPORT("ntdll",name)

//=================================================================

//=============================================

#if _MSC_VER < 1500 // 2008
// REMOTE_PROTOCOL_INFO_FLAG_LOOPBACK is defined in windows.h with FILE_BASIC_INFO
#ifndef REMOTE_PROTOCOL_INFO_FLAG_LOOPBACK

typedef struct _FILE_BASIC_INFO {
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    DWORD FileAttributes;
} FILE_BASIC_INFO, *PFILE_BASIC_INFO;

#endif
#endif

typedef enum _FILE_INFO_BY_HANDLE_CLASS {
    FileBasicInfo,
    FileStandardInfo,
    FileNameInfo,
    FileRenameInfo,
    FileDispositionInfo,
    FileAllocationInfo,
    FileEndOfFileInfo,
    FileStreamInfo,
    FileCompressionInfo,
    FileAttributeTagInfo,
    FileIdBothDirectoryInfo,
    FileIdBothDirectoryRestartInfo,
    FileIoPriorityHintInfo,
    FileRemoteProtocolInfo, 
    MaximumFileInfoByHandleClass
} FILE_INFO_BY_HANDLE_CLASS;

typedef LONG NTSTATUS;
typedef enum _FILE_INFORMATION_CLASS { 
  FileDirectoryInformation                 = 1,
  FileFullDirectoryInformation,
  FileBothDirectoryInformation,
  FileBasicInformation,
  FileStandardInformation,
  FileInternalInformation,
  FileEaInformation,
  FileAccessInformation,
  FileNameInformation,
  FileRenameInformation,
  FileLinkInformation,
  FileNamesInformation,
  FileDispositionInformation,
  FilePositionInformation,
  FileFullEaInformation,
  FileModeInformation,
  FileAlignmentInformation,
  FileAllInformation,
  FileAllocationInformation,
  FileEndOfFileInformation,
  FileAlternateNameInformation,
  FileStreamInformation,
  FilePipeInformation,
  FilePipeLocalInformation,
  FilePipeRemoteInformation,
  FileMailslotQueryInformation,
  FileMailslotSetInformation,
  FileCompressionInformation,
  FileObjectIdInformation,
  FileCompletionInformation,
  FileMoveClusterInformation,
  FileQuotaInformation,
  FileReparsePointInformation,
  FileNetworkOpenInformation,
  FileAttributeTagInformation,
  FileTrackingInformation,
  FileIdBothDirectoryInformation,
  FileIdFullDirectoryInformation,
  FileValidDataLengthInformation,
  FileShortNameInformation,
  FileIoCompletionNotificationInformation,
  FileIoStatusBlockRangeInformation,
  FileIoPriorityHintInformation,
  FileSfioReserveInformation,
  FileSfioVolumeInformation,
  FileHardLinkInformation,
  FileProcessIdsUsingFileInformation,
  FileNormalizedNameInformation,
  FileNetworkPhysicalNameInformation,
  FileIdGlobalTxDirectoryInformation,
  FileIsRemoteDeviceInformation,
  FileAttributeCacheInformation,
  FileNumaNodeInformation,
  FileStandardLinkInformation,
  FileRemoteProtocolInformation,
  FileMaximumInformation
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

typedef struct _IO_STATUS_BLOCK {
  union {
    NTSTATUS Status;
    PVOID    Pointer;
  };
  ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef struct _FILE_NAME_INFORMATION {
  ULONG FileNameLength;
  WCHAR FileName[1];
} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;

//=============================================================================================================

NTSTATUS 
(NTAPI * fp_NtQueryInformationFile)(
     HANDLE FileHandle,
    IO_STATUS_BLOCK * IoStatusBlock,
    PVOID FileInformation,
     ULONG Length,
     FILE_INFORMATION_CLASS FileInformationClass
) = 0;

DWORD (WINAPI *fp_GetFinalPathNameByHandleA)(
     HANDLE hFile,
    LPSTR lpszFilePath,
     DWORD cchFilePath,
     DWORD dwFlags
) = NULL;

BOOL
(WINAPI *
fp_GetFileInformationByHandleEx)(
    __in  HANDLE hFile,
    __in  FILE_INFO_BY_HANDLE_CLASS FileInformationClass,
    __out_bcount(dwBufferSize) LPVOID lpFileInformation,
    __in  DWORD dwBufferSize
) = 0;

BOOL
(WINAPI *
fp_SetFileInformationByHandle)(
    __in  HANDLE hFile,
    __in  FILE_INFO_BY_HANDLE_CLASS FileInformationClass,
    __in_bcount(dwBufferSize)  LPVOID lpFileInformation,
    __in  DWORD dwBufferSize
) = 0;

//#define GetFileInformationByHandleEx  CALL_KERNEL32(GetFileInformationByHandleEx)

//=========================================================================================

typedef enum _OBJECT_INFORMATION_CLASS {

ObjectBasicInformation, ObjectNameInformation, ObjectTypeInformation, ObjectAllInformation, ObjectDataInformation

} OBJECT_INFORMATION_CLASS, *POBJECT_INFORMATION_CLASS;

typedef struct _UNICODE_STRING {
  USHORT Length;
  USHORT MaximumLength;
  PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _OBJECT_NAME_INFORMATION {

	UNICODE_STRING Name;
	WCHAR NameBuffer[1];

} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;


NTSTATUS
(NTAPI *
fp_NtQueryObject)(
IN HANDLE ObjectHandle, IN OBJECT_INFORMATION_CLASS ObjectInformationClass, OUT PVOID ObjectInformation, IN ULONG Length, OUT PULONG ResultLength )
= 0;

//=========================================================================================

struct MOUNTMGR_TARGET_NAME { USHORT DeviceNameLength; WCHAR DeviceName[1]; };
struct MOUNTMGR_VOLUME_PATHS { ULONG MultiSzLength; WCHAR MultiSz[1]; };

#define MOUNTMGRCONTROLTYPE ((ULONG) 'm')
#define IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH \
    CTL_CODE(MOUNTMGRCONTROLTYPE, 12, METHOD_BUFFERED, FILE_ANY_ACCESS)

union ANY_BUFFER {
    MOUNTMGR_TARGET_NAME TargetName;
    MOUNTMGR_VOLUME_PATHS TargetPaths;
    char Buffer[4096];
};


//=========================================================================================

START_CB

/*

MapNtDriveName

maps a "from" in "NT" namespace :

\Device\HarddiskVolume1

to a "to" in "Win32" namespace :

c:\

*/

static bool MapNtDriveName_IoControl(const wchar_t * from,wchar_t * to)
{
    ANY_BUFFER nameMnt;
    
    int fromLen = strlen32(from);
	// DeviceNameLength is in *bytes*
    nameMnt.TargetName.DeviceNameLength = (USHORT) ( 2 * fromLen );
    strcpy(nameMnt.TargetName.DeviceName, from );
    
    HANDLE hMountPointMgr = CreateFile( ("\\\\.\\MountPointManager"),
        0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, 0, NULL);
        
    ASSERT_RELEASE( hMountPointMgr != 0 );
        
    DWORD bytesReturned;
    BOOL success = DeviceIoControl(hMountPointMgr,
        IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH, &nameMnt,
        sizeof(nameMnt), &nameMnt, sizeof(nameMnt),
        &bytesReturned, NULL);

    CloseHandle(hMountPointMgr);
    
    if ( success && nameMnt.TargetPaths.MultiSzLength > 0 )
    {    
		strcpy(to,nameMnt.TargetPaths.MultiSz);

		return true;    
    }
    else
    {    
		return false;
	}
}

//=========================================================================================

static bool MapNtDriveName_QueryDosDevice(const wchar_t * from,wchar_t * to)
{
	// Translate path with device name to drive letters.
	wchar_t allDosDrives[BUFSIZE];
	allDosDrives[0] = '\0';

	// GetLogicalDriveStrings
	//	gives you the DOS drives on the system
	//	including substs and network drives
	if (GetLogicalDriveStringsW(BUFSIZE-1, allDosDrives)) 
	{
		wchar_t ntDriveName[BUFSIZE];
		wchar_t * pDosDrives = allDosDrives;

		do 
		{
			// Copy the drive letter to the template string
			wchar_t dosDrive[3] = (L" :");
			*dosDrive = *pDosDrives;

			// Look up each device name
			if ( QueryDosDeviceW(dosDrive, ntDriveName, ARRAY_SIZE(ntDriveName)) )
			{
				size_t ntDriveNameLen = strlen(ntDriveName);

				if ( strnicmp(from, ntDriveName, ntDriveNameLen) == 0
						 && (*(from + ntDriveNameLen) == ('\\') ||
						 *(from + ntDriveNameLen) == 0) )
				{
					strcpy(to,dosDrive);
					strcat(to,from+ntDriveNameLen);
							
					return true;
				}
			}

			// Go to the next NULL character.
			while (*pDosDrives++);

		} while ( *pDosDrives); // double-null is end of drives list
	}

	return false;
}


bool MapNtDriveName(const wchar_t * from,wchar_t * to)
{
	// hard-code network drives :
	if ( strisame(from,L"\\Device\\Mup") || strisame(from,L"\\Device\\LanmanRedirector") )
	{
		strcpy(to,L"\\");
		return true;
	}
	
	//return MapNtDriveName_IoControl(from,to);
	return MapNtDriveName_QueryDosDevice(from,to);
}

//=================================================
      
BOOL GetFileNameFromHandleW_Map(HANDLE hFile,wchar_t * pszFilename,int pszFilenameSize)
{
	BOOL bSuccess = FALSE;
	HANDLE hFileMap;

	pszFilename[0] = 0;

	// Get the file size.
	DWORD dwFileSizeHi = 0;
	DWORD dwFileSizeLo = GetFileSize(hFile, &dwFileSizeHi); 

	if( dwFileSizeLo == 0 && dwFileSizeHi == 0 )
	{
		lprintf(("Cannot map a file with a length of zero.\n"));
		return FALSE;
	}

	// Create a file mapping object.
	hFileMap = CreateFileMapping(hFile, 
					NULL, 
					PAGE_READONLY,
					0, 
					1,
					NULL);

	if (hFileMap) 
	{
		// Create a file mapping to get the file name.
		void* pMem = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 1);

		if (pMem) 
		{
			if (GetMappedFileNameW(GetCurrentProcess(), 
								 pMem, 
								 pszFilename,
								 MAX_PATH)) 
			{
				//pszFilename is an NT-space name :
				//pszFilename = "\Device\HarddiskVolume4\devel\projects\oodle\z.bat"

				wchar_t temp[BUFSIZE];
				strcpy(temp,pszFilename);
				MapNtDriveName(temp,pszFilename);


			}
			bSuccess = TRUE;
			UnmapViewOfFile(pMem);
		} 

		CloseHandle(hFileMap);
	}
	else
	{
		return FALSE;
	}

	return(bSuccess);
}

//=======================================================================================

/*

T: : \??\C:\trans
fmName : t:\prefs.js
FILE_NAME_INFORMATION: (\trans\prefs.js)
OBJECT_NAME_INFORMATION: (\Device\HarddiskVolume4\trans\prefs.js)
drive: (\Device\HarddiskVolume4)
drive2: (C:)
buffer: (C:\trans\prefs.js)
success
toName : C:\trans\prefs.js

-----------------

Y: : \Device\LanmanRedirector\;Y:0000000000034569\charlesbpc\C$
fmName : y:\xfer\path.txt
FILE_NAME_INFORMATION: (\charlesbpc\C$\xfer\path.txt)
OBJECT_NAME_INFORMATION: (\Device\Mup\charlesbpc\C$\xfer\path.txt)
drive: (\Device\Mup)


*/

BOOL GetFinalPathNameByHandleW_NtQuery(HANDLE f,wchar_t * outName,DWORD bufSize)
{
	//return GetFileNameFromHandleXP(f,buffer,bufSize);

	wchar_t relName[BUFSIZE];
	wchar_t absName[BUFSIZE];

	IO_STATUS_BLOCK block = { 0 };
	char infobuf[4096];

	try
	{

	CALL_NT(NtQueryInformationFile)(f,
		&block,
		infobuf,
		sizeof(infobuf),
		FileNameInformation);

	}
	catch(...)
	{
		lprintf("NtQueryInformationFile not found!\n");
		return 0;
	}

	{
	FILE_NAME_INFORMATION * pinfo = (FILE_NAME_INFORMATION *) infobuf;

	pinfo->FileName[(pinfo->FileNameLength)/2] = 0;
	
	strcpy(relName,pinfo->FileName);
	
	lprintf_v2("FILE_NAME_INFORMATION: (%S)\n",relName);
	}

	if ( relName[1] == ':' )
	{
		strcpy(outName,relName);
		return TRUE;
	}
	
	ASSERT_RELEASE( relName[0] == '\\' );

	ULONG ResultLength = 0;

	try
	{

	CALL_NT(NtQueryObject)(f,
		ObjectNameInformation,
		infobuf,
		sizeof(infobuf),
		&ResultLength);

	}
	catch(...)
	{
		lprintf("NtQueryInformationFile not found!\n");
		return FALSE;
	}

	{
	OBJECT_NAME_INFORMATION * pinfo = (OBJECT_NAME_INFORMATION *) infobuf;

	wchar_t * ps = pinfo->NameBuffer;
	// info->Name.Length is in BYTES , not wchars
	ps[ pinfo->Name.Length / 2 ] = 0;

	strcpy(absName,ps);
	
	lprintf_v2("OBJECT_NAME_INFORMATION: (%S)\n",absName);
	}	
	
	if ( absName[1] == ':' )
	{
		// absName is fullName
		strcpy(outName,absName);
		return TRUE;
	}
	
	int relLen = strlen32(relName);
	int absLen = strlen32(absName);
	
	/*
	
	FILE_NAME_INFORMATION: (\devel\projects\oodle\examples\oodle_future.h)
	OBJECT_NAME_INFORMATION: (\Device\HarddiskVolume1\devel\projects\oodle\examples\oodle_future.h)
	drive: (\Device\HarddiskVolume1)
	drive2: (C:)
	fullName: (C:\devel\projects\oodle\examples\oodle_future.h)

	*/
	
	if ( absLen > relLen )
	{
		wchar_t drive[BUFSIZE];
		strcpy(drive,absName);
		drive[ absLen - relLen ] = 0;
		
		lprintf_v2("drive: (%S)\n",drive);
		
		wchar_t drive2[BUFSIZE];
		drive2[0] = 0;
		
		if ( MapNtDriveName(drive,drive2) )
		{
			lprintf_v2("drive2: (%S)\n",drive2);
		}
		else
		{
			lprintf("MapNtDriveName failed (%s)!\n",drive);
		}
		
		strcpy(outName,drive2);
		strcat(outName,relName);
		
		return TRUE;
	}
	else
	{	
		// huh ?		
		// @@@@
			
		return FALSE;
	}
}

BOOL GetFinalPathNameByHandleA_NtQuery(HANDLE f,char * buffer,DWORD bufSize)
{
	wchar_t fullName[BUFSIZE];
	
	if ( ! GetFinalPathNameByHandleW_NtQuery(f,fullName,sizeof(fullName)) )
		return FALSE;

	cb::StrConv_UnicodeToAnsi(buffer,fullName,bufSize);

	lprintf_v2("buffer: (%s)\n",buffer);
	
	return TRUE;
}

BOOL MyGetFinalPathNameByHandleA(HANDLE f,char * buffer,DWORD bufSize)
{
	if ( GET_IMPORT("kernel32",GetFinalPathNameByHandleA) == NULL )
	//if ( 1 )
	{
		// y:\xfer\path.txt
		// toName : \\charlesbpc\C$\xfer\path.txt
		
		return GetFinalPathNameByHandleA_NtQuery(f,buffer,bufSize);
	}
	else
	{
		// y:\xfer\path.txt
		// toName : UNC\charlesbpc\C$\xfer\path.txt

		char temp[BUFSIZE];

		DWORD ret = CALL_IMPORT("kernel32",GetFinalPathNameByHandleA)(f,temp,sizeof(temp),0);

		if ( ret < bufSize )
		{
			// "\\?\UNC\" -> "\\"
			//  "
			
			char * p = temp;
			
			if ( stripresameadvance(&p,"\\\\?\\UNC\\") )
			{
				//temp	= "\\?\UNC\charlesbpc\C$\xfer\path.txt"
		
				strcpy(buffer,"\\\\");
				strcat(buffer,p);
			
				// buffer = "\\charlesbpc\C$\xfer\path.txt"
			}
			else if ( stripresameadvance(&p,"\\\\?\\") )
			{
				strcpy(buffer,p);
			}
			else
			{
				strcpy(buffer,temp);
			}
		
			return TRUE;
		}
		
		return FALSE;
	}
}

//=========================================================================================

bool GetDeSubstName(const char * from, char * to)
{
	strcpy(to,from);

	HANDLE f = CreateFile(from,
		FILE_READ_ATTRIBUTES |
		STANDARD_RIGHTS_READ
		,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,0);
    
    if ( f == INVALID_HANDLE_VALUE )
    {
		//lprintf("ERROR : CreateFile\n");
		return false;
	}

    BY_HANDLE_FILE_INFORMATION info;
    
    BOOL ok1 = GetFileInformationByHandle(f,&info);
    if ( ! ok1 )
    {
		//lprintf("ERROR : GetFileInformationByHandle\n");		
		return false;
    }
    
    char buffer[BUFSIZE];
    DWORD bufSize = ARRAY_SIZE(buffer);

    //DWORD ret = GetFinalPathNameByHandleA(f,buffer,bufSize,FILE_NAME_NORMALIZED);

    BOOL ok = MyGetFinalPathNameByHandleA(f,buffer,bufSize);

    CloseHandle(f);
    f = 0;
    
	if ( ok )
	{
		strcpy(to,buffer);
		
		return true;
	}

	return false;
}

/**

GetDeSubstFullPath - 

it does a GetDeSubstName on each portion of the path
this is needed for *new* file names
eg. if some prefix of the path exists, but the suffix is new
you can only do GetDeSubstName on the prefix that exists

**/
bool GetDeSubstFullPath(const char * from, char * to)
{
	char temp[BUFSIZE];
	strcpy(temp,from);
	
	// start at the end and back up
	//	so we try to DeSubst the longest prefix we can
	//	(needed for symlinks)
	char * pKill = temp + strlen(temp);					
	for(;;)
	{
		char save = *pKill;
		*pKill = 0;

		if ( GetDeSubstName(temp,to) )
		{
			if ( save != 0 )
			{
				char * ptoEnd = to + strlen(to);
				ptoEnd[0] = save;
				ptoEnd[1] = 0;
				strcat(ptoEnd,pKill+1);
			}
			
			return true;
		}
		
		char * nextKill = cb::strrchr2(temp,'\\','/');
		*pKill = save;
		pKill = nextKill;
		if ( pKill <= temp+2 )
			break;		
	}
	
	strcpy(to,from);
	return false;
}

//=================================================================
			
static void FixCaseAtEnd(char * name)
{
	// can't do wilds :
	if ( strchr(name,'*') != NULL )
		return;
	
	WIN32_FIND_DATA find;
	HANDLE h = FindFirstFile(name,&find);
	if ( h == INVALID_HANDLE_VALUE )
	{
		return;
	}
	FindClose(h);
	
	char * pFilePart = const_cast<char *>( cb::filepart(name) );
	
	// must be only case change :
	ASSERT_RELEASE( stricmp(pFilePart,find.cFileName) == 0 );
	
	strcpy(pFilePart,find.cFileName);
}

void WinUtil_GetFullPathProperCase(const char * fm, char * to)
{
	char temp1[_MAX_PATH];
	GetFullPathName(fm,_MAX_PATH,temp1,NULL);

	char temp2[_MAX_PATH];
	DWORD copied = GetLongPathName(temp1,temp2,_MAX_PATH);
	
	if ( copied == 0 )
	{
		// file doesn't exist
		strcpy(temp2,temp1);
	}

	char temp[_MAX_PATH];
	GetDeSubstFullPath(temp2,temp);
		
	ASSERT( temp[1] == ':' );
	ASSERT( temp[2] == '\\' );
	
	// CB 2-22-05 : there seems to be no way to get the drive letter right; just force it to LOWER CASE
	temp[0] = (char) tolower(temp[0]);
	
	char * pKill = temp + strlen(temp);
	for(;;)
	{
		char save = *pKill;
		*pKill = 0;
		
		FixCaseAtEnd(temp);
		
		char * nextKill = strrchr(temp,'\\');
		*pKill = save;
		pKill = nextKill;
		if ( pKill <= temp+2 )
			break;
	}
	
	strcpy(to,temp);
}

//=====================================================

/**

LogDosDrives :

C: : \Device\HarddiskVolume4
D: : \Device\HarddiskVolume2
E: : \Device\CdRom0
M: : \??\D:\misc
P: : \??\C:\Users\cbloom\Private
R: : \??\C:\ramdisk
S: : \Device\ImDisk0
T: : \??\C:\trans
V: : \??\C:
W: : \??\C:\myweb
Y: : \Device\LanmanRedirector\;Y:0000000000034569\charlesbpc\C$

A: : \Device\Floppy0
C: : \Device\HarddiskVolume1
D: : \Device\HarddiskVolume2
E: : \Device\CdRom0
H: : \Device\CdRom1
I: : \Device\CdRom2
M: : \??\D:\misc
R: : \??\D:\ramdisk
S: : \??\D:\ramdisk
T: : \??\D:\trans
V: : \??\C:
W: : \Device\LanmanRedirector\;W:0000000000024326\radnet\raddevel
Y: : \Device\LanmanRedirector\;Y:0000000000024326\radnet\radmedia
Z: : \Device\LanmanRedirector\;Z:0000000000024326\charlesb-pc\c

**/

void LogDosDrives()
{
	// Translate path with device name to drive letters.
	wchar_t szTemp[BUFSIZE];
	szTemp[0] = '\0';

	// GetLogicalDriveStrings
	//	gives you the DOS drives on the system
	//	including substs and network drives
	if (GetLogicalDriveStringsW(BUFSIZE-1, szTemp)) 
	{
	  wchar_t szName[MAX_PATH];
	  wchar_t szDrive[3] = (L" :");

	  wchar_t * p = szTemp;

	  do 
	  {
		// Copy the drive letter to the template string
		*szDrive = *p;

		// Look up each device name
		if (QueryDosDeviceW(szDrive, szName, MAX_PATH))
		{
			lprintf("%S \"%S\"\n",szDrive,szName);
		}

		// Go to the next NULL character.
		while (*p++);
		
	  } while ( *p); // double-null is end of drives list
	}

	return;
}

//=======================================================

END_CB

//=========================================================================================

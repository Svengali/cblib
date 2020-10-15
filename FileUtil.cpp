#include <sys/stat.h>
#include <direct.h>
#include <sys/utime.h>
#include <io.h>
#include <errno.h>

#include "FileUtil.h"
#include "Win32Util.h"
#include "Log.h"
#include "Timer.h"
#include "File.h"

START_CB

//-----------------------------------------------------------

bool MkDir(const char * name)
{
	BOOL res = CreateDirectoryA(name,NULL);
	
	return !! res;
}

bool RenameFile( const char * fm, const char * to, bool force )
{
	DWORD flags = 0;
	if ( force ) flags = MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING;
	BOOL res = MoveFileExA( fm, to , force);
	return !! res;
}

bool MyDeleteFile( const char * name )
{
	BOOL res = ::DeleteFileA( name );
	return !! res;
}

	
//=================================================================

const char * extensionpart(const char * path)
{
	const char * dot = strrchr(path,'.');
	if ( dot == NULL )
		return path + strlen(path);
	return (dot+1);
}
	
const char * filepart(const char * path)
{
	const char * p1 = strrchr(path,'\\');
	const char * p2 = strrchr(path,'/');

	const char * p = MAX(p1,p2);

	if ( p != NULL )
	{
		ASSERT( *p == '\\' || *p == '/' );
		return (p+1);
	}

	return path;
}

char * extensionpart(char * path)
{
	return (char *) extensionpart( (const char *)path );
}
char * filepart(char * path)
{
	return (char *) filepart( (const char *)path );
}

void getpathpart(const char * path,char * into)
{
	strcpy(into,path);
	char * fp = filepart(into);
	*fp = 0;
	// NOTE : this leaves the trailing slash on the path
}

char LastChar(const char * str)
{
	int len = strlen32(str);
	if ( len == 0 )
		return 0;
	return str[len-1];
}
wchar_t LastChar(const wchar_t * str)
{
	int len = strlen32(str);
	if ( len == 0 )
		return 0;
	return str[len-1];
}

bool IsPathDelim(char c)
{
	return ( c == '\\' || c == '/' );
}

void StandardizePathDelims(char * ptr)
{
	while( *ptr )
	{
		if ( IsPathDelim(*ptr) )
		{
			*ptr = '\\';
		}
		ptr++;
	}
}

void StandardizePathDelims(wchar_t * ptr)
{
	while( *ptr )
	{
		if ( IsPathDelim(*ptr) )
		{
			*ptr = '\\';
		}
		ptr++;
	}
}

void CutEndPath(char *Path)
{
	char * EndBase = &Path[strlen(Path)-1];
	if ( IsPathDelim(*EndBase) )
		*EndBase = 0;
}

void CatPaths(char *onto,const char *add)
{
	CutEndPath(onto);
	
	if ( ! IsPathDelim(*add) )
	{
		strcat(onto,"\\");
	}
	
	strcat(onto,add);
}

String filepart(const String & path)
{
	return String( filepart(path.CStr()) );
}
String pathpart(const String & path)
{
	return getpathpart(path.CStr());
}

String getpathpart(const char * path)
{
	String ret(path);
	const char * fp = filepart(ret.CStr());
	ret[fp] = 0;
	// NOTE : this leaves the trailing slash on the path
	return ret;
}

char LastChar(const String & str)
{
	if ( str.Length() == 0 ) return 0;
	else return str[ str.Length() - 1 ];
}

void CutEndPath(String * pStr)
{
	if ( IsPathDelim( LastChar(*pStr) ) )
		pStr->PopBack();
}

void RemoveExtension(String * pStr)
{	
	const char * path = pStr->CStr();
	const char * dot = strrchr(path,'.');
	if ( dot )
	{
		int len = ptr_diff_32( dot - path );
		ASSERT( (*pStr)[len] == '.' );
		pStr->Truncate( len );
	}
}

void ChangeExtension(String * pStr,const char * newExt)
{
	RemoveExtension(pStr);
	if ( *newExt != '.' )
		*pStr += '.';
	*pStr += newExt;
}
	
String CatPaths(const String & p1,const String & p2)
{
	if ( p2.IsEmpty() )
		return p1;

	String onto(p1);
	CutEndPath(&onto);
	
	if ( ! IsPathDelim(p2[0]) )
	{
		onto += '\\';
	}
	
	onto += p2;
	
	return onto;
}

String CombinePaths(const String & p1,const String & p2)
{
	//char temp[2048];
	vector<char> temp(p1.Length()+p2.Length()+1024,(char)0);
	CombinePaths(p1.CStr(),p2.CStr(),temp.data());
	return String(temp.data());
}

String PrefixCurDir(const String & AddTo)
{
	return CombinePaths( GetCurDir(), AddTo );
}

/** this func works even when AddTo has backwards-path
				components. i.e. it works as a full 'cd' :
		PrefixCurDir("/") or PrefixCurDir("..\..") work fine

		note : if you are in a drive, and you do cd("/") you will
			go up one dir - to the null path, which on the Amiga is
			your boot drive.  I'm not sure what the null path is under
			WinNT, but it would be nice if it were the My Computer thing.
	**/

void PrefixCurDir(char *AddTo)
{
	char CurDir[1024];

	getcwd(CurDir,1024);
	if ( *CurDir == 0 ) return;
	
	char Temp[2048];
	CombinePaths(CurDir,AddTo,Temp);
	
	strcpy(AddTo,Temp);
}

void CombinePaths(const char *base,const char *add, char * into)
{
	// should I do the "c:" thing dos does to treat it as the current cd in c: drive?
	if ( add[1] == ':' && ! IsPathDelim(add[2]) )
	{
		int drive = tolower(add[0]) - 'a' + 1;
		char cwd[1024];
		if ( _getdcwd(drive,cwd,sizeof(cwd)) != NULL )
		{
			CombinePaths(cwd,add+2,into);
			return;
		}
	}

	char CurDir[2048];
	strcpy(CurDir,base);

	// single \ means root , single / means parent
	if ( *add == ':' || *add == '\\' )
	{
		if ( CurDir[1] == ':' )
		{
			//DrivePartInsert(CurDir,AddTo);
			into[0] = CurDir[0];
			into[1] = CurDir[1];
			into[2] = '\\';
			strcpy(into+3,add+1);
		}
		else
		{
			// lame?
			strcpy(into,add+1);
		}
		return;
	}

	if ( strchr(add,':') )
	{
		strcpy(into,add);
		return;
	}
  
	CatPaths(CurDir,"");

	const char * AddToPtr = add;

	// single forward slash means parent
	while ( *AddToPtr == '/' || ( AddToPtr[0] == '.' && AddToPtr[1] == '.' ) )
	{
		if ( *AddToPtr == '/' ) AddToPtr++;
		else
		{
			AddToPtr += 2;
			if ( IsPathDelim(*AddToPtr) ) AddToPtr++;
		}

		int CurLen = strlen32(CurDir);
		ASSERT_RELEASE( CurLen > 0 );
		*(CurDir+CurLen-1) = 0; /* cut off path delim */
		*(filepart(CurDir)) = 0; /* cut one dir */

		if ( *CurDir == 0 ) return;
	}

	// .\ means curdir, skip it
	while ( AddToPtr[0] == '.' && AddToPtr[1] == '\\' )
	{
		AddToPtr += 2;
	}
	// single dot is okay too :
	if ( AddToPtr[0] == '.' && AddToPtr[1] == 0 )
	{
		AddToPtr += 1;
	}

	strcat(CurDir,AddToPtr);
	strcpy(into,CurDir);
	StandardizePathDelims(into);
	
	int len = strlen32(into);
	if ( into[len-1] == '.' && into[len-2] == '\\' )
	{
		into[len-2] = 0;
	}
}

void MakeRelativePath(const char * absPath,const char * fromDir, char * into)
{
	if ( absPath[0] != fromDir[0] )
	{
		strcpy(into,absPath);
		return;
	}
	
	const char * pTo = absPath;
	const char * pFm = fromDir;
	
	// as long as slashed portions are the same, step forward :
	for(;;)
	{
		const char * pNextFm = strchrset(pFm,"/\\");
		if ( ! pNextFm )
		{
			pNextFm = strend(pFm);
		}
		int len = ptr_diff_32( pNextFm - pFm );
		if ( len == 0 )
			break;
			
		if ( IsPathDelim(pFm[len]) && ! IsPathDelim(pTo[len]) )
			break;
			
		if ( memicmp(pFm,pTo,len) == 0 )
		{
			pFm += len;
			pTo += len;
			
			if ( IsPathDelim(*pFm) ) pFm ++;
			if ( IsPathDelim(*pTo) ) pTo ++;
		}
		else
		{
			break;
		}
	}
	
	into[0] = 0;
	
	// now some amount not the same ; step back from fromDir then forward to pTo
	if ( IsPathDelim(*pFm) ) pFm ++;

	while ( *pFm )	
	{
		const char * pNextFm = strchrset(pFm,"/\\");
		if ( pNextFm == NULL )
			pNextFm = strend(pFm);
		
		strcat(into,"..\\");
		pFm = pNextFm;
		if ( IsPathDelim(*pFm) )
			pFm++;
	}
	
	strcat(into,pTo);
}

void CombinePaths(const wchar_t *base,const wchar_t *add, wchar_t * into)
{
	// should I do the "c:" thing dos does to treat it as the current cd in c: drive?
	if ( add[1] == ':' && ! IsPathDelim(add[2]) )
	{
		int drive = tolower(add[0]) - 'a' + 1;
		wchar_t cwd[1024];
		if ( _wgetdcwd(drive,cwd,wsizeof(cwd)) != NULL )
		{
			CombinePaths(cwd,add+2,into);
			return;
		}
	}
	
	wchar_t CurDir[2048];
	strcpy(CurDir,base);

	// single \ means root , single / means parent
	if ( *add == ':' || *add == '\\' )
	{
		if ( CurDir[1] == ':' )
		{
			//DrivePartInsert(CurDir,AddTo);
			into[0] = CurDir[0];
			into[1] = CurDir[1];
			into[2] = '\\';
			strcpy(into+3,add+1);
		}
		else
		{
			// lame?
			strcpy(into,add+1);
		}
		return;
	}

	if ( strchr(add,':') )
	{
		strcpy(into,add);
		return;
	}
  
	CatPaths(CurDir,L"");

	const wchar_t * AddToPtr = add;

	// single forward slash means parent
	while ( *AddToPtr == '/' || ( AddToPtr[0] == '.' && AddToPtr[1] == '.' ) )
	{
		if ( *AddToPtr == '/' ) AddToPtr++;
		else
		{
			AddToPtr += 2;
			if ( IsPathDelim(*AddToPtr) ) AddToPtr++;
		}

		int CurLen = strlen32(CurDir);
		ASSERT_RELEASE( CurLen > 0 );
		*(CurDir+CurLen-1) = 0; /* cut off path delim */
		*(filepart(CurDir)) = 0; /* cut one dir */

		if ( *CurDir == 0 ) return;
	}

	// .\ means curdir, skip it
	while ( AddToPtr[0] == '.' && AddToPtr[1] == '\\' )
	{
		AddToPtr += 2;
	}

	strcat(CurDir,AddToPtr);
	strcpy(into,CurDir);
	StandardizePathDelims(into);
	
	int len = strlen32(into);
	if ( into[len-1] == '.' && into[len-2] == '\\' )
	{
		into[len-2] = 0;
	}
}

//=================================================================

const wchar_t * extensionpart(const wchar_t * path)
{
	const wchar_t * dot = strrchr(path,'.');
	if ( dot == NULL )
		return path + strlen(path);
	return (dot+1);
}
	
const wchar_t * filepart(const wchar_t * path)
{
	const wchar_t * p1 = strrchr(path,'\\');
	const wchar_t * p2 = strrchr(path,'/');

	const wchar_t * p = MAX(p1,p2);

	if ( p != NULL )
	{
		ASSERT( *p == '\\' || *p == '/' );
		return (p+1);
	}

	return path;
}

wchar_t * extensionpart(wchar_t * path)
{
	return (wchar_t *) extensionpart( (const wchar_t *)path );
}
wchar_t * filepart(wchar_t * path)
{
	return (wchar_t *) filepart( (const wchar_t *)path );
}

void getpathpart(const wchar_t * path,wchar_t * into)
{
	strcpy(into,path);
	wchar_t * fp = filepart(into);
	*fp = 0;
}

bool IsPathDelim(wchar_t c)
{
	return ( c == '\\' || c == '/' );
}

void CutEndPath(wchar_t *Path)
{
	wchar_t * EndBase = &Path[strlen(Path)-1];
	if ( IsPathDelim(*EndBase) )
		*EndBase = 0;
}

void CatPaths(wchar_t *onto,const wchar_t *add)
{
	CutEndPath(onto);
	
	if ( ! IsPathDelim(*add) )
	{
		strcat(onto,L"\\");
	}
	
	strcat(onto,add);
}

/** this func works even when AddTo has backwards-path
				components. i.e. it works as a full 'cd' :
		PrefixCurDir("/") or PrefixCurDir("..\..") work fine

		note : if you are in a drive, and you do cd("/") you will
			go up one dir - to the null path, which on the Amiga is
			your boot drive.  I'm not sure what the null path is under
			WinNT, but it would be nice if it were the My Computer thing.
	**/

void PrefixCurDir(wchar_t *AddTo)
{
	wchar_t CurDir[1024];

	_wgetcwd(CurDir,1024);
	if ( *CurDir == 0 ) return;
	
	wchar_t Temp[2048];
	CombinePaths(CurDir,AddTo,Temp);
	
	strcpy(AddTo,Temp);
}

//-----------------------------------------------------------------------------

void StrConv_AnsiToUnicode(wchar_t * to,const char * from,int maxlen)
{
	MultiByteToWideChar(CP_ACP,0,from,-1,to,maxlen);
}
void StrConv_ConsoleToUnicode(wchar_t * to,const char * from,int maxlen)
{
	MultiByteToWideChar(GetConsoleCP(),0,from,-1,to,maxlen);
}
void StrConv_UnicodeToAnsi(char * to,const wchar_t * from,int maxlen)
{
	WideCharToMultiByte(CP_ACP,0,from,-1,to,maxlen,NULL,NULL);
}
void StrConv_UnicodeToConsole(char * to,const wchar_t * from,int maxlen)
{
	WideCharToMultiByte(GetConsoleCP(),0,from,-1,to,maxlen,NULL,NULL);
}
	
String UnicodeToAnsi(const wchar_t * from)
{
	char temp[1024];
	StrConv_UnicodeToAnsi(temp,from,1024);
	return String(temp);
}

String UnicodeToConsole(const wchar_t * from)
{
	char temp[1024];
	StrConv_UnicodeToConsole(temp,from,1024);
	return String(temp);
}
	
bool GetUnicodeFileNameFromMatch(wchar_t * to,const wchar_t * findPath,const char * findName,int maxlen,EMatchType matchType)
{
	wchar_t wFindSpec[1024];
	strcpy(wFindSpec,findPath);
	CatPaths(wFindSpec,L"*");

	WIN32_FIND_DATAW data;
		
	HANDLE handle = FindFirstFileW(wFindSpec,&data);
	if ( handle == INVALID_HANDLE_VALUE )
	{
		return false;
	}
	
	bool found = false;
	
	do	
	{
		// process data
		bool match = false;
		
		if ( matchType == eMatch_Ansi || matchType == eMatch_Either )
		{
			char ansiName[1024];
			StrConv_UnicodeToAnsi(ansiName,data.cFileName,1024);
			match |= strisame(ansiName,findName);
		}
		
		if ( matchType == eMatch_Console || matchType == eMatch_Either )
		{
			char oemName[1024];
			StrConv_UnicodeToConsole(oemName,data.cFileName,1024);
			match |= strisame(oemName,findName);
		}
	
		if ( match )
		{
			if ( found )
			{
				lprintf("DANGER : Unicode ambiguity, file found twice!!\n");
				break;
			}
		
			wcscpy(to,wFindSpec);
			int len = (int) wcslen(to);
			len--;
			to[len]=0;
			wcscpy(to+len,data.cFileName);

			found = true;
			// don't break now because we want to check for multiples
		}
	
	}
	while ( FindNextFileW(handle,&data) );
		
	FindClose(handle);
	
	return found;	
}

// GetUnicodeFileNameFromAnsi
//	from name must be a full path spec
//	does a search for the uni name that matches the ansi
// only works if the dir names are ansi !! does not support uni dir names !!
bool GetUnicodeFileNameFromMatch(wchar_t * to,const char * fromStr,int maxlen,EMatchType matchType)
{
	// have to do a search :
	
	char fromFullPath[1024];
	strcpy(fromFullPath,fromStr);
	if ( fromStr[1] != ':' )
	{
		PrefixCurDir(fromFullPath);
	}
	
	const char * filePart = strrchr(fromFullPath,'\\');
	if ( ! filePart )
		return false;
	filePart++;
	
	char pathpart[1024];
	getpathpart(fromFullPath,pathpart);
	
	wchar_t wpathpart[1024];
	StrConv_AnsiToUnicode(wpathpart,pathpart,1024);

	return cb::GetUnicodeFileNameFromMatch(to,wpathpart,filePart,maxlen,matchType);
}

/*
void GetUnicodeMatchOrCopy(wchar_t * to,const char * fromStr,int maxlen,EMatchType matchType)
{
	if ( GetUnicodeFileNameFromMatch(to,fromStr,maxlen,matchType) )
		return;

	if ( matchType == eMatch_Ansi )	
		StrConv_AnsiToUnicode(to,fromStr,maxlen);
	else
		StrConv_ConsoleToUnicode(to,fromStr,maxlen);
}
*/

bool FileExistsMatch(const char * fromStr) // uses GetUnicodeFileNameFromMatch
{
	wchar_t temp[1024];
	if ( MakeUnicodeNameFullMatch(temp,fromStr,1024) )
		return true;
	else
		return false;
}


bool MakeUnicodeNameFullMatch(wchar_t * to,const char * fromStr,int maxlen,EMatchType matchType)
{
	char work[2048];
	strcpy(work,fromStr);
	PrefixCurDir(work);
	StandardizePathDelims(work);
	
	ASSERT( work[1] == ':' );
	ASSERT( work[2] == '\\' );

	char * curStart = work+3;
	
	// copy drive :
	to[0] = work[0];
	to[1] = work[1];
	to[2] = work[2];
	to[3] = 0;
	
	bool found = true;
	
	for(;;)
	{
		wchar_t curMatch[1024];

		if ( *curStart == 0 )
		{
			break;
		}
		
		char * nextSlash = strchr(curStart,'\\');
		if ( nextSlash )
		{
			*nextSlash = 0;
		}
	
		bool match = GetUnicodeFileNameFromMatch(curMatch,to,curStart,1024,matchType);
		
		if ( nextSlash )
		{
			*nextSlash = '\\';
		}
		
		if ( ! match )
		{
			// no match, just put on the rest
			
			if ( matchType == eMatch_Ansi )	
				StrConv_AnsiToUnicode(curMatch,curStart,maxlen);
			else
				StrConv_ConsoleToUnicode(curMatch,curStart,maxlen);
		
			CatPaths(to,curMatch);
			
			found = false;
			break;
		}
		
		// put on current partial match and continue
		CatPaths(to,filepart(curMatch));
		
		if ( ! nextSlash )
		{
			found = true;
			break;
		}
		
		curStart = nextSlash+1;		
	}
	
	// if from ended in a slash, so do we :
	if ( IsPathDelim(LastChar(fromStr)) )
	{
		CatPaths(to,L"");
	}
	
	if ( ! found )
	{
		// if we didn't match, what we made is junk
	
		if ( matchType == eMatch_Ansi )	
			StrConv_AnsiToUnicode(to,fromStr,maxlen);
		else
			StrConv_ConsoleToUnicode(to,fromStr,maxlen);
	}
	
	return found;
}

//-----------------------------------------------------------------------------

FILE * fmemopen(void * buffer,int size)
{
	// makes a file in root ! WTF
	//FILE * ret = tmpfile();
		
	FILE * ret = fopen( GetTempName().CStr(), "w+b" );
	
	fwrite(buffer,1,size,ret);
	fflush(ret);
	rewind(ret);
	
	return ret;
}

void RemoveExtension(char * path)
{
	char * dot = strrchr(path,'.');
	if ( dot )
		*dot = 0;
}


// stdc tmpnam seems to put files in the fucking root !?
void MakeTempFileName(char * into,int intoSize)
{
	// use better routines on windows

	do
	{
		char tempPath[MAX_PATH];
		GetTempPath(sizeof(tempPath),tempPath);

		// Windows GetTempFileName thing is pretty retarded as well
		//	it will actually speculatively open to find stuff

		static uint32 s_seqNum = 1;
		
		// seqNum = 0 means use time to make a unique name
		//UINT seqNum = 0;
		uint32 seqNum = s_seqNum; ++s_seqNum; // not thread safe
		
		// just make my own, christ :
		
		Timer::tsc_type tsc = Timer::rdtsc();
		uint32 tscLow = (uint32)tsc;
			
		char tempFile[MAX_PATH];
		sprintf(tempFile,"cb_%d_%08X.tmp",seqNum,tscLow);
		
		CombinePaths(tempPath,tempFile,into);
		into[intoSize-1] = 0;
		
		// retry while this name exists :
		//	(would be very bad luck)
		
	} while( FileExists(into) );
	
}

String GetTempName()
{
	char buffer[_MAX_PATH];
	MakeTempFileName(buffer,sizeof(buffer));
	return String(buffer);
}
	
String GetTempDir()
{
	// GetTempPath gets %TMP%

	char buffer[_MAX_PATH];
	GetTempPath(sizeof(buffer),buffer);
	String ret(buffer);
	if ( ! IsPathDelim(LastChar(ret)) )
		ret += '\\';
	return ret;
}

String GetCurDir() // has a backslash on it
{
	char buffer[_MAX_PATH];
	getcwd(buffer,_MAX_PATH);
	String ret(buffer);
	if ( ! IsPathDelim(LastChar(ret)) )
		ret += '\\';
	return ret;
}

#ifndef S_ISDIR
#define S_ISDIR( m )	(((m) & S_IFMT) == S_IFDIR)
#endif

bool IsDir(const struct _stat64 & st)
{
	return S_ISDIR(st.st_mode);
}

/**

A note on stat() :

Normally for doing a stat on dirs you must remove the trailing slash
However for the drive root, you must have the trailing slash

**/

bool StatMatch(const char * fromStr,struct _stat64 * st)
{
	wchar_t uni[1024];
	
	if ( ! MakeUnicodeNameFullMatch(uni,fromStr,1024) )
		return false;
	
	if ( uni[1] == ':' && uni[2] == 0 )
	{
		strcat(uni,L"\\");
	}
		
	return ( _wstat64(uni,st) == 0 );
}

bool Stat(const char * name,struct _stat64 * st)
{
	memset(st,0,sizeof(st));
	char temp[1024];
	strcpy(temp,name);
	CutEndPath(temp);
	
	// Stat fails on drive roots !?
	if ( temp[1] == ':' && temp[2] == 0 )
	{
		strcat(temp,"\\");
	}
	
	return ( _stat64(temp,st) == 0 );	
}

bool Stat(const wchar_t * name,struct _stat64 * st)
{
	memset(st,0,sizeof(st));
	wchar_t temp[1024];
	strcpy(temp,name);
	CutEndPath(temp);
	
	// Stat fails on drive roots !?
	if ( temp[1] == ':' && temp[2] == 0 )
	{
		strcat(temp,L"\\");
	}
	
	return ( _wstat64(temp,st) == 0 );
}
	
bool copystat(const wchar_t * tofile,const wchar_t *fmfile)
{
struct _stat64 s;
	if ( Stat(fmfile,&s) != true )
		return false;
return setstat(tofile,&s);
}

bool setstat(const wchar_t * file,const struct _stat64 *s)
{
	struct _utimbuf ut;

	_wchmod(file,S_IREAD|S_IWRITE|S_IEXEC);

	ut.actime  = s->st_atime;
	ut.modtime = s->st_mtime;
	if ( _wutime(file, &ut) != 0 )
	{
		switch(errno)
		{
		case EACCES:
			lputs("utime failed : access denied");
			break;
		case EINVAL:
			lputs("utime failed : invalid date");
			break;
		case EMFILE:
			lputs("utime failed : too many files open !?");
			break;
		case ENOENT:
			lputs("utime failed : target file not found");
			break;
		default:
			lputs("utime failed : unknown error");
			break;
		}

		return false;
	}

	if ( _wchmod(file, s->st_mode ) != 0 )
		return false;

return true;
}

//-----------------------------------------------------------------------------


bool MyMoveFile(const char *fm,const char *to)
{
	if ( ! MoveFileEx(fm,to,MOVEFILE_REPLACE_EXISTING) )
	{
		DWORD err = GetLastError();
		if ( err == ERROR_FILE_NOT_FOUND )
		{
			// that's okay
			return true;
		}
		else
		{
			return false;
		}
	}
	
	return true;
}

int64 GetFileLength(const char *name)
{
	struct _stat64 st;

	if ( Stat(name,&st) )
		return st.st_size;

	return CB_FILE_LENGTH_INVALID;
}

bool FileExists(const char *Name)
{
	struct _stat64 st;

	if ( Stat(Name,&st) )
		return true;

	return false;
}

time_t FileModTime(const char *Name)
{
	struct _stat64 st;
	
	if ( Stat(Name,&st) )
		return st.st_mtime;

	return 0;
}


bool NameIsDir(const char *Name)
{
	struct _stat64 st;

	if ( Stat(Name,&st) )
	{
		return S_ISDIR(st.st_mode);
	}
	
	return false;
}


bool MyMoveFile(const wchar_t *fm,const wchar_t *to)
{
	if ( ! MoveFileExW(fm,to,MOVEFILE_REPLACE_EXISTING) )
	{
		DWORD err = GetLastError();
		if ( err == ERROR_FILE_NOT_FOUND )
		{
			// that's okay
			return true;
		}
		else
		{
			return false;
		}
	}
	
	return true;
}

int64 GetFileLength(const wchar_t *name)
{
	struct _stat64 st;

	if ( Stat(name,&st) )
		return st.st_size;

	return CB_FILE_LENGTH_INVALID;
}

bool FileExists(const wchar_t *Name)
{
	struct _stat64 st;

	if ( Stat(Name,&st) )
		return true;

	return false;
}

time_t FileModTime(const wchar_t *Name)
{
	struct _stat64 st;
	
	if ( Stat(Name,&st) )
		return st.st_mtime;

	return 0;
}

bool NameIsDir(const wchar_t *Name)
{
	struct _stat64 st;

	if ( Stat(Name,&st) )
	{
		return ( S_ISDIR(st.st_mode) );
	}
	
	return false;
}

//===============================================================

char * ReadWholeFile(FILE * fp,int64 * pLength)
{
	//fseek(fp,0,SEEK_END);
	_fseeki64(fp,0,SEEK_END);
	
//	int64 length = _ftelli64(fp);
	int64 length = ftell64(fp);

	if ( pLength )
		*pLength = length;	
		
	//fseek(fp,0,SEEK_SET);
	rewind(fp);

	//char * data = (char *) CBALLOC( check_value_cast<size_t>(length) +16);
	char * data = (char *) CBALLOC( check_value_cast<size_t>(length) + 1024);

	if ( data )
	{
		FRead(fp,data,length);
		data[length] = 0;
		// put some junk on the end :
		data[length+1] = 104;
		data[length+2] = 17;
	}
		
	return data;
}

char * ReadWholeFile(const char *name,int64 * pLength)
{
	FILE * fp = fopen(name,"rb");
	if ( ! fp )
		return NULL;
	
	char * data = ReadWholeFile(fp,pLength);
	
	fclose(fp);
	
	return data;
}

char * ReadWholeFile(const wchar *name,int64 * pLength)
{
	FILE * fp = _wfopen(name,L"rb");
	if ( ! fp )
		return NULL;
	
	char * data = ReadWholeFile(fp,pLength);
	
	fclose(fp);
	
	return data;
}

char * ReadWholeFile(File & file)
{
	FILE * fp = file.Get();
	if ( ! fp )
		return NULL;
	
	char * ret = ReadWholeFile(fp);
	
	fclose(fp);

	return ret;
}

bool WriteWholeFile(FILE * fp,const void * buffer,int64 length)
{
	fseek64(fp,0,SEEK_SET);

	int64 count = FWrite(fp,(void *)buffer,length);

	if ( count != length )
		return false;

	return true;
}

bool WriteWholeFile(const char *name,const void * buffer,int64 length)
{
	FILE * fp = fopen(name,"wb");
	if ( ! fp )
		return false;
	
	bool ret = WriteWholeFile(fp,buffer,length);
	
	fclose(fp);
	
	return ret;
}

bool WriteWholeFile(const wchar *name,const void * buffer,int64 length)
{
	FILE * fp = _wfopen(name,L"wb");
	if ( ! fp )
		return false;
	
	bool ret = WriteWholeFile(fp,buffer,length);
	
	fclose(fp);
	
	return ret;
}

bool WriteWholeFile(File & file,const void * buffer,int64 length)
{
	FILE * fp = file.Get();
	if ( ! fp )
		return false;
	
	bool ret = WriteWholeFile(fp,buffer,length);
	
	fclose(fp);

	return ret;
}

//===============================================================

void ReserveFileSpace(const char * name,int toReserve)
{
	HANDLE fh = CreateFile(name,GENERIC_WRITE,0,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
	if ( fh  == INVALID_HANDLE_VALUE )
	{
		return;
	}
	
	int64 oldSize = GetFileSize64(fh);
	
	int64 newSize = oldSize + toReserve;
	
	int64 dwSet = SetFilePointer64(fh,newSize,FILE_BEGIN);
	ASSERT_RELEASE( dwSet != INVALID_SET_FILE_POINTER );
	
	SetEndOfFile(fh);
	
	CloseHandle(fh);
	
	fh = CreateFile(name,GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	
	dwSet = SetFilePointer64(fh,oldSize,FILE_BEGIN);
	ASSERT_RELEASE( dwSet != INVALID_SET_FILE_POINTER );
	
	SetEndOfFile(fh);
	
	CloseHandle(fh);
}

int64 GetFileSize64(void * handle)
{
	LARGE_INTEGER lint;
	GetFileSizeEx(handle,&lint);
	return same_size_bit_cast_u<int64>(lint);
}

#define INVALID_SET_FILE_POINTER_64	((int64)-1)
	
int64 SetFilePointer64(void * hFile,int64 distance, int moveType)
{
	LARGE_INTEGER lint,lintNew;
	same_size_bit_cast_u<int64>(lint) = distance;
	if ( ! SetFilePointerEx(hFile,lint,&lintNew,moveType) )
		return INVALID_SET_FILE_POINTER_64;
	return same_size_bit_cast_u<int64>(lintNew);
}

int64 ReadFile64(void * hFile,void * buffer, int64 toRead)
{
	char * ptr = (char *) buffer;
	while ( toRead > 0 )
	{
		DWORD curRead = (DWORD) MIN( toRead, (4<<20) );
		
		DWORD numGot = 0;
		ReadFile(hFile,ptr,curRead,&numGot,NULL);
		
		ptr += numGot;
		toRead -= numGot;
		
		if ( numGot != curRead )
			break;
	}
	
	return (ptr - (char*)buffer);
}
   
int64 WriteFile64(void * hFile,void * buffer, int64 toRead)
{
	char * ptr = (char *) buffer;
	while ( toRead > 0 )
	{
		DWORD curRead = (DWORD) MIN( toRead, (4<<20) );
		
		DWORD numGot = 0;
		WriteFile(hFile,ptr,curRead,&numGot,NULL);
		
		ptr += numGot;
		toRead -= numGot;
		
		if ( numGot != curRead )
			break;
	}
	
	return (ptr - (char*)buffer);
}
    
void AppendToFile(const char * onto,const char * from,bool andDelete)
{
	HANDLE fromFH = CreateFile(from,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	if ( fromFH == INVALID_HANDLE_VALUE )
	{
		if ( GetLastError() == ERROR_FILE_NOT_FOUND )
			return; // that's fine
			 
		LogLastError("AppendToFile CreateFile read failed");
		return;
	}
	
	int64 fromSize = GetFileSize64(fromFH);

	if ( fromSize <= 0 )
	{
		CloseHandle(fromFH);
		return;
	}	
	
	HANDLE ontoFH = CreateFile(onto,GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	if ( ontoFH  == INVALID_HANDLE_VALUE )
	{
		CloseHandle(fromFH);

		// just rename it :		
		MyMoveFile(from,onto);
		return;
	}
	
	int64 dwSet = SetFilePointer64(ontoFH,0,FILE_END);
	ASSERT_RELEASE( dwSet != INVALID_SET_FILE_POINTER_64 );
	
	lprintf("  appending %s onto %s , %d bytes\n",from,onto,fromSize);

	cb::vector<char> buffer;
	buffer.resize( check_value_cast<vecsize_t>(fromSize) );
	
	int64 dwRead = ReadFile64(fromFH,buffer.data(),fromSize);
	ASSERT_RELEASE( dwRead == fromSize );
	
	CloseHandle(fromFH);
	
	int64 dwWrote = WriteFile64(ontoFH,buffer.data(),fromSize);
	ASSERT_RELEASE( dwWrote == fromSize );
	
	CloseHandle(ontoFH);
	
	if ( andDelete )
	{
		if ( ! MyDeleteFile(from) )
		{
			LogLastError("AppendToFile DeleteFile failed");
		}
	}
}

//-----------------------------------------------------------

int64 GetFileLength(FILE *fp)
{
	int64 cur = ftell64(fp);
	fseek64(fp,0,SEEK_END);
	int64 length = ftell64(fp);
	fseek64(fp,cur,SEEK_SET);
	return length;
}

int64 ftell64(FILE * fp)
{
	fpos_t pos;
	fgetpos(fp,&pos);
	int64 p2 = _ftelli64(fp);
	ASSERT_RELEASE( pos == p2 );
	return (int64) pos;
}

int64 FRead (FILE *fp, void * buf, int64 count)
{
	size_t ret = fread(buf,1,check_value_cast<size_t>(count),fp);
	return (int64) ret;
}

int64 FWrite(FILE *fp, const void * buf, int64 count)
{
	size_t ret = fwrite(buf,1,check_value_cast<size_t>(count),fp);
	return (int64) ret;
}

//===================================================


int64 FileSameLen(const char * f1,const char * f2, int64 * pf1Len, int64 * pf2Len)
{
	int64 sameLen = 0;
	
	char * buf1 = ReadWholeFile(f1,pf1Len);
	char * buf2 = ReadWholeFile(f2,pf2Len);
	
	if ( buf1 && buf2 )
	{
		int64 lessLen = MIN( *pf1Len, *pf2Len );
		while ( buf1[sameLen] == buf2[sameLen] && sameLen < lessLen )
		{
			sameLen++;
		}
	}
	
	CBFREE(buf1);
	CBFREE(buf2);
	
	return sameLen;
}

bool FilesSame(const char * f1,const char * f2)
{
	int64 l1,l2,ls;
	ls = FileSameLen(f1,f2,&l1,&l2);
	return ( ls == MAX(l1,l2) );
}

// returns if they were different
bool MoveTempIfChanged(const char * from,const char * to)
{
	if ( FilesSame(from,to) )
		return false;
		
	if ( ! MyMoveFile(from,to) )
	{
		lprintf("MyMoveFile failed : %s -> %s\n",from,to);
	}
	
	MyDeleteFile(from);
	
	return true;
}	
	
END_CB

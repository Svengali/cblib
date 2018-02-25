#include "cblib/FileUtil.h"
#include "cblib/Win32Util.h"
#include "cblib/Log.h"
#include <sys/stat.h>


START_CB

FILE * fmemopen(void * buffer,int size)
{
	FILE * ret = tmpfile();
	
	fwrite(buffer,1,size,ret);
	fflush(ret);
	rewind(ret);
	
	return ret;
}

String GetTempDir()
{
	// GetTempPath gets %TMP%

	char buffer[_MAX_PATH];
	GetTempPathA(sizeof(buffer),buffer);
	String ret(buffer);
	if ( ret[ret.Length()-1] != '\\' )
		ret += '\\';
	return ret;
}

bool MyMoveFile(const char *fm,const char *to)
{
	if ( ! MoveFileExA(fm,to,MOVEFILE_REPLACE_EXISTING) )
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

size_t GetFileLength(const char *name)
{
	struct stat st;

	if ( stat(name,&st) == 0 )
		return st.st_size;

	return 0;
}

bool FileExists(const char *Name)
{
	struct stat st;

	if ( stat(Name,&st) == 0 )
		return true;

	return false;
}

time_t FileModTime(const char *Name)
{
	struct stat st;
	
	if ( stat(Name,&st) == 0 )
		return st.st_mtime;

	return 0;
}

char * ReadWholeFile(FILE * fp,int * pLength)
{
	fseek(fp,0,SEEK_END);
	
	long length = ftell(fp);

	if ( pLength )
		*pLength = length;	
		
	//fseek(fp,0,SEEK_SET);
	rewind(fp);

	char * data = (char *) CBALLOC(length+1);

	if ( data )
	{
		fread(data,length,1,fp);
		data[length] = 0;
	}
		
	return data;
}

char * ReadWholeFile(const char *name,int * pLength)
{
	FILE * fp = fopen(name,"rb");
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

void ReserveFileSpace(const char * name,int toReserve)
{
	HANDLE fh = CreateFileA(name,GENERIC_WRITE,0,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
	if ( fh  == INVALID_HANDLE_VALUE )
	{
		return;
	}
	
	DWORD oldSize = GetFileSize(fh,NULL);
	
	DWORD newSize = oldSize + toReserve;
	
	DWORD dwSet = SetFilePointer(fh,newSize,NULL,FILE_BEGIN);
	ASSERT_RELEASE( dwSet != INVALID_SET_FILE_POINTER );
	
	SetEndOfFile(fh);
	
	CloseHandle(fh);
	
	fh = CreateFileA(name,GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	
	dwSet = SetFilePointer(fh,oldSize,NULL,FILE_BEGIN);
	ASSERT_RELEASE( dwSet != INVALID_SET_FILE_POINTER );
	
	SetEndOfFile(fh);
	
	CloseHandle(fh);
}

void AppendToFile(const char * onto,const char * from,bool andDelete)
{
	HANDLE fromFH = CreateFileA(from,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	if ( fromFH == INVALID_HANDLE_VALUE )
	{
		if ( GetLastError() == ERROR_FILE_NOT_FOUND )
			return; // that's fine
			 
		LogLastError("AppendToFile CreateFile read failed");
		return;
	}
	
	DWORD fromSize = GetFileSize(fromFH,NULL);

	if ( fromSize <= 0 )
	{
		CloseHandle(fromFH);
		return;
	}	
	
	HANDLE ontoFH = CreateFileA(onto,GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	if ( ontoFH  == INVALID_HANDLE_VALUE )
	{
		CloseHandle(fromFH);

		// just rename it :		
		MyMoveFile(from,onto);
		return;
	}
	
	DWORD dwSet = SetFilePointer(ontoFH,0,NULL,FILE_END);
	ASSERT_RELEASE( dwSet != INVALID_SET_FILE_POINTER );
	
	lprintf("  appending %s onto %s , %d bytes\n",from,onto,fromSize);

	cb::vector<char> buffer;
	buffer.resize(fromSize);
	
	DWORD dwRead;
	ReadFile(fromFH,buffer.data(),fromSize,&dwRead,NULL);
	ASSERT_RELEASE( dwRead == fromSize );
	
	CloseHandle(fromFH);
	
	DWORD dwWrote;
	WriteFile(ontoFH,buffer.data(),fromSize,&dwWrote,NULL);
	ASSERT_RELEASE( dwWrote == fromSize );
	
	CloseHandle(ontoFH);
	
	if ( andDelete )
	{
		if ( ! DeleteFileA(from) )
		{
			LogLastError("AppendToFile DeleteFile failed");
		}
	}
}

END_CB

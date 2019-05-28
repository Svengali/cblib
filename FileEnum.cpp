#include "FileEnum.h"
#include "FileUtil.h"
#include "Log.h"
#include "Win32Util.h"

START_CB

bool EnumFilesIfDir(const char * fileOrDir,bool recurse, 
					cb::vector<cb::String> * pFiles,
					cb::vector<cb::String> * pDirs)
{
	if ( NameIsDir(fileOrDir) )
	{
		return EnumFiles(fileOrDir,recurse,pFiles,pDirs);
	}
	else if ( FileExists(fileOrDir) )
	{
		pFiles->push_back(cb::String(fileOrDir));
		return true;
	}
	else
	{
		return false;
	}
}

bool EnumFiles(const char * baseDir,bool recurse, 
				cb::vector<cb::String> * pFiles,
				cb::vector<cb::String> * pDirs)
{
	if ( baseDir[0] == 0 )
		return false;

	cb::vector<cb::String> dirs;
	dirs.push_back( cb::String(baseDir) );
	
	while( ! dirs.empty() )
	{
		cb::String curDir = dirs.back();
		dirs.pop_back();
		
		if ( curDir[curDir.Length()-1] != '\\' )
			curDir += '\\';

		WIN32_FIND_DATA data;
		
		cb::String findSpec = curDir;
		findSpec += '*';
		HANDLE handle = FindFirstFile(findSpec.CStr(),&data);
		if ( handle == INVALID_HANDLE_VALUE )
		{
			continue; //return false;
		}

		do	
		{
			// process data
		
			// @@ should be optional :
			if ( data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
			{
				// do not chase symlinks
				continue;
			}
		
			// @@ should be optional :
			if ( data.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY )
			{
				// no temporaries
				continue;
			}
			
			if ( strsame(data.cFileName,".") || 
				strsame(data.cFileName,"..") )
			{
				continue;
			} 
		
			cb::String fullName(curDir);
			fullName += data.cFileName;
		
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if ( pDirs )
					pDirs->push_back(fullName);
			
				if ( recurse )
				{
					dirs.push_back(fullName);
				}
			}
			else
			{
				if ( pFiles )
					pFiles->push_back(fullName);
			}
		
		}
		while ( FindNextFile(handle,&data) );
		
		FindClose(handle);
	}
	
	return true;
}

void MakeDirEnum(const char * dir,DirEnum * dirEnum)
{
	cb::vector<cb::String> subdirs;

	dirEnum->name = dir;
	
	dirEnum->files.reserve(1024);

	{
		cb::String curDir(dir);
		
		if ( curDir[curDir.Length()-1] != '\\' )
			curDir += '\\';
			
		WIN32_FIND_DATA data;
		
		cb::String findSpec = curDir;
		findSpec += '*';
		HANDLE handle = FindFirstFile(findSpec.CStr(),&data);
		if ( handle == INVALID_HANDLE_VALUE )
		{
			return;
		}

		do	
		{
			// process data
		
			// @@ should be optional :
			if ( data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
			{
				// do not chase symlinks
				continue;
			}
		
			// @@ should be optional :
			if ( data.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY )
			{
				// no temporaries
				continue;
			}
			
			if ( strsame(data.cFileName,".") || 
				strsame(data.cFileName,"..") )
			{
				continue;
			} 
		
			cb::String fullName(curDir);
			fullName += data.cFileName;
		
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				subdirs.push_back(fullName);
			}
			else
			{
				dirEnum->files.push_back(fullName);
				
				LARGE_INTEGER fileSize;
				fileSize.HighPart = data.nFileSizeHigh;
				fileSize.LowPart = data.nFileSizeLow;
				
				dirEnum->fileSizeCur += fileSize.QuadPart;
			}
		
		}
		while ( FindNextFile(handle,&data) );
		
		FindClose(handle);
	}

	dirEnum->files.tighten();

	dirEnum->dirs.resize(subdirs.size());

	dirEnum->fileSizeRecursed = dirEnum->fileSizeCur;
	dirEnum->numFilesRecursed = dirEnum->files.size();
	dirEnum->numDirsRecursed = dirEnum->dirs.size();

	for(int i=0;i<subdirs.size32();i++)
	{
		MakeDirEnum( subdirs[i].CStr(), &dirEnum->dirs[i] );
		
		dirEnum->fileSizeRecursed += dirEnum->dirs[i].fileSizeRecursed;
		dirEnum->numFilesRecursed += dirEnum->dirs[i].numFilesRecursed;
		dirEnum->numDirsRecursed += dirEnum->dirs[i].numDirsRecursed;
	}
	
	
	
	return;
}

String SelectOneIfDir( const char * fmName )
{
	// take dir arg and pick random in dir :
	if ( NameIsDir(fmName) )
	{
		vector<String> files;
		EnumFiles(fmName,false,&files);
		lprintf_v2("SelectOneIfDir : %s has %d files\n",fmName,files.size());
		if ( files.empty() )
			return String();
		int i = GetAndAdvanceRegistryCounter();
		i = i % files.size();
		lprintf_v2(" selected %d : %s\n",i,files[ i ]);
		return files[ i ];
	}
	else
	{
		return String(fmName);
	}
}

END_CB
#pragma once

#include "Base.h"
#include "String.h"
#include "vector.h"

START_CB


/**

EnumFiles ; recursive or not from start dir

pFiles or pDirs can be NULL

vectors get the full path names

**/

bool EnumFiles(const char * dir,bool recurse, 
					cb::vector<cb::String> * pFiles,
					cb::vector<cb::String> * pDirs = NULL);

// EnumFilesIfDir : if arg is a file, just add it
//	 if it's a dir, enum

bool EnumFilesIfDir(const char * fileOrDir,bool recurse, 
					cb::vector<cb::String> * pFiles,
					cb::vector<cb::String> * pDirs = NULL);


struct DirEnum
{
	String	name;
	vector<DirEnum>	dirs;
	vector<String>	files;
	int64	fileSizeCur;
	int64	fileSizeRecursed;
	int64	numFilesRecursed;
	int64	numDirsRecursed;
	
	DirEnum() : fileSizeCur(0), fileSizeRecursed(0), numFilesRecursed(0)
	{ }
};

void MakeDirEnum(const char * dir,DirEnum * dirEnum);

// SelectOneIfDir : nice way to munge an input file name arg for test apps
//	if fmName is a file, it's returned
//	if fmName is a dir, some file within the dir is returned
//	  will step through files in dir each successive call
String SelectOneIfDir( const char * fmName );

END_CB

#pragma once

#include "String.h"
#include "File.h"

/********************
FileUtil :

"File" helpers plus other junk

************************/

#include <crtdefs.h>
//typedef long time_t;
struct _stat64;

typedef wchar_t wchar;

#include <cwchar>


START_CB

//class File;


	//-----------------------------------------------------------------
	// !! copy of char funcs with String

	extern String GetTempName();
	extern String GetTempDir(); // has a backslash on it
	extern String GetCurDir(); // has a backslash on it

	String filepart(const String & path);
	String pathpart(const String & path);
	String getpathpart(const char * path);
	char LastChar(const String & str);
	// CatPaths just sticks them together without thinking
	// CombinePaths does proper handling of .. and :\ and so on
	String CatPaths(const String & p1,const String & p2);
	String CombinePaths(const String & p1,const String & p2);
	String PrefixCurDir(const String & AddTo);

	// PrefixCurDir is "MakeAbsolutePath"

	// removes trailing / if any :
	void CutEndPath(String * pStr);
	// removes the dot as well :
	void RemoveExtension(String * pStr);
	// newExt can be with . or not, I add it if not
	void ChangeExtension(String * pStr,const char * newExt);

	//-----------------------------------------------------------------
	
	// extensionpart points at the first char after the . ; eg "txt"
	//	if none it points at the terminating null
	const char * extensionpart(const char * path);
	const char * filepart(const char * path);
	char * extensionpart(char * path);
	char * filepart(char * path);
	void getpathpart(const char * path,char * into); // into DOES include the ending slash

	char LastChar(const char * str);
	wchar LastChar(const wchar * str);
	bool IsPathDelim(char c);
	void CutEndPath(char *Path);
	void StandardizePathDelims(char * ptr);
	
	// CatPaths is a simple strcat
	void CatPaths(char *onto,const char *add);

	// CombinePaths does .. and such
	void CombinePaths(const char *base,const char *add, char * into);
	void PrefixCurDir(char *AddTo); // stick cwd in front of AddTo ; works with ".." in AddTo

	void MakeRelativePath(const char * absPath,const char * fromDir, char * into);

	void MakeTempFileName(char * into,int intoSize);

	// RemoveExtension removes the dot also
	void RemoveExtension(char * str);

	//-----------------------------------------------------------
	// !! copy of char funcs with wchar
	
	// extensionpart points at the first wchar after the . ; eg "txt"
	//	if none it points at the terminating null
	const wchar * extensionpart(const wchar * path);
	const wchar * filepart(const wchar * path);
	wchar * extensionpart(wchar * path);
	wchar * filepart(wchar * path);
	void getpathpart(const wchar * path,wchar * into);

	inline bool matchextension(const char * str,const char * ext)
	{
		return strisame( extensionpart(str), ext );
	}

	bool IsPathDelim(wchar c);
	void CutEndPath(wchar *Path);
	
	// CatPaths is a simple strcat
	void CatPaths(wchar *onto,const wchar *add);

	// CombinePaths does .. and such
	void CombinePaths(const wchar *base,const wchar *add, wchar * into);
	void PrefixCurDir(wchar *AddTo); // stick cwd in front of AddTo ; works with ".." in AddTo

	//-----------------------------------------------------------
	
	void StrConv_AnsiToUnicode(wchar * to,const char * from,int maxlen);
	void StrConv_ConsoleToUnicode(wchar * to,const char * from,int maxlen);
	void StrConv_UnicodeToAnsi(char * to,const wchar * from,int maxlen);
	void StrConv_UnicodeToConsole(char * to,const wchar * from,int maxlen);

	// you cannot printf unicode, instead do this :
	//printf("%-s\n",UnicodeToConsole(FullPath).CStr());
	
	String UnicodeToAnsi(const wchar * from);
	String UnicodeToConsole(const wchar * from);

	// GetUnicodeFileNameFromAnsi
	//	from name must be a full path spec
	//	does a search for the uni name that matches the ansi
	// only works if the dir names are ansi !! does not support uni dir names !!
	enum EMatchType
	{
		eMatch_Ansi,
		eMatch_Console,
		eMatch_Either,
	};
	//old :
	bool GetUnicodeFileNameFromMatch_Deprecated(wchar * to,const char * fromStr,int maxlen,EMatchType matchType = eMatch_Either);
	//new :
	bool GetUnicodeFileNameFromMatch(wchar * to,const wchar * findPath,const char * findName,int maxlen,EMatchType matchType = eMatch_Either);

	// get rid of me :
	//void GetUnicodeMatchOrCopy(wchar * to,const char * fromStr,int maxlen,EMatchType matchType = eMatch_Either);

	bool FileExistsMatch(const char * fromStr); // uses GetUnicodeFileNameFromMatch

	bool MakeUnicodeNameFullMatch(wchar * to,const char * fromStr,int maxlen,EMatchType matchType = eMatch_Either);

	//-----------------------------------------------------------
	
	// Stat utils :
	bool IsDir(const struct _stat64 & st);
	bool StatMatch(const char * fromStr,struct _stat64 * st);
	// Stat returns true on success
	bool Stat(const char * name,struct _stat64 * st);
	bool Stat(const wchar * name,struct _stat64 * st);
	
	bool copystat(const wchar * tofile,const wchar *fmfile);
	bool setstat(const wchar * file,const struct _stat64 *s);

	extern bool MyMoveFile(const char *fm,const char *to);
	extern bool MyMoveFile(const wchar *fm,const wchar *to);
	
	#define CB_FILE_LENGTH_INVALID	((int64)(-1))
	// @@ UGLY - be careful - GetFileLength does NOT return 0 for non-existing files
	//	it returns (-1) unsigned.  (this is because files can exist and be 0 bytes long)
	extern int64 GetFileLength(const char *name);
	extern int64 GetFileLength(const wchar *name);
	
	extern bool FileExists(const char *Name);
	extern bool FileExists(const wchar *Name);
	
	// returns 0 if doesn't exist
	extern time_t FileModTime(const char *Name);
	extern time_t FileModTime(const wchar *Name);
	
	extern bool NameIsDir(const char *Name);
	extern bool NameIsDir(const wchar *Name);

	//-----------------------------------------------------------
	
	// free results of ReadWholeFile with CBFREE()
	// IMPORTANT : ReadWholeFile allocates one more and adds a null, so when you read
	//	pure text files, you can treat the return value as a string !
	extern char * ReadWholeFile(FILE * fp,int64 * pLength = NULL);
	extern char * ReadWholeFile(const char *name,int64 * pLength = NULL);
	extern char * ReadWholeFile(const wchar *name,int64 * pLength = NULL);
	extern char * ReadWholeFile(File & file);

	bool WriteWholeFile(FILE * fp,const void * buffer,int64 length);
	bool WriteWholeFile(const char *name,const void * buffer,int64 length);
	bool WriteWholeFile(const wchar *name,const void * buffer,int64 length);
	bool WriteWholeFile(File & file,const void * buffer,int64 length);

	extern void AppendToFile(const char * onto,const char * from,bool andDelete);
	extern void ReserveFileSpace(const char * name,int toReserve);

	//-----------------------------------------------------------
	
	int64 FRead (FILE *fp, void * buf, int64 count);
	int64 FWrite(FILE *fp, const void * buf, int64 count);
	extern int64 GetFileLength(FILE *fp);
		
	// for use with Windows HFILE :
	#define INVALID_SET_FILE_POINTER_64	((int64)-1)
	int64 GetFileSize64(void * handle);
	int64 SetFilePointer64(void * hFile,int64 distance, int moveType);
	int64 ReadFile64(void * hFile,void * buffer, int64 toRead);
	int64 WriteFile64(void * hFile,void * buffer, int64 toRead);
	
	//-----------------------------------------------------------
	
	bool MkDir(const char * name);
	bool RenameFile( const char * fm, const char * to, bool force );
	bool MyDeleteFile( const char * name );
	
	//-----------------------------------------------------------
	// ReadBytes/WriteBytes
	//	raw reads/writes for classes
	//	probably generally should *not* be used

	template <class T> void ReadBytes(File & gf, T & entry )
	{
		gf->Read( &entry, sizeof(T) );
	}

	template <class T> void WriteBytes(File & gf, const T & entry )
	{
		gf->Write( &entry, sizeof(T) );
	}

	template <class T> void ReadBytesArray(File & gf, T * pArray, const int numEntries )
	{
		ASSERT( pArray );
		gf->Read( pArray, sizeof(T)*numEntries );
	}

	template <class T> void WriteBytesArray(File & gf, const T * pArray, const int numEntries )
	{
		ASSERT( pArray );
		gf->Write( pArray, sizeof(T)*numEntries );
	}

	// read/write a gvector (or look-alike)
	//	write the elements as raw bytes

	template <class Vector> void ReadBytesVector( File & gf, Vector & vec)
	{
		//ASSERT(gf != NULL);
		const int numEntries = gf.Get32();
		vec.resize(numEntries);
		if ( numEntries > 0 )
		{
			gf.Read(  vec.data(), sizeof(Vector::value_type)*numEntries );
		}
	}

	template <class Vector> void WriteBytesVector(File & gf, const Vector & vec)
	{
		//ASSERT(gf != NULL);
		const int numEntries = vec.size();
		gf.Put32(numEntries);
		if ( numEntries > 0 )
		{
			gf.Write( vec.data(), sizeof(Vector::value_type)*numEntries );
		}
	}

	// read/write a gvector (or look-alike)
	//	write the elements via Read/Write member functions on the elements

	template <class Vector> void ReadElementsVector( File & gf, Vector & vec)
	{
		//ASSERT(gf);
		const int numEntries = gf.Get32();
		vec.resize(numEntries);
		for(int i=0;i<numEntries;i++)
		{
			vec[i].Read(gf);
		}
	}

	template <class Vector> void WriteElementsVector(File & gf, const Vector & vec)
	{
		//ASSERT(gf);
		const int numEntries = vec.size();
		gf.Put32(numEntries);
		for(int i=0;i<numEntries;i++)
		{
			vec[i].Write(gf);
		}
	}

	//-----------------------------------------------------------

	template <class T> void IOBytes(File & gf, T & entry )
	{
		gf.IO( &entry, sizeof(T) );
	}
	template <class T> void IORW(File & gf, T & entry )
	{
		if ( gf.IsReading() )
		{
			entry.Read(gf);
		}
		else
		{
			entry.Write(gf);
		}
	}

	//-----------------------------------------------------------


	inline void myfwrite4(const void * ptr,FILE * fp)
	{
		const char * p = (const char *)ptr;
		macroputc(p[0],fp);
		macroputc(p[1],fp);
		macroputc(p[2],fp);
		macroputc(p[3],fp);
	}
	
	inline void fputcs(FILE * fp,const char * ptr)
	{
		while ( *ptr ) { macroputc(*ptr,fp); ptr++; }
	}

	//-----------------------------------------------------------

	// open a memory buffer as a FILE
	// the type is always "w+b"
	//	which means read/write binary
	FILE * fmemopen(void * buffer,int size);

	//-------------------------------------------------

	int64 FileSameLen(const char * f1,const char * f2, int64 * pf1Len, int64 * pf2Len);
	bool FilesSame(const char * f1,const char * f2);
	// moves "from" onto "to" - only if not the same
	//	if they are the same, deletes "from"
	bool MoveTempIfChanged(const char * from,const char * to);

END_CB

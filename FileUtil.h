#pragma once
#ifndef GALAXY_FILEUTIL_H
#define GALAXY_FILEUTIL_H

#include "cblib/File.h"

/********************
gFileUtil :

non-member helpers on gFile

************************/

START_CB

	extern bool MyMoveFile(const char *fm,const char *to);

	extern String GetTempDir(); // has a backslash on it

	// free results of ReadWholeFile with CBFREE()
	// IMPORTANT : ReadWholeFile allocates one more and adds a null, so when you read
	//	pure text files, you can treat the return value as a string !
	extern char * ReadWholeFile(FILE * fp,int * pLength = NULL);
	extern char * ReadWholeFile(const char *name,int * pLength = NULL);
	extern char * ReadWholeFile(File & file);

	extern size_t GetFileLength(const char *name);
	extern bool FileExists(const char *Name);
	extern time_t FileModTime(const char *Name);
	//extern bool NameIsDir(char *Name);

	extern void AppendToFile(const char * onto,const char * from,bool andDelete);
	extern void ReserveFileSpace(const char * name,int toReserve);

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
		ASSERT(gf != NULL);
		const int numEntries = gf->ReadL();
		vec.resize(numEntries);
		if ( numEntries > 0 )
		{
			gf->Read(  vec.data(), sizeof(Vector::value_type)*numEntries );
		}
	}

	template <class Vector> void WriteBytesVector(File & gf, const Vector & vec)
	{
		ASSERT(gf != NULL);
		const int numEntries = vec.size();
		gf->WriteL(numEntries);
		if ( numEntries > 0 )
		{
			gf->Write( vec.data(), sizeof(Vector::value_type)*numEntries );
		}
	}

	// read/write a gvector (or look-alike)
	//	write the elements via Read/Write member functions on the elements

	template <class Vector> void ReadElementsVector( File & gf, Vector & vec)
	{
		ASSERT(gf);
		const int numEntries = gf->ReadL();
		vec.resize(numEntries);
		for(int i=0;i<numEntries;i++)
		{
			vec[i].Read(gf);
		}
	}

	template <class Vector> void WriteElementsVector(File & gf, const Vector & vec)
	{
		ASSERT(gf);
		const int numEntries = vec.size();
		gf->WriteL(numEntries);
		for(int i=0;i<numEntries;i++)
		{
			vec[i].Write(gf);
		}
	}

	//-----------------------------------------------------------

	template <class T> void IOBytes(File & gf, T & entry )
	{
		gf->IO( &entry, sizeof(T) );
	}
	template <class T> void IORW(File & gf, T & entry )
	{
		if ( gf->IsReading() )
		{
			entry.Read(gf);
		}
		else
		{
			entry.Write(gf);
		}
	}

	//-----------------------------------------------------------

	// myfread is faster for small reads ?
	inline void myfread(void * ptr,size_t count,FILE * fp)
	{
		char * p = (char *)ptr;
		while(count--)
		{
			*p++ = getc(fp);
		}
	}

	inline void myfwrite(const void * ptr,size_t count,FILE * fp)
	{
		const char * p = (const char *)ptr;
		while(count--)
		{
			// putc is a macro, don't use *p++ in it
			putc(*p,fp);
			p++;
		}
	}

	//-----------------------------------------------------------

	// open a memory buffer as a FILE
	// the type is always "w+b"
	//	which means read/write binary
	FILE * fmemopen(void * buffer,int size);

END_CB

#endif // GALAXY_FILEUTIL_H

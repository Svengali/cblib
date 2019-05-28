#pragma once

#include "cblib/Base.h"
#include "cblib/SPtr.h"
#include "cblib/String.h"
#include "cblib/Timer.h"
#include "cblib/Util.h"
#include "cblib/FileUtil.h"
#include <stdio.h>

START_CB

/*
	File
	wrapper for stdio FILE
*/
	
class File
{
public:
	File() : m_fp(NULL), m_lastFlush(0), m_reading(false)
	{
	}

	explicit File(FILE * fp) : m_fp(fp), m_lastFlush(0), m_reading(false)
	{
		// @@ m_reading ?  is there no way to ask a file its mode !?
	}
	
	File(FILE * fp,bool reading) : m_fp(fp), m_lastFlush(0), m_reading(reading)
	{
	}
	
	File(const char * name,const char * access) : // throw(int) :
		m_fp(NULL), m_lastFlush(0), m_reading(false)
	{
		if ( ! Open(name,access) )
		{
			THROW;
		}
	}
	
	~File()
	{
		Close();
	}

	//----------------------------------------------------

	bool Open(const char * name,const char * access,bool interactive = true)
	{
		Close();
		m_fp = myfopen(name,access,interactive);
		m_name = name;
		m_reading = (strichr(access,'r') != NULL);
		m_lastFlush = Timer::GetSeconds();
		return (m_fp != NULL);
	}

	const String & GetNameString() const { return m_name; }
	const char * GetName() const { return m_name.CStr(); }

	void Close()
	{
		if ( m_fp != NULL )
		{
			Flush();
			fclose(m_fp);
			m_fp = NULL;
		}
	}

	void Write(const void * bits,const int count)
	{
		ASSERT( m_fp != NULL );
		//return fwrite(bits,1,count,m_fp);
		myfwrite(bits,count,m_fp);
	}

	// returns the number of bytes read
	void Read(void * bits,const int count)
	{
		ASSERT( m_fp != NULL );
		//return fread(bits,1,count,m_fp);
		myfread(bits,count,m_fp);
	}

	// IO reads or writes depending on File
	void IO(void * bits,const int count)
	{
		if ( m_reading )
			Read(bits,count);
		else
			Write(bits,count);
	}
	
	// DANGEROUS IO() : straight binary IO of arbitrary types; use IOZ for classes!
	template <typename T>
	void IO(T & t)
	{
		if ( m_reading )
			Read(&t,sizeof(t));
		else
			Write(&t,sizeof(t));
	}

	void Flush()
	{
		ASSERT( m_fp != NULL );
		fflush(m_fp);
		m_lastFlush = Timer::GetSeconds();
	}

	void PeriodicFlush(const double period = 5.0)
	{
		const double now = Timer::GetSeconds();
		if ( (now - m_lastFlush) > period )
		{
			Flush();
		}
	}

	int64 Tell() const
	{
		ASSERT( m_fp != NULL );
		return NS_CB::ftell64(m_fp);
	}

	int64 GetFileSize() const;

	void Seek(int64 offset, int origin = SEEK_SET)
	{
		ASSERT( m_fp != NULL );
		Flush();
		fseek64(m_fp,offset,origin);
	}

	FILE * Get() { return m_fp; }

	bool IsOpen() const { return m_fp != NULL; }
	bool IsReading() const { return m_reading; }

	bool IsEOF() const { return !!feof(m_fp); }

	//----------------------------------------------------
	
	void  Put8(const uint8 val ) { macroputc((int)val,m_fp); }
	uint8 Get8( ) { return (uint8) macrogetc(m_fp); }
		
	void Put16(const uint16 val)
	{
		Put8( (uint8)(val>>8) );
		Put8( (uint8)((val)&0xFF) );
	}

	uint16 Get16()
	{
		uint16 val;
		val = ((uint16)Get8())<<8;
		val |= (uint16)Get8();
		return val;
	}
	
	void Put32(const uint32 val)
	{
		Put8((uint8)( (val>>24)&0xFF ));
		Put8((uint8)( (val>>16)&0xFF ));
		Put8((uint8)( (val>>8)&0xFF ));
		Put8((uint8)( (val)&0xFF ));
	}

	uint32 Get32()
	{
		uint32 val;
		val  = ((uint32)Get8())<<24;
		val |= ((uint32)Get8())<<16;
		val |= ((uint32)Get8())<<8;
		val |= ((uint32)Get8());
		return val;
	}

	void Put64(const uint64 val)
	{
		uint32 top = (uint32)(val>>32);
		Put32(top);
		Put32((uint32)val);
	}

	uint64 Get64()
	{
		uint64 v;
		v  = ((uint64)Get32())<<32;
		v |= Get32();
		return v;
	}

	//----------------------------------------------------
	
	// String will have the \n or whatever in it
	String GetS()
	{
		const int c_maxStringLen = 4096;
		char buffer[c_maxStringLen];
		if ( ! fgets(buffer,c_maxStringLen,m_fp) )
			return String();
		buffer[c_maxStringLen-1] = 0;
		return String(buffer);
	}
	
	// String should have the \n or whatever in it
	void PutS(const String & str)
	{
		fputs(str.CStr(),m_fp);
	}
	
	void PutS(const char * str)
	{
		fputs(str,m_fp);
	}
	
	//----------------------------------------------------
	
	// ReadString returns the length of the string, NOT the # of bytes read
	int ReadCString(char * into,int maxSize)
	{
		int len = (int) Get32();
		if ( len >= maxSize )
		{
			FAIL("String too big in ReadStringBinary");
			return 0;
		}
		Read(into,len);		
		into[len] = 0;
		return len;
	}
	
	void WriteCString(const char * str)
	{
		int len = (int) strlen(str);
		Put32( (uint32) len );
		Write(str,len);
	}
	
	void WriteString(const String & str)
	{
		int len = str.Length();
		Put32( (uint32) len );
		Write(str.CStr(),len);
	}
	
	String ReadString()
	{
		String ret;
		int len = (int) Get32();
		char * ptr = ret.WriteableCStr(len+1);
		Read(ptr,len);		
		ptr[len] = 0;
		ASSERT( strlen32(ptr) == len );
		ret.Truncate(len);
		return ret;		
	}
	
	//----------------------------------------------------
private:
	FILE *	m_fp;
	String	m_name;
	double	m_lastFlush;
	bool	m_reading;

	FORBID_CLASS_STANDARDS(File);
};

SPtrFwd(FileRC); // FileRCPtr

class FileRC : public File, public RefCounted
{
public:
	static FileRCPtr Create(const char * name,const char * access);

	static FileRCPtr CreateMemFile(void * buffer,int size,bool reading);
	
	static FileRCPtr CreateForWriteWithP4(const char * name);
	
	static FileRCPtr Create()
	{
		return FileRCPtr( new FileRC() );
	}
protected:
	FileRC() { }
	FileRC(const char * name,const char * access) : File(name,access) { }
	FileRC(FILE * fp,bool reading) : File(fp,reading) { }
	~FileRC() { }
};

END_CB

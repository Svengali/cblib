#pragma once

#include "cblib/Base.h"
#include "cblib/SPtr.h"
#include "cblib/String.h"
#include "cblib/Timer.h"
#include "cblib/Util.h"
#include <stdio.h>

START_CB

/*
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

	bool Open(const char * name,const char * access)
	{
		Close();
		m_fp = fopen(name,access);
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

	size_t Write(const void * bits,const int count)
	{
		ASSERT( m_fp != NULL );
		return fwrite(bits,1,count,m_fp);
	}

	// returns the number of bytes read
	size_t Read(void * bits,const int count)
	{
		ASSERT( m_fp != NULL );
		return fread(bits,1,count,m_fp);
	}

	// IO reads or writes depending on File
	size_t IO(void * bits,const int count)
	{
		if ( m_reading )
			return Read(bits,count);
		else
			return Write(bits,count);
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

	long Tell() const
	{
		ASSERT( m_fp != NULL );
		return ftell(m_fp);
	}

	long GetFileSize() const;

	void Seek(long offset, int origin)
	{
		ASSERT( m_fp != NULL );
		Flush();
		fseek(m_fp,offset,origin);
	}

	FILE * Get() { return m_fp; }

	bool IsOpen() const { return m_fp != NULL; }
	bool IsReading() const { return m_reading; }

	bool IsEOF() const { return !!feof(m_fp); }

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
	
	static FileRCPtr Create()
	{
		return FileRCPtr( new FileRC() );
	}
protected:
	FileRC() { }
	FileRC(const char * name,const char * access) : File(name,access) { }
	~FileRC() { }
};

END_CB

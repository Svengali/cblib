#pragma once

#include "cblib/SPtr.h"
#include "cblib/Token.h"
#include "cblib/File.h"
#include "cblib/String.h"
//#include "cblib/StrUtil.h"
#include "cblib/vecsortedpair.h"
#include <stdio.h> // for sprintf

START_CB

/*

Pref structs just need to implement this :

void PrefIO(PrefBlock & block,MyStruct & me);

and call block.IO() from within there

-----------------------

override the template ReadFromText/WriteToText to add new basic type IO

-----------------------

WARNING : All prefs should fill themselves with default values before
	trying to Read() ; if the pref values, the members will not be touched

-----------------------

The new hotness way to define Prefs is just to use REFLECT.  Implement Reflection() in your struct and you're done.

For new "basic" types implement ReadFromText/WriteToText as below.

For structs that you don't own (so you can't just do Reflection) such as Windows types, you can implement a PrefIO in the "old" way, just
calling IO on the members.  See the examples for Vec2, etc. here.

-----------------------

There's not really versioning in Prefs ; if you change a variable name it just stops loading.
If you add a new variable it will just be the default value at first.
Of course you can do conversions this way by setting the new values from the old in the transition period.
To update to a new structure just load all prefs in & save them back out again.

-----------------------

Pref files do not know their type at all.  There is no class factory.  
It's entirely up to the code to ask for prefs of the right type.
(BTW this wouldn't be that hard to change/fix, you just have to write the class name at the top of the pref file)

-----------------------

Any compound type that implements PrefIO() will get it's own braced block.

If you want your type to be written on just one line you can implement ReadFromText & WriteToText

For example, if Token used a PrefIO to output its members it would look like :

m_myTokenVar: {
	m_hash: 234934
}

Instead I implement a ReadFromText so it can just be

m_myTokenVar: 234934

-----------------------

If you have no PrefIO defined for a type you will get an error message like this :

c:\src\cblib\Reflection.h(211) : error C2228: left of '.Reflection' must have class/struct/union type
        type is 'COLORREF'

The system assumes unknown types are classes that implement Reflection() and tries to call it.

You may then choose to implement one of the ways to IO that type :
	Reflection : best for your classes
	PrefIO() : best for composites that you don't own
	ReadFromText/WriteToText : best for basic types

In this case you just needed to include CommonPrefs.h

-----------------------

SEE ALSO "CommonPrefs.h"

*/

//-----------------------------------------------------------------

SPtrFwd(PrefFile);

template <typename T> void ReadFromText(T * pValue, const char * const text);
template <typename T> void WriteToText(const T & val, String * pInto);

//-----------------------------------------------------------------

#define PREFIO( block,parent,var)	block.IO(#var,&parent.var);
#define PREFIOV(block,parent,var)	block.IOV(#var,&parent.var);
#define PREFIOAsInt( block,parent,var)	block.IOAsInt(#var,&parent.var);

//-----------------------------------------------------------------
// client code should never talk to PrefBlock_Reader or _Writer ,
//	but they have to be in the header because of the templates

class PrefBlock_Writer
{
public:
	PrefBlock_Writer();
	~PrefBlock_Writer();

	void Reset() { m_data.Clear(); }
	const String & GetBuffer() const { return m_data; }

	template <typename T> void IO( const char * const name, T * pValue )
	{
		m_data += name;
		m_data += ':';
		WriteToText(*pValue,&m_data);
		m_data += '\n';
	}

	template <typename T> void IOV( const char * const name, vector<T> * pVector )
	{
		m_data += name;
		m_data += ':';
		int count = pVector->size();
		WriteToText(count,&m_data);
		for(int i=0;i<count;i++)
		{
			m_data += ',';
			WriteToText(pVector->at(i),&m_data);			
		}
		m_data += '\n';
	}

private:
	String	m_data;
};

//-----------------------------------------------------------------

class PrefBlock_Reader
{
public:
	PrefBlock_Reader();
	~PrefBlock_Reader();

	void Reset() { m_data.clear(); }
	void Read(const char * text);

	typedef vecsortedpair< vector< std::pair<Token,String> > >	t_vec;

	template <typename T> void IO( const char * const name, T * pValue ) const
	{
		const Token token(name);
		ASSERT( pValue != NULL );
		t_vec::const_iterator it = m_data.find(token);
		if ( it == m_data.end() )
		{
			// failed to find it !
		}
		else
		{
			// found it
			const String & payload = it->second;
			ReadFromText(pValue,payload.CStr());
		}
	}

	template <typename T> void IOV( const char * const name, vector<T> * pVector ) const
	{
		const Token token(name);
		ASSERT( pVector != NULL );
		pVector->clear();
		t_vec::const_iterator it = m_data.find(token);
		if ( it == m_data.end() )
		{
			// failed to find it !
			return;
		}

		// found it
		// make a copy
		String workspace = it->second;
		char * ptr = (char *)workspace.CStr();
		char * next = TokToComma(ptr);
		int count = 0;
		ReadFromText(&count,ptr);
		pVector->resize(count);

		for(int i=0;i<count;i++)
		{
			ptr = next;
			next = TokToComma(ptr);
			ReadFromText(&(pVector->at(i)),ptr);
		}

		//workspace.Kill();
	}

private:

	t_vec m_data;
};

//-----------------------------------------------------------------

class PrefBlock
{
public:
	
	enum ERead { eRead };
	enum EWrite { eWrite };

	PrefBlock(ERead,FileRCPtr & file);
	PrefBlock(ERead,const char * const fileName);
	PrefBlock(EWrite,FileRCPtr & file);
	PrefBlock(EWrite,const char * const fileName);

	PrefBlock(PrefBlock_Reader * pReader);
	PrefBlock(PrefBlock_Writer * pWriter);

	~PrefBlock();

	void FinishWriting();
	bool IsWriting() const { return m_pWriter != NULL; }

	template <typename T> void IO( const char * const name, T * pValue ) const
	{
		if ( m_pReader )	m_pReader->IO(name,pValue);
		else				m_pWriter->IO(name,pValue);
	}
	
	// IOAsInt is for enums and shorts and such that can be converted to int
	template <typename T> void IOAsInt( const char * const name, T * pValue ) const
	{
		int x = (int) *pValue;
		if ( m_pReader )	m_pReader->IO(name,&x);
		else				m_pWriter->IO(name,&x);
		*pValue = (T) x;
	}

	template <typename T> void IOV( const char * const name, vector<T> * pVector ) const
	{
		if ( m_pReader )	m_pReader->IOV(name,pVector);
		else				m_pWriter->IOV(name,pVector);
	}

private:
	bool				m_owns;
	FileRCPtr			m_writeFile;
	PrefBlock_Reader * m_pReader;
	PrefBlock_Writer * m_pWriter;

	FORBID_CLASS_STANDARDS(PrefBlock);
};


//===============================================================================
// ReadFromText / WriteToText implementations
//	-> defaults to calling PrefIO()
//	void PrefIO(PrefBlock & block,MyStruct & me);

template <class T>
void ReadFromText(T * pValue, const char * const text)
{
	PrefBlock_Reader reader;
	reader.Read(text);
	PrefBlock block(&reader);
	PrefIO( block, *pValue );
}

template <class T>
void WriteToText(const T & val, String * pInto)
{
	PrefBlock_Writer writer;
	PrefBlock block(&writer);
	PrefIO( block, const_cast<T &>(val) );
	pInto->Append("{\n");
	pInto->Append(writer.GetBuffer());
	pInto->Append("}");
}

//===============================================================================
// ReadFromText / WriteToText implementations

template <>
inline void ReadFromText<char>(char * pValue, const char * const text)
{
	*pValue = *text;
}

template <>
inline void WriteToText<char>(const char & val, String * pInto)
{
	pInto->Append(val);
}

#define INT_TEXT_IO(intlike_type)		\
template <> inline void ReadFromText<intlike_type>(intlike_type * pValue, const char * const text) \
{	\
	*pValue = (intlike_type) atoi(text);	\
}	\
	\
template <> inline void WriteToText<intlike_type>(const intlike_type & val, String * pInto)	\
{	\
	pInto->CatPrintf("%d",val);	\
}

INT_TEXT_IO(int);
INT_TEXT_IO(short);
INT_TEXT_IO(long);

#define UINT_TEXT_IO(intlike_type)		\
template <> inline void ReadFromText<intlike_type>(intlike_type * pValue, const char * const text) \
{	\
	*pValue = (intlike_type) strtoul(text,NULL,10);	\
}	\
	\
template <> inline void WriteToText<intlike_type>(const intlike_type & val, String * pInto)	\
{	\
	pInto->CatPrintf("%ul",val);	\
}

UINT_TEXT_IO(ulong);
UINT_TEXT_IO(uword);
UINT_TEXT_IO(ubyte);

extern bool Prefs_ReadBool(const char * const text);

template <>
inline void ReadFromText<bool>(bool * pValue, const char * const text)
{
	*pValue = Prefs_ReadBool(text);
}

template <>
inline void WriteToText<bool>(const bool & val, String * pInto)
{
	/*
	char buffer[60];
	sprintf(buffer,"%d",val ? 1 : 0);
	pboolo->Append(buffer);
	*/
	if ( val )
		pInto->Append("yes");
	else
		pInto->Append("no");
}

template <>
inline void ReadFromText<float>(float * pValue, const char * const text)
{
	*pValue = (float) atof(text);
}

template <>
inline void WriteToText<float>(const float & val, String * pInto)
{
	pInto->CatPrintf("%.6g",val);
}

template <>
inline void ReadFromText<double>(double * pValue, const char * const text)
{
	*pValue = atof(text);
}

template <>
inline void WriteToText<double>(const double & val, String * pInto)
{
	pInto->CatPrintf("%.8g",val);
}

extern void Prefs_ReadString(String * pValue, const char * const text);

template <>
inline void ReadFromText<String>(String * pValue, const char * const text)
{
	Prefs_ReadString(pValue,text);
}

template <>
inline void WriteToText<String>(const String & val, String * pInto)
{
	pInto->Append('\"');
	pInto->Append(val);
	pInto->Append('\"');
}

inline void ReadFromTextULHex(ulong * pValue, const char * const text)
{
	ulong ul = 0;
	sscanf(text,"%x",&ul);
	*pValue = ul;
}

inline void WriteToTextULHex(const ulong val, String * pInto)
{
	pInto->CatPrintf("%08X",val);
}

// Tokens in prefs is retarded
/*
template <>
inline void ReadFromText<Token>(Token * pValue, const char * const text)
{
	ulong ul;
	ReadFromTextULHex(&ul,text);
	pValue->SetHash(ul);
}

template <>
inline void WriteToText<Token>(const Token & val, String * pInto)
{
	WriteToTextULHex(val.GetHash(),pInto);
}
*/

//===============================================================================

END_CB

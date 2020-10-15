#include "PrefBlock.h"
//#include "StrUtil.h"
#include "FileUtil.h"
//#include "ResMgr.h"
#include "Log.h"
#include "MoreUtil.h"

START_CB

/*

A pref block looks like :

:Key:Data \n
:Key:Data \n
:Key:{ Data,
Data,
//Data,
Data
} \n

-----------------------------------

Leading & trailing whitespace in the "Data" is ignored

The Key is whitespace sensitive.

I generally write the data in a rigid format but try to be flexible reading it because it may be hand edited in a text file.

-----------------------------------

Writing is done like this :

each member just calls write to text sequentially
a compound member just writes its start & end brace
they all just tack junk into a String in memory

the Block_Writer does a post-process to tab it and format it nice.


Reading is done like this :

for each brace block we read all the key:data pairs into a map
that map is passed to the io routine for that block
the io routine looks up the keys it wants in the map
if found it parses the value out of the data text


*/


//=======================================================================

PrefBlock_Writer::PrefBlock_Writer()
{
}

PrefBlock_Writer::~PrefBlock_Writer()
{
}

PrefBlock_Reader::PrefBlock_Reader()
{
}

PrefBlock_Reader::~PrefBlock_Reader()
{
}

PrefBlock::PrefBlock(ERead,FileRCPtr & file) :
	m_pReader(NULL),
	m_pWriter(NULL),
	m_owns(true)
{
	char * buffer = (char *)ReadWholeFile(file.GetRef());
	m_pReader = new PrefBlock_Reader;
	m_pReader->Read(buffer);
	CBFREE( buffer );
}

PrefBlock::PrefBlock(ERead,const char * const resourceName) :
	m_pReader(NULL),
	m_pWriter(NULL),
	m_owns(true)
{
	FileRCPtr file = FileRC::Create(resourceName,"rb");
	
	if ( file == NULL )
	{
		lprintf("PrefBlock couldn't read file (%s)\n",resourceName);
	
		m_pReader = new PrefBlock_Reader;
	}
	else
	{
		char * buffer = (char *)ReadWholeFile(file.GetRef());
		m_pReader = new PrefBlock_Reader;
		m_pReader->Read(buffer);
		CBFREE( buffer );
	}
}

PrefBlock::PrefBlock(EWrite,FileRCPtr & file) :
	m_pReader(NULL),
	m_pWriter(NULL),
	m_owns(true)
{
	m_writeFile = file;
	m_pWriter = new PrefBlock_Writer;
}

PrefBlock::PrefBlock(EWrite,const char * const resourceName) :
	m_pReader(NULL),
	m_pWriter(NULL),
	m_owns(true)
{
	m_writeFile = FileRC::CreateForWriteWithP4(resourceName);

	if ( m_writeFile == NULL )
	{	
		lprintf("PrefBlock couldn't write file (%s)\n",resourceName);
	}

	m_pWriter = new PrefBlock_Writer;
}

PrefBlock::PrefBlock(PrefBlock_Reader * pReader) :
	m_pReader(pReader),
	m_pWriter(NULL),
	m_owns(false)
{
}
PrefBlock::PrefBlock(PrefBlock_Writer * pWriter) :
	m_pReader(NULL),
	m_pWriter(pWriter),
	m_owns(false)
{
}

// MakePrettyTabs is a post-process on prefs to
//	beautify
static String MakePrettyTabs(const String & fromS)
{
	String out;
	const char * from = fromS.CStr();
	int tabs = 0;
	for(;;)
	{
		char c = *from++;
		if ( c == 0 )
			break;
		if ( c == '{' )
		{
			tabs++;
			out += c;
		}
		else if ( c == '}' )
		{
			// remove last char if it's a tab
			char t = out.PopBack();
			if ( t != '\t' )
				out += t;
			tabs--;
			out += c;
		}
		else if ( c == '\n' )
		{
			out += c;
			// now output tabs
			for(int i=0;i<tabs;i++)
			{
				out += '\t';
			}
		}
		else
		{
			out += c;
		}
	}
	return out;
}

void PrefBlock::FinishWriting()
{
	if ( m_writeFile != NULL )
	{
		String pretty = MakePrettyTabs(m_pWriter->GetBuffer());
		pretty.WriteText(m_writeFile->Get());
		m_writeFile = NULL;
	}
}

PrefBlock::~PrefBlock()
{
	FinishWriting();
	if ( m_owns )
	{
		if ( m_pReader ) delete m_pReader;
		if ( m_pWriter ) delete m_pWriter;
	}
	m_pReader = NULL;
	m_pWriter = NULL;
}

//=======================================================================

void PrefBlock_Reader::Read(const char * text)
{
	// read text to fill m_data;
	m_data.clear();

	const char * ptr = text;
	ptr = skipwhitespace(ptr);
	if ( *ptr == '{' )
	{
		ptr++;
	}
	for(;;)
	{
		if ( *ptr == 0 ) break;
		if ( *ptr == '}' ) break;

		ptr = skipwhitespace(ptr);
		ptr = SkipCppComments(ptr);
		ptr = skipwhitespace(ptr);

		// starting a pref
		const char * next = strchr(ptr,':');
		if ( next == NULL )
		{
			lprintf("bad format in prefs : %s\n",ptr);
			break;
		}

		String key(String::eSubString, ptr, ptr_diff_32(next-ptr));

		ptr = next+1;
		ptr = skipwhitespace(ptr);

		if ( *ptr == '{' ) //}
		{
			next = FindMatchingBrace(ptr,'{','}');
			if ( next == NULL )
			{
				lprintf("bad format in prefs : %s\n",ptr);
				break;
			}
			ptr++;
		}
		else
		{
			next = strchr(ptr,'\n');
			if ( next == NULL )
			{
				next = strend(ptr);
			}
		}

		// data is the pref string data payload
		String data(String::eSubString, ptr, ptr_diff_32(next-ptr));

		if ( data.Length() > 0 && data[ data.Length() - 1 ] == '\r' )
			data.PopBack();
		while ( data.Length() > 0 && iswhitespace(data[ data.Length() - 1 ]) )
			data.PopBack();

		Token tok(key);

		m_data.insert(tok,data);

		ptr = next;
		ptr = skipwhitespace(ptr);
	}

	// all done, ready to use
}

//===========================================================================================

void Prefs_ReadString(String * pValue, const char * const text)
{
	const char * ptr = text;
		
	if ( *ptr == '\"' )
	{
		// its quoted
		ptr++;
		const char * pEnd = strrchrorend(ptr,'\"');
		pValue->Set(ptr);
		pValue->Truncate( ptr_diff_32(pEnd - ptr) );
	}
	else
	{
		// not quoted, old style
		pValue->Set(ptr);
	}
}

bool Prefs_ReadBool(const char * const text)
{
	if ( stripresame(text,"yes") ||
		stripresame(text,"true") ||
		stripresame(text,"on") )
	{
		return true;
	}
	else if ( stripresame(text,"no") ||
		stripresame(text,"false") ||
		stripresame(text,"off") )
	{
		return false;
	}
	else
	{
		int i = atoi(text);
		return (i != 0);
	}
}

END_CB

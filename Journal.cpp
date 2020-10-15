#include "Journal.h"
#include "File.h"
#include <conio.h> // for getch

#define WIN32_LEAN_AND_MEAN
#include <windows.h> // for MoveFile

START_CB

static Journal::EMode s_mode = Journal::eNone;
static const char * s_journalName = ".\\log\\journal";
static const char * s_journalPrevName = ".\\log\\journal_prev";
static File s_file;

void Journal::StartSaving()
{
	try
	{
		s_mode = eSaving;
		
		// move old journal to prev :
		DeleteFile(s_journalPrevName);
		MoveFile(s_journalName,s_journalPrevName);
	
		// clear it :
		s_file.Open(s_journalName,"wb");
		s_file.Close();
	}
	catch(...)
	{
		s_mode = eNone;
	}
}

void Journal::StartLoading()
{
	try
	{
		s_mode = eLoading;
		s_file.Open(s_journalName,"rb");
	}
	catch(...)
	{
		s_mode = eNone;
	}
}

Journal::EMode Journal::GetMode()
{
	return s_mode;
}

void Journal::Read(void * bits,int size)
{
	ASSERT( s_mode == eLoading );
	s_file.Read(bits,size);
	
		/*
	try
	{
		// s_file should be already open
		if ( s_file.Read(bits,size) != size )
		{
			// journal is over! stop reading
			fprintf(stderr,"==============================================================\n");
			fprintf(stderr,"Hit end of journal, no more reading!!\n");
			fprintf(stderr,"==============================================================\n");
			s_mode = eNone;

			memset(bits,0,size);
		}
	}
	catch(...)
	{
	}
		*/
}

void Journal::Write(const void * bits,int size)
{
	ASSERT( s_mode == eSaving );
	try
	{
		File file(s_journalName,"ab");
		file.Write(bits,size);
	}
	catch(...)
	{
	}
}

void Journal::Checkpoint()
{
	static const uint32 c_check = 0xC0CAC01A;
	uint32 check = c_check;
	IO(check);
	ASSERT( check == c_check );
}

char Journal::getch()
{
	if ( IsLoading() )
	{
		char c;
		IO(c);
		return c;
	}
	else
	{
		char c = (char) ::getch();
		IO(c);
		return c;
	}
}

END_CB

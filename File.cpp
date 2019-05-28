#include "cblib/File.h"
#include "cblib/FileUtil.h"
#include "cblib/Log.h"
#include <conio.h>
#include <ctype.h>

START_CB

FILE * myfopen(const char * fname,const char * access,bool interactive)
{
	char namebuf[256];
	
	if ( ! interactive )
	{
		FILE * fp = fopen(fname,access);
		
		if ( fp == NULL && access[0] == 'w' )
		{
			MakeTempFileName(namebuf,sizeof(namebuf));
			lprintf("open (%s) for write failed; changed name to (%s) \n",fname,namebuf);
			fname = namebuf;

			fp = fopen(fname,access);
		}
		
		return fp;
	}

	for(;;)
	{
		FILE * fp = fopen(fname,access);
		if ( fp )
			return fp;
		
		lprintf("fopen failed : (%s)(%s)\n",fname,access);
		lprintf("(r)etry, return (n)ull, e(x)it app, rena(m)e, rename (t)emp\n"); 

		int c = getch();
		while( iswhitespace((char)c) )
			c = getch();
		switch(tolower(c))
		{
		case 'r':
			break; // will loop
		case 'n':
			return NULL;
		case 'x':
			exit(10);
		case 't':
		{
			MakeTempFileName(namebuf,sizeof(namebuf));
			fname = namebuf;
			lprintf("changed name to : (%s)\n",fname);
			break;
		}
		case 'm':
		{
			lprintf("enter new name :\n");
			namebuf[0] = 0;
			fgets(namebuf,sizeof(namebuf),stdin);
			fname = namebuf;
			lprintf("changed name to : (%s)\n",fname);
			break;
		}
		default:
			lprintf(" invalid selection.\n");
			break;
		}
	}
}

int64 File::GetFileSize() const
{
	int64 cur = Tell();
	
	const_cast<File *>(this)->Seek(0,SEEK_END);
	int64 size = Tell();
	
	const_cast<File *>(this)->Seek(cur,SEEK_SET);
	
	return size;
}

FileRCPtr FileRC::Create(const char * name,const char * access)
{
	FileRCPtr ret;
	TRY
	{
		ret = new FileRC(name,access);
	}
	CATCH
	{
		ret = NULL;
	}
	return ret;
}

FileRCPtr FileRC::CreateMemFile(void * buffer,int size,bool reading)
{
	FILE * fp = fmemopen(buffer,size);

	FileRCPtr ret( new FileRC(fp,reading) );
	
	return ret;
}

FileRCPtr FileRC::CreateForWriteWithP4(const char * name)
{
	FileRCPtr ret = FileRC::Create(name,"wb");

	if ( ret == NULL && FileExists(name) )
	{
		char command[1024];
		sprintf(command,"p4 edit %s\n",name);
		system(command);
		
		ret = FileRC::Create(name,"wb");
	}
	
	return ret;
	
}

/*
void File_Test()
{
	File f;
	String x;
	f.IO(x);
	f.IO(&x,sizeof(x));
};
/**/

END_CB

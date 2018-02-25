#include "cblib/File.h"

START_CB

long File::GetFileSize() const
{
	long cur = Tell();
	
	const_cast<File *>(this)->Seek(0,SEEK_END);
	long size = Tell();
	
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

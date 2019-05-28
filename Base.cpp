#include "cblib/Base.h"
#include "cblib/File.h"
#include "cblib/Log.h"

START_CB

void AssertMessage(const char * fileName,const int line,const char * message)
{
	// prevent recursive calls, since we use gLog::Error ,
	//	and it's possible that he could ASSERT when he fails
	static bool inAssertMessage = false;
	if ( inAssertMessage )
		return;

	inAssertMessage = true;

	fprintf(stderr,"%s (%d) : %s\n",fileName,line,message);
	lprintf("%s (%d) : %s\n",fileName,line,message);
	
	inAssertMessage = false;
}

END_CB


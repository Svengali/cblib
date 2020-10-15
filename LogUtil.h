#pragma once

#include "Log.h"
#include "String.h"

START_CB

void lprintfCommandLine(int argc,const char ** argv);
//void lprintfCommandLine2(const char * progName,int argc,const char ** argv);

// inline so it picks up date & time :
// log the command line + build time :
static inline void lprintfCommandLine2(const char * progName,int argc,const char ** argv)
{
	lprintf("%s built %s, %s\n",progName,__DATE__,__TIME__);	
	lprintf("args: ");
	lprintfCommandLine(argc,argv);
}

void lprintfBinary(uint64 val,int bits);

// print as C variables that you can compile in :
void lprintfCByteArray(const uint8 * data,int size,const char * name,int columns = 30);
void lprintfCIntArray(const int * data,int size,const char * name,int columns,int width);
void lprintfCHexArray(const uint32 * data,int size,const char * name,int columns,int width);
void lprintfCFloatArray(const float * data,int size,const char * name,int columns,int width,int decimals);
void lprintfCDoubleArray(const double * data,int size,const char * name,int columns,int width,int decimals);

// lprintfCompression does NOT do a \n
void lprintfCompression(int64 UnPackedLen,int64 PackedLen);

// lprintfThroughput does a \n
void lprintfThroughput(const char * message, double seconds, uint64 ticks, int64 count );

END_CB

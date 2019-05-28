#pragma once

#include "Log.h"

START_CB

//---------------------------------------------------
// thread log :

void ThreadLogOpen();
void ThreadLogClose();

// called from thread to add logs :
//  ThreadLog is just a raw printf, it doesn't get all the fancy junk printf gets
void ThreadLog(const char * fmt,...);

// called from main thread to flush out logs :
//  they go out via printf with whatever cuent settings are
void ThreadLogFlush();

END_CB

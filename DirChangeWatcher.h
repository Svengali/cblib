#pragma once

#include "Base.h"
#include "Sptr.h"
#include "vector.h"
#include "String.h"

START_CB


/*
#define FILE_ACTION_ADDED                   0x00000001   
#define FILE_ACTION_REMOVED                 0x00000002   
#define FILE_ACTION_MODIFIED                0x00000003   
#define FILE_ACTION_RENAMED_OLD_NAME        0x00000004   
#define FILE_ACTION_RENAMED_NEW_NAME        0x00000005   
*/

struct DirChangeRecord
{
	// Command is added from the thread
	char	path[_MAX_PATH];
	uint32	action;
	time_t	time;
};

extern const uint32 c_dirChangeDefaultNotifyFlags;
extern const char * c_dirChangeActionStrings[];

//-----------------------------------------------------

SPtrFwd(DirChangeWatcher);

class DirChangeWatcher : public RefCounted
{
public:
	virtual ~DirChangeWatcher() { }

	//returns if any
	virtual bool GetDirChanges(cb::vector<DirChangeRecord> * pInto ) = 0;

	// same as above but just discards the records
	virtual bool GetDirChangesDiscard() = 0;
	
	// stop watching dirs
	virtual void Stop() = 0;
	
protected:
	DirChangeWatcher() { }
};

DirChangeWatcherPtr StartWatchingDirs(const char ** dirs,const int numDirs,uint32 notifyFlags);
DirChangeWatcherPtr StartWatchingDirs(const vector<String> & dirs,uint32 notifyFlags);


END_CB

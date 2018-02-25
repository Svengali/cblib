/************************************************************
    Module Name: MeteredSection.h
    Author: Dan Chou
    Description: Defines the metered section synchronization object

http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dndllpro/html/msdn_metrsect.asp
    
CB notes :

    CRITICAL_SECTION cannot be used across processes!!  METERED_SECTION can.
    
	the actual METERED_SECTION struct is just a local view of the shared object
	it is allocated with malloc & free, which could be unsafe in some places
    
    METERED_SECTION can be made on the stack or as a static and all zeros is a good initializer
    
************************************************************/

#ifndef _METERED_SECTION_H_
#define _METERED_SECTION_H_

#define MAX_METSECT_NAMELEN 128

/*
#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
*/

//-------------------------------------------------------------------------------------------

// Shared info needed for metered section
typedef struct _METSECT_SHARED_INFO {
    LONG   fInitialized;     // Is the metered section initialized? (a BOOL)
    LONG   lSpinLock;        // Used to gain access to this structure
    LONG   lThreadsWaiting;  // Count of threads waiting
    LONG   lAvailableCount;  // Available resource count
    LONG   lMaximumCount;    // Maximum resource count
} METSECT_SHARED_INFO, *LPMETSECT_SHARED_INFO;

// The opaque Metered Section data structure
typedef struct _METERED_SECTION {
    HANDLE hEvent;           // Handle to a kernel event object
    HANDLE hFileMap;         // Handle to memory mapped file
    LPMETSECT_SHARED_INFO lpSharedInfo;
} METERED_SECTION, *LPMETERED_SECTION;

//-------------------------------------------------------------------------------------------

// Interface functions
LPMETERED_SECTION CreateMeteredSection(LONG lInitialCount, LONG lMaximumCount, LPCTSTR lpName);

/*
Parameters

lInitialCount
    Specifies an initial count of open slots for the metered section. This value must be greater than or equal to zero and less than or equal to lMaximumCount. A slot is available when the count is greater than zero and none is available when it is zero. The count is decreased by one whenever the EnterMeteredSection function releases a thread that was waiting for the metered section. The count is increased by a specified amount by calling the LeaveMeteredSection function.
lMaximumCount
    Specifies the maximum number of available slots for the metered section. This value must be greater than zero.
lpName
    Pointer to a null-terminated string specifying the name of the metered section. The name is limited to MAX_METSECT_NAMELEN characters, and can contain any character except the backslash path-separator character (\). Name comparison is case sensitive.

    If lpName matches the name of an existing named metered section, the lInitialCount and lMaximumCount parameters are ignored because they have already been set by the process that originally created the metered section.

    If lpName is NULL, the metered section is created without a name. 

Return values

If the function succeeds, the return value is a pointer to a metered section. If the named metered section existed before the function call, the function returns a pointer to the existing metered section and GetLastError returns ERROR_ALREADY_EXISTS.

If the function fails, the return value is NULL. To get extended error information, call GetLastError.

*/

#ifndef _WIN32_WCE
LPMETERED_SECTION OpenMeteredSection(LPCTSTR lpName);
#endif

/*

Parameters

lpName
    Pointer to a null-terminated string that names the metered section to be opened. Name comparisons are case sensitive.

Return values

If the function succeeds, the return value is a pointer to a metered section. If the function fails, the return value is NULL. To get extended error information, call GetLastError.

*/

DWORD EnterMeteredSection(LPMETERED_SECTION lpMetSect, DWORD dwMilliseconds);

/*
The EnterMeteredSection function returns when one of the following occurs:

    * The specified metered section has an available slot.
    * The time-out interval elapses.
    * The following code shows the EnterMeteredSection function prototype:

DWORD EnterMeteredSection(
  LPMETERED_SECTION lpMetSect, // Pointer to a metered section
  DWORD dwMilliseconds         // Time-out interval in milliseconds
);
 

Parameters

lpMetSect
    Pointer to a metered section. 
dwMilliseconds
    Specifies the time-out interval, in milliseconds.
    The function returns if the interval elapses, even if the metered section does not have an open slot. 
    If dwMilliseconds is zero, the function tests the metered section's state and returns immediately. 
    If dwMilliseconds is INFINITE, the function's time-out interval never elapses.

Return values

If the function succeeds, the return value is WAIT_OBJECT_0. If the function times out, the return value is WAIT_TIMEOUT. 
To get extended error information, call GetLastError.

Remarks

The EnterMeteredSection function checks for an available slot in the specified metered section. If the metered section does not have an available slot, the calling thread enters an efficient wait state. The thread consumes very little processor time while waiting for a slot to become free.

*/

BOOL LeaveMeteredSection(LPMETERED_SECTION lpMetSect, LONG lReleaseCount, LPLONG lpPreviousCount);

/*
The LeaveMeteredSection function increases the number of available slots of a metered section by a specified amount.

The following code shows the LeaveMeteredSection function prototype:

BOOL LeaveMeteredSection(
  LPMETERED_SECTION lpMetSect, // Pointer to a metered section
  LONG lReleaseCount,          // Amount to add to current count
  LPLONG lpPreviousCount       // Address of previous count
);

Parameters

lpMetSect
    Pointer to a metered section. The CreateMeteredSection or OpenMeteredSection function returns this pointer.
lReleaseCount
    Specifies the number of the metered section's slots to release. The value must be greater than zero. If the specified amount would cause the metered section's count to exceed the maximum number of available slots specified during creation of the metered section, the number of available slots is not changed and the function returns FALSE.
lpPreviousCount
    Pointer to a 32-bit LONG to receive the previous metered section open slot count. This parameter can be NULL if the previous count is not required.

Return values

If the function succeeds, the return value is nonzero. If the function fails, the return value is zero. To get extended error information, call GetLastError.

*/

void CloseMeteredSection(LPMETERED_SECTION lpMetSect);

/*

The CloseMeteredSection function closes an open metered section pointer.

The following code shows the CloseMeteredSection function prototype:

void CloseMeteredSection(
  LPMETERED_SECTION lpMetSect // Pointer to the metered section to close
);

Parameters

lpMetSect
    Identifies a metered section pointer.

Return values

None

*/

//-------------------------------------------------------------------------------------------

// Init/Delete for structs on the stack like CRITICAL_SECTION :
//	"Delete" is really an in-place DeInit , it doesn't free the pointer like Close() does
BOOL InitMeteredSection(LPMETERED_SECTION lpMetSect, LONG lInitialCount, LONG lMaximumCount, LPCTSTR lpName, BOOL bOpenOnly);
void DeInitMeteredSection(LPMETERED_SECTION lpMetSect);

// standard accessors for only one accesor :
inline BOOL InitMeteredSection(LPMETERED_SECTION lpMetSect, LPCTSTR lpName)
{
	return ::InitMeteredSection(lpMetSect,1,1,lpName,FALSE);
}

inline BOOL LeaveMeteredSection(LPMETERED_SECTION lpMetSect)
{
	return ::LeaveMeteredSection(lpMetSect,1,NULL);
}

//-------------------------------------------------------------------------------------------

class UseMeteredSection
{
public:
	UseMeteredSection() : m_ptr(NULL)
	{
	}
	~UseMeteredSection()
	{
		Leave();
	}
	
	void Leave()
	{
		if ( m_ptr )
		{
			LeaveMeteredSection(m_ptr);
			m_ptr = NULL;
		}
	}
	DWORD Enter(LPMETERED_SECTION pMS,DWORD dwMillis)
	{
		DWORD dwRet = EnterMeteredSection(pMS,dwMillis);
		if ( dwRet == WAIT_OBJECT_0 )
		{
			// acquired the lock, so we'll free it
			m_ptr = pMS;
		}
		return dwRet;
	}

private:
	LPMETERED_SECTION m_ptr;
};

/*
#ifdef __cplusplus
}
#endif // __cplusplus
*/

#endif // _METERED_SECTION_H_

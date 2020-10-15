#pragma once

#include "Base.h"

START_CB

// internal helpers for DeSubst
void LogDosDrives();
bool MapNtDriveName(const wchar_t * from,wchar_t * to);

//BOOL GetFinalPathNameByHandleW_NtQuery(HANDLE f,wchar_t * outName,DWORD bufSize);

bool GetDeSubstName(const char * from, char * to);
bool GetDeSubstFullPath(const char * from, char * to);

// WinUtil_GetFullPathProperCase does DeSubst
//	and then also checks the disk to fix the case
void WinUtil_GetFullPathProperCase(const char * fm, char * to);

END_CB

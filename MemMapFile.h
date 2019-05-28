#pragma once

#include "cblib/Base.h"

typedef void * HANDLE;

START_CB

//-------------------------------------------------------------

struct MemoryMappedFile
{
	HANDLE	m_hFile;
	HANDLE	m_hMapping;
	void *	m_memory;
	int64	m_size;
	
	bool OpenMapping(const char * file);
	bool OpenReadWhole(const char * file);
	void Close();
	
	MemoryMappedFile() : m_hFile(0),m_hMapping(0),m_memory(0),m_size(0) { }
	~MemoryMappedFile() { Close(); }
};

END_CB

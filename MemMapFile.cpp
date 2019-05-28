#include "MemMapFile.h"
#include "FileUtil.h"
#include "Win32Util.h"

START_CB

bool MemoryMappedFile::OpenReadWhole(const char *name)
{
	Close();

	m_memory = ReadWholeFile(name,&m_size);

	return true;
}

bool MemoryMappedFile::OpenMapping(const char *name)
{
	Close();

	m_hFile = CreateFile(name,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	if ( ! m_hFile ) return false;

	m_size = GetFileSize64(m_hFile);
	
	if ( m_size <= 0 )
	{
		CloseHandle(m_hFile);
		m_hFile = 0;
		return false;
	}

	// if m_size is small, switch to OpenReadWholeFile ?
	if ( m_size < (128<<20) ) // 128 MB
	{
		CloseHandle(m_hFile);
		m_hFile = 0;
		return OpenReadWhole(name);
	}
	
	m_hMapping = CreateFileMapping(m_hFile,NULL,PAGE_READONLY|SEC_COMMIT,(DWORD)(m_size>>32),(DWORD)(m_size),NULL);
	if ( ! m_hMapping ) return false;
	
	m_memory = MapViewOfFile(m_hMapping,FILE_MAP_READ,0,0,check_value_cast<SIZE_T>(m_size));
	if ( ! m_memory ) return false;

	return true;
}

void MemoryMappedFile::Close()
{
	if ( m_hMapping )
	{
		if ( m_memory )
			UnmapViewOfFile(m_memory);
		CloseHandle(m_hMapping); 
	}
	else
	{
		if ( m_memory )
			CBFREE( m_memory );
	}	
	
	if ( m_hFile )
		CloseHandle(m_hFile);

	m_memory = 0;
	m_hMapping = 0;
	m_hFile = 0;
	m_size = 0;
}

END_CB

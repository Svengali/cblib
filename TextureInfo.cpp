#include "TextureInfo.h"
#include "cblib/Log.h"
#include "cblib/Util.h"
#include "cblib/File.h"

#include <windows.h>
#include <d3d9types.h> // for D3DFMT

START_CB

TextureInfo::TextureInfo() : m_fileName(String::eReserve,128)
{
	Reset();
}

void TextureInfo::Reset()
{
	m_fileName.Clear();
	m_width = m_height = 0;
	m_gammaCorrected = true;
	m_eEdgeMode = eEdge_Clamp;
	m_alpha = eAlpha_None;
	m_usage = eUsage_Color;
	m_format = D3DFMT_UNKNOWN;
	m_heightMapAltitude = 1.f;
}

const char * const TextureInfo::GetFormatDescription(const D3DFORMAT format)
{
	switch(format)
	{
		case D3DFMT_R8G8B8:		return "R8G8B8";		
		case D3DFMT_A8R8G8B8:	return "A8R8G8B8";	
		case D3DFMT_X8R8G8B8:	return "X8R8G8B8";	
		case D3DFMT_R5G6B5:		return "R5G6B5";		
		case D3DFMT_X1R5G5B5:	return "X1R5G5B5";	
		case D3DFMT_A1R5G5B5:	return "A1R5G5B5";	
		case D3DFMT_A4R4G4B4:	return "A4R4G4B4";	
		case D3DFMT_R3G3B2:		return "R3G3B2";		
		case D3DFMT_A8:			return "A8";			
		case D3DFMT_A8R3G3B2:	return "A8R3G3B2";	
		case D3DFMT_X4R4G4B4:	return "X4R4G4B4";	
		case D3DFMT_A8P8:		return "A8P8";		
		case D3DFMT_P8:			return "P8";			
		case D3DFMT_L8:			return "L8";			
		case D3DFMT_A8L8:		return "A8L8";		
		case D3DFMT_A4L4:		return "A4L4";		
		case D3DFMT_V8U8:		return "V8U8";		
		case D3DFMT_L6V5U5:		return "L6V5U5";		
		case D3DFMT_X8L8V8U8:	return "X8L8V8U8";	
		case D3DFMT_Q8W8V8U8:	return "Q8W8V8U8";	
		case D3DFMT_V16U16:		return "V16U16";		
//		case D3DFMT_W11V11U10:	return "W11V11U10";	
		case D3DFMT_UYVY:		return "UYVY";		
		case D3DFMT_YUY2:		return "YUY2";		
		case D3DFMT_DXT1:		return "DXT1";		
		case D3DFMT_DXT2:		return "DXT2";		
		case D3DFMT_DXT3:		return "DXT3";		
		case D3DFMT_DXT4:		return "DXT4";		
		case D3DFMT_DXT5:		return "DXT5";
		case D3DFMT_D32:		return "D32";	
		case D3DFMT_D15S1:		return "D15S1";		
		case D3DFMT_D24S8:		return "D24S8";	
		case D3DFMT_D16:		return "D16";	
		case D3DFMT_D24X8:		return "D24X8";		
		case D3DFMT_D24X4S4:	return "D24X4S4";
		case D3DFMT_VERTEXDATA:	return "VERTEXDATA";
		case D3DFMT_INDEX16:	return "INDEX16";	
		case D3DFMT_INDEX32:	return "INDEX32";
		case D3DFMT_D16_LOCKABLE:	return "D16_LOCKABLE";

		default: return "unknown";
	}
}

void TextureInfo::Log() const
{
	lprintf("TextureInfo : %s\n",m_fileName.CStr());
	lprintf("w : %d, h : %d, mips : %d, format = %04X = %s\n",m_width,m_height,m_format,
					GetFormatDescription(m_format));
}

// binary IO :
void TextureInfo::IO(File & file)
{
	static const ulong c_tag = 'TxIn';
	static const ulong c_ver = 1;

	ulong tag = c_tag;
	file.IO(tag);
	if ( tag != c_tag )
		throw String("didn't match tag in TextureInfo::IO");

	ulong ver = c_ver;
	file.IO(ver);
	if ( ver != c_ver )
		throw String("didn't match ver in TextureInfo::IO");

	if ( file.IsReading() )
		m_fileName.ReadBinary(file.Get());
	else
		m_fileName.WriteBinary(file.Get());
	file.IO(m_width);
	file.IO(m_height);
	file.IO(m_gammaCorrected);
	file.IO(m_eEdgeMode);
	file.IO(m_alpha);
	file.IO(m_format);
	file.IO(m_usage);
	file.IO(m_heightMapAltitude);
}

END_CB

#pragma once

/*!
	TextureInfo describes all the side-data of a texture

*/

#include "Base.h"
#include "String.h"
#include "File.h"

typedef enum _D3DFORMAT D3DFORMAT;

START_CB

struct TextureInfo
{

	enum EEdgeMode
	{
		eEdge_Wrap,
		eEdge_Mirror,
		eEdge_Clamp,
		eEdge_Shared // acts like Clamp
		//! \todo @@@@ need eEdge_Extrapolate
		//	Extrapolate should take an edge like [123] and make [123]456..
		/**
		examples , on [123]

		Wrap   : 123[123]123
		Mirror :  32[123]21
		Clamp  : 111[123]333
		Shared : 111[123]333 (and edge pixels always made only from edge pixels!)
		Extrapolate : 000[123]456

		"shared pixel edge" is when you know the artists
			will be putting the textures onto quads and
			making them line up with other textures; in
			this case the pixel boundary of the image
			is shared; the boundary of the mip-maps must
			be made only from the boundary of the original.

		**/
	};
	enum EUsage
	{
		eUsage_Color,
		eUsage_Heightmap,
		eUsage_Normalmap
	};
	enum EAlphaType
	{
		eAlpha_None,
		eAlpha_Test,
		eAlpha_Blend
	};

	static const char * const GetFormatDescription(const D3DFORMAT format);

	// source info :
	int				m_width,m_height;
	bool			m_gammaCorrected;	//!< figured from format & usage, but may be overridden
	EEdgeMode		m_eEdgeMode;		//!< needs artist input !
	EAlphaType		m_alpha;			//!< figured from format, but may be overridden
	float			m_heightMapAltitude;  //!< only used if the source is eUsage_Heightmap 
	EUsage			m_usage;
	String			m_fileName;
	D3DFORMAT		m_format;

	// hasPalette & palette data

	TextureInfo();
	
	void Log() const;

	void Reset();

	// binary IO :
	void IO(File & file);

	// @@@@ todo : read/write as Text ?

	static int Index(const int i,const int size,TextureInfo::EEdgeMode eEdgeMode);

};


inline int TextureInfo::Index(const int i,const int size,TextureInfo::EEdgeMode eEdgeMode)
{
	if ( i >= 0 && i < size )
		return i;
	
	switch(eEdgeMode)
	{
		case TextureInfo::eEdge_Wrap:
		{
			int iMod = i;
			while ( iMod < 0 )
				iMod += size;
			while ( iMod >= size )
				iMod -= size;
			return iMod;
		}
		case TextureInfo::eEdge_Mirror:
		{				
			if ( i < 0 )
				return Index( - i , size, eEdgeMode ); // -1 -> 1
			else
				return Index( - i + 2*size - 2 , size, eEdgeMode ); // size -> size-2
		}
		case TextureInfo::eEdge_Clamp:
		case TextureInfo::eEdge_Shared:
		{
			if ( i < 0 )
				return 0;
			else
				return size - 1;
		}
		default :
			CANT_GET_HERE();
			return 0;
	}
	//ASSERT(false);
	//return 0;
	
	//CANT_GET_HERE();
	// 1>c:\src\cblib\textureinfo.h(127) : warning C4715: 'cb::TextureInfo::Index' : not all control paths return a value
}
	

END_CB

#pragma once

#include "cblib/Base.h"
#include "cblib/Color.h"
#include "cblib/TextureInfo.h"
#include "cblib/RefCounted.h"

// I use some Windoze types and Windowsisms, but this module should actually be OS-independent
typedef enum _D3DFORMAT D3DFORMAT;
typedef struct tagPALETTEENTRY PALETTEENTRY;

START_CB

class ColorDW;

/**

BmpImage ; raw bitmap image similar to a .bmp

The bits are stored in memory the windows way (BGR) but are always top-down.

A raw 32-bit color value is the same way it's stored in a .bmp which is the same as a ColorDW which is an RGBQUAD
	so you can use the bits here directly as windows DIBits
	for 32-bit Bmps you can cast m_pData directly to a ColorDW array

(note that COLORREF has a different order)

**/

SPtrFwd(BmpImage);
class BmpImage : public RefCounted
{
public:
    ~BmpImage();

	// use these to make one :
	static BmpImagePtr Create() { return BmpImagePtr( new BmpImage ); }
	static BmpImagePtr Create(const char * fileName);

	//-------------------------------------

    ubyte*          m_pData;
    bool            m_bDeleteData;
    
    ColorDW *		m_pPalette;
    bool            m_bDeletePalette;
    
    D3DFORMAT       m_format;
    ulong           m_width;
    ulong           m_height;
    ulong           m_pitch;	// note : pitch is in BYTES

	// tells it whether the destructor should delete

	//-------------------------------------

	// generally you should use Create() not this :
    bool Load( const void* pData, ulong dwSize );
	bool Load( const char * strFilename );

    static D3DFORMAT GetFormatForBytes(int n);
    static int GetBytesFromFormat(D3DFORMAT fmt);

    int GetBytesPerPel() const;
	bool IsGrey8() const; // (use MakeGreyPaletteL8 first)
	bool Is24Bit() const;
	bool Is32Bit() const;
    
    // generally don't call these directly, just use Load()
    bool LoadBMP( const void* pData, ulong dwSize );
    bool LoadDIB( const void* pData, ulong dwSize );
    bool LoadTGA( const void* pData, ulong dwSize );
    bool LoadPPM( const void* pData, ulong dwSize );

	bool WriteBMP(const char* outfile);
	void WriteTGA(const char* outfile);

	void	GetTextureInfo(TextureInfo * pInfo) const;
	ColorDW	GetColor(const int x,const int y) const;
	ColorDW	GetColorMirror(const int x,const int y) const;
	void SetColor(const int x,const int y,const ColorDW c) const;

	// u,v is from [0,0] to [w-1,h-1] ; 0,0 is the center of the first pixel
	//	that float * version fills into with RGBA in [0,255] - not like a ColorF but like a ColorDW
	void SampleBilinear(float * into,const float u,const float v) const;
	void SampleBilinear(ColorF * into,const float u,const float v) const;
	ColorDW SampleBilinear(const float u,const float v) const;

	void ForceRGB24orL8();
	void Force24Bit(); // format will be D3DFMT_R8G8B8 after (can depalettize, etc.)
	void Force32Bit();
    void MakeGreyPaletteL8(); // if it's P8 grey it becomes L8
	void MakePaletteIndicesColors(); // make palette index N color RGB(N,N,N)
    bool Depalettize(); // makes it 32 bit
	void FlipVerticalAndBGR();
	void FlipVertical();

	void Transpose(); // only on 24 bit
	void Allocate8Bit(int w,int h);
	void Allocate24Bit(int w,int h);
	void Allocate32Bit(int w,int h);

	void Release(); // frees existing and zeros out
	
private:
    BmpImage();
    FORBID_CLASS_STANDARDS(BmpImage);
};

END_CB

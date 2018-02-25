#include "BmpImage.h"
#include "cblib/Log.h"
#include "cblib/File.h"
#include "cblib/Util.h"
#include "cblib/Color.h"

#include <windows.h>
#include <d3d9types.h> // for D3DFMT

START_CB

// Fake bitmap structures...
#if 0
namespace
{
#pragma pack(push)
#pragma pack(1)
	
	typedef struct tagBITMAPFILEHEADER {
		WORD bfType; 
		ulong bfSize; 
		WORD bfReserved1; 
		WORD bfReserved2; 
		ulong bfOffBits; 
	} BITMAPFILEHEADER;

	typedef struct tagBITMAPINFOHEADER{ 
		ulong biSize; 
		LONG biWidth; 
		LONG biHeight; 
		WORD biPlanes; 
		WORD biBitCount;
		ulong biCompression; 
		ulong biSizeImage; 
		LONG biXPelsPerMeter; 
		LONG biYPelsPerMeter; 
		ulong biClrUsed; 
		ulong biClrImportant; 
	} BITMAPINFOHEADER;

	typedef ulong FXPT2DOT30;
	typedef struct tagCIEXYZ {
		FXPT2DOT30 ciexyzX; 
		FXPT2DOT30 ciexyzY; 
		FXPT2DOT30 ciexyzZ; 
	} CIEXYZ;
	typedef CIEXYZ* LPCIEXYZ; 

	typedef struct tagCIEXYZTRIPLE {
		CIEXYZ  ciexyzRed; 
		CIEXYZ  ciexyzGreen; 
		CIEXYZ  ciexyzBlue; 
	} CIEXYZTRIPLE;
	typedef CIEXYZTRIPLE* LPCIEXYZTRIPLE; 

	typedef struct {
		ulong        bV4Size;
		LONG         bV4Width;
		LONG         bV4Height;
		WORD         bV4Planes;
		WORD         bV4BitCount;
		ulong        bV4V4Compression;
		ulong        bV4SizeImage;
		LONG         bV4XPelsPerMeter;
		LONG         bV4YPelsPerMeter;
		ulong        bV4ClrUsed;
		ulong        bV4ClrImportant;
		ulong        bV4RedMask;
		ulong        bV4GreenMask;
		ulong        bV4BlueMask;
		ulong        bV4AlphaMask;
		ulong        bV4CSType;
		CIEXYZTRIPLE bV4Endpoints;
		ulong        bV4GammaRed;
		ulong        bV4GammaGreen;
		ulong        bV4GammaBlue;
	} BITMAPV4HEADER, *PBITMAPV4HEADER;
	const ulong BI_RGB        = 0L;
	const ulong BI_RLE8       = 1L;
	const ulong BI_RLE4       = 2L;
	const ulong BI_BITFIELDS  = 3L;

	typedef struct tagRGBQUAD { 
		ubyte rgbBlue;
		ubyte rgbGreen;
		ubyte rgbRed;
		ubyte rgbReserved;
	} RGBQUAD;
#pragma pack(pop)

} // namespace {}
#endif //

static const int BM_TAG	= 0x4D42;		// 'BM'


//-----------------------------------------------------------------------------
// Name: BmpImage()
// Desc: Initializes object
//-----------------------------------------------------------------------------
BmpImage::BmpImage()
{
	m_width = 0;
	m_height = 0;
	m_pitch = 0;
    m_format         = D3DFMT_UNKNOWN;
    m_pData          = NULL;
    m_pPalette       = NULL;
    m_bDeleteData    = false;
    m_bDeletePalette = false;
}




//-----------------------------------------------------------------------------
// Name: ~BmpImage()
// Desc: Frees resources held by the object
//-----------------------------------------------------------------------------
BmpImage::~BmpImage()
{
	Release();
}

void BmpImage::Release()
{
    if( m_pData && m_bDeleteData )
        delete[] m_pData;

    if( m_pPalette && m_bDeletePalette )
        delete[] m_pPalette;
        
	m_width = 0;
	m_height = 0;
	m_pitch = 0;
    m_format         = D3DFMT_UNKNOWN;
    m_pData          = NULL;
    m_pPalette       = NULL;
    m_bDeleteData    = false;
    m_bDeletePalette = false;       
}


//-----------------------------------------------------------------------------
// Name: Load()
// Desc: Attempts to load the given data as an image
//-----------------------------------------------------------------------------
bool BmpImage::Load( const void* pData, ulong cbData )
{

    // Try all known image loading subroutines
	int i;
    for( i = 0; i < 4; i++ )
    {
		bool hr = false;

		try
		{
			switch(i)
			{
				case 0: hr = LoadBMP( pData, cbData ); break;
				case 1: hr = LoadPPM( pData, cbData ); break;
				case 2: hr = LoadTGA( pData, cbData ); break;
				case 3: hr = LoadDIB( pData, cbData ); break;
				default: ASSERT(false); hr = false;
			}
		}
		catch( ... )
		{
			hr = false;
		}

        if( hr == true )
            break;

        if( m_pData && m_bDeleteData )
            delete[] m_pData;

        if( m_pPalette && m_bDeletePalette )
            delete[] m_pPalette;

        m_pData          = NULL;
        m_pPalette       = NULL;
        m_bDeleteData    = false;
        m_bDeletePalette = false;
    }

    if( 4 == i )
    {
        //lprintf("Unsupported file format");
        return false;
    }

    return true;
}


//-----------------------------------------------------------------------------
// Name: Load()
// Desc: Reads the data from the file and tries to load it as an image
//-----------------------------------------------------------------------------
bool BmpImage::Load( const char * strFilename )
{
    HANDLE hFile = CreateFileA( strFilename, GENERIC_READ, FILE_SHARE_READ, NULL, 
                                OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL );
    if( INVALID_HANDLE_VALUE == hFile )
        return false;

    // Allocate memory
    ulong dwFileSize = GetFileSize( hFile, NULL );
    void* pFileData  = CBALLOC( dwFileSize );

    // Read it the file
    DWORD dwRead;
    ReadFile( hFile, (void*)pFileData, dwFileSize, &dwRead, NULL );

    bool hr = Load( pFileData, dwFileSize );

    // Free stuff
    CloseHandle( hFile );
    CBFREE( pFileData );

    return hr;
}



//-----------------------------------------------------------------------------
// Name: Depalettize()
// Desc: Depalettize palettized images
//-----------------------------------------------------------------------------
bool BmpImage::Depalettize()
{
    if( NULL == m_pPalette )
        return true;

    ulong* pNewData = new ulong[m_width*m_height];

    //ubyte*  pOldData = (ubyte*)pSrcData;

    // Loop through all texels and get 32-bit color from the 8-bit palette index
    for( ulong y=0; y<m_height; y++ )
    {
		ubyte*  pSrcData = (ubyte*)m_pData + y * m_pitch;
		ulong*  pDstData = ((ulong*)pNewData) + y * m_width;
        for( ulong x=0; x<m_width; x++ )
        {
            ubyte  index = *pSrcData++;

			*pDstData++ = m_pPalette[index].GetDW();

			/*
            ulong red   = m_pPalette[index].peRed;
            ulong green = m_pPalette[index].peGreen;
            ulong blue  = m_pPalette[index].peBlue;
            ulong alpha = m_pPalette[index].peFlags;

            *pDstData++ = (alpha<<24) | (red<<16) | (green<<8) | (blue<<0);
            */
        }
    }

    // Delete the old palette
    if( m_bDeletePalette )
        delete[] m_pPalette;
    m_pPalette       = NULL;
    m_bDeletePalette = false;

    // Delete the old data, and assign the new data
    if( m_bDeleteData )
        delete[] m_pData;
    m_pData          = (ubyte *)pNewData;
    m_bDeleteData    = true;

    // The format is now A8R8G8B8
    m_format = D3DFMT_A8R8G8B8;
    m_pitch  = m_width * sizeof(ulong);

    return true;
}


//-----------------------------------------------------------------------------
// Name: LoadBMP()
// Desc: Attempts to load the given data as a BMP
//-----------------------------------------------------------------------------
bool BmpImage::LoadBMP( const void* pvData, ulong cbData )
{
    // Examine header
    if(cbData < sizeof(BITMAPFILEHEADER))
        return false;

    BITMAPFILEHEADER *pFH = (BITMAPFILEHEADER *) pvData;

    if(pFH->bfType != (('B') | ('M' << 8)) || pFH->bfSize > cbData)
        return false;

    return LoadDIB((ubyte *) pvData + sizeof(BITMAPFILEHEADER), cbData - sizeof(BITMAPFILEHEADER));
}


D3DFORMAT BmpImage::GetFormatForBytes(int n)
{
	switch(n)
	{
	case 1:
        return D3DFMT_P8;
    case 2:
        return D3DFMT_X1R5G5B5;
    case 3:
        return D3DFMT_R8G8B8;
    case 4:
        return D3DFMT_X8R8G8B8;
	default:
		return D3DFMT_UNKNOWN;
	}	
}

int BmpImage::GetBytesFromFormat(D3DFORMAT fmt)
{
	switch(fmt)
	{
	case D3DFMT_P8:
	case D3DFMT_L8:
        return 1;
    case D3DFMT_X1R5G5B5:
    case D3DFMT_R5G6B5:
        return 2;
    case D3DFMT_R8G8B8:
        return 3;
    case D3DFMT_X8R8G8B8:
    case D3DFMT_A8R8G8B8:
        return 4;
	default:
		return 0; // ack!
	}
}

int BmpImage::GetBytesPerPel() const
{
	return GetBytesFromFormat(m_format);
}

//-----------------------------------------------------------------------------
// Name: LoadDIB()
// Desc: Attempts to load the given data as a DIB
//-----------------------------------------------------------------------------
bool BmpImage::LoadDIB( const void* pvData, ulong cbData )
{
    UNALIGNED BITMAPINFOHEADER *pIH;
    ulong     dwWidth, dwHeight, dwOffset, dwClrUsed;

    if(cbData < sizeof(BITMAPINFOHEADER))
        return false;

    pIH = (BITMAPINFOHEADER *) pvData;

    if(pIH->biSize < sizeof(BITMAPINFOHEADER))
        return false;

    dwWidth   = (ulong) (pIH->biWidth);
    dwHeight  = (ulong) (pIH->biHeight > 0 ? pIH->biHeight : -pIH->biHeight);
    dwClrUsed = (ulong) (pIH->biClrUsed);

    if((pIH->biBitCount <= 8) && (0 == dwClrUsed))
        dwClrUsed = (ulong) (1 << pIH->biBitCount);

    dwOffset  = (ulong) pIH->biSize + dwClrUsed * sizeof(ulong);

    if(dwOffset > (ulong) cbData)
        return false;

    if(pIH->biPlanes != 1)
        return false;


    // Only RGB and BITFIELD bitmaps can be inverted
    if(pIH->biHeight < 0 && pIH->biCompression != BI_RGB && pIH->biCompression != BI_BITFIELDS)
        return false;


    // Compute format
    ulong dwB, dwG, dwR, dwA;
    D3DFORMAT Format = D3DFMT_UNKNOWN;

    switch(pIH->biCompression)
    {
        case BI_RGB:
        case BI_RLE4:
        case BI_RLE8:

            switch(pIH->biBitCount)
            {
                case 1:
                case 4:
                case 8:
                    Format = D3DFMT_P8;
                    break;

                case 16:
                    Format = D3DFMT_X1R5G5B5;
                    break;

                case 24:
                    Format = D3DFMT_R8G8B8;
                    break;

                case 32:
                    Format = D3DFMT_X8R8G8B8;
                    break;

                default:
                    return false;
            }
            break;

        case BI_BITFIELDS:
            if(pIH->biSize < sizeof(BITMAPV4HEADER))
                return false;

            dwB = ((BITMAPV4HEADER *) pIH)->bV4BlueMask;
            dwG = ((BITMAPV4HEADER *) pIH)->bV4GreenMask;
            dwR = ((BITMAPV4HEADER *) pIH)->bV4RedMask;
            dwA = ((BITMAPV4HEADER *) pIH)->bV4AlphaMask;

            switch(pIH->biBitCount)
            {
                case 16:
                    if(dwB == 0x00ff && dwG == 0x00ff && dwR == 0x00ff && dwA == 0xff00)
                        Format = D3DFMT_A8L8;

                    else if(dwB == 0x001f && dwG == 0x07e0 && dwR == 0xf800 && dwA == 0x0000)
                        Format = D3DFMT_R5G6B5;

                    else if(dwB == 0x001f && dwG == 0x03e0 && dwR == 0x7c00 && dwA == 0x0000)
                        Format = D3DFMT_X1R5G5B5;

                    else if(dwB == 0x001f && dwG == 0x03e0 && dwR == 0x7c00 && dwA == 0x8000)
                        Format = D3DFMT_A1R5G5B5;

                    else if(dwB == 0x000f && dwG == 0x00f0 && dwR == 0x0f00 && dwA == 0xf000)
                        Format = D3DFMT_A4R4G4B4;

                    /*
					else if(dwB == 0x0003 && dwG == 0x001c && dwR == 0x00e0 && dwA == 0xff00)
                        Format = D3DFMT_A8R3G3B2;
					*/

                    break;

                case 24:
                    if(dwB == 0x0000ff && dwG == 0x00ff00 && dwR == 0xff0000 && dwA == 0x000000)
                        Format = D3DFMT_X8R8G8B8;
                    break;

                case 32:
                    if(dwB == 0x000000ff && dwG == 0x0000ff00 && dwR == 0x00ff0000 && dwA == 0x00000000)
                        Format = D3DFMT_X8R8G8B8;

                    else if(dwB == 0x000000ff && dwG == 0x0000ff00 && dwR == 0x00ff0000 && dwA == 0xff000000)
                        Format = D3DFMT_A8R8G8B8;

                    break;
            }

            break;

        default:
            lprintf("LoadBMP: JPEG compression not supported");
            return false;
    }


    if(D3DFMT_UNKNOWN == Format)
    {
        return false;
    }


    if(D3DFMT_P8 == Format)
    {
        ulong dwClrUsed = pIH->biClrUsed;

        if(!dwClrUsed)
            dwClrUsed = 1 << pIH->biBitCount;

        m_bDeletePalette = true;

        m_pPalette = new ColorDW[256];
        memset(m_pPalette,0xFF,sizeof(ColorDW)*256);

        ulong dw;
        RGBQUAD* prgb = (RGBQUAD*) (((ubyte *) pIH) + pIH->biSize);

        for(dw = 0; dw < dwClrUsed; dw++, prgb++)
        {
            m_pPalette[dw].Set( prgb->rgbRed, prgb->rgbGreen, prgb->rgbBlue, 0xff );
        }

		/*
        for(dw = dwClrUsed; dw < 256; dw++)
        {
            m_pPalette[dw].peRed   = 0xff;
            m_pPalette[dw].peGreen = 0xff;
            m_pPalette[dw].peBlue  = 0xff;
            m_pPalette[dw].peFlags = 0xff;
        }
        */
    }

    ulong dwWidthBytes;
    ulong dwSrcInc, dwDstInc;

    switch(pIH->biBitCount)
    {
        case 1:
            dwWidthBytes = dwWidth;
            dwSrcInc = ((dwWidth >> 3) + 3) & ~3;
            break;

        case 4:
            dwWidthBytes = dwWidth;
            dwSrcInc = ((dwWidth >> 1) + 3) & ~3;
            break;

        default:
            dwWidthBytes = (dwWidth * (pIH->biBitCount >> 3));
            dwSrcInc = (dwWidthBytes + 3) & ~3;
            break;
    }
	
    m_format  = Format;
    m_pitch   = (ulong)((dwWidthBytes + 3) & ~3);
    m_width   = (ulong)dwWidth;
    m_height  = (ulong)dwHeight;
               	
	// BmpImage promotes 24 bit to 32 on load; that should be optional
	//	32-bit is good for use in d3d textures
	
	/*
	// convert 24-bit to 32 bit
    if (pIH->biBitCount == 24 && Format == D3DFMT_R8G8B8)
    {
		m_format = Format = D3DFMT_X8R8G8B8;
		
        ulong*          pdwDst;
        int             nStrideDst;
        UINT            i, j;

        dwWidthBytes = (dwWidth * (32 >> 3));
        m_pitch      = (ulong)((dwWidthBytes + 3) & ~3);

        m_bDeleteData = true;

        m_pData = new ubyte[dwHeight * m_pitch];

        if (pIH->biHeight < 0)
        {
            pdwDst = (ulong*)m_pData;
            nStrideDst = m_pitch >> 2;
        }
        else
        {
            pdwDst = (ulong*)((ubyte*)m_pData + m_pitch * (dwHeight - 1));
            nStrideDst = -(int)(m_pitch >> 2);
        }

        dwSrcInc = (dwWidth*3 + 3) & ~3;

		UNALIGNED ubyte* pbSrcRow;
        pbSrcRow = ((ubyte*)pvData) + dwOffset;
        for (i = 0; i < dwHeight; i++)
        {
			UNALIGNED ubyte* pbSrc;
			pbSrc = pbSrcRow;
            for (j = 0; j < dwWidth; j++)
            {
                pdwDst[j] = pbSrc[2] << 16 | pbSrc[1] << 8 | *pbSrc;
                pbSrc += 3;
            }
			pbSrcRow += dwSrcInc;

            pdwDst += nStrideDst;
        }

        return true;
    }
    **/

    if(pIH->biHeight < 0 && pIH->biBitCount >= 8)
    {
        // The data is in the correct format already in the file.
        m_pData  = new ubyte[dwHeight * m_pitch];
        memcpy( m_pData, ((ubyte *)pvData) + dwOffset, dwHeight * m_pitch );
        m_bDeleteData = true;

        return true;
    }

    // Data in file needs to be converted.. so lets allocate the destination
    // buffer which will contain the image..

    m_bDeleteData = true;
    m_pData  = new ubyte[dwHeight * m_pitch];

    UNALIGNED ubyte *pbSrc, *pbDest, *pbDestMin, *pbDestLim, *pbDestLine;

    pbSrc = ((ubyte *) pvData) + dwOffset;

    if(pIH->biHeight < 0)
    {
        dwDstInc = m_pitch;
        pbDest = (ubyte *) m_pData;
    }
    else
    {
        dwDstInc = 0 - (ulong)m_pitch;
        pbDest = (ubyte *) m_pData + (dwHeight - 1) * m_pitch;
    }

    pbDestLine = pbDest;
    pbDestMin = (ubyte *) m_pData;
    pbDestLim = (ubyte *) m_pData + dwHeight * m_pitch;

    if(BI_RLE4 == pIH->biCompression)
    {
        // RLE4. Always encoded upsidedown.

        while(pbDest >= pbDestMin)
        {
            if(0 == pbSrc[0])
            {
                switch(pbSrc[1])
                {
                    case 0:
                        ASSERT(pbDest == pbDestLine + dwWidth);
                        pbDestLine -= m_pitch;
                        pbDest = pbDestLine;
                        break;

                    case 1:
                        pbDest = pbDestMin - m_pitch;
                        break;

                    case 2:
                        pbDest += pbSrc[2] - pbSrc[3] * m_pitch;
                        pbSrc += 2;
                        break;

                    default:
                        for(int i = 0; i < pbSrc[1]; i++)
                            pbDest[i] = (ubyte)( (i & 1) ?  (pbSrc[2 + (i >> 1)] & 0x0f) : (pbSrc[2 + (i >> 1)] >> 4) );

                        pbDest += pbSrc[1];
                        pbSrc += ((pbSrc[1] >> 1) + 1) & ~1;
                        break;
                }
            }
            else
            {
                for(int i = 0; i < pbSrc[0]; i++)
                    pbDest[i] = (ubyte)( (i & 1) ? (pbSrc[1] & 0x0f) : (pbSrc[1] >> 4) );

                pbDest += pbSrc[0];
            }

            pbSrc += 2;
        }

        return true;
    }

    if(pIH->biCompression == BI_RLE8)
    {
        // RLE8. Always encoded upsidedown.

        while(pbDest >= pbDestMin)
        {
            if(0 == pbSrc[0])
            {
                switch(pbSrc[1])
                {
                    case 0:
                        ASSERT(pbDest == pbDestLine + dwWidth);
                        pbDestLine -= m_pitch;
                        pbDest = pbDestLine;
                        break;

                    case 1:
                        pbDest = pbDestMin - m_pitch;
                        break;

                    case 2:
                        pbDest += pbSrc[2] - pbSrc[3] * m_pitch;
                        pbSrc += 2;
                        break;

                    default:
                        memcpy(pbDest, pbSrc + 2, pbSrc[1]);
                        pbDest += pbSrc[1];
                        pbSrc += (pbSrc[1] + 1) & ~1;
                        break;
                }
            }
            else
            {
                memset(pbDest, pbSrc[1], pbSrc[0]);
                pbDest += pbSrc[0];
            }

            pbSrc += 2;
        }

        return true;
    }


    if(1 == pIH->biBitCount)
    {
        while(pbDest >= pbDestMin && pbDest < pbDestLim)
        {
            for(UINT i = 0; i < dwWidth; i++)
                pbDest[i] = ubyte((pbSrc[i >> 3] >> (7 - (i & 7))) & 1);

            pbDest += dwDstInc;
            pbSrc  += dwSrcInc;
        }

        return true;
    }

    if(4 == pIH->biBitCount)
    {
        while(pbDest >= pbDestMin && pbDest < pbDestLim)
        {
            for(UINT i = 0; i < dwWidth; i++)
                pbDest[i] = (ubyte)( (i & 1) ? pbSrc[i >> 1] & 0x0f : (pbSrc[i >> 1] >> 4) );

            pbDest += dwDstInc;
            pbSrc  += dwSrcInc;
        }

        return true;
    }


    while(pbDest >= pbDestMin && pbDest < pbDestLim)
    {
        memcpy(pbDest, pbSrc, dwWidthBytes);

        pbDest += dwDstInc;
        pbSrc  += dwSrcInc;
    }

    return true;
}




//-----------------------------------------------------------------------------
// Name: struct TGAHEADER
// Desc: Defines the header format for TGA files
//-----------------------------------------------------------------------------
#pragma pack(1)
struct TGAHEADER
{
    ubyte IDLength;
    ubyte ColormapType;
    ubyte ImageType;

    WORD wColorMapIndex;
    WORD wColorMapLength;
    ubyte bColorMapBits;

    WORD wXOrigin;
    WORD wYOrigin;
    WORD wWidth;
    WORD wHeight;
    ubyte PixelDepth;
    ubyte ImageDescriptor;
};
#pragma pack()




//-----------------------------------------------------------------------------
// Name: LoadTGA()
// Desc: Attempts to load the given data as a TGA file
//-----------------------------------------------------------------------------
bool BmpImage::LoadTGA( const void* pvData, ulong cbData )
{
    // Validate header.  TGA files don't seem to have any sort of magic number
    // to identify them.  Therefore, we will proceed as if this is a real TGA
    // file, until we see something we don't understand.

    ubyte*      pbData = (ubyte*)pvData;
    TGAHEADER* pFH    = (TGAHEADER*)pbData;

    if( cbData < sizeof(TGAHEADER) )
        return false;

    if( pFH->ColormapType & ~0x01 )
        return false;

    if( pFH->ImageType & ~0x0b )
        return false;

    if( !pFH->wWidth || !pFH->wHeight )
        return false;


#pragma PRAGMA_MESSAGE("BAD : BmpImage promotes 24 bit to 32 on load; that should be optional")
	//	@@ dunno if it would work to just disable this block
	//	 promoting 24->32 is pretty good by default cuz it makes me all nice and aligned
	//	 though fatter in memory so w/e

    // Colormap size and format
    UINT uColorMapBytes = ((UINT) pFH->bColorMapBits + 7) >> 3;
    D3DFORMAT ColorMapFormat = D3DFMT_UNKNOWN;

    if(pFH->ColormapType)
    {
        switch(pFH->bColorMapBits)
        {
            case 15: ColorMapFormat = D3DFMT_X1R5G5B5; break;
            case 16: ColorMapFormat = D3DFMT_A1R5G5B5; break;
            case 24: ColorMapFormat = D3DFMT_X8R8G8B8; break;
            case 32: ColorMapFormat = D3DFMT_A8R8G8B8; break;
            default: return false;
        }
    }


    // Image size and format
    UINT uBytes = ((UINT) pFH->PixelDepth + 7) >> 3;
    D3DFORMAT Format = D3DFMT_UNKNOWN;

    switch(pFH->ImageType & 0x03)
    {
        case 1:
            if(!pFH->ColormapType)
                return false;

            switch(pFH->PixelDepth)
            {
                case 8: Format = D3DFMT_P8; break;
                default: return false;
            }
            break;

        case 2:
            switch(pFH->PixelDepth)
            {
                case 15: Format = D3DFMT_X1R5G5B5; break;
                case 16: Format = D3DFMT_A1R5G5B5; break;
                case 24: Format = D3DFMT_X8R8G8B8;   break;
                case 32: Format = D3DFMT_A8R8G8B8; break;
                default: return false;
            }
            break;

        case 3:
            switch(pFH->PixelDepth)
            {
                case 8: Format = D3DFMT_L8; break;
                default: return false;
            }
            break;

        default:
            return false;
    }

    bool bRLE         = (pFH->ImageType & 0x08) != 0;
    bool bTopToBottom = 0x20 == (pFH->ImageDescriptor & 0x20);
    bool bLeftToRight = 0x10 != (pFH->ImageDescriptor & 0x10);

    pbData += sizeof(TGAHEADER);
    cbData -= sizeof(TGAHEADER);


    // Skip ID
    if(cbData < pFH->IDLength)
        return false;

    pbData += pFH->IDLength;
    cbData -= pFH->IDLength;


    // Color map
    UINT cbColorMap = (UINT) pFH->wColorMapLength * uColorMapBytes;

    if(cbData < cbColorMap)
        return false;

    if(D3DFMT_P8 == Format)
    {
        if(pFH->wColorMapIndex + pFH->wColorMapLength > 256)
            return false;

        m_pPalette = new ColorDW[256];

        m_bDeletePalette = true;
        memset(m_pPalette, 0xff, 256 * sizeof(PALETTEENTRY));

        ubyte *pb = pbData;
        ColorDW *pColor = m_pPalette + pFH->wColorMapIndex;
        ColorDW *pColorLim = pColor + pFH->wColorMapLength;

        while(pColor < pColorLim)
        {
            UINT u, uA, uR, uG, uB;

            switch(ColorMapFormat)
            {
                case D3DFMT_X1R5G5B5:
                    u = *((WORD *) pb);

                    uA = 0xff;
                    uR = (u >> 10) & 0x1f;
                    uG = (u >>  5) & 0x1f;
                    uB = (u >>  0) & 0x1f;

                    uR = (uR << 3) | (uR >> 2);
                    uG = (uG << 3) | (uG >> 2);
                    uB = (uB << 3) | (uB >> 2);

                    pb += 2;
                    break;

                case D3DFMT_A1R5G5B5:
                    u = *((WORD *) pb);

                    uA = (u >> 15) * 0xff;
                    uR = (u >> 10) & 0x1f;
                    uG = (u >>  5) & 0x1f;
                    uB = (u >>  0) & 0x1f;

                    uR = (uR << 3) | (uR >> 2);
                    uG = (uG << 3) | (uG >> 2);
                    uB = (uB << 3) | (uB >> 2);

                    pb += 2;
                    break;

                case D3DFMT_X8R8G8B8:
                    uA = 0xff;
                    uR = pb[2];
                    uG = pb[1];
                    uB = pb[0];

                    pb += 3;
                    break;

                case D3DFMT_A8R8G8B8:
                    u = *((ulong *) pb);

                    uA = (u >> 24) & 0xff;
                    uR = (u >> 16) & 0xff;
                    uG = (u >>  8) & 0xff;
                    uB = (u >>  0) & 0xff;

                    pb += 4;
                    break;

				default:
					FAIL("Bad default case");
                    uA = 0xff;
                    uR = 0xff;
                    uG = 0xff;
                    uB = 0xff;
					break;
			}
            
            pColor->Set(uR,uG,uB,uA);
        
            pColor++;
        }
    }

    pbData += cbColorMap;
    cbData -= cbColorMap;


    // Image data
    UINT cbImage;
    if(Format == D3DFMT_X8R8G8B8)
        cbImage = (UINT) pFH->wWidth * (UINT) pFH->wHeight * (uBytes+1);
    else
        cbImage = (UINT) pFH->wWidth * (UINT) pFH->wHeight * uBytes;

    m_format  = Format;
    m_pData   = pbData;
    m_pitch   = (UINT) pFH->wWidth * uBytes;

    m_width  = pFH->wWidth;
    m_height = pFH->wHeight;


    if(!bRLE && bTopToBottom && bLeftToRight )
    {
        // Data is already in a format usable to D3D.. no conversion is necessary
        
        m_pData = new ubyte[cbImage];
        memcpy( m_pData, pbData, min(cbData, cbImage) );
        m_bDeleteData = true;

        pbData += cbImage;
        cbData -= cbImage;
    }
    else
    {
        // Image data is compressed, or does not have origin at top-left
        m_pData = new ubyte[cbImage];

        m_bDeleteData = true;


        ubyte *pbDestY = bTopToBottom ? (ubyte *) m_pData : ((ubyte *) m_pData + (pFH->wHeight - 1) * m_pitch);

        for(UINT uY = 0; uY < pFH->wHeight; uY++)
        {
            ubyte *pbDestX = bLeftToRight ? pbDestY : (pbDestY + m_pitch - uBytes);

            for(UINT uX = 0; uX < pFH->wWidth; )
            {
                bool bRunLength;
                UINT uCount;

                if(bRLE)
                {
                    if(cbData < 1)
                        return false;

                    bRunLength = (*pbData & 0x80) != 0;
                    uCount = (*pbData & 0x7f) + 1;

                    pbData++;
                    cbData--;
                }
                else
                {
                    bRunLength = false;
                    uCount = pFH->wWidth;
                }

                uX += uCount;

                while(uCount--)
                {
                    if(cbData < uBytes)
                        return false;

                    memcpy(pbDestX, pbData, uBytes);

                    if(!bRunLength)
                    {
                        pbData += uBytes;
                        cbData -= uBytes;
                    }

                    pbDestX = bLeftToRight ? (pbDestX + uBytes) : (pbDestX - uBytes);
                }

                if(bRunLength)
                {
                    pbData += uBytes;
                    cbData -= uBytes;
                }
            }

            pbDestY = bTopToBottom ? (pbDestY + m_pitch) : (pbDestY - m_pitch);
        }
    }

    if(Format == D3DFMT_X8R8G8B8)
    {
        //convert from 24-bit R8G8B8 to 32-bit X8R8G8B8
        // do the conversion in-place
        ubyte *pSrc, *pDst;
        pSrc = (ubyte *)m_pData + (m_height)*(m_width*uBytes) - uBytes;
        pDst = (ubyte *)m_pData + (m_height)*(m_width*(uBytes+1)) - (uBytes+1);
            
        while(pSrc >= m_pData)
        {
            *(pDst+3) = 0xff;       //A
            *(pDst+2) = *(pSrc+2);  //R
            *(pDst+1) = *(pSrc+1);  //G
            *(pDst+0) = *pSrc;      //B
            pSrc -= 3;
            pDst -= 4;
        }
        m_pitch   = m_width * (uBytes+1);
    }

    return true;
}




//-----------------------------------------------------------------------------
// Name: Anonymous enum
// Desc: Enumerations used for loading PPM files
//-----------------------------------------------------------------------------
enum
{
    PPM_WIDTH, PPM_HEIGHT, PPM_MAX, PPM_DATA_R, PPM_DATA_G, PPM_DATA_B
};




//-----------------------------------------------------------------------------
// Name: LoadPPM()
// Desc: Attempts to load the given data as a PPM file
//-----------------------------------------------------------------------------
bool BmpImage::LoadPPM( const void* pvData, ulong cbData )
{
    ubyte *pbData = (ubyte *)pvData;

    // Check header
    bool bAscii;

    if(cbData < 2)
        return false;

    if('P' == pbData[0] && '3' == pbData[1])
        bAscii = true;
    else if('P' == pbData[0] && '6' == pbData[1])
        bAscii = false;
    else
        return false;

    pbData += 2;
    cbData -= 2;

    // Image data
    UINT uMode   = PPM_WIDTH;
    UINT uWidth  = 0;
    UINT uHeight = 0;
    UINT uMax    = 255;

    ulong* pdw    = NULL;
	ulong* pdwLim = NULL;

    while(cbData)
    {
        if(!bAscii && PPM_DATA_R == uMode)
        {
            // Binary image data
            if(uMax > 255)
                return false;

            if(cbData > 1 && '\r' == *pbData)
            {
                pbData++;
                cbData--;
            }

            pbData++;
            cbData--;

            while(cbData && pdw < pdwLim)
            {
                *pdw++ = ((255 * pbData[0] / uMax) << 16) |
                         ((255 * pbData[1] / uMax) <<  8) |
                         ((255 * pbData[2] / uMax) <<  0) | 0xff000000;

                pbData += 3;
                cbData -= 3;
            }

            if(pdw != pdwLim)
                return false;

            return true;
        }
        if(isspace(*pbData))
        {
            // Whitespace
            pbData++;
            cbData--;
        }
        else if('#' == *pbData)
        {
            // Comment
            while(cbData && '\n' != *pbData)
            {
                pbData++;
                cbData--;
            }

            pbData++;
            cbData--;
        }
        else
        {
            // Number
            UINT u = 0;

            while(cbData && !isspace(*pbData))
            {
                if(!isdigit(*pbData))
                    return false;

                u = u * 10 + (*pbData - '0');

                pbData++;
                cbData--;
            }

            switch(uMode)
            {
                case PPM_WIDTH:
                    uWidth = u;

                    if(0 == uWidth)
                        return false;

                    break;

                case PPM_HEIGHT:
                    uHeight = u;

                    if(0 == uHeight)
                        return false;

                    m_pData = new ubyte[uWidth * uHeight * sizeof(ulong)];

                    m_bDeleteData = true;

                    pdw = (ulong *) m_pData;
                    pdwLim = pdw + uWidth * uHeight;

                    m_format = D3DFMT_X8R8G8B8;
                    m_pitch  = uWidth * sizeof(ulong);

                    m_width  = uWidth;
                    m_height = uHeight;
                    break;

                case PPM_MAX:
                    uMax = u;

                    if(0 == uMax)
                        return false;

                    break;

                case PPM_DATA_R:
                    if(pdw >= pdwLim)
                        return false;

                    *pdw  = ((u * 255 / uMax) << 16) | 0xff000000;
                    break;

                case PPM_DATA_G:
                    *pdw |= ((u * 255 / uMax) <<  8);
                    break;

                case PPM_DATA_B:
                    *pdw |= ((u * 255 / uMax) <<  0);

                    if(++pdw == pdwLim)
                        return true;

                    uMode = PPM_DATA_R - 1;
                    break;
            }

            uMode++;
        }
    }

    return false;
}

void BmpImage::MakePaletteIndicesColors()
{
	if ( m_format != D3DFMT_P8 )
	{
		return;
	}
	
	for(int i=0;i<256;i++)
	{
		m_pPalette[i].Set(i,i,i);
	}
}

void BmpImage::MakeGreyPaletteL8() // convert P8 grey to L8
{
	if ( m_format != D3DFMT_P8 )
	{
		return;
	}
	
	for(int i=0;i<256;i++)
	{
		if ( m_pPalette[i].GetR() != m_pPalette[i].GetG() ||
			 m_pPalette[i].GetB() != m_pPalette[i].GetG() )
		{
			// not grey
			return;
		}
	}
	
	// it's grey !

    // Loop through all texels and get 32-bit color from the 8-bit palette index
    for( ulong y=0; y<m_height; y++ )
    {
	    ubyte * pData = m_pData + y * m_pitch;
        for( ulong x=0; x<m_width; x++ )
        {
            ubyte  index = *pData;

			*pData++ = m_pPalette[index].GetR();
        }
    }

    // Delete the old palette
    if( m_bDeletePalette )
        delete[] m_pPalette;
    m_pPalette       = NULL;
    m_bDeletePalette = false;

    m_format = D3DFMT_L8;	
}

void BmpImage::ForceRGB24orL8()
{
	MakeGreyPaletteL8();
	
	if ( m_format == D3DFMT_L8 )
		return;

	Force24Bit();
}

void Copy(BmpImage * to,BmpImage * fm)
{
    to->m_pData			=fm->m_pData			;
    to->m_bDeleteData	=fm->m_bDeleteData	;
						 
    to->m_pPalette		=fm->m_pPalette		;
    to->m_bDeletePalette=fm->m_bDeletePalette;
						 
    to->m_format		=fm->m_format		;
    to->m_width			=fm->m_width			;
    to->m_height		=fm->m_height		;
    to->m_pitch			=fm->m_pitch			;
}

void BmpImage::Force24Bit()
{
	if ( m_format == D3DFMT_R8G8B8 )
	{
		return;
	}
	
	// copy this
	BmpImage prev;
	Copy(&prev,this);
	
	m_pPalette = NULL;
	m_bDeletePalette = false;
	m_format = D3DFMT_R8G8B8;
	m_pitch = m_width * 3;
	m_pData = new unsigned char [m_pitch * m_height];
	m_bDeleteData = true;

	for(int y=0;y<m_height;y++)
	{
		ubyte * ptr = ((ubyte *)m_pData) + y *m_pitch;
		for(int x=0;x<m_width;x++)
		{
			ColorDW cdw = prev.GetColor(x,y);
			ptr[0] = cdw.GetB();
			ptr[1] = cdw.GetG();
			ptr[2] = cdw.GetR();
			ptr += 3;
		}
	}
	
	// prev will now destruct
	prev.Release();
}

void BmpImage::Force32Bit()
{
	if ( Is32Bit() )
	{
		return;
	}
	
	// copy this
	BmpImage prev;
	Copy(&prev,this);
	
	m_pPalette = NULL;
	m_bDeletePalette = false;
	m_format = D3DFMT_A8R8G8B8;
	m_pitch = m_width * 4;
	m_pData = new unsigned char [m_pitch * m_height];
	m_bDeleteData = true;

	for(int y=0;y<m_height;y++)
	{
		ubyte * ptr = ((ubyte *)m_pData) + y *m_pitch;
		ColorDW * pul = (ColorDW *)ptr;
		for(int x=0;x<m_width;x++)
		{
			ColorDW cdw = prev.GetColor(x,y);
			pul[x] = cdw;
		}
	}
	
	// prev will now destruct
	prev.Release();
}

void BmpImage::FlipVertical()
{
	for(int y=0;;y++)
	{
		int yy = m_height -1 - y;
		if ( yy <= y )
			break;
		for(int x=0;x<m_width;x++)
		{
			ColorDW cy = GetColor(x,y);
			ColorDW cyy = GetColor(x,yy);
			SetColor(x,y,cyy);
			SetColor(x,yy,cy);
		}
	}
}

void BmpImage::FlipVerticalAndBGR()
{
	for(int y=0;;y++)
	{
		int yy = m_height -1 - y;
		if ( yy < y ) // need to do the y == yy also to get the BGR
			break;
		for(int x=0;x<m_width;x++)
		{
			ColorDW cy = GetColor(x,y);
			cy.Set( cy.GetB(), cy.GetG(), cy.GetR(), cy.GetA() );
			ColorDW cyy = GetColor(x,yy);
			cyy.Set( cyy.GetB(), cyy.GetG(), cyy.GetR(), cyy.GetA() );
			SetColor(x,y,cyy);
			SetColor(x,yy,cy);
		}
	}
}

void BmpImage::Allocate32Bit(int w,int h)
{
	Release();

	m_format = D3DFMT_A8R8G8B8;
	m_width = w;
	m_height = h;
	m_pitch = m_width * 4;
	m_pData = new unsigned char [m_pitch * m_height];
	m_bDeleteData = true;
}

void BmpImage::Allocate24Bit(int w,int h)
{
	Release();

	m_format = D3DFMT_R8G8B8;
	m_width = w;
	m_height = h;
	m_pitch = m_width * 3;
	m_pData = new unsigned char [m_pitch * m_height];
	m_bDeleteData = true;
}

void BmpImage::Allocate8Bit(int w,int h)
{
	Release();

	m_format = D3DFMT_L8;
	m_width = w;
	m_height = h;
	m_pitch = m_width;
	m_pData = new unsigned char [m_pitch * m_height];
	m_bDeleteData = true;
}

bool BmpImage::Is24Bit() const
{
	return m_format == D3DFMT_R8G8B8;
}

bool BmpImage::Is32Bit() const
{
	return m_format == D3DFMT_A8R8G8B8 || m_format == D3DFMT_X8R8G8B8;
}

bool BmpImage::IsGrey8() const
{
	/// or _P8 with a grey palette !!
	 // (use MakeGreyPaletteL8 first)
	return m_format == D3DFMT_L8;
}

void	BmpImage::GetTextureInfo(TextureInfo * pInfo) const
{
	pInfo->Reset();

	pInfo->m_width = m_width;
	pInfo->m_height = m_height;
	pInfo->m_gammaCorrected = true;
	pInfo->m_eEdgeMode = TextureInfo::eEdge_Mirror;
	pInfo->m_alpha = TextureInfo::eAlpha_None;
	pInfo->m_usage = TextureInfo::eUsage_Color;
	pInfo->m_usage = TextureInfo::eUsage_Color;
	pInfo->m_format = m_format;
		
	switch(m_format)
	{
	case D3DFMT_X8R8G8B8:
	case D3DFMT_R8G8B8:
		break;
	case D3DFMT_A8R8G8B8:
		pInfo->m_alpha = TextureInfo::eAlpha_Blend;
		break;
	case D3DFMT_L8:
//		pInfo->m_format = TextureInfo::eFormat_Grey;
		break;
	case D3DFMT_P8:
//		pInfo->m_format = TextureInfo::eFormat_Palette;
		break;
	default:
		//FAIL("Unhandled D3DFMT in BmpImage::FillBasegTextureInfo!");
		break;
	}
}

void BmpImage::Transpose()
{
	if ( m_format != D3DFMT_R8G8B8 )
	{
		FAIL("Only 24bit");
		return;
	}
	
	ubyte * old = (ubyte *) m_pData;
	bool oldDelete = m_bDeleteData;
	int oldPitch = m_pitch;
	
	Swap(m_width,m_height);
	
	m_pData = new ubyte [m_width * m_height * 3];
	m_bDeleteData = true;
	m_pitch = m_width * 3;
	
	for(int y=0;y<m_height;y++)
	{
		for(int x=0;x<m_width;x++)
		{
			ubyte * p1 = m_pData + y * m_pitch + x*3;
			ubyte * p2 = old + x * oldPitch + y*3;
			
			p1[0] = p2[0];
			p1[1] = p2[1];
			p1[2] = p2[2];
		}
	}
	
	if ( oldDelete )
	{
		delete [] old;
	}
}

void BmpImage::SetColor(const int x,const int y,const ColorDW c) const
{
	ASSERT( x >= 0 && x < m_width );
	ASSERT( y >= 0 && y < m_height );

	ubyte * ptr = (ubyte *) m_pData;
	ptr += y * m_pitch;

	switch(m_format)
	{
	case D3DFMT_X8R8G8B8:
	{
		ColorDW * pul = (ColorDW *) ptr;
		pul[x] = c;
		//pul[x].SetA( 255 );
		break;
	}
	case D3DFMT_A8R8G8B8:
	{
		ColorDW * pul = (ColorDW *) ptr;
		pul[x] = c;
		break;
	}
	case D3DFMT_R8G8B8:
	{
		ptr += x * 3;
		ptr[0] = c.GetB();
		ptr[1] = c.GetG();
		ptr[2] = c.GetR();
		break;
	}
	case D3DFMT_L8:
	{
		int p = (c.GetR() + 2*c.GetG() + c.GetB() + 2)/4;
		ptr[x] = (ubyte)p;
		break;
	}
	default:
		FAIL("Unhandled D3DFMT in BmpImage::SetColor!");
		break;
	}

	return;
}

ColorDW	BmpImage::GetColor(const int x,const int y) const
{
	ASSERT( x >= 0 && x < m_width );
	ASSERT( y >= 0 && y < m_height );

	const ubyte * ptr = (const ubyte *) m_pData;
	ptr += y * m_pitch;

	switch(m_format)
	{
	case D3DFMT_X8R8G8B8:
	{
		const ColorDW * pul = (const ColorDW *) ptr;
		ColorDW dw( pul[x] );
		dw.SetA( 255 );
		return dw;
	}
	case D3DFMT_A8R8G8B8:
	{
		const ColorDW * pul = (const ColorDW *) ptr;
		return pul[x];
	}
	case D3DFMT_R8G8B8:
	{
		ptr += x * 3;
		return ColorDW( ptr[2], ptr[1], ptr[0] );
		break;
	}
	/*
	case D3DFMT_X1R5G5B5:
	{
		ptr += x * 2;
		uword w = *((uword *)ptr);
		
	}
	*/
	case D3DFMT_P8:
	{
		ptr += x;
		int p = *ptr;
		ASSERT(m_pPalette);
		
		return m_pPalette[p];
	}
	case D3DFMT_L8:
	{
		ptr += x;
		int p = *ptr;
		return ColorDW(p,p,p);
	}
	default:
		FAIL("Unhandled D3DFMT in BmpImage::GetColor!");
		break;
	}

	return ColorDW::debug;
}

ColorDW	BmpImage::GetColorMirror(const int x,const int y) const
{
	int xx = x, yy =y;
	
	if ( xx < 0 )
	{
		xx = -xx;
	}
	else if ( xx >= m_width )
	{
		xx = 2*m_width - xx - 2;
	}
	
	if ( yy < 0 )
	{
		yy = -yy;
	}
	else if ( yy >= m_height )
	{
		yy = 2*m_height - yy - 2;
	}
	
	return GetColor(xx,yy);
}

// u,v is from [0,0] to [w-1,h-1] ; 0,0 is the center of the first pixel
void BmpImage::SampleBilinear(float * into,const float u,const float v) const
{
	int x = (int)u;
	int y = (int)v;
	// sample in [x,y] to [x+1,y+1]
	float HX = u - x;
	float HY = v - y;
	float LX = 1.f - HX;
	float LY = 1.f - HY;
		
	ColorDW LL = GetColorMirror(x  ,y  );
	ColorDW LH = GetColorMirror(x  ,y+1);
	ColorDW HL = GetColorMirror(x+1,y  );
	ColorDW HH = GetColorMirror(x+1,y+1);

	into[0] = HX*(HY*HH.GetR() + LY*HL.GetR()) + LX*(HY*LH.GetR() + LY*LL.GetR());
	into[1] = HX*(HY*HH.GetG() + LY*HL.GetG()) + LX*(HY*LH.GetG() + LY*LL.GetG());
	into[2] = HX*(HY*HH.GetB() + LY*HL.GetB()) + LX*(HY*LH.GetB() + LY*LL.GetB());
	into[3] = HX*(HY*HH.GetA() + LY*HL.GetA()) + LX*(HY*LH.GetA() + LY*LL.GetA());
	
}

void BmpImage::SampleBilinear(ColorF * into,const float u,const float v) const
{
	float f[4];
	SampleBilinear(f,u,v);
	into->Set( ColorITOF(f[0]), ColorITOF(f[1]), ColorITOF(f[2]), ColorITOF(f[3]) );
}
	
ColorDW BmpImage::SampleBilinear(const float u,const float v) const
{
	ColorDW ret;
	float f[4];
	SampleBilinear(f,u,v);
	ret.Set( froundint(f[0]), froundint(f[1]), froundint(f[2]), froundint(f[3]) );
	return ret;
}

/** Write out the float image as a .bmp file. */
bool BmpImage::WriteBMP(const char* outfile)
{
	FILE* fp = fopen(outfile, "wb");
	if ( fp == NULL )
	{
		return false;
	}

	int	width = m_width;
	int	height = m_height;
	
	BITMAPFILEHEADER bmfh = { 0 };
	BITMAPINFOHEADER bmih = { 0 };

	bmih.biSize = sizeof(BITMAPINFOHEADER);

	bmih.biWidth = width;
	//	 this needs to be negative, but the morons at Adobe can't handle
	//		negative height BMPs !
	bmih.biHeight= height;

	bmih.biPlanes = 1;
	bmih.biCompression = 0;
	bmih.biXPelsPerMeter = bmih.biYPelsPerMeter = 10000;
	bmih.biClrImportant = bmih.biClrUsed = 0;
	bmih.biBitCount = 0;

	int palBytes = 0;

	switch(m_format)
	{
		case D3DFMT_P8:
		case D3DFMT_L8:
			bmih.biBitCount = 8; 
			palBytes = 1024;
			bmih.biClrImportant = bmih.biClrUsed = 256;
			break;
		/*
		case D3DFMT_X8R8G8B8:
		case D3DFMT_A8R8G8B8:
			bmih.biBitCount = 32;
			break;
		*/
		default:
			// everything else is written as 24 bit
			bmih.biBitCount = 24;
			break;
	}
	
	int bpp = (bmih.biBitCount/8);
	const int bmp_row_bytes = (bmih.biWidth * bpp + 3)&(~3);
	bmih.biSizeImage = bmp_row_bytes * abs(bmih.biHeight);

	bmfh.bfType = BM_TAG;
	bmfh.bfSize = sizeof(BITMAPFILEHEADER) + bmih.biSize + bmih.biSizeImage + palBytes;
	bmfh.bfOffBits = bmfh.bfSize - bmih.biSizeImage ;

	fwrite(&bmfh, 1, sizeof(bmfh), fp);
	fwrite(&bmih, 1, sizeof(bmih), fp);

	const int bmp_row_pad = bmp_row_bytes - (width * bpp);
	ASSERT( bmp_row_pad <= 3 );
	
	if ( bpp == 1 ) 
	{
		if ( m_pPalette )
		{
			fwrite(m_pPalette, 1, palBytes, fp);
		}
		else 
		{
			ColorDW pal[256];
			for(int i=0;i<256;i++) 
			{
				pal[i].Set(i,i,i,255);
			}
			fwrite(pal, 1, palBytes, fp);
		}
		
		// flip vertically to write bottom up :
		for(int y= height-1; y>=0; y--)
		{
			ubyte * ptr = m_pData + y*m_pitch;
			for(int x=0;x<width;x++)
			{
				fputc( *ptr++, fp );
			}
			
			for(int pad=0;pad<bmp_row_pad;pad++)
			{
				fputc( 0 , fp );
			}
		}
	}
	else
	{
		// flip vertically to write bottom up :
		for(int y= height-1; y>=0; y--)
		{
			for(int x=0;x<width;x++)
			{
				ColorDW cdw = GetColor(x,y);
				fputc( cdw.GetB(), fp );
				fputc( cdw.GetG(), fp );
				fputc( cdw.GetR(), fp );
			}
			
			for(int pad=0;pad<bmp_row_pad;pad++)
			{
				fputc( 0 , fp );
			}
		}
	}
	
	fclose(fp);
	return true;
}

/** Write out the float image as a .tga file. */
void BmpImage::WriteTGA(const char* outfile)
{
	FILE* fp = fopen(outfile, "wb");
	if ( fp == NULL )
	{
		return;
	}

	int	width = m_width;
	int	height = m_height;
	
	TGAHEADER header;
	memset(&header,0,sizeof(TGAHEADER));

	header.wWidth  = (WORD) width;
	header.wHeight = (WORD) height;
	header.PixelDepth = 32;
	header.ImageType = 2;
	header.ImageDescriptor = 8;

	fwrite(&header,sizeof(TGAHEADER),1,fp);

	for(int y= height-1; y>=0; y--)
	//for(int y=0;y<height;y++)
	{
		for(int x=0;x<width;x++)
		{
			ColorDW cdw = GetColor(x,y);
			fwrite( &cdw, 4,1, fp );
		}
	}

	fclose(fp);
}

END_CB

#include "BmpImage.h"
#include "BmpImageJpeg.h"
#include "FileUtil.h"

// Toggle here :
//#define USE_JPEGLIB
#undef USE_JPEGLIB

//-------------------------------------------------------------------

#ifdef USE_JPEGLIB

#pragma PRAGMA_MESSAGE("using jpeg-6b")

extern "C" {
#include "jpeg-6b/jpeglib.h"
}

#ifdef _DEBUG
//#pragma comment(lib,"c:/src/cblib/jpeg-6b/jpgd.lib")
#pragma comment(lib,"c:/src/cblib/jpeg-6b/jpg.lib")
#pragma comment(linker,"/nodefaultlib:libc.lib")
#else
#pragma comment(lib,"c:/src/cblib/jpeg-6b/jpg.lib")
#endif

#else

#pragma PRAGMA_MESSAGE("jpeg support disabled")

#endif // USE_JPEGLIB

//-------------------------------------------------------------------

START_CB

bool LoadJpeg(const char *, BmpImage *) 
{
#ifdef USE_JPEGLIB

	//const J_DITHER_MODE DITHER_MODE = JDITHER_FS;
	const J_DITHER_MODE DITHER_MODE = JDITHER_NONE;
	//const J_DITHER_MODE DITHER_MODE = JDITHER_ORDERED;

	const bool TWO_PASS_QUANTIZE = TRUE;
	//const bool TWO_PASS_QUANTIZE = FALSE;


    FILE *f = fopen(filename, "rb");
    if (f == NULL)
    {
		printf("fopen failed on %s\n",filename);
		 return false;
	}
	
    jpeg_decompress_struct cinfo = { 0 };
    jpeg_error_mgr jerr = { 0 };

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    jpeg_stdio_src(&cinfo, f);
    jpeg_read_header(&cinfo, TRUE);

    cinfo.dither_mode = DITHER_MODE;
    cinfo.two_pass_quantize = TWO_PASS_QUANTIZE;

    jpeg_start_decompress(&cinfo);

    int width = cinfo.image_width;
    int height = cinfo.image_height;
    int bpp = cinfo.num_components;
    int stride = width * bpp;

	// seems like IJG only does 2 rows at a time max anyway
	#define READ_ROWS	(8)
    JSAMPROW buffer[READ_ROWS] = { 0 };

    unsigned char *data = new unsigned char[height * stride];
    unsigned char *ptr = data; // + width * height * bpp;

    bool success = true;

    // XXX Maybe this will be faster if we read more than one
    // scanline at a time (requires setting up array of char **).
    while (cinfo.output_scanline < cinfo.output_height) 
    {
		for(int i=0;i<READ_ROWS;i++)
		{
	        buffer[i] = ptr + i *stride;
		}
        int num_scanlines = jpeg_read_scanlines(&cinfo, buffer, READ_ROWS);

        if (num_scanlines == 0) 
        {
            success = false;
            break;
        }

		//*
		// flip to BGR ; arg arg
		if ( bpp == 3 )
		{
			for(int y=0;y<num_scanlines;y++)
			{
				unsigned char * row = ptr + y * stride;
				for(int x=0;x<width;x++)
				{
					Swap(row[x*3+0],row[x*3+2]);
				}
			}
		}
		/**/
		
		ptr += num_scanlines * stride;
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    fclose(f);

    if (jerr.num_warnings) success = false;
    //if (bpp != 3) success = false;

    if (!success) 
    {
        delete [] data;
        return false;
    }

    result->m_pData = data;
    result->m_bDeleteData = true;
    result->m_format = BmpImage::GetFormatForBytes(bpp);
    result->m_width = width;
    result->m_height = height;
    result->m_pitch = stride;

    // Done.
    return true;

#else

	return false;

#endif // USE_JPEGLIB
}

bool TryOpens(const char *filename, BmpImage *result)
{
	if ( result->Load(filename) )
		return true;
	
	if ( stristr(filename,"jpg") || stristr(filename,"jpeg") )
		return LoadJpeg(filename,result);

	return false;
}

bool LoadGeneric(const char *filename, BmpImage *result)
{
	if ( FileExists(filename) )
		return TryOpens(filename,result);
		
	String s;
	
	s = filename;
	s += ".bmp";
	if ( FileExists(s.CStr()) )
		return TryOpens(s.CStr(),result);
	
	s = filename;
	s += ".tga";
	if ( FileExists(s.CStr()) )
		return TryOpens(s.CStr(),result);
	
	s = filename;
	s += ".jpg";
	if ( FileExists(s.CStr()) )
		return TryOpens(s.CStr(),result);
	
	return false;
}

BmpImagePtr BmpImage::Create(const char * fileName)
{
	BmpImagePtr ret( new BmpImage );
	if ( LoadGeneric(fileName,ret.GetPtr()) )
		return ret;
	else
		return BmpImagePtr(NULL);
}

END_CB


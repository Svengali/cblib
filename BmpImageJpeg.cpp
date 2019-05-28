#include "BmpImage.h"
#include "BmpImageJpeg.h"
#include "FileUtil.h"
#include "Log.h"
#include "cblib_config.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <d3d9.h> // for D3DFMT

// defaults if not set :
#ifndef CBLIB_USE_JPEGLIB
#define CBLIB_USE_JPEGLIB 0
#endif

#ifndef CBLIB_USE_PNGLIB
#define CBLIB_USE_PNGLIB  0
#endif

#ifndef CBLIB_USE_GIFLIB
#define CBLIB_USE_GIFLIB  0
#endif

//-------------------------------------------------------------------

#if CBLIB_USE_JPEGLIB

//#pragma PRAGMA_MESSAGE("using libjpg")

extern "C" {
#include "C:\src\cblib\External\libjpg/jpeglib.h"
}

#ifdef CB_64

#ifdef _DEBUG
#pragma comment(lib,"C:/src/cblib/External/libjpg/jpg64.lib")
#pragma comment(linker,"/nodefaultlib:libc.lib")
#pragma comment(linker,"/nodefaultlib:libcmt.lib")
#else
#pragma comment(lib,"C:/src/cblib/External/libjpg/jpg64.lib")
#endif

#else

#ifdef _DEBUG
// for speed don't bother with the debug lib :
//#pragma comment(lib,"C:/src/cblib/External/libjpg/jpgd.lib")
#pragma comment(lib,"C:/src/cblib/External/libjpg/jpg.lib")
#pragma comment(linker,"/nodefaultlib:libc.lib")
#pragma comment(linker,"/nodefaultlib:libcmt.lib")
#else
#pragma comment(lib,"C:/src/cblib/External/libjpg/jpg.lib")
#endif

#endif

#else // CBLIB_USE_JPEGLIB

#pragma PRAGMA_MESSAGE("jpeg support disabled")

#endif // CBLIB_USE_JPEGLIB

//-------------------------------------------------------------------

#if CBLIB_USE_PNGLIB

/*

God damn PNG is a cock muncher

This makes me depend on libpng12.dll and zlib1.dll

*/

#include "external/libpng/include/zlib.h"
#include "external/libpng/include/png.h"

#ifdef CB_64
#define PNG_NAME	"libpng13x64"
#define ZLIB_NAME	"zlib1x64"
#else
#define PNG_NAME	"libpng3"
#define ZLIB_NAME	"zlib1"
#endif

#else

#pragma PRAGMA_MESSAGE("png support disabled")

#endif // CBLIB_USE_PNGLIB

//-------------------------------------------------------------------

#if CBLIB_USE_GIFLIB

#include "../external/giflib/giflib-4.2.3/lib/gif_lib.h"

#ifdef CB_64
#ifndef _DEBUG
#pragma comment(lib,"C:\\src\\External\\giflib\\x64\\Release\\giflib.lib")
#else
#pragma comment(lib,"C:\\src\\External\\giflib\\x64\\Debug\\giflibd.lib")
#endif
#else
#ifndef _DEBUG
#pragma comment(lib,"C:\\src\\External\\giflib\\Release\\giflib.lib")
#else
#pragma comment(lib,"C:\\src\\External\\giflib\\Debug\\giflibd.lib")
#endif
#endif

#endif

//-------------------------------------------------------------------

START_CB

bool LoadJpeg(const char *filename, BmpImage *result) 
{
#if CBLIB_USE_JPEGLIB

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

	cinfo.quantize_colors = FALSE;
    cinfo.dither_mode = JDITHER_NONE;
    cinfo.do_fancy_upsampling = TRUE;
    cinfo.do_block_smoothing = FALSE;
	cinfo.dct_method = JDCT_ISLOW;
	//cinfo.dct_method = JDCT_FLOAT;

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

#endif // CBLIB_USE_JPEGLIB
}

//=======================================================


bool WriteJpeg(const char *filename, const BmpImage *source,int jqval)
{
#if CBLIB_USE_JPEGLIB

    FILE *f = fopen(filename, "wb");
    if (f == NULL)
    {
		printf("fopen failed on %s\n",filename);
		 return false;
	}
	
	if ( jqval <= 0 )
	{
		// parse from file name :
		jqval = get_last_number(filename);
		if ( jqval < 20 )
			jqval = 90;
	}
	jqval = Clamp(jqval,30,100);

	lprintf_v1("NOTE : WriteJpeg , Q = %d\n",jqval);
	
    jpeg_compress_struct cinfo = { 0 };
    jpeg_error_mgr jerr = { 0 };

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    jpeg_stdio_dest(&cinfo, f);
    //jpeg_read_header(&cinfo, TRUE);

	cinfo.image_width  = source->m_width;
	cinfo.image_height = source->m_height;
	
	// always write the JPEG as either 1 channel or 3 channel
	//	regardless of the BMP
	// because that's what's widely supported
	if ( source->GetBytesPerPel() == 1 )
	{
		cinfo.input_components = 1;
		cinfo.in_color_space = JCS_GRAYSCALE;	
	}
	else
	{
		cinfo.input_components = 3;
		cinfo.in_color_space = JCS_RGB;
	}
	
	jpeg_set_defaults(&cinfo);

    //cinfo.dither_mode = DITHER_MODE;
    //cinfo.two_pass_quantize = TWO_PASS_QUANTIZE;

	cinfo.optimize_coding = 1;
	cinfo.do_fancy_downsampling = 1;
	//cinfo.dct_method = JDCT_FLOAT;
	cinfo.dct_method = JDCT_ISLOW;

	jpeg_set_quality(&cinfo,jqval,FALSE);

	/*
	// set up qtables :
	for (int tblno = 0; tblno < NUM_QUANT_TBLS; tblno++) 
	{
	  cinfo.q_scale_factor[tblno] = jpeg_quality_scaling(jqval);
	}	
	jpeg_default_qtables(&cinfo, FALSE);
	/**/
  
	// set up chroma subsample :
    if ( cinfo.input_components > 1 )
    {
		cinfo.comp_info[0].h_samp_factor = 2;
		cinfo.comp_info[0].v_samp_factor = 2;
			
		for (int ci = 1; ci < MAX_COMPONENTS; ci++) 
		{
			cinfo.comp_info[ci].h_samp_factor = 1;
			cinfo.comp_info[ci].v_samp_factor = 1;
		}
	}
     
     // jpeg_simple_progression
     
    jpeg_start_compress(&cinfo,TRUE);

    int stride = cinfo.image_width * cinfo.input_components;

	// seems like IJG only does 2 rows at a time max anyway
    JSAMPROW rowptrs[1] = { 0 };

    unsigned char *data = new unsigned char[stride];

    bool success = true;

    // XXX Maybe this will be faster if we read more than one
    // scanline at a time (requires setting up array of char **).
    while (cinfo.next_scanline < cinfo.image_height )
    {
		// flip to BGR ; arg arg
		if ( cinfo.in_color_space == JCS_RGB )
		{
			int y = cinfo.next_scanline;
			int w = (int)cinfo.image_width;
			
			rowptrs[0] = data;
			unsigned char * p = data;
			
			for(int x=0;x<w;x++)
			{
				ColorDW color = source->GetColor(x,y);
			
				*p++ = (unsigned char) color.GetR();
				*p++ = (unsigned char) color.GetG();
				*p++ = (unsigned char) color.GetB();
			}
			
		}
		else
		{
			uint8 * pSource = source->m_pData + cinfo.next_scanline * source->m_pitch;
		
			rowptrs[0] = pSource;
		}
		
        int num_scanlines = jpeg_write_scanlines(&cinfo, rowptrs, 1);

        if (num_scanlines == 0) 
        {
            success = false;
            break;
        }
		
    }
       
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    delete [] data;

    fclose(f);

    if (jerr.num_warnings) success = false;
    //if (bpp != 3) success = false;

    if (!success) 
    {
        return false;
    }

    // Done.
    return true;

#else

	return false;

#endif // CBLIB_USE_JPEGLIB
}

//===================================================================

//=======================================================

#if CBLIB_USE_PNGLIB

//---------------------------------------------------------------------

template <typename t_func_type>
t_func_type GetImport( t_func_type * pFunc , const char * funcName, HMODULE hm )
{
    if ( *pFunc == 0 )
    {
        ASSERT( hm != 0 );
        t_func_type fp = (t_func_type) GetProcAddress( hm, funcName );
        // not optional :
        ASSERT_RELEASE_THROW( fp != 0 );
        *pFunc = fp;
    }
    return (*pFunc); 
}

#define CALL_IMPORT(name,hm) (*GetImport(&STRING_JOIN(fp_,name),STRINGIZE(name),hm))


static HMODULE hm_png = 0;
#define HMODULE_FAILED	(HMODULE) 1

static bool my_png_init()
{
	if ( hm_png ) 
	{
		return ( hm_png == HMODULE_FAILED ) ? false : true;
	}
	
	HMODULE hm_zl = LoadLibrary(ZLIB_NAME);
	if ( hm_zl == 0 )
	{
		lprintf("Couldn't load Zlib (%s)\n",ZLIB_NAME);
		hm_png = HMODULE_FAILED;
		return false;
	}
	
	hm_png = LoadLibrary(PNG_NAME);
	if ( hm_png == 0 )
	{
		lprintf("Couldn't load PNG lib (%s)\n",PNG_NAME);
		hm_png = HMODULE_FAILED;
		return false;
	}
	
	lprintf_v2("Using libpng.\n");
				
	return true;
}

#define CALL_PNG(name)		CALL_IMPORT(name,hm_png)

//---------------------------------------------------------------------

#undef PNG_EXPORT
#undef PNGARG

#define PNG_EXPORT(ret,func)	static ret (PNGAPI * STRING_JOIN(fp_,func))
#define PNGARG(args)			args = 0

// these are copied straight out of png.h :

PNG_EXPORT(png_voidp,png_get_io_ptr) PNGARG((png_structp png_ptr));

PNG_EXPORT(png_structp,png_create_read_struct)
   PNGARG((png_const_charp user_png_ver, png_voidp error_ptr,
   png_error_ptr error_fn, png_error_ptr warn_fn));
   
/* Allocate and initialize the info structure */
PNG_EXPORT(png_infop,png_create_info_struct)
   PNGARG((png_structp png_ptr));
   
PNG_EXPORT(void,png_destroy_read_struct) PNGARG((png_structpp
   png_ptr_ptr, png_infopp info_ptr_ptr, png_infopp end_info_ptr_ptr));
   
PNG_EXPORT(void,png_set_read_fn) PNGARG((png_structp png_ptr,
   png_voidp io_ptr, png_rw_ptr read_data_fn));
   
PNG_EXPORT(void,png_set_sig_bytes) PNGARG((png_structp png_ptr,
   int num_bytes));
   
PNG_EXPORT(void, png_read_png) PNGARG((png_structp png_ptr,
                        png_infop info_ptr,
                        int transforms,
                        png_voidp params));
PNG_EXPORT(void, png_write_png) PNGARG((png_structp png_ptr,
                        png_infop info_ptr,
                        int transforms,
                        png_voidp params));
                        
PNG_EXPORT(png_bytepp,png_get_rows) PNGARG((png_structp png_ptr,png_infop info_ptr));

/* Allocate and initialize png_ptr struct for writing, and any other memory */
PNG_EXPORT(png_structp,png_create_write_struct)
   PNGARG((png_const_charp user_png_ver, png_voidp error_ptr,
   png_error_ptr error_fn, png_error_ptr warn_fn));

/* free any memory associated with the png_struct and the png_info_structs */
PNG_EXPORT(void,png_destroy_write_struct)
   PNGARG((png_structpp png_ptr_ptr, png_infopp info_ptr_ptr));

PNG_EXPORT(void,png_set_write_fn) PNGARG((png_structp png_ptr,
   png_voidp io_ptr, png_rw_ptr write_data_fn, png_flush_ptr output_flush_fn));
      
PNG_EXPORT(void,png_set_IHDR) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 width, png_uint_32 height, int bit_depth,
   int color_type, int interlace_method, int compression_method,
   int filter_method));

PNG_EXPORT(void,png_set_filter) PNGARG((png_structp png_ptr, int method,
   int filters));
      
PNG_EXPORT(void,png_set_compression_level) PNGARG((png_structp png_ptr,
   int level));

PNG_EXPORT(void,png_set_compression_strategy)
   PNGARG((png_structp png_ptr, int strategy));
   
PNG_EXPORT(png_voidp,png_malloc) PNGARG((png_structp png_ptr,
   png_uint_32 size));
   
/* frees a pointer allocated by png_malloc() */
PNG_EXPORT(void,png_free) PNGARG((png_structp png_ptr, png_voidp ptr));

PNG_EXPORT(void,png_set_rows) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_bytepp row_pointers));
   
//---------------------------------------------------------------------

#include <setjmp.h>

static void my_png_read(png_structp png_ptr,png_bytep data, png_size_t length)
{
	FILE * fp = (FILE *) CALL_PNG(png_get_io_ptr)(png_ptr);
	
	fread(data,1,length,fp);
}

static void my_png_write(png_structp png_ptr,png_bytep data, png_size_t length)
{
	FILE * fp = (FILE *) CALL_PNG(png_get_io_ptr)(png_ptr);
	
	fwrite(data,1,length,fp);
}

static void my_png_flush(png_structp png_ptr)
{
}

//-----------------------------------------------------------

#endif


bool LoadPNG(const char *filename, BmpImage *result) 
{
#if CBLIB_USE_PNGLIB

	if ( ! my_png_init() )
		return false;

	// Checker's loader
	
	// BTW see IC's loader in NVTT - it's got gamma and all kinds of goodies

	/******************** png- Portable Network Graphics***********************/           
	//static png_structp png_ptr = NULL;
	//static png_infop info_ptr = NUL;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	png_bytep *row_pointers = NULL;
	FILE *fp = NULL;

	png_ptr = CALL_PNG(png_create_read_struct)(PNG_LIBPNG_VER_STRING, NULL,NULL,NULL);
	if (png_ptr == NULL) {
		return false;
	}

	info_ptr = CALL_PNG(png_create_info_struct)(png_ptr);
	if (info_ptr == NULL) {
		CALL_PNG(png_destroy_read_struct)(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return false;
	}

	#pragma warning (disable:4611) // setjmp & c++ exceptions
	if (setjmp(png_jmpbuf(png_ptr))) 
	{
		CALL_PNG(png_destroy_read_struct)(&png_ptr, &info_ptr, png_infopp_NULL);
		
		if ( fp )
		{
			fclose(fp);
		}
		
		return false;
	}
	#pragma warning (default:4611) // setjmp & c++ exceptions
	
	// Open the png file:
	fp = fopen(filename, "rb");
	if(!fp)
	{
		CALL_PNG(png_destroy_read_struct)(&png_ptr, &info_ptr, png_infopp_NULL);

		return false;
	}
	//png_init_io(png_ptr, fp);
	
	CALL_PNG(png_set_read_fn)(png_ptr, fp, my_png_read );
	
	CALL_PNG(png_set_sig_bytes)(png_ptr, 0);
	
	
	// Read the png file
	int transform = PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_BGR;
	/*
	#ifdef __LITTLEENDIAN__
	// PNG 16 bit is big endian, so if I'm on a little endian machine, swap it :	
	transform |= PNG_TRANSFORM_SWAP_ENDIAN;
	#endif
	*/
	CALL_PNG(png_read_png)(png_ptr, info_ptr, transform, png_voidp_NULL);

	int width    = info_ptr->width;
	int height   = info_ptr->height;
	int channels = info_ptr->channels;
	int bitsPerChannel = info_ptr->bit_depth;
	//ASSERT_ALWAYS( bitsPerChannel == 8 || bitsPerChannel == 16 );
	if ( bitsPerChannel < 8 )
		bitsPerChannel = 8; // PNG_TRANSFORM_PACKING does this for you
	int bytesPerChannel = bitsPerChannel/8;
	
	int stride = width * channels * bytesPerChannel;
	int size = stride * height;
	
	uint8 * data = new uint8 [ size];
		 	
	if ( ! data )
	{
		lprintf("failed to malloc memory for image : %d\n", size );

		CALL_PNG(png_destroy_read_struct)(&png_ptr, &info_ptr, png_infopp_NULL);
		
		if ( fp )
		{
			fclose(fp);
		}
				
		return false;
	}

	// Copy the read data into img
	row_pointers = CALL_PNG(png_get_rows)(png_ptr, info_ptr);
	for (int y=0; y < (int)height; y++) 
	{
		uint8 *rowpng = row_pointers[y];
		uint8 *rowimg = data + y * stride;
		memcpy(rowimg, rowpng, stride);
	}

	CALL_PNG(png_destroy_read_struct)(&png_ptr, &info_ptr, png_infopp_NULL);
	png_ptr = NULL;
	info_ptr = NULL;
	
	fclose(fp); fp = NULL;
		  
    result->m_pData = data;
    result->m_bDeleteData = true;
    result->m_format = BmpImage::GetFormatForBytes(channels);
    result->m_width = width;
    result->m_height = height;
    result->m_pitch = stride;
    
	// I don't support palettes yet :
    ASSERT( ! result->IsPalettized() );
    	    
	return true;

#else

	return false;

#endif // CBLIB_USE_PNGLIB
}

bool WritePNG(const char *filename, const BmpImage * source,int zlevel)
{
#if CBLIB_USE_PNGLIB

	if ( ! my_png_init() )
		return false;
		
	/******************** png- Portable Network Graphics***********************/           
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	png_uint_32  width = source->m_width;
	png_uint_32  height = source->m_height;
	FILE *fp = NULL;

	int color_type = 0;
	if ( source->IsGrey8() ) color_type = PNG_COLOR_TYPE_GRAY;
	else if ( source->Is24Bit() )color_type = PNG_COLOR_TYPE_RGB;
	else if ( source->Is32Bit() )color_type = PNG_COLOR_TYPE_RGBA;
	else return false;
	
	png_ptr = CALL_PNG(png_create_write_struct)(PNG_LIBPNG_VER_STRING, NULL,NULL,NULL);
	if (png_ptr == NULL) {
		return false;
	}

	info_ptr = CALL_PNG(png_create_info_struct)(png_ptr);
	if (info_ptr == NULL) {
		CALL_PNG(png_destroy_write_struct)(&png_ptr, png_infopp_NULL );
		return false;
	}

	#pragma warning (disable:4611) // setjmp & c++ exceptions
	if (setjmp(png_jmpbuf(png_ptr))) 
	{
		CALL_PNG(png_destroy_write_struct)(&png_ptr, &info_ptr);
		
		if ( fp )
		{
			fclose(fp);
		}
		
		return false;
	}
	#pragma warning (default:4611) // setjmp & c++ exceptions
	
	// Open the png file:
	fp = fopen(filename, "wb");
	if(!fp)
	{
		return false;
	}
	//png_init_io(png_ptr, fp);
	
	CALL_PNG(png_set_write_fn)(png_ptr, fp, my_png_write, my_png_flush );
	
	CALL_PNG(png_set_sig_bytes)(png_ptr, 0);

	//int filter_type = PNG_INTRAPIXEL_DIFFERENCING; // MNG - not baseline
	int filter_type = PNG_FILTER_TYPE_BASE;
	int bitsPerChannel = 8;
	
	CALL_PNG(png_set_IHDR)(png_ptr, info_ptr, width, height,
                     bitsPerChannel, color_type, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_BASE, filter_type);

    CALL_PNG(png_set_filter)(png_ptr, filter_type, PNG_ALL_FILTERS);

    CALL_PNG(png_set_compression_level)(png_ptr,
        //8); // 1-9 zlib setting
        zlevel);

    CALL_PNG(png_set_compression_strategy)(png_ptr,
        //Z_DEFAULT_STRATEGY);
		Z_FILTERED); // higher min match length, usually better for images

	// build rows
	void** rows = (void**)CALL_PNG(png_malloc)(png_ptr, sizeof(void*) * height);
	for (int y = 0; y < (int)height; ++y) 
	{
		rows[y] = source->m_pData + y * source->m_pitch;
	}
	CALL_PNG(png_set_rows)(png_ptr, info_ptr, (png_bytepp)rows);
	info_ptr->valid |= PNG_INFO_IDAT;

	
	// write the png file
	int transform = PNG_TRANSFORM_BGR;
	// PNG 16 bit is big endina, so if I'm on a little endian machine, swap it :	
	/*
	#ifdef __LITTLEENDIAN__
	transform |= PNG_TRANSFORM_SWAP_ENDIAN;
	#endif
	*/
	CALL_PNG(png_write_png)(png_ptr, info_ptr, transform, png_voidp_NULL);

	// clean up memory
	CALL_PNG(png_free)(png_ptr, rows);

	CALL_PNG(png_destroy_write_struct)(&png_ptr, &info_ptr);
	png_ptr = NULL;
	info_ptr = NULL;
	
	fclose(fp); fp = NULL;
    
	return true;

#else

	return false;

#endif // CBLIB_USE_PNGLIB
}

//===========================================================================

static void PrintGifError()
{
#if CBLIB_USE_GIFLIB
	char * str = GifErrorString();
	lprintf("GIF error : %s\n",str);
#endif
}

bool LoadGIF(const char *filename, BmpImage *result)
{
#if CBLIB_USE_GIFLIB
	static const int
	    InterlacedOffset[] = { 0, 4, 2, 1 }, /* The way Interlaced image should. */
		InterlacedJumps[] = { 8, 8, 4, 2 };    /* be read - offsets and jumps... */
    
    int	i, j, Row, Col, Width, Height, ExtCode, Count;
    GifRecordType RecordType;
    GifByteType *Extension;
    GifFileType *GifFile;

	if ((GifFile = DGifOpenFileName(filename)) == NULL) 
	{
	    PrintGifError();
	    return false;
	}

	int SHeight = GifFile->SHeight;
	int SWidth = GifFile->SWidth;

	result->Allocate8Bit(SWidth,SHeight);
	result->m_format = D3DFMT_P8;
	result->m_pPalette = new ColorDW[256];
	result->m_bDeletePalette = true;
	
	uint8 * pixels = result->m_pData;
	int pixels_pitch = result->m_pitch;
	
	int background = GifFile->SBackGroundColor;
	ASSERT( background >= 0 && background <= 255 );

	/* Set its color to BackGround. */
	memset(pixels,background,result->m_height * pixels_pitch);
	
	// GCE defaults if not found :
	bool transparency = false;
	int transparent_color = 255;
	int frame_delay_centiseconds = 0;
					
    /* Scan the content of the GIF file and load the image(s) in: */
    do
    {
    
	if (DGifGetRecordType(GifFile, &RecordType) == GIF_ERROR)
	{
	    PrintGifError();
	    break;
	}
	
	switch (RecordType)
	{
	    case IMAGE_DESC_RECORD_TYPE:
	    {
		if (DGifGetImageDesc(GifFile) == GIF_ERROR)
		{
		    PrintGifError();
		    break;
		}
		
		int ImageNum = GifFile->ImageCount;
		ImageNum; // starts at 1 , counts up
		
		Row = GifFile->Image.Top; /* Image Position relative to Screen. */
		Col = GifFile->Image.Left;
		Width = GifFile->Image.Width;
		Height = GifFile->Image.Height;
		
		//GifQprintf("\n%s: Image %d at (%d, %d) [%dx%d]:     ", PROGRAM_NAME, ++ImageNum, Col, Row, Width, Height);
		
		if (GifFile->Image.Left + GifFile->Image.Width > GifFile->SWidth ||
		   GifFile->Image.Top + GifFile->Image.Height > GifFile->SHeight) 
		{
		    lprintf("GIF : Image is not confined to screen dimension, aborted.\n");
		    //exit(EXIT_FAILURE);
		    break;
		}
		
		if (GifFile->Image.Interlace) 
		{
		    /* Need to perform 4 passes on the images: */
		    for (Count = i = 0; i < 4; i++)
		    {
				for (j = Row + InterlacedOffset[i]; j < Row + Height;
							 j += InterlacedJumps[i])
				{
					//GifQprintf("\b\b\b\b%-4d", Count++);
					// &ScreenBuffer[j][Col]
					uint8 * ptr = pixels + j * pixels_pitch + Col;
					if (DGifGetLine(GifFile, ptr,
						Width) == GIF_ERROR) 
					{
						PrintGifError();
						return false;
					}
				}
			}
		}
		else
		{
		    for (i = 0; i < Height; i++)
		    {
				//GifQprintf("\b\b\b\b%-4d", i);
				uint8 * ptr = pixels + Row * pixels_pitch + Col;
				Row++;
				// &ScreenBuffer[Row++][Col]
				if (DGifGetLine(GifFile, ptr,
					Width) == GIF_ERROR)
				{
					PrintGifError();
					return false;
				}
			}
		}
		
		// colormap could be different per frame
		/* Lets dump it - set the global variables required and do it: */
		ColorMapObject * ColorMap = (GifFile->Image.ColorMap
			? GifFile->Image.ColorMap
			: GifFile->SColorMap);
	    
		if (ColorMap == NULL)
		{
			lprintf("GIF : WARNING : Image does not have a colormap\n");
			result->MakePaletteIndicesColors();
		}
		else
		{    
			int ColorCount = MIN(256, ColorMap->ColorCount);
			for(int i=0;i<ColorCount;i++)
			{
				int r = ColorMap->Colors[i].Red;
				int g = ColorMap->Colors[i].Green;
				int b = ColorMap->Colors[i].Blue;
				result->m_pPalette[i].Set( r,g,b, 255 );
			}
		}
		
		if ( transparency )
		{
			result->m_pPalette[transparent_color].SetA(0);
		}
		
		}
		break;
		
	    case EXTENSION_RECORD_TYPE:
	    {
			/* Skip any extension blocks in file: */
			if (DGifGetExtension(GifFile, &ExtCode, &Extension) == GIF_ERROR)
			{
				PrintGifError();
				return false;
			}
			while (Extension != NULL)
			{
				/* Pascal strings notation (pos. 0 is len.). */
				
				if ( ExtCode == 255 )
				{
					// says "NETSCAPE" and loop count
				}
				else if ( ExtCode == 249 )
				{
					int len = Extension[0];
					ASSERT_RELEASE( len == 4 );
					int flags = Extension[1];
					transparency = flags & 1;
					transparent_color = Extension[4];
					// little endian 16 bit :
					frame_delay_centiseconds = *((uint16 *)(&Extension[2]));
					//lprintfvar(frame_delay_centiseconds);
					// 0 is common for frame_delay_centiseconds
					// but the gif should be shown slowly
				}
			
				if (DGifGetExtensionNext(GifFile, &Extension) == GIF_ERROR)
				{
					PrintGifError();
					return false;
				}
			}
		}
		break;

	    case TERMINATE_RECORD_TYPE:
		break;
		
	    default:		    /* Should be traps by DGifGetRecordType. */
		break;
	}
	
    } while (RecordType != TERMINATE_RECORD_TYPE);
    
    if (DGifCloseFile(GifFile) == GIF_ERROR)
    {
		PrintGifError();
		return false;
    }

	return true;
    
#else
	return false;
#endif // CBLIB_USE_GIFLIB
}

//===========================================================================

END_CB



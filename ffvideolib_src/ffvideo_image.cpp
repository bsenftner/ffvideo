

#include "ffvideo.h"


#ifdef __cplusplus
extern "C" {
#include "jpeglib.h"
#include "jerror.h"
}
#endif 

#include <turbojpeg.h>

#include "dlib/image_io.h"
#include <dlib/image_processing/generic_image.h>
#include <dlib/image_transforms.h>
//
using namespace dlib;

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"


#include <excpt.h>

// used when writing out jpegs:
typedef struct
{
	uint8_t r, g, b;
} FF_RGB;



////////////////////////////////////////////////////////////////////////
array2d<rgb_pixel>* DLIB_MakeImage(FFVideo_Image& im)
{
	array2d<rgb_pixel>* p_img = new array2d<rgb_pixel>((long)im.m_height, (long)im.m_width);
	array2d<rgb_pixel>& img = *p_img;

	for (uint32_t y = 0; y < im.m_height - 1; y++)
	{
	 	uint32_t iy = im.m_height - y - 1;

		for (uint32_t x = 0; x < im.m_width - 1; x++)
		{
			memcpy((void*)(&img[iy][x]), im.Pixel(x, y), sizeof(uint8_t) * 3); // copy 3 bytes
		}
	} 

	return p_img;
}

////////////////////////////////////////////////////////////////////////
void DLIB_DeleteImage(array2d<rgb_pixel>* p_img)
{
	delete p_img;
}


////////////////////////////////////////////////////////////////////////
void DLIB_SaveJpeg(const std::string& filename, array2d<rgb_pixel>* p_img, int32_t quality = 75)
{
	dlib::save_jpeg( *p_img, filename, quality = 75 );
}

/* this works but is slow in the scaling
//////////////////////////////////////////////////////////////////////////////
bool FFVideo_Image::SaveJpgTest( const char* fname, BCTime& timer, double& duration, int32_t quality, float scale_factor)
{
double scale_start = timer.micro();

array2d<rgb_pixel>* p_dlib_src_img = DLIB_MakeImage(*this);

int32_t  rescaled_width  = (int32_t)((float)m_width * scale_factor + 0.5f);
int32_t	 rescaled_height = (int32_t)((float)m_height * scale_factor + 0.5f);
//
array2d<rgb_pixel> img((long)rescaled_height, (long)rescaled_width);
//
resize_image( *p_dlib_src_img, img );

double scale_end = timer.micro();
duration = scale_end - scale_start;

const std::string filename(fname);

DLIB_SaveJpeg( filename, &img, quality);

DLIB_DeleteImage(p_dlib_src_img); 
return true;
} /* */


//////////////////////////////////////////////////////////////////////////////////////
// read a file into memory and return a pointer to the data:
//////////////////////////////////////////////////////////////////////////////////////
uint8_t *ReadFileBytes(char *fname, uint32_t *byteSize) 
{
	FILE    *stream;
	uint8_t *readBytes = NULL;

	if ((stream = fopen(fname, "rb")) != NULL)
	{
		// move file pointer to the end of the file: 
		if (fseek(stream, 0, SEEK_END) != 0)
			goto error;

		// Get position, which is the length of the file: 
		if ((*byteSize = (int)ftell(stream)) < 0)
			goto error;

		// increment size by 1 so I can put a NULL there: 
		*byteSize = *byteSize + 1;

		// move file pointer back to the file's beginning: 
		if (fseek(stream, 0, SEEK_SET) != 0)
			goto error;

		// readBytes = new char[(*byteSize)]; 
		readBytes = (uint8_t *)malloc(sizeof(uint8_t) * (*byteSize));
		if (!readBytes)
			goto error;

		if (fread(readBytes, (*byteSize) - 1, 1, stream) != 1)
			goto error1;

		fclose(stream);

		// put NULL char as last char in file's data: 
		readBytes[*byteSize - 1] = '\0';
	}

	return readBytes;

error1:
	free(readBytes);
error:
	fclose(stream);
	*byteSize = 0;
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////
bool WriteFileBytes(const char* path, uint8_t* write_data, uint32_t byte_count)
{
	FILE *fh = fopen(path, "wb");
	if (fh != NULL)
	{
		fwrite(write_data, sizeof(uint8_t)*byte_count, 1, fh);
		fclose(fh);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////
// returns true/false if success filling a byte array of RGBA data
////////////////////////////////////////////////////////////////////////////////
bool ReadJPEG(const char* pszInfile, uint8_t** ppOutImage, uint32_t& nOutRows, uint32_t& nOutCols)
{
	nOutRows = 0;
	nOutCols = 0;

	// open the file up for reading
	FILE* hInfile = fopen(pszInfile, "rb");
	if (NULL == hInfile)
	{
		printf("Unable to open file %s\n", pszInfile);
		return false;
	}

	// first check it's a real jpeg 
	// the jpeg lib does this but exist upon failure, we want something more graceful
	uint8_t c1, c2;
	fread(&c1, sizeof(char), 1, hInfile);
	fread(&c2, sizeof(char), 1, hInfile);
	if (c1 != 0xFF || c2 != 0xD8)
	{
		printf("ReadJPEG() Not a JPEG file first marker was %d %d\n", (int32_t)c1, (int32_t)c2);
		fclose(hInfile);
		return false;
	}

	// seek to beginning of file as it correctly passed the JPEG test
	fseek(hInfile, 0, SEEK_SET);

	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	jpeg_stdio_src(&cinfo, hInfile);

	jpeg_read_header(&cinfo, TRUE);

	jpeg_start_decompress(&cinfo);


	if (cinfo.output_components > 4)  // normal RGB
	{
		jpeg_destroy_decompress(&cinfo);
		printf("ReadJPEG(). Error, file format not supported!.\n");
		fclose(hInfile);
		return false;
	}

	uint32_t nRows = cinfo.output_height;
	uint32_t nCols = cinfo.output_width;

	int32_t nWidth = (int32_t)cinfo.output_components * nCols;
	JSAMPLE* pScanLines = new JSAMPLE[nWidth];

	uint8_t* pOutImage = new uint8_t[cinfo.output_height * cinfo.output_width * 4];

	int32_t nCurRow = cinfo.output_height - cinfo.output_scanline - 1;
	while (cinfo.output_scanline < cinfo.output_height)
	{
		// read the image one line at a time
		(void)jpeg_read_scanlines(&cinfo, (JSAMPARRAY)&pScanLines, (JDIMENSION)1);

		if (cinfo.output_components == 4)
		{
			for (uint32_t nCol = 0; nCol < nCols; nCol++)
			{
				int32_t nIndex = (nCurRow * nCols * 4) + (nCol * 4);
				int32_t nC = nCol * cinfo.output_components;

				pOutImage[nIndex] = pScanLines[nC];
				pOutImage[nIndex + 1] = pScanLines[nC + 1];
				pOutImage[nIndex + 2] = pScanLines[nC + 2];
				pOutImage[nIndex + 3] = pScanLines[nC + 3];
			}
		}
		else if (cinfo.output_components == 3)
		{
			for (uint32_t nCol = 0; nCol < nCols; nCol++)
			{
				int32_t nIndex = (nCurRow * nCols * 4) + (nCol * 4);
				int32_t nC = nCol * cinfo.output_components;

				pOutImage[nIndex] = pScanLines[nC];
				pOutImage[nIndex + 1] = pScanLines[nC + 1];
				pOutImage[nIndex + 2] = pScanLines[nC + 2];
				pOutImage[nIndex + 3] = 255; // set alpha to 255 if missing
			}
		}
		else
		{
			for (uint32_t nCol = 0; nCol < nCols; nCol++)
			{
				int32_t nIndex = (nCurRow * nCols * 4) + (nCol * 4);

				pOutImage[nIndex] = pScanLines[nCol];			// gray scale, set all pixels the same
				pOutImage[nIndex + 1] = pScanLines[nCol];
				pOutImage[nIndex + 2] = pScanLines[nCol];
				pOutImage[nIndex + 3] = 255;							// set alpha to 255 if missing
			}
		}
		nCurRow--;
	}

	__try
	{
		jpeg_finish_decompress(&cinfo);        // finish the decompression
		jpeg_destroy_decompress(&cinfo);       // and destroy the object (free the mem)
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		fclose(hInfile);
		delete[] pScanLines;	// don't leak memory!
		delete[] pOutImage;
		return false;
	}

	fclose(hInfile);

	*ppOutImage = pOutImage;
	nOutRows = nRows;
	nOutCols = nCols;

	delete[] pScanLines;

	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool SaveJpeg(const char* filepath, FFVideo_Image& image, int32_t quality = 80 )
{
	uint32_t total_pixels = image.m_width * image.m_height;

	FF_RGB* write_pixels = new FF_RGB[total_pixels];

	int32_t bytes_per_pixel = 4;

	uint32_t width = image.m_width;
	uint32_t height = image.m_height;

	// copying image data vertically flipped:
	for (uint32_t y = 0; y < height; y++)
	{
		uint8_t* srcScanLine = &image.mp_pixels[width * bytes_per_pixel * (height - (y + 1))];
		FF_RGB* dstScanLine = &write_pixels[width * y];

		for (uint32_t x = 0; x < width; x++)
		{
			dstScanLine[x].r = srcScanLine[0];
			dstScanLine[x].g = srcScanLine[1];
			dstScanLine[x].b = srcScanLine[2];
			srcScanLine += bytes_per_pixel;
		}
	}

	_set_errno(0);
	FILE* outfile = fopen(filepath, "wb");
	if (!outfile)
	{
		errno_t err;
		_get_errno(&err);

		delete[] write_pixels;
		return false;
	}

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, outfile);

	cinfo.image_height = height;
	cinfo.image_width = width;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, 80, TRUE);
	jpeg_start_compress(&cinfo, TRUE);

	JSAMPROW row_pointer;
	while (cinfo.next_scanline < cinfo.image_height)
	{
		row_pointer = (unsigned char*)(&write_pixels[cinfo.next_scanline * width]);
		jpeg_write_scanlines(&cinfo, &row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);        // finish the decompression
	jpeg_destroy_compress(&cinfo);       // and destroy the object (free the mem)

	delete[] write_pixels;
	fclose(outfile);

	return true;
}


////////////////////////////////////////////////////////////////////////////////
bool SaveJpegTurbo(const char* filepath, FFVideo_Image& image, int32_t jpegQual = 80 )
{
	uint32_t total_pixels = image.m_width * image.m_height;

	FF_RGB* rgb_pixels = new FF_RGB[total_pixels];

	int32_t bytes_per_pixel = 4;

	uint32_t width = image.m_width;
	uint32_t height = image.m_height;

	// copying image data vertically flipped from RGBA representation:
	for (uint32_t y = 0; y < height; y++)
	{
		uint8_t* srcScanLine = &image.mp_pixels[width * bytes_per_pixel * (height - (y + 1))];
		FF_RGB* dstScanLine = &rgb_pixels[width * y];

		for (uint32_t x = 0; x < width; x++)
		{
			dstScanLine[x].r = srcScanLine[0];
			dstScanLine[x].g = srcScanLine[1];
			dstScanLine[x].b = srcScanLine[2];
			srcScanLine += bytes_per_pixel;
		}
	}

	// libturbo-jpeg logic starts here:
	tjhandle handle = tjInitCompress();
	if(handle == NULL)
	{
		// no longer needed:
		delete[] rgb_pixels;

		const char *err = (const char *)tjGetErrorStr();
		// cerr << "TJ Error: " << err << " UNABLE TO INIT TJ Compressor Object\n";
		return false;
	}

	int32_t tjwidth = image.m_width;
	int32_t tjheight = image.m_height;
	int32_t nbands = 3;
	int32_t flags = 0;
	uint8_t* jpegBuf = NULL;
	int32_t pitch = width * nbands;
	int32_t pixelFormat = TJPF_RGB;
	int32_t jpegSubsamp = TJSAMP_422;
	unsigned long jpegSize = 0;

	int tj_stat = tjCompress2( handle, (const uint8_t*)rgb_pixels, tjwidth, pitch, tjheight,
														 pixelFormat, &(jpegBuf), &jpegSize, jpegSubsamp, jpegQual, flags);
	// no longer needed:
	delete[] rgb_pixels;

	if(tj_stat != 0)
	{
		const char *err = (const char *) tjGetErrorStr();
		// cerr << "TurboJPEG Error: " << err << " UNABLE TO COMPRESS JPEG IMAGE\n";
		tjDestroy(handle);
		handle = NULL;
		return false;
	}

	bool ret_val = WriteFileBytes( filepath, jpegBuf, jpegSize );

	tjFree( jpegBuf ); // its on disk now, free the RAM

	int32_t tjstat = tjDestroy(handle); //should deallocate data buffer
	handle = 0;

	return ret_val;
}


////////////////////////////////////////////////////////////////////////////////
FFVideo_Image::FFVideo_Image() : mp_pixels(NULL), m_height(0), m_width(0), m_type(1) { }

////////////////////////////////////////////////////////////////////////////////
FFVideo_Image::FFVideo_Image(uint8_t* p_pixels, uint32_t height, uint32_t width, uint32_t type)
	: mp_pixels(NULL), m_height(0), m_width(0), m_type(1)
{
	Clone(p_pixels, height, width, type);
}

////////////////////////////////////////////////////////////////////////////////
FFVideo_Image::~FFVideo_Image()
{
	Empty();
}

////////////////////////////////////////////////////////////////////////////////
FFVideo_Image::FFVideo_Image(const FFVideo_Image& im)
	: mp_pixels(NULL), m_height(0), m_width(0), m_type(1)
{
	Clone(im);
}

////////////////////////////////////////////////////////////////////////////////
// copy assignement operator
FFVideo_Image& FFVideo_Image::operator = (const FFVideo_Image& im)
{
	if (this != &im)
	{
		Clone(im);
	}
	return (*this);
}

////////////////////////////////////////////////////////////////////////////////
void FFVideo_Image::Empty(void)
{
	if (mp_pixels) delete[] mp_pixels;
	mp_pixels = NULL;
	m_height = 0;
	m_width = 0;
	m_type = 1;
}

////////////////////////////////////////////////////////////////////////////////
uint32_t FFVideo_Image::CalcSize(uint32_t height, uint32_t width, uint32_t type) const
{
	uint32_t n_pixels = height * width;
	if (type == 0 || type == 3) return n_pixels * 3;
	if (type == 1 || type == 4) return n_pixels * 4;
	if (type == 2) return n_pixels;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
uint32_t FFVideo_Image::Size(void) const
{
	return CalcSize( m_height, m_width, m_type );
}

////////////////////////////////////////////////////////////////////////////////
bool FFVideo_Image::Reallocate( uint32_t height, uint32_t width, uint32_t type )
{
	if (type > 4)
		 return false;

	uint32_t size = CalcSize( height, width, type );
	if (!size)
		 return false;

	// not already allocated
	if (!mp_pixels)
	{
		mp_pixels = new uint8_t[size]; // allocate
	}
	// check to see if the memory is already allocated
	else if (Size() != size)
	{
		delete[] mp_pixels; // remove previous allocation as it is not the correct size
		mp_pixels = new uint8_t[size]; // allocate
	}
	if (!mp_pixels)
	{
		m_height = 0;
		m_width = 0;
		return false;
	}
	m_height = height;
	m_width = width;
	m_type = type;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool FFVideo_Image::Clone(const FFVideo_Image& im)
{
	return Clone(im.mp_pixels, im.m_height, im.m_width, im.m_type);
}

////////////////////////////////////////////////////////////////////////////////
bool FFVideo_Image::Clone(uint8_t* p_pixels, uint32_t height, uint32_t width, uint32_t type)
{
	if (!p_pixels) return false; // can't allocate an empty image

	if (!Reallocate( height, width, type ))
		 return false;

	uint32_t size = CalcSize( height, width, type );
	if (!size)
		return false;

	memcpy(mp_pixels, p_pixels, size);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// expected to contain image data which is clipped to a sub-rect, making this image that sub-rect
bool FFVideo_Image::ClipToRect( uint32_t xmin, uint32_t ymin, uint32_t xmax, uint32_t ymax )
{
	if (!mp_pixels || xmin >= xmax || ymin >= ymax) 
	   return false;
		 
	uint32_t new_w = xmax - xmin + 1;
	uint32_t new_h = ymax - ymin + 1;

	if (xmax >= m_width || ymax >= m_height)
	   return false;

	uint32_t n_bytes_per_pixel(0);
	uint32_t n_bytes_per_old_row(0),        n_bytes_per_new_row(0);
	uint32_t n_pixels_per_old_row(m_width), n_pixels_per_new_row(new_w);

	if (m_type == 0 || m_type == 3) 
	{
		n_bytes_per_pixel = 3;	
	}
	else if (m_type == 1 || m_type == 4) 
	{
		n_bytes_per_pixel = 4;
	}
	else if (m_type == 2) 
	{
		n_bytes_per_pixel = 1;
	}
	n_bytes_per_old_row = n_pixels_per_old_row * n_bytes_per_pixel;
	n_bytes_per_new_row = n_pixels_per_new_row * n_bytes_per_pixel;

	// allocate new pixels for sub-rect:
	uint8_t* p_new_pixels = new uint8_t[n_bytes_per_new_row * new_h]; 

	for (uint32_t i = ymin, j = 0; i <= ymax; i++, j++)
	{
		// figure out pointer to subrect scanline in src data:
		uint8_t* left_src = &mp_pixels[ n_bytes_per_old_row * i + xmin * n_bytes_per_pixel];

		uint8_t* left_dst = &p_new_pixels[j * n_bytes_per_new_row];

		memcpy( left_dst, left_src, n_bytes_per_new_row ); 
	}

	delete [] mp_pixels;
	mp_pixels = p_new_pixels;
	m_width = new_w;
	m_height = new_h;

	return true;
}


////////////////////////////////////////////////////////////////////////////////
void FFVideo_Image::MirrorVertical(void)
{
	if (!mp_pixels) return; 

	uint32_t n_bytes_per_row = 0;
	uint32_t n_pixels_per_row = m_width;
	if (m_type == 0 || m_type == 3) n_bytes_per_row = n_pixels_per_row * 3;
	if (m_type == 1 || m_type == 4) n_bytes_per_row = n_pixels_per_row * 4;
	if (m_type == 2) n_bytes_per_row = n_pixels_per_row;

	uint8_t* p_row_pixels = new uint8_t[n_bytes_per_row]; // allocate
	
	for (uint32_t i = 0; i <= m_height / 2; i++)
	{
		if ( &mp_pixels[i * n_bytes_per_row] !=  &mp_pixels[ (m_height - i - 1) * n_bytes_per_row ])
		{
			memcpy( p_row_pixels, &mp_pixels[i * n_bytes_per_row], n_bytes_per_row ); 

			memcpy( &mp_pixels[ i * n_bytes_per_row ], &mp_pixels[ (m_height - i - 1) * n_bytes_per_row ], n_bytes_per_row );

			memcpy( &mp_pixels[ (m_height - i - 1) * n_bytes_per_row ], p_row_pixels, n_bytes_per_row );
		}
	}

	delete [] p_row_pixels;
}

////////////////////////////////////////////////////////////////////////////////
bool FFVideo_Image::Load(const char* fname)
{
	Empty();

	// load a JPEG image
	mp_pixels = NULL;
	uint32_t height, width;
	if (!ReadJPEG(fname, &mp_pixels, height, width))
	{
		printf("Failed to load %s\n", fname);
		return false;
	}
	// set relevent values
	m_height = height;
	m_width = width;
	m_type = 1; // RGBA

	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool FFVideo_Image::SaveJpg(const char* fname, int32_t quality)
{
	return SaveJpeg(fname, *this, quality);
}

////////////////////////////////////////////////////////////////////////////////
bool FFVideo_Image::SaveJpgTurbo(const char* fname, int32_t quality)
{
	return SaveJpegTurbo(fname, *this, quality);
}

////////////////////////////////////////////////////////////////////////////////
bool FFVideo_Image::SetAlpha(uint8_t alpha)
{
	if (m_type != 1 && m_type != 4)
		return false;

	uint32_t size = CalcSize(m_height, m_width, m_type);
	if (!size)
		return false;

	for (uint32_t i = 3; i < size; i += 4)
		mp_pixels[i] = alpha;

	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool FFVideo_Image::SetAlphaTweak(uint8_t alpha_threshold)
{
	if (m_type != 1 && m_type != 4)
		return false;

	uint32_t size = CalcSize(m_height, m_width, m_type);
	if (!size)
		return false;

	uint8_t r, g, b, alpha;
	for (uint32_t i = 0; i < size; i += 4)
	{
		r = mp_pixels[i + 0];
		g = mp_pixels[i + 1];
		b = mp_pixels[i + 2];

		if (r <= alpha_threshold)
			alpha = 0;
		else alpha = 255;

		mp_pixels[i + 3] = alpha;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
uint8_t* FFVideo_Image::Pixel(uint32_t x, uint32_t y)
{
	if (m_type != 1 && m_type != 4 && mp_pixels)
		 return NULL;
	if (m_width > 0 && x >= m_width)
	   return NULL;
	if (m_height > 0 && y >= m_height)
	   return NULL;

	uint8_t* pixel = &mp_pixels[ y * m_width * 4 + x * 4 ];
	
	return pixel;
}


////////////////////////////////////////////////////////////////////////////////
bool FFVideo_Image::Rescale( uint32_t new_height, uint32_t new_width )
{
	if (m_type != 1 && m_type != 4 && mp_pixels)
		return NULL;

	FFVideo_Image rescaled;

	rescaled.Reallocate(new_height, new_width);

	stbir_resize_uint8(          mp_pixels,          m_width,          m_height, 0,
											rescaled.mp_pixels, rescaled.m_width, rescaled.m_height, 0, 4);

	Reallocate(new_height, new_width, m_type);

	uint32_t size = CalcSize( m_height, m_width, m_type );
	if (!size)
		return false;

	memcpy(mp_pixels, rescaled.mp_pixels, size);

	return true;
}

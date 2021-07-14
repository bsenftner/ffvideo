#pragma once
#ifndef _FFVIDEO_IMAGE_H_
#define _FFVIDEO_IMAGE_H_


//------------------------------------------------------------------------------
// an image in RAM
class FFVideo_Image
{
public:
	FFVideo_Image(uint8_t* p_pixels, uint32_t height, uint32_t width, uint32_t type);
	FFVideo_Image();
	~FFVideo_Image();

	// copy constructor 
	FFVideo_Image(const FFVideo_Image& im);

	// copy assignement operator
	FFVideo_Image& operator = (const FFVideo_Image& im);

	bool     Load(const char* fname);
	bool		 SaveJpg(const char* fname, int32_t quality = 80 );
	bool		 SaveJpgTurbo(const char* fname, int32_t quality = 80 );

	bool     Clone(const FFVideo_Image& im);
	bool     Clone(uint8_t* p_pixels, uint32_t height, uint32_t width, uint32_t type = 1);
	bool     Reallocate(uint32_t height, uint32_t width, uint32_t type = 1);
	void     MirrorVertical(void);
	void     Empty(void);
	uint32_t Size(void) const;
	uint32_t CalcSize(uint32_t height, uint32_t width, uint32_t type = 1) const;
	bool     SetAlpha(uint8_t alpha);
	bool     SetAlphaTweak(uint8_t alpha_threshold);
	uint8_t* Pixel(uint32_t x, uint32_t y);
	bool     Rescale( uint32_t new_height, uint32_t new_width );

	uint8_t* mp_pixels;
	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_type;    // 0=RGB, 1=RGBA, 2=Gray, 3=BGR, 4=BGRA
};


#endif // _FFVIDEO_IMAGE_H_
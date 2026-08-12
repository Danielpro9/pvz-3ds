#ifndef __IMAGELIB_H__
#define __IMAGELIB_H__

#include <string>

// Compact image forms: a paletted PNG stays as a 256-entry colour table plus one
// index byte per pixel instead of being expanded to w*h*4 at decode time. A
// 1400x600 background costs ~840KB this way rather than 3.36MB, and -- the part
// that matters more than the total -- the texture upload reads the indices
// directly (CopyImageToTexture* in platform/pc/graphics/GLInterface.cpp), so
// the 3.36MB *contiguous* block is never requested at all. Consumers that do
// need packed pixels call GetBits(), which expands on demand.
//
// This macro CHANGES THE LAYOUT of ImageLib::Image and the vtable of
// MemoryImage, so every translation unit has to agree on it. The condition is
// deliberately spelled out from compiler-defined platform macros in both
// headers rather than shared through an include, so the two cannot drift; the
// twin definition is in graphics/MemoryImage.h.
#if defined(PS2_PLATFORM) || defined(WII_PLATFORM)
#define SEXY_COMPACT_IMAGES
#endif

namespace ImageLib
{

class Image
{
public:
	int						mWidth;
	int						mHeight;
	uint32_t*				mBits;
#ifdef SEXY_COMPACT_IMAGES
	uint32_t*				mColorTable;
	unsigned char*			mColorIndices;
	// 24-bit RGB, PS2 only: its texture upload expands one piece at a time, and
	// no other target has a path that can read this form. Always NULL elsewhere.
	unsigned char*			mRGBBits;
	bool					mHasTrans;
	bool					mHasAlpha;
	// mBits was allocated with one spare uint32_t past w*h, so a MemoryImage
	// can adopt the buffer as-is and write its MEMORYCHECK_ID canary there
	// instead of allocating a duplicate w*h*4 block (see LoadLazyImageBits).
	bool					mBitsHasCanarySlot;
#endif

public:
	Image();
	virtual ~Image();

	int						GetWidth();
	int						GetHeight();
	uint32_t*				GetBits();
};

bool WriteJPEGImage(const std::string& theFileName, Image* theImage);
bool WritePNGImage(const std::string& theFileName, Image* theImage);
bool WriteTGAImage(const std::string& theFileName, Image* theImage);
bool WriteBMPImage(const std::string& theFileName, Image* theImage);
extern int gAlphaComposeColor;
extern bool gAutoLoadAlpha;
extern bool gIgnoreJPEG2000Alpha;  // I've noticed alpha in jpeg2000's that shouldn't have alpha so this defaults to true


Image* GetImage(const std::string& theFileName, bool lookForAlphaImage = true);

// Reads only the file header to get the decoded dimensions GetImage() would
// produce (including the IMG_DOWNSCALE division and the alpha-only-companion
// fallback), without decoding any pixels. Returns false if no file was found
// or the header could not be parsed.
bool GetImageDims(const std::string& theFileName, int& theWidth, int& theHeight);

//void InitJPEG2000();
//void CloseJPEG2000();
//void SetJ2KCodecKey(const std::string& theKey);

}

#endif //__IMAGELIB_H__

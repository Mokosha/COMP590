/**
 *  Copyright 2013 Mike Reed
 *
 *  COMP 590 -- Fall 2013
 */

#ifndef GBitmap_DEFINED
#define GBitmap_DEFINED

#include "GPixel.h"

struct GBitmap {
    int     fWidth;
    int     fHeight;
    GPixel* fPixels;
    size_t  fRowBytes;
};

#endif


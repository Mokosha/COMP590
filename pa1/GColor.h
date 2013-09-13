/**
 *  Copyright 2013 Mike Reed
 *
 *  COMP 590 -- Fall 2013
 */

#ifndef GColor_DEFINED
#define GColor_DEFINED

#include "GTypes.h"

/**
 *  Holds a nonpremultiplied color, with each component
 *  normalized to [0...1].
 */
class GColor {
public:
    float   fA, fR, fG, fB;
};

#endif

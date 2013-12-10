#ifndef GBLEND_H_
#define GBLEND_H_

#include "GPixel.h"

enum EBlendOp {
  eBlendOp_SrcOver,
  eBlendOp_Src
};

typedef GPixel (*BlendFunc)(GPixel dst, GPixel src);

inline uint32_t fixed_multiply(uint32_t a, uint32_t b) {
  return (a * b + 127) / 255;
}

inline GPixel blend_src(GPixel dst, GPixel src) {
  return src;
}

inline GPixel blend_srcover(GPixel dst, GPixel src) {
  uint32_t srcA = GPixel_GetA(src);
  if(srcA == 255) {
    return src;
  }

  uint32_t srcR = GPixel_GetR(src);
  uint32_t srcG = GPixel_GetG(src);
  uint32_t srcB = GPixel_GetB(src);

  uint32_t dstA = GPixel_GetA(dst);
  uint32_t dstR = GPixel_GetR(dst);
  uint32_t dstG = GPixel_GetG(dst);
  uint32_t dstB = GPixel_GetB(dst);

  return GPixel_PackARGB(srcA + fixed_multiply(dstA, 255 - srcA),
                         srcR + fixed_multiply(dstR, 255 - srcA),
                         srcG + fixed_multiply(dstG, 255 - srcA),
                         srcB + fixed_multiply(dstB, 255 - srcA));
}

inline BlendFunc GetBlendFunc(EBlendOp op) {
  switch(op) {
  case eBlendOp_Src:
    return blend_src;
   
  case eBlendOp_SrcOver:
    return blend_srcover;
  }
}

#endif // GBLEND_H_

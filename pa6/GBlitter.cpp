
#include "GBlitter.h"

#include "GBitmap.h"
#include "GColor.h"

#include <algorithm>

static GPixel *GetRow(const GBitmap &bm, int row) {
  uint8_t *rowPtr = reinterpret_cast<uint8_t *>(bm.fPixels) + row*bm.fRowBytes;
  return reinterpret_cast<GPixel *>(rowPtr);
}

static bool ContainsPoint(const GRect &r, const float x, const float y) {
  return
    r.fLeft <= x && x < r.fRight &&
    r.fTop <= y && y < r.fBottom;
}

GConstBlitter
::GConstBlitter(const GColor &color, BlendFunc blend)
  : GBlitter()
  , m_Pixel(ColorToPixel(color))
  , m_Blend(blend)
{ }

void GConstBlitter
::blitRow(const GBitmap &dst, uint32_t startX, uint32_t endX, uint32_t y) const {
  GASSERT(endX <= dst.width());
  GASSERT(startX <= endX);

  GPixel *row = GetRow(dst, y);
  for(uint32_t i = startX; i < endX; i++) {
    // We have the infrastructure to support any blending function from this mode..
    // but we get better performance out of specifying the blend mode...
    // row[i] = m_Blend(row[i], m_Pixel);
    row[i] = blend_srcover(row[i], m_Pixel);
  }
}

GOpaqueBlitter
::GOpaqueBlitter(const GColor &color) 
  : GBlitter()
  , m_Pixel(ColorToPixel(color))
{ }

void GOpaqueBlitter
::blitRow(const GBitmap &dst, uint32_t startX, uint32_t endX, uint32_t y) const {
  GPixel *row = GetRow(dst, y);
  for(uint32_t i = startX; i < endX; i++) {
    row[i] = m_Pixel;
  }
}

static GVec3f TransformCoord(const GMatrix3x3f &m, uint32_t x, uint32_t y) {
  GVec3f ctxPt(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f, 1.0f);
  return m * ctxPt;
}

static void FindBitmapBounds(const GMatrix3x3f &m, const GBitmap &bm,
                             uint32_t &sx, uint32_t &ex, uint32_t y) {
  GIRect bmRect = GIRect::MakeWH(bm.width(), bm.height());
  bool contained = false;
  for(; sx < ex && !contained; sx++) {
    GVec3f ctxPt = TransformCoord(m, sx, y);
    contained = ContainsPoint(bmRect, ctxPt[0], ctxPt[1]);
  }
  if(contained)
    sx--;
  
  contained = false;
  for(; sx < ex && !contained; ex--) {
    GVec3f ctxPt = TransformCoord(m, ex - 1, y);
    contained = ContainsPoint(bmRect, ctxPt[0], ctxPt[1]);
  }
  if(contained)
    ex++;
}

GBitmapBlitter
::GBitmapBlitter(const GMatrix3x3f &invCTM, const GBitmap &bm, const float alpha)
  : GBlitter()
  , m_BM(bm)
  , m_CTMInv(invCTM)
  , m_Alpha(static_cast<uint32_t>(alpha * 255.0f + 0.5f))
{ }


void GBitmapBlitter
::blitRow(const GBitmap &dst, uint32_t startX, uint32_t endX, uint32_t y) const {
  FindBitmapBounds(m_CTMInv, m_BM, startX, endX, y);

  GPixel *row = GetRow(dst, y);
  for(uint32_t i = startX; i < endX; i++) {
    
    GVec3f ctxPt = TransformCoord(m_CTMInv, i, y);

    uint32_t xx = static_cast<uint32_t>(ctxPt[0]);
    uint32_t yy = static_cast<uint32_t>(ctxPt[1]);

    GPixel *srcRow = GetRow(m_BM, yy);
    GPixel *dstRow = GetRow(dst, y);

    uint32_t srcA = fixed_multiply(GPixel_GetA(srcRow[xx]), m_Alpha);
    uint32_t srcR = fixed_multiply(GPixel_GetR(srcRow[xx]), m_Alpha);
    uint32_t srcG = fixed_multiply(GPixel_GetG(srcRow[xx]), m_Alpha);
    uint32_t srcB = fixed_multiply(GPixel_GetB(srcRow[xx]), m_Alpha);
    GPixel src = GPixel_PackARGB(srcA, srcR, srcG, srcB);
    dstRow[i] = blend_srcover(dstRow[i], src);
  }
}

GOBMBlitter
::GOBMBlitter(const GMatrix3x3f &invCTM, const GBitmap &bm)
  : GBlitter()
  , m_BM(bm)
  , m_CTMInv(invCTM)
{ }


void GOBMBlitter
::blitRow(const GBitmap &dst, uint32_t startX, uint32_t endX, uint32_t y) const {
  FindBitmapBounds(m_CTMInv, m_BM, startX, endX, y);

  GPixel *row = GetRow(dst, y);
  for(uint32_t i = startX; i < endX; i++) {

    GVec3f ctxPt = TransformCoord(m_CTMInv, i, y);

    uint32_t xx = static_cast<uint32_t>(ctxPt[0]);
    uint32_t yy = static_cast<uint32_t>(ctxPt[1]);

    GPixel *srcRow = GetRow(m_BM, yy);
    GPixel *dstRow = GetRow(dst, y);

    dstRow[i] = blend_srcover(dstRow[i], srcRow[xx]);
  }
}

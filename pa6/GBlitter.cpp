
#include "GBlitter.h"

#include "GBitmap.h"
#include "GColor.h"

#include <algorithm>

template<typename T>
static inline T Clamp(const T &v, const T &minVal, const T &maxVal) {
  return ::std::max(::std::min(v, maxVal), minVal);
}

static inline GColor ClampColor(const GColor &c) {
  GColor r;
  r.fA = Clamp(c.fA, 0.0f, 1.0f);
  r.fR = Clamp(c.fR, 0.0f, 1.0f);
  r.fG = Clamp(c.fG, 0.0f, 1.0f);
  r.fB = Clamp(c.fB, 0.0f, 1.0f);
  return r;
}

static GPixel ColorToPixel(const GColor &c) {
  GColor dc = ClampColor(c);
  dc.fR *= dc.fA;
  dc.fG *= dc.fA;
  dc.fB *= dc.fA;

  return GPixel_PackARGB(static_cast<unsigned>((dc.fA * 255.0f) + 0.5f),
                         static_cast<unsigned>((dc.fR * 255.0f) + 0.5f),
                         static_cast<unsigned>((dc.fG * 255.0f) + 0.5f),
                         static_cast<unsigned>((dc.fB * 255.0f) + 0.5f));
}

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
  uint32_t nPixels = endX - startX;

  GASSERT(endX <= dst.width());
  GASSERT(startX <= endX);

  GPixel *row = GetRow(dst, y);
  for(uint32_t i = startX; i < endX; i++) {
    row[i] = m_Blend(row[i], m_Pixel);
  }
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
  GIRect bmRect = GIRect::MakeWH(m_BM.width(), m_BM.height());

  GPixel *row = GetRow(dst, y);
  for(uint32_t i = startX; i < endX; i++) {
    GVec3f ctxPt(static_cast<float>(i) + 0.5f, static_cast<float>(y) + 0.5f, 1.0f);
    ctxPt = m_CTMInv * ctxPt;

    if(ContainsPoint(bmRect, ctxPt[0], ctxPt[1])) {
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
}

GOBMBlitter
::GOBMBlitter(const GMatrix3x3f &invCTM, const GBitmap &bm)
  : GBlitter()
  , m_BM(bm)
  , m_CTMInv(invCTM)
{ }


void GOBMBlitter
::blitRow(const GBitmap &dst, uint32_t startX, uint32_t endX, uint32_t y) const {
  GIRect bmRect = GIRect::MakeWH(m_BM.width(), m_BM.height());

  GPixel *row = GetRow(dst, y);
  for(uint32_t i = startX; i < endX; i++) {
    GVec3f ctxPt(static_cast<float>(i) + 0.5f, static_cast<float>(y) + 0.5f, 1.0f);
    ctxPt = m_CTMInv * ctxPt;

    if(ContainsPoint(bmRect, ctxPt[0], ctxPt[1])) {
      uint32_t xx = static_cast<uint32_t>(ctxPt[0]);
      uint32_t yy = static_cast<uint32_t>(ctxPt[1]);

      GPixel *srcRow = GetRow(m_BM, yy);
      GPixel *dstRow = GetRow(dst, y);

      dstRow[i] = blend_srcover(dstRow[i], srcRow[xx]);
    }
  }
}

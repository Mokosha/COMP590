#include "GContext.h"

#include <cstring>
#include <cassert>
#include <algorithm>

#include "GBitmap.h"
#include "GColor.h"
#include "GIRect.h"

template<typename T>
inline T Clamp(const T &v, const T &minVal, const T &maxVal) {
  return ::std::max(::std::min(v, maxVal), minVal);
}

inline GColor ClampColor(const GColor &c) {
  GColor r;
  r.fA = Clamp(c.fA, 0.0f, 1.0f);
  r.fR = Clamp(c.fR, 0.0f, 1.0f);
  r.fG = Clamp(c.fG, 0.0f, 1.0f);
  r.fB = Clamp(c.fB, 0.0f, 1.0f);
  return r;
}

class GDeferredContext : public GContext {
 public:
  GDeferredContext() { }

  virtual void getBitmap(GBitmap *bm) const {
    if(bm)
      *bm = GetInternalBitmap();
  }

  virtual void clear(const GColor &c) {
    const GBitmap &bm = GetInternalBitmap();
    GIRect rect = GIRect::MakeWH(bm.fWidth, bm.fHeight);
    fillIRect(rect, c, eBlendOp_Src);
  }

  virtual void fillIRect(const GIRect &rect, const GColor &c) {
    fillIRect(rect, c, eBlendOp_SrcOver);
  }

 protected:
  virtual const GBitmap &GetInternalBitmap() const = 0;

  enum EBlendOp {
    eBlendOp_SrcOver,
    eBlendOp_Src
  };

  static void memsetPixel(GPixel *dst, GPixel v, uint32_t count) {
    for(uint32_t i = 0; i < count; i++) {
      dst[i] = v;
    }
  }

  static uint32_t fixed_multiply(uint32_t a, uint32_t b) {
    return (a * b + 128) / 255;
  }

  static GPixel blend_srcover(GPixel dst, GPixel src) {
    uint32_t srcA = GPixel_GetA(src);
    uint32_t dstA = GPixel_GetA(dst);

    uint32_t resA = srcA + fixed_multiply(dstA, 255 - srcA);

    uint32_t srcR = GPixel_GetR(src);
    uint32_t dstR = GPixel_GetR(dst);
    uint32_t srcG = GPixel_GetG(src);
    uint32_t dstG = GPixel_GetG(dst);
    uint32_t srcB = GPixel_GetB(src);
    uint32_t dstB = GPixel_GetB(dst);

    return GPixel_PackARGB(resA,
                           srcR + fixed_multiply(dstR, 255 - srcA),
                           srcG + fixed_multiply(dstG, 255 - srcA),
                           srcB + fixed_multiply(dstB, 255 - srcA));
  }

  static GPixel blend(GPixel &dst, GPixel src, EBlendOp op) {
    switch(op) {
      case eBlendOp_Src: return src;
      case eBlendOp_SrcOver:
        if(GPixel_GetA(src) == 255) {
          return src;
        } else {
          return blend_srcover(dst, src);
        }
    }
  }

  static GIRect IntersectRect(const GIRect &a, const GIRect &b) {
    return GIRect::MakeLTRB(
      std::max(a.fLeft, b.fLeft),
      std::max(a.fTop, b.fTop),
      std::min(a.fRight, b.fRight),
      std::min(a.fBottom, b.fBottom)
    );
  }

  static GPixel *GetRowOffset(const GBitmap &bm, int row) {
    uint8_t *rowPtr = reinterpret_cast<uint8_t *>(bm.fPixels) + row*bm.fRowBytes;
    return reinterpret_cast<GPixel *>(rowPtr);
  }

  void drawBitmap(const GBitmap &bm, int x, int y, float alpha) {
    const GBitmap &ctxbm = GetInternalBitmap();
    GIRect ctxRect = GIRect::MakeXYWH(0, 0, ctxbm.width(), ctxbm.height());
    GIRect bmRect = GIRect::MakeXYWH(x, y, bm.width(), bm.height());

    GIRect rect = IntersectRect(ctxRect, bmRect);
    if(rect.isEmpty()) {
      return;
    }

    uint32_t h = rect.height();
    uint32_t w = rect.width();

    if(alpha >= (254.5f / 255.0f)) {
      for(uint32_t j = 0; j < h; j++) {
        GPixel *srcRow = GetRowOffset(bm, j);
        GPixel *dstRow = GetRowOffset(ctxbm, j+y) + x;
        for(uint32_t i = 0; i < w; i++) {
          dstRow[i] = blend(dstRow[i], srcRow[i], eBlendOp_SrcOver);
        }
      }
    } else {
      const uint8_t alphaVal = static_cast<uint8_t>(alpha * 255.0f);
      for(uint32_t j = 0; j < h; j++) {
        GPixel *srcRow = GetRowOffset(bm, j);
        GPixel *dstRow = GetRowOffset(ctxbm, j+y) + x;
        for(uint32_t i = 0; i < w; i++) {
          uint8_t srcA = fixed_multiply(GPixel_GetA(srcRow[i]), alphaVal);
          uint8_t srcR = fixed_multiply(GPixel_GetR(srcRow[i]), alphaVal);
          uint8_t srcG = fixed_multiply(GPixel_GetG(srcRow[i]), alphaVal);
          uint8_t srcB = fixed_multiply(GPixel_GetB(srcRow[i]), alphaVal);
          GPixel src = GPixel_PackARGB(srcA, srcR, srcG, srcB);
          dstRow[i] = blend(dstRow[i], src, eBlendOp_SrcOver);
        }
      }
    }
  }

  void fillIRect(const GIRect &rect, const GColor &c, EBlendOp op) {

    const GBitmap &bitmap = GetInternalBitmap();

    uint32_t h = bitmap.fHeight;
    uint32_t w = bitmap.fWidth;

    GIRect bmRect = IntersectRect(rect, GIRect::MakeWH(w, h));
    if(bmRect.isEmpty())
      return;

    h = bmRect.height();
    w = bmRect.width();

    unsigned char *pixels = reinterpret_cast<unsigned char *>(bitmap.fPixels);

    // premultiply alpha ...
    GColor dc = ClampColor(c);
    dc.fR *= dc.fA;
    dc.fG *= dc.fA;
    dc.fB *= dc.fA;

    GPixel clearValue = GPixel_PackARGB(static_cast<unsigned>((dc.fA * 255.0f) + 0.5f),
                                        static_cast<unsigned>((dc.fR * 255.0f) + 0.5f),
                                        static_cast<unsigned>((dc.fG * 255.0f) + 0.5f),
                                        static_cast<unsigned>((dc.fB * 255.0f) + 0.5f));

    if(bitmap.fRowBytes == w * sizeof(GPixel)) {
      GPixel *p = bitmap.fPixels + bmRect.fTop*bitmap.fWidth + bmRect.fLeft;
      switch(op) {
        case eBlendOp_Src:
          memsetPixel(p, clearValue, w*h);
          break;

        default: {
          GPixel *end = bitmap.fPixels + ((bmRect.fBottom - 1) * w + bmRect.fRight);
          for(; p != end; p++) {
            *p = blend(*p, clearValue, op);
          }
          break;
        }
      }

    } else {
      for(uint32_t j = bmRect.fTop; j < bmRect.fBottom; j++) {
        const uint32_t offset = (j * bitmap.fRowBytes);

        switch(op) {
          case eBlendOp_Src: {
            GPixel *p = reinterpret_cast<GPixel *>(pixels + offset);
            memsetPixel(p, clearValue, w);
            break;
          }

          default: {
            GPixel *rowPixels = reinterpret_cast<GPixel *>(pixels + offset);
            GPixel oldP = rowPixels[0];
            GPixel newP = blend(oldP, clearValue, op);
            rowPixels[0] = newP;

            for(uint32_t i = bmRect.fLeft + 1; i < bmRect.fRight; i++) {
              if(oldP == rowPixels[i]) {
                rowPixels[i] = newP;
              } else {
                oldP = rowPixels[i];
                newP = rowPixels[i] = blend(rowPixels[i], clearValue, op);
              }
            }
            break;
          }
        }  // switch
      }  // for
    }  // else
  }
};

class GContextProxy : public GDeferredContext {
 public:
  GContextProxy(const GBitmap &bm): GDeferredContext(), m_Bitmap(bm) { }

  virtual ~GContextProxy() { }

 private:
  virtual const GBitmap &GetInternalBitmap() const {
    return m_Bitmap;
  };

  GBitmap m_Bitmap;
};

class GContextLocal : public GDeferredContext {
 public:
  GContextLocal(int width, int height)
    : GDeferredContext() {
    m_Bitmap.fWidth = width;
    m_Bitmap.fHeight = height;
    m_Bitmap.fPixels = new GPixel[width * height];
    m_Bitmap.fRowBytes = width * sizeof(GPixel);
  }

  virtual ~GContextLocal() {
    delete [] m_Bitmap.fPixels;
  }

  bool Valid() {
    bool ok = true;
    ok = ok && static_cast<bool>(m_Bitmap.fPixels);
    return ok;
  }

 private:
  virtual const GBitmap &GetInternalBitmap() const {
    return m_Bitmap;
  };

  GBitmap m_Bitmap;
};

/**
 *  Create a new context that will draw into the specified bitmap. The
 *  caller is responsible for managing the lifetime of the pixel memory.
 *  If the new context cannot be created, return NULL.
 */
GContext* GContext::Create(const GBitmap &bm) {
  // If the context has no pixels defined, then there's no way this
  // can be a valid context...
  if(!bm.fPixels)
    return NULL;

  // Weird dimensions?
  if(bm.fWidth <= 0 || bm.fHeight <= 0)
    return NULL;

  // Is our rowbytes less than a sane number of bytes we need for the width
  // that's specified?
  if(bm.fRowBytes < bm.fWidth * sizeof(GPixel))
    return NULL;

  // Is our rowbytes word aligned?
  // FIXME: I'm not totally sure this check needs to be made...
  if(static_cast<uint32_t>(bm.fRowBytes) % sizeof(GPixel))
    return NULL;

  // Think we're ok then...
  return new GContextProxy(bm);
}

/**
 *  Create a new context is sized to match the requested dimensions. The
 *  context is responsible for managing the lifetime of the pixel memory.
 *  If the new context cannot be created, return NULL.
 */
GContext* GContext::Create(int width, int height) {
  // Check for weird sizes...
  if(width <= 0 || height <= 0)
    return NULL;

  // That's as weird as it gets... let's try to create
  // the context...
  GContextLocal *ctx = new GContextLocal(width, height);

  // Did it work?
  if(!ctx || !ctx->Valid()) {
    if(ctx)
      delete ctx;
    return NULL;
  }

  // Guess it did...
  return ctx;
}

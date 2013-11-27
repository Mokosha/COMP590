#ifndef GBLITTER_H_
#define GBLITTER_H_

#include "GTypes.h"
#include "GBlend.h"
#include "GColor.h"
#include "GMatrix.h"
#include "GVector.h"

#include <algorithm>

// Forward declarations
class GBitmap;

class GBlitter {
 protected:
  GBlitter() { }
  virtual ~GBlitter() { }

 public:
  virtual void blitRow(const GBitmap &dst, uint32_t startX, uint32_t endX, uint32_t y) const = 0;
};

class GConstBlitter : public GBlitter {
 private:
  const GPixel m_Pixel;
  const BlendFunc m_Blend;

 public:
  GConstBlitter(const GColor &color, BlendFunc func);
  virtual ~GConstBlitter() { }

  virtual void blitRow(const GBitmap &dst, uint32_t startX, uint32_t endX, uint32_t y) const;
};

class GOpaqueBlitter : public GBlitter {
 private:
  const GPixel m_Pixel;

 public:
  GOpaqueBlitter(const GColor &color);
  virtual ~GOpaqueBlitter() { }

  virtual void blitRow(const GBitmap &dst, uint32_t startX, uint32_t endX, uint32_t y) const;
};

class GBitmapBlitter : public GBlitter { 
 private:
  const GMatrix3x3f m_CTMInv;
  const GBitmap &m_BM;
  const uint32_t m_Alpha;

 public:
  GBitmapBlitter(const GMatrix3x3f &invCTM, const GBitmap &bm, const float alpha);
  virtual ~GBitmapBlitter() { }

  virtual void blitRow(const GBitmap &dst, uint32_t startX, uint32_t endX, uint32_t y) const;
};

// Opaque bitmap blitter
class GOBMBlitter : public GBlitter {
 private:
  const GMatrix3x3f m_CTMInv;
  const GBitmap &m_BM;

 public:
  GOBMBlitter(const GMatrix3x3f &invCTM, const GBitmap &bm);
  virtual ~GOBMBlitter() { }

  virtual void blitRow(const GBitmap &dst, uint32_t startX, uint32_t endX, uint32_t y) const;
};

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

inline GPixel ColorToPixel(const GColor &c) {
  GColor dc = ClampColor(c);
  dc.fR *= dc.fA;
  dc.fG *= dc.fA;
  dc.fB *= dc.fA;

  return GPixel_PackARGB(static_cast<unsigned>((dc.fA * 255.0f) + 0.5f),
                         static_cast<unsigned>((dc.fR * 255.0f) + 0.5f),
                         static_cast<unsigned>((dc.fG * 255.0f) + 0.5f),
                         static_cast<unsigned>((dc.fB * 255.0f) + 0.5f));
}

#endif // GBLITTER_H_

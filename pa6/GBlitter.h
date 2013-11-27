#ifndef GBLITTER_H_
#define GBLITTER_H_

#include "GTypes.h"
#include "GBlend.h"
#include "GMatrix.h"
#include "GVector.h"

// Forward declarations
class GBitmap;
class GColor;

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

class GOpaqueBlitter : public GConstBlitter {
 public:
  GOpaqueBlitter(const GColor &color) : GConstBlitter(color, blend_src) { }
  virtual ~GOpaqueBlitter() { }
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

#endif // GBLITTER_H_

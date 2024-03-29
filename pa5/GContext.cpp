#include "GContext.h"

#include <cstring>
#include <cassert>
#include <algorithm>
#include <vector>

#include "GMatrix.h"
#include "GBitmap.h"
#include "GPaint.h"
#include "GColor.h"
#include "GRect.h"

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
  GDeferredContext() {
    SetCTM(GMatrix3x3f());
  }

  virtual void getBitmap(GBitmap *bm) const {
    if(bm)
      *bm = GetInternalBitmap();
  }

  virtual void clear(const GColor &c) {
    const GBitmap &bm = GetInternalBitmap();
    fillIRect(GIRect::MakeWH(bm.fWidth, bm.fHeight), c, eBlendOp_Src);
  }

 protected:
  ::std::vector<GMatrix3x3f> m_CTMStack;
  GMatrix3x3f m_CTM;
  GMatrix3x3f m_CTMInv;
  bool m_ValidCTM;

  virtual void onSave() {
    m_CTMStack.push_back(m_CTM);
  }

  virtual void onRestore() {
    uint32_t sz = m_CTMStack.size();
    assert(sz > 0);
    SetCTM(m_CTMStack[sz - 1]);
    m_CTMStack.pop_back();
  }

  void MultiplyCTM(const GMatrix3x3f &m) {
    SetCTM(m_CTM * m);
  }

  virtual void translate(float tx, float ty) {
    GMatrix3x3f m;
    m(0, 2) = tx;
    m(1, 2) = ty;
    MultiplyCTM(m);
  }

  virtual void scale(float sx, float sy) {
    GMatrix3x3f m;
    m(0, 0) = sx;
    m(1, 1) = sy;
    MultiplyCTM(m);
  }

 private:
  void SetCTM(const GMatrix3x3f &m) {
    m_CTM = m;
    m_CTMInv = m_CTM;
    m_ValidCTM = m_CTMInv.Invert();
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
    return (a * b + 127) / 255;
  }

  static GPixel blend_src(GPixel dst, GPixel src) {
    return src;
  }

  static GPixel blend_srcover(GPixel dst, GPixel src) {
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

  typedef GPixel (*BlendFunc)(GPixel dst, GPixel src);

  static BlendFunc GetBlendFunc(EBlendOp op) {
    switch(op) {
      case eBlendOp_Src:
        return blend_src;
   
      case eBlendOp_SrcOver:
        return blend_srcover;
    }
  }

  static GPixel *GetRow(const GBitmap &bm, int row) {
    uint8_t *rowPtr = reinterpret_cast<uint8_t *>(bm.fPixels) + row*bm.fRowBytes;
    return reinterpret_cast<GPixel *>(rowPtr);
  }

  static bool CheckSkew(const GMatrix3x3f &m) {
    if(m(0, 1) != 0 || m(1, 0) != 0) {
      return true;
    }
    return false;
  }

  static void AddPoint(GRect &r, const GVec3f &v) {
    r.fLeft = std::min(r.fLeft, v.X());
    r.fTop = std::min(r.fTop, v.Y());
    r.fRight = std::max(r.fRight, v.X());
    r.fBottom = std::max(r.fBottom, v.Y());
  }

  static bool ContainsPoint(const GRect &r, const float x, const float y) {
    return
      r.fLeft <= x && x < r.fRight &&
      r.fTop <= y && y < r.fBottom;
  }

  GRect GetTransformedBoundingBox(const GRect &r) {
    // Top Left
    GVec3f tl(r.fLeft, r.fTop, 1.0f);

    // Top right
    GVec3f tr(r.fRight, r.fTop, 1.0f);

    // Bottom left
    GVec3f bl(r.fLeft, r.fBottom, 1.0f);

    // Bottom right
    GVec3f br(r.fRight, r.fBottom, 1.0f);

    tl = m_CTM * tl;
    tr = m_CTM * tr;
    bl = m_CTM * bl;
    br = m_CTM * br;

    GRect ret = GRect::MakeXYWH(tl.X(), tl.Y(), 0, 0);
    AddPoint(ret, tr);
    AddPoint(ret, bl);
    AddPoint(ret, br);
    return ret;
  }

  void drawXFormPixel(const uint32_t i, const uint32_t j,
                      const GRect &srcRect, const GIRect &dstRect,
                      const GBitmap &src, const GBitmap &dst,
                      const BlendFunc blend = blend_srcover) {

    GVec3f ctxPt(static_cast<float>(dstRect.fLeft + i) + 0.5f,
                 static_cast<float>(dstRect.fTop + j) + 0.5f,
                 1.0f);

    ctxPt = m_CTMInv * ctxPt;

    if(ContainsPoint(srcRect, ctxPt[0], ctxPt[1])) {
      uint32_t xx = static_cast<uint32_t>(ctxPt[0] - srcRect.fLeft);
      uint32_t yy = static_cast<uint32_t>(ctxPt[1] - srcRect.fTop);
      
      GPixel *srcRow = GetRow(src, yy);
      GPixel *dstRow = GetRow(dst, j+dstRect.fTop) + dstRect.fLeft;
      dstRow[i] = blend(dstRow[i], srcRow[xx]);
    }
  }

  void drawXFormPixelWithAlpha(const uint32_t i, const uint32_t j,
                               const GRect &srcRect, const GIRect &dstRect,
                               const GBitmap &src, const GBitmap &dst,
                               const uint8_t alpha,
                               const BlendFunc blend = blend_srcover) {

    GVec3f ctxPt(static_cast<float>(dstRect.fLeft + i) + 0.5f,
                 static_cast<float>(dstRect.fTop + j) + 0.5f,
                 1.0f);

    ctxPt = m_CTMInv * ctxPt;

    if(ContainsPoint(srcRect, ctxPt[0], ctxPt[1])) {
      uint32_t x = static_cast<uint32_t>(ctxPt[0] - srcRect.fLeft);
      uint32_t y = static_cast<uint32_t>(ctxPt[1] - srcRect.fTop);

      GPixel *srcRow = GetRow(src, y);
      GPixel *dstRow = GetRow(dst, j+dstRect.fTop) + dstRect.fLeft;

      uint32_t srcA = fixed_multiply(GPixel_GetA(srcRow[x]), alpha);
      uint32_t srcR = fixed_multiply(GPixel_GetR(srcRow[x]), alpha);
      uint32_t srcG = fixed_multiply(GPixel_GetG(srcRow[x]), alpha);
      uint32_t srcB = fixed_multiply(GPixel_GetB(srcRow[x]), alpha);
      GPixel src = GPixel_PackARGB(srcA, srcR, srcG, srcB);
      dstRow[i] = blend(dstRow[i], src);
    }
  }

  // If the alpha value is above this value, then it will round to
  // an opaque pixel during quantization.
  static const float kOpaqueAlpha = (254.5f / 255.0f);
  static const float kTransparentAlpha = (0.499999f / 255.0f);

  // This code draws a bitmap using the full m_CTM transform without any thought
  // to whether or not the transform has any special properties.
  void drawBitmapXForm(const GBitmap &bm, const GPaint &paint) {
    const GBitmap &ctxbm = GetInternalBitmap();
    GRect ctxRect = GRect::MakeXYWH(0, 0, ctxbm.width(), ctxbm.height());
    GIRect bmRect = GIRect::MakeXYWH(0, 0, bm.width(), bm.height());
    GRect pixelRect = GetTransformedBoundingBox(bmRect);

    GRect rect;
    if(!(rect.setIntersection(ctxRect, pixelRect))) {
      return;
    }

    // Rein everything back into integer land
    GIRect dstRect = rect.round();
    if(dstRect.isEmpty()) {
      return;
    }

    float alpha = paint.getAlpha();
    if(alpha >= kOpaqueAlpha) {
      for(uint32_t j = 0; j < dstRect.height(); j++) {
        for(uint32_t i = 0; i < dstRect.width(); i++) {
          drawXFormPixel(i, j, dstRect, bmRect, bm, ctxbm);
        }
      }
    } else {
      const uint32_t alphaVal = static_cast<uint32_t>((alpha * 255.0f) + 0.5f);
      for(uint32_t j = 0; j < dstRect.height(); j++) {
        for(uint32_t i = 0; i < dstRect.width(); i++) {
          drawXFormPixelWithAlpha(i, j, dstRect, bmRect, bm, ctxbm, alphaVal);
        }
      }
    }
  }

  // This code draws a bitmap assuming that we only have translation and scale,
  // which allows us to perform certain optimizations...
  void drawBitmapSimple(const GBitmap &bm, const GPaint &paint) {
    const GBitmap &ctxbm = GetInternalBitmap();
    GRect ctxRect = GRect::MakeXYWH(0, 0, ctxbm.width(), ctxbm.height());
    GRect bmRect = GRect::MakeXYWH(0, 0, bm.width(), bm.height());
    GRect pixelRect = GetTransformedBoundingBox(bmRect);

    GRect rect;
    if(!(rect.setIntersection(ctxRect, pixelRect))) {
      return;
    }

    // We know that since we're only doing scale and translation, that all of the pixel
    // centers contained in rect are going to be drawn, so we only need to know the
    // dimensions of the mapping...
    GVec3f origin(0, 0, 1);
    GVec3f offset(1, 1, 1);

    origin = m_CTM * origin;
    offset = m_CTM * offset;

    float xScale = 1.0f / (offset.X() - origin.X());
    float yScale = 1.0f / (offset.Y() - origin.Y());

    GVec2f start = GVec2f(0, 0);
    if(xScale < 0.0f) {
      start.X() = pixelRect.fRight - 1.0f;
    }
    if(yScale < 0.0f) {
      start.Y() = pixelRect.fBottom - 1.0f;
    }

    GIRect dstRect = rect.round();

    // Construct new bitmap
    int32_t offsetX = ::std::max(0, -dstRect.fLeft);
    int32_t offsetY = ::std::max(0, -dstRect.fTop);
    GBitmap fbm;
    fbm.fWidth = bm.width();
    fbm.fHeight = bm.height();
    fbm.fPixels = GetRow(bm, offsetY) + offsetX;
    fbm.fRowBytes = bm.fRowBytes;

    BlendFunc blend = blend_srcover;

    float alpha = paint.getAlpha();
    if(alpha >= kOpaqueAlpha) {
      for(uint32_t j = 0; j < dstRect.height(); j++) {

        uint32_t srcIdxY = static_cast<uint32_t>(start.Y() + static_cast<float>(j) * yScale);
        GPixel *srcPixels = GetRow(fbm, Clamp<int>(srcIdxY, 0, fbm.height()));
        GPixel *dstPixels = GetRow(ctxbm, dstRect.fTop + j) + dstRect.fLeft;

        for(uint32_t i = 0; i < dstRect.width(); i++) {
          uint32_t srcIdxX = static_cast<uint32_t>(start.X() + static_cast<float>(i) * xScale);
          dstPixels[i] = blend(dstPixels[i], srcPixels[Clamp<int>(srcIdxX, 0, fbm.width())]);
        }
      }
    } else {
      const uint32_t alphaVal = static_cast<uint32_t>((alpha * 255.0f) + 0.5f);
      for(uint32_t j = 0; j < dstRect.height(); j++) {

        uint32_t srcIdxY = static_cast<uint32_t>(start.Y() + static_cast<float>(j) * yScale);
        GPixel *srcPixels = GetRow(fbm, srcIdxY);
        GPixel *dstPixels = GetRow(ctxbm, dstRect.fTop + j) + dstRect.fLeft;

        for(uint32_t i = 0; i < dstRect.width(); i++) {
          uint32_t srcIdxX = static_cast<uint32_t>(start.X() + static_cast<float>(i) * xScale);
          uint32_t srcA = fixed_multiply(GPixel_GetA(srcPixels[srcIdxX]), alphaVal);
          uint32_t srcR = fixed_multiply(GPixel_GetR(srcPixels[srcIdxX]), alphaVal);
          uint32_t srcG = fixed_multiply(GPixel_GetG(srcPixels[srcIdxX]), alphaVal);
          uint32_t srcB = fixed_multiply(GPixel_GetB(srcPixels[srcIdxX]), alphaVal);
          GPixel src = GPixel_PackARGB(srcA, srcR, srcG, srcB);
          dstPixels[i] = blend(dstPixels[i], src);
        }
      }
    }
  }

  void drawBitmap(const GBitmap &bm, float x, float y, const GPaint &paint) {

    float alpha = paint.getAlpha();
    if(alpha < kTransparentAlpha) {
      return;
    }

    save();
    translate(x, y);

    if(CheckSkew(m_CTM)) {
      drawBitmapXForm(bm, paint);
    } else {
      drawBitmapSimple(bm, paint);
    }

    restore();
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

  void drawRect(const GRect &rect, const GPaint &p) {
    const GBitmap &ctxbm = GetInternalBitmap();
    GRect ctxRect = GRect::MakeXYWH(0, 0, ctxbm.width(), ctxbm.height());
    GRect pixelRect = GetTransformedBoundingBox(rect);

    if(pixelRect.isEmpty()) {
      return;
    }

    GRect trRect;
    if(!(trRect.setIntersection(ctxRect, pixelRect))) {
      return;
    }

    // Rein everything back into integer land
    GIRect dstRect = trRect.round();
    if(dstRect.isEmpty()) {
      return;
    }

    if(!(CheckSkew(m_CTM))) {
      fillIRect(dstRect, p.getColor(), eBlendOp_SrcOver);
      return;
    }

    GPixel clearValue = ColorToPixel(p.getColor());

    // If the alpha value is above this value, then it will round to
    // an opaque pixel during quantization.
    const float kOpaqueAlpha = (254.5f / 255.0f);
    float alpha = p.getAlpha();

    // Blend func is currently just srcover
    BlendFunc blend = blend_srcover;

    for(uint32_t j = 0; j < dstRect.height(); j++) {
      for(uint32_t i = 0; i < dstRect.width(); i++) {

        GVec3f ctxPt(static_cast<float>(dstRect.fLeft + i) + 0.5f,
                     static_cast<float>(dstRect.fTop + j) + 0.5f,
                     1.0f);

        ctxPt = m_CTMInv * ctxPt;

        if(ContainsPoint(rect, ctxPt[0], ctxPt[1])) {
          uint32_t x = static_cast<uint32_t>(ctxPt[0] - rect.fLeft);
          uint32_t y = static_cast<uint32_t>(ctxPt[1] - rect.fTop);

          GPixel *dstRow = GetRow(ctxbm, j+dstRect.fTop) + dstRect.fLeft;
          dstRow[i] = blend(dstRow[i], clearValue);
        }
      }
    }
  }

  void fillIRect(const GIRect &rect, const GColor &c, EBlendOp op) {

    const GBitmap &bitmap = GetInternalBitmap();

    uint32_t h = bitmap.fHeight;
    uint32_t w = bitmap.fWidth;

    GIRect bmRect;
    if(!(bmRect.setIntersection(rect, GIRect::MakeWH(w, h))))
      return;

    h = bmRect.height();
    w = bmRect.width();

    // premultiply alpha ...
    GPixel clearValue = ColorToPixel(c);

    // Figure out blendfunc
    BlendFunc blend = GetBlendFunc(op);

    if(bitmap.fRowBytes == w * sizeof(GPixel)) {
      GPixel *p = bitmap.fPixels + bmRect.fTop*bitmap.fWidth + bmRect.fLeft;
      switch(op) {
        case eBlendOp_Src:
          memsetPixel(p, clearValue, w*h);
          break;

        default: {
          GPixel *end = bitmap.fPixels + ((bmRect.fBottom - 1) * w + bmRect.fRight);
          for(; p != end; p++) {
            *p = blend(*p, clearValue);
          }
          break;
        }
      }

    } else {
      
      for(uint32_t j = bmRect.fTop; j < bmRect.fBottom; j++) {
        GPixel *rowPixels = GetRow(bitmap, j) + bmRect.fLeft;

        switch(op) {
          case eBlendOp_Src: {
            memsetPixel(rowPixels, clearValue, w);
            break;
          }

          default: {
            GPixel oldP = rowPixels[0];
            GPixel newP = blend(oldP, clearValue);
            rowPixels[0] = newP;
            
            for(uint32_t i = 1; i < w; i++) {
              if(oldP == rowPixels[i]) {
                rowPixels[i] = newP;
              } else {
                oldP = rowPixels[i];
                newP = rowPixels[i] = blend(rowPixels[i], clearValue);
              }
            }
            break;
          }
        }  // switch
      }  // for
    }  // else
  }

  GPoint Vert2Point(const GVec3f &vert) {
    GPoint ret;
    ret.set(vert[0] / vert[2], vert[1] / vert[2]);
    return ret;
  }

  static bool ComputeLine(const GPoint &p1, const GPoint &p2, float &m, float &b) {
    float dx = (p2.fX - p1.fX);
    if(dx == 0) {
      return true;
    }
    m = (p2.fY - p1.fY) / dx;
    b = p1.fY - m*p1.fX;
    return false;
  }

  bool TestCollinear(const GVec3f *verts, const uint32_t nVerts) {
    if(nVerts <= 2)
      return true;

    float m, b;
    bool vertical = ComputeLine(Vert2Point(verts[0]), Vert2Point(verts[1]), m, b);

    for(uint32_t i = 2; i < nVerts; i++) {
      float nm, nb;
      bool nv = ComputeLine(Vert2Point(verts[i]), Vert2Point(verts[i-1]), nm, nb);

      // If any line isn't collinear, then just exit
      if(vertical) {
        if(!nv) {
          return false;
        }
      } else if(fabs(nm - m) > 0.0001 || fabs(nb - b) > 0.0001) {
        return false;
      }
    }

    return true;
  }

  struct GEdge {
    GPoint p1;
    GPoint p2;
    GEdge(GPoint _p1, GPoint _p2) : p1(_p1), p2(_p2) { }
    bool ComputeLine(float &m, float &b) const {
      return GDeferredContext::ComputeLine(p1, p2, m, b);
    }
  };

  void WalkEdges(const GEdge e1, const GEdge e2, const GPaint &paint) {

    const GBitmap &bm = GetInternalBitmap();
    int h = bm.fHeight;
    int w = bm.fWidth;

    GASSERT(e1.p1.y() == e2.p1.y());
    int startY = Clamp(static_cast<int>(e1.p1.y() + 0.5f), 0, h-1);
    GASSERT(e1.p2.y() == e2.p2.y());
    int endY = Clamp(static_cast<int>(e1.p2.y() + 0.5f), 0, h-1);

    if(endY == startY) {
      return;
    }

    GASSERT(endY > startY);

    // Initialize to NAN
    float m1 = 0.0f/0.0f, b1;
    float m2 = 0.0f/0.0f, b2;
    bool vert1 = e1.ComputeLine(m1, b1);
    bool vert2 = e2.ComputeLine(m2, b2);

    if(m1 == 0 || m2 == 0) {
      return;
    }

    // Collinear?
    if(vert2 && vert1 && e1.p1.x() == e2.p1.x()) {
      return;
    } else if(m1 == m2 && b1 == b2) {
      return;
    }

    float stepX1 = vert1? 0 : 1/m1;
    float stepX2 = vert2? 0 : 1/m2;

    GPoint p1, p2;
    float sY = static_cast<float>(startY) + 0.5f;
    if(vert1) {
      p1.set(e1.p1.x(), sY);
    } else {
      p1.set((sY - b1) / m1, sY);
    }

    if(vert2) {
      p2.set(e2.p1.x(), sY);
    } else {
      p2.set((sY - b2) / m2, sY);
    }

    // Make sure that p1 is always less than p2 to avoid
    // doing a min/max in the inner loop
    if(p1.x() > p2.x()) {
      std::swap(p1, p2);
      std::swap(stepX1, stepX2);
    }

    GPixel color = ColorToPixel(paint.getColor());
    BlendFunc blend = GetBlendFunc(eBlendOp_SrcOver);

    uint32_t nSteps = endY - startY;
    for(uint32_t i = 0; i < nSteps; i++) {

      // Since we haven't implemented clipping yet, take care
      // not to go beyond our bounds...
      const int x1 = Clamp<int>(p1.fX + 0.5f, 0, w-1);
      const int x2 = Clamp<int>(p2.fX + 0.5f, 0, w-1);

      GPixel *row = GetRow(bm, startY + i);
      for(int x = x1; x < x2; x++) {
        row[x] = blend(row[x], color);
      }

      p1.fX += stepX1;
      p2.fX += stepX2;
    }
  }

  void drawTriangle(const GPoint vertices[3], const GPaint &paint) {
    GVec3f verts[3] = {
      GVec3f(vertices[0].fX, vertices[0].fY, 1.0f),
      GVec3f(vertices[1].fX, vertices[1].fY, 1.0f),
      GVec3f(vertices[2].fX, vertices[2].fY, 1.0f)
    };

    GPoint points[3] = {
      Vert2Point(m_CTM * verts[0]),
      Vert2Point(m_CTM * verts[1]),
      Vert2Point(m_CTM * verts[2])
    };

    // Sort based on y
    for(uint32_t i = 0; i < 3; i++) {
      for(uint32_t j = i+1; j < 3; j++) {
        if(points[i].y() > points[j].y()) {
          std::swap(points[i], points[j]);
        }
      }
    }

    // Determine first half of triangle
    // Initialize to NaN
    float m = 0.0f/0.0f, b;
    bool vertical = ComputeLine(points[0], points[2], m, b);

    // If the line from 0 to 2 is horizontal, then since we're ordered in y,
    // all of the points must be collinear...
    if(m == 0) {
      return;
    }

    // Compute intersection of this line with the second point
    GPoint p;
    p.fY = points[1].y();
    if(vertical) {
      p.fX = points[0].x();
    } else {
      p.fX = (p.fY - b) / m;
    }

    // Walk edges...
    WalkEdges(GEdge(points[0], points[1]), GEdge(points[0], p), paint);
    WalkEdges(GEdge(points[1], points[2]), GEdge(p, points[2]), paint);
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

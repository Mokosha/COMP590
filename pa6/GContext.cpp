#include "GContext.h"
#include <cstring>
#include <cassert>
#include <algorithm>
#include <vector>

#include "GMatrix.h"
#include "GBitmap.h"
#include "GBlend.h"
#include "GBlitter.h"
#include "GPaint.h"
#include "GColor.h"
#include "GRect.h"

static void memsetPixel(GPixel *dst, GPixel v, uint32_t count) {
  for(uint32_t i = 0; i < count; i++) {
    dst[i] = v;
  }
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
    if(bm.fRowBytes == bm.fWidth * 4) {
      memsetPixel(bm.fPixels, ColorToPixel(c), bm.fWidth * bm.fHeight);
      return;
    }

    GRect rect = GRect::MakeWH(bm.fWidth, bm.fHeight);
    GOpaqueBlitter blitter(c);
    drawRawRect(rect, blitter);
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

  virtual void rotate(float angle) {
    GMatrix3x3f m;
    float sa = sin(angle);
    float ca = cos(angle);
    m(0, 0) = ca; m(0, 1) = -sa;
    m(1, 0) = sa; m(1, 1) = ca;
    MultiplyCTM(m);
  }

 private:
  void SetCTM(const GMatrix3x3f &m) {
    m_CTM = m;
    m_CTMInv = m_CTM;
    m_ValidCTM = m_CTMInv.Invert();
  }

  // Hackery
  unsigned char m_BlitBuffer[32];
  GBlitter *m_Blitter;

 protected:
  virtual const GBitmap &GetInternalBitmap() const = 0;

  static bool CheckSkew(const GMatrix3x3f &m) {
    if(m(0, 1) != 0 || m(1, 0) != 0) {
      return true;
    }
    return false;
  }

  // If the alpha value is above this value, then it will round to
  // an opaque pixel during quantization.
  static const float kOpaqueAlpha = (254.5f / 255.0f);
  static const float kTransparentAlpha = (0.499999f / 255.0f);

  // !FIXME! Just because alpha is opaque doesn't mean that there's
  // no blending operation. This is restricted to the COMP590 assignment...
  void SetBlitter(const GPaint &p, const EBlendOp op = eBlendOp_SrcOver) {
    float alpha = p.getAlpha();
    if(alpha > kOpaqueAlpha) {
      m_Blitter = new (m_BlitBuffer) GOpaqueBlitter(p.getColor());
      GASSERT(sizeof(GOpaqueBlitter) < 32);
    }

    m_Blitter = new (m_BlitBuffer) GConstBlitter(p.getColor(), GetBlendFunc(op));
    GASSERT(sizeof(GConstBlitter) < 32);
  }

  GPoint Vert2Point(const GVec3f &vert) {
    GPoint ret;
    ret.set(vert[0] / vert[2], vert[1] / vert[2]);
    return ret;
  }

  GVec3f Point2Vert(const GPoint &p) {
    return GVec3f(p.fX, p.fY, 1.0f);
  }

  GRect AddPoint(const GRect &rect, const GVec3f &v) {
    GRect ret;
    ret.fLeft = std::min(rect.fLeft, v[0]);
    ret.fRight = std::max(rect.fRight, v[0]);
    ret.fTop = std::min(rect.fTop, v[1]);
    ret.fBottom = std::max(rect.fBottom, v[1]);
    return ret;
  }

  GRect TransformRect(const GRect &rect) {
    GPoint verts[4];
    rect.toQuad(verts);

    GVec3f v = m_CTM * Point2Vert(verts[0]);
    GRect ret = GRect::MakeLTRB(v[0], v[1], v[0], v[1]);
    for(uint32_t i = 1; i < 4; i++) {
      ret = AddPoint(ret, m_CTM * Point2Vert(verts[i]));
    }
    return ret;
  }

  void drawRawRect(const GRect &rect, const GBlitter &blitter) {
    const GBitmap &ctxbm = GetInternalBitmap();
    const GIRect ctxRect = GIRect::MakeWH(ctxbm.width(), ctxbm.height());

    GRect dst;
    if(!dst.setIntersection(GRect(ctxRect), rect)) {
      return;
    }

    GIRect dstRect = dst.round();
    for(uint32_t y = dstRect.fTop; y < dstRect.fBottom; y++) {
      blitter.blitRow(ctxbm, dstRect.fLeft, dstRect.fRight, y);
    }
  }

  void drawBitmap(const GBitmap &bm, float x, float y, const GPaint &paint) {

    float alpha = paint.getAlpha();
    if(alpha < kTransparentAlpha) {
      return;
    }

    save();
    translate(x, y);

    if(alpha > kOpaqueAlpha) {
      GOBMBlitter blitter(m_CTMInv, bm);
      drawRectWithBlitter(GRect::MakeWH(bm.width(), bm.height()), blitter);
    } else {
      GBitmapBlitter blitter(m_CTMInv, bm, paint.getAlpha());
      drawRectWithBlitter(GRect::MakeWH(bm.width(), bm.height()), blitter);
    }

    restore();
  }

  void drawRectWithBlitter(const GRect &rect, const GBlitter &blitter) {

    if(!CheckSkew(m_CTM)) {
      GRect xform = TransformRect(rect);
      drawRawRect(xform, blitter);
      return;
    }

    GPoint vertices[4];
    vertices[0].set(rect.fLeft, rect.fTop);
    vertices[1].set(rect.fRight, rect.fTop);
    vertices[2].set(rect.fLeft, rect.fBottom);
    vertices[3].set(rect.fRight, rect.fBottom);

    drawTriangleWithBlitter(vertices, blitter);
    drawTriangleWithBlitter(vertices + 1, blitter);
  }

  void drawRect(const GRect &rect, const GPaint &p) {

    // If the alpha value is above this value, then it will round to
    // an opaque pixel during quantization.
    float alpha = p.getAlpha();
    if(alpha <= kTransparentAlpha) {
      return;
    }

    SetBlitter(p);
    drawRectWithBlitter(rect, *m_Blitter);
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

  struct GEdge {
    GPoint p1;
    GPoint p2;
    GEdge(GPoint _p1, GPoint _p2) : p1(_p1), p2(_p2) { }
    bool ComputeLine(float &m, float &b) const {
      return GDeferredContext::ComputeLine(p1, p2, m, b);
    }
  };

  void WalkEdges(const GEdge e1, const GEdge e2, const GBlitter &blitter) {

    const GBitmap &bm = GetInternalBitmap();
    int h = bm.fHeight;
    int w = bm.fWidth;

    GASSERT(e1.p1.y() == e2.p1.y());
    int startY = Clamp(static_cast<int>(e1.p1.y() + 0.5f), 0, h);
    GASSERT(e1.p2.y() == e2.p2.y());
    int endY = Clamp(static_cast<int>(e1.p2.y() + 0.5f), 0, h);

    if(startY == endY) {
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

    uint32_t nSteps = endY - startY;
    p1.fX += 0.5;
    p2.fX += 0.5;
    for(uint32_t i = 0; i < nSteps; i++) {

      // Since we haven't implemented clipping yet, take care
      // not to go beyond our bounds...
      const int x1 = Clamp<int>(p1.fX, 0, w);
      const int x2 = Clamp<int>(p2.fX, 0, w);

      blitter.blitRow(bm, x1, x2, startY + i);

      p1.fX += stepX1;
      p2.fX += stepX2;
    }
  }

  void drawTriangle(const GPoint vertices[3], const GPaint &paint) {
    SetBlitter(paint);
    drawTriangleWithBlitter(vertices, *m_Blitter);
  }

  void drawTriangleWithBlitter(const GPoint vertices[3], const GBlitter &blitter) {
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
    WalkEdges(GEdge(points[0], points[1]), GEdge(points[0], p), blitter);
    WalkEdges(GEdge(points[1], points[2]), GEdge(p, points[2]), blitter);
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

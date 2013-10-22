#include "GContext.h"

#include <cstring>
#include <cassert>
#include <algorithm>
#include <vector>

#include "GBitmap.h"
#include "GPaint.h"
#include "GColor.h"
#include "GRect.h"

template <typename T, const int N>
class GVector {
 protected:

  // Vector representation
  T vec[N];

 public:
    
  GVector() { }
  GVector(const GVector<T, N> &other) {
    for(int i = 0; i < N; i++) vec[i] = other[i];
  }

  GVector(T *_vec) {
    for(int i = 0; i < N; i++) {
      vec[i] = _vec[i];
    }
  }

  // Accessors
  T &operator()(int idx) { return vec[idx]; }
  T &operator[](int idx) { return vec[idx]; }
  const T &operator()(int idx) const { return vec[idx]; }
  const T &operator[](int idx) const { return vec[idx]; }

  // Allow casts to the respective array representation...
  operator T *() const { return vec; }
  GVector<T, N> &operator=(const T *v) {
    for(int i = 0; i < N; i++) vec[i] = v[i];
    return *this;
  }

  // Allows casting to other vector types if the underlying type system does as well...
  template<typename _T>
  operator GVector<_T, N>() const { 
    return GVector<_T, N>(vec); 
  }

  // Operators
  template<typename _T>
  GVector<T, N> operator+(const GVector<_T, N> &v) const {
    GVector a;
    for(int i = 0; i < N; i++) {
      a.vec[i] = v(i) + vec[i];
    }
    return a;
  }

  template<typename _T>
  GVector<T, N> &operator+=(const GVector<_T, N> &v) const {
    for(int i = 0; i < N; i++) {
      vec[i] += v(i);
    }
    return *this;
  }

  template<typename _T>
  GVector<T, N> operator-(const GVector<_T, N> &v) const {
    GVector<T, N> a;
    for(int i = 0; i < N; i++) {
      a(i) = vec[i] - v[i];
    }
    return a;
  }

  template<typename _T>
  GVector<T, N> &operator-=(const GVector<_T, N> &v) const {
    for(int i = 0; i < N; i++) {
      vec[i] -= v[i];
    }
    return *this;
  }

  template<typename _T>
  GVector<T, N> &operator=(const GVector<_T, N> &v) {
    for(int i = 0; i < N; i++) {
      vec[i] = v[i];
    }
    return *this;
  }

  GVector<T, N> operator*(const T &s) const {
    GVector<T, N> a;
    for(int i = 0; i < N; i++) a[i] = vec[i] * s;
    return a;
  }
  
  friend GVector<T, N> operator*(const T &s, const GVector<T, N> &v) {
    GVector<T, N> a;
    for(int i = 0; i < N; i++)
      a[i] = v[i] * s;
    return a;
  }

  GVector<T, N> operator/(const T &s) const {
    GVector<T, N> a;
    for(int i = 0; i < N; i++) a[i] = vec[i] / s;
    return a;
  }
  
  friend GVector<T, N> operator/(const T &s, const GVector<T, N> &v) {
    GVector<T, N> a;
    for(int i = 0; i < N; i++) a[i] = v[i] / s;
    return a;
  }

  void operator*=(const T &s) {
    for(int i = 0; i < N; i++) vec[i] *= s;
  }

  void operator/=(const T &s) {
    for(int i = 0; i < N; i++) vec[i] /= s;
  }

  // Vector operations
  template<typename _T>
  T Dot(const GVector<_T, N> &v) const {
    T sum = 0;
    for(int i = 0; i < N; i++) sum += vec[i] * v[i];
    return sum;
  }

  T LengthSq() const { return this->Dot(*this); }
  T Length() const { return sqrt(LengthSq()); }
};

typedef GVector<float, 2> GVec2f;
typedef GVector<float, 3> GVec3f;

template<typename T, const int nRows, const int nCols>
class GMatrix {
 protected:
  static const int kNumElements = nRows * nCols;
  T mat[kNumElements];

 public:
  // Constructors
  GMatrix() { }
  GMatrix(const GMatrix<T, nRows, nCols> &other) {
    for(int i = 0; i < kNumElements; i++) {
      mat[i] = other[i];
    }
  }

  // Accessors
  T &operator()(int idx) { return mat[idx]; }
  const T &operator()(int idx) const { return mat[idx]; }
  T &operator()(int r, int c) { return mat[r * nCols + c]; }
  const T &operator()(int r, int c) const { return mat[r * nCols + c]; }

  T &operator[](int idx) { return mat[idx]; }
  const T &operator[](int idx) const { return mat[idx]; }

  GMatrix<T, nRows, nCols> &operator=(const GMatrix<T, nRows, nCols> &other) {
    for(int i = 0; i < kNumElements; i++) {
      mat[i] = other[i];
    }    
  }

  // Operators
  template<typename _T>
  GMatrix<T, nRows, nCols> operator+(
    const GMatrix<_T, nRows, nCols> &m
  ) {
    GMatrix<T, nRows, nCols> a;
    for(int i = 0; i < kNumElements; i++) {
      a[i] = mat[i] + m[i];
    }
    return a;
  }

  template<typename _T>
  GMatrix<T, nRows, nCols> &operator+=(
    const GMatrix<_T, nRows, nCols> &m
  ) {
    for(int i = 0; i < kNumElements; i++) {
      mat[i] += m[i];
    }
    return *this;
  }

  template<typename _T>
  GMatrix<T, nRows, nCols> operator-(
    const GMatrix<_T, nRows, nCols> &m
  ) {
    GMatrix<T, nRows, nCols> a;
    for(int i = 0; i < kNumElements; i++) {
      a[i] = mat[i] - m[i];
    }
    return a;
  }

  template<typename _T>
  GMatrix<T, nRows, nCols> &operator-=(
    const GMatrix<_T, nRows, nCols> &m
  ) {
    for(int i = 0; i < kNumElements; i++) {
      mat[i] -= m[i];
    }
    return *this;
  }

  template<typename _T>
  GMatrix<T, nRows, nCols> operator*(_T s) {
    GMatrix<T, nRows, nCols> a;
    for(int i = 0; i < kNumElements; i++) {
      a[i] = mat[i] * s;
    }
    return a;
  }

  template<typename _T>
  GMatrix<T, nRows, nCols> &operator*=(_T s) {
    for(int i = 0; i < kNumElements; i++) {
      mat[i] *= s;
    }
    return *this;
  }

  template<typename _T>
  GMatrix<T, nRows, nCols> operator/(_T s) {
    GMatrix<T, nRows, nCols> a;
    for(int i = 0; i < kNumElements; i++) {
      a[i] = mat[i] / s;
    }
    return a;
  }

  template<typename _T>
  GMatrix<T, nRows, nCols> &operator/=(_T s) {
    for(int i = 0; i < kNumElements; i++) {
      mat[i] /= s;
    }
    return *this;
  }

  // Matrix multiplication
  template<typename _T, const int nTarget>
  GMatrix<T, nRows, nTarget> operator*(
    const GMatrix<_T, nCols, nTarget> &m
  ) {
    GMatrix<T, nRows, nTarget> result;
    for(int r = 0; r < nRows; r++)
    for(int c = 0; c < nTarget; c++) {
      result(r, c) = 0;
      for(int j = 0; j < nCols; j++) {
        result(r, c) += (*this)(r, j) * m(j, c);
      }
    }
    return result;
  }

  template<typename _T, const int nTarget>
  GMatrix<T, nRows, nTarget> &operator*=(
    const GMatrix<_T, nCols, nTarget> &m
  ) {
    (*this) = (*this) * m;
    return (*this);
  }

  // Vector multiplication
  template<typename _T>
  GVector<T, nCols> operator*(const GVector<_T, nCols> &v) {
    GVector<T, nCols> result;
    for(int r = 0; r < nRows; r++) {
      result(r) = 0;
      for(int j = 0; j < nCols; j++) {
        result(r) += (*this)(r, j) * v(j);
      }
    }
    return result;
  }
};

typedef GMatrix<float, 2, 2> GMatrix2x2f;
typedef GMatrix<float, 3, 3> GMatrix3x3f;

// Return an identity matrix of a given size
template<unsigned size>
static GMatrix<float, size, size> Identity() {
  typedef GMatrix<float, size, size> MatrixType;
  MatrixType m;
  for(uint32_t j = 0; j < size; j++) {
    for(uint32_t i = 0; i < size; i++) {
      m(i, j) = (i == j)? 1.0f : 0.0f;
    }
  }
  return m;
}

static float Determinant3x3f(GMatrix3x3f &m) {
  return 0.0f
    + m(0, 0) * (m(1, 1) * m(2, 2) - m(2, 1) * m(1, 2))
    - m(0, 1) * (m(1, 0) * m(2, 2) - m(2, 0) * m(0, 2))
    + m(0, 2) * (m(1, 0) * m(2, 1) - m(2, 0) * m(1, 1));
}

static void Invert3x3f(GMatrix3x3f &m) {

  float determinant = Determinant3x3f(m);
  // assert(fabs(determinant) > 1e-6f);

  float d = 1.0f / determinant;

  float m00 = m(2, 2) * m(1, 1) - m(2, 1) * m(1, 2);
  float m01 = m(0, 2) * m(2, 1) - m(2, 2) * m(0, 1);
  float m02 = m(0, 1) * m(1, 2) - m(1, 1) * m(0, 2);
  float m10 = m(1, 2) * m(2, 0) - m(2, 2) * m(1, 0);
  float m11 = m(0, 0) * m(2, 2) - m(2, 0) * m(0, 2);
  float m12 = m(0, 2) * m(1, 0) - m(1, 2) * m(0, 0);
  float m20 = m(1, 0) * m(2, 1) - m(2, 0) * m(1, 1);
  float m21 = m(0, 1) * m(2, 0) - m(2, 1) * m(0, 0);
  float m22 = m(0, 0) * m(1, 1) - m(1, 0) * m(0, 1);
  m(0, 0) = m00;
  m(0, 1) = m01;
  m(0, 2) = m02;
  m(1, 0) = m10;
  m(1, 1) = m11;
  m(1, 2) = m12;
  m(2, 0) = m20;
  m(2, 1) = m21;
  m(2, 2) = m22;
  m *= d;
}

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
    SetCTM(Identity<3>());
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

  virtual void onSave() {
    m_CTMStack.push_back(m_CTM);
    m_CTM = Identity<3>();
    for(uint32_t i = 0; i < m_CTMStack.size(); i++) {
      m_CTM *= m_CTMStack[i];
    }
    SetCTM(m_CTM);
  }

  virtual void onRestore() {
    uint32_t sz = m_CTMStack.size();
    if(sz > 0) {
      SetCTM(m_CTMStack[sz - 1]);
      m_CTMStack.pop_back();
    } else {
      assert(!"This is an error.");
      SetCTM(Identity<3>());
    }
  }

  void SetCTM(const GMatrix3x3f &m) {
    m_CTM = m;
    m_CTMInv = m_CTM;
    Invert3x3f(m_CTMInv);    
  }

  void MultiplyCTM(const GMatrix3x3f &m) {
    SetCTM(m_CTM * m);
  }

  virtual void translate(float tx, float ty) {
    GMatrix3x3f m = Identity<3>();
    m(0, 2) = tx;
    m(1, 2) = ty;
    MultiplyCTM(m);
  }

  virtual void scale(float sx, float sy) {
    GMatrix3x3f m = Identity<3>();
    m(0, 0) = sx;
    m(1, 1) = sy;
    MultiplyCTM(m);
  }

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

  static GPixel blend_srcover(GPixel dst, GPixel src) {
    uint32_t srcA = GPixel_GetA(src);
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

  template<typename RectType>
  static RectType IntersectRect(const RectType &a, const RectType &b) {
    RectType ret;
    ret.fLeft = std::max(a.fLeft, b.fLeft);
    ret.fTop = std::max(a.fTop, b.fTop);
    ret.fRight = std::min(a.fRight, b.fRight);
    ret.fBottom = std::min(a.fBottom, b.fBottom);
    return ret;
  }

  static GPixel *GetRow(const GBitmap &bm, int row) {
    uint8_t *rowPtr = reinterpret_cast<uint8_t *>(bm.fPixels) + row*bm.fRowBytes;
    return reinterpret_cast<GPixel *>(rowPtr);
  }

  GRect GetTransformedBoundingBox(const GRect &r) {
    // Top Left
    GVec3f tl;
    tl[0] = r.fLeft;
    tl[1] = r.fTop;
    tl[2] = 1.0f;

    // Top right
    GVec3f tr;
    tr[0] = r.fRight;
    tr[1] = r.fTop;
    tr[2] = 1.0f;

    // Bottom left
    GVec3f bl;
    bl[0] = r.fLeft;
    bl[1] = r.fBottom;
    bl[2] = 1.0f;

    // Bottom right
    GVec3f br;
    br[0] = r.fRight;
    br[1] = r.fBottom;
    br[2] = 1.0f;

    tl = m_CTM * tl;
    tr = m_CTM * tr;
    bl = m_CTM * bl;
    br = m_CTM * br;

    GRect ret;
    ret.fLeft = std::min(std::min(std::min(tl[0], tr[0]), bl[0]), br[0]);
    ret.fTop = std::min(std::min(std::min(tl[1], tr[1]), bl[1]), br[1]);
    ret.fRight = std::max(std::max(std::max(tl[0], tr[0]), bl[0]), br[0]);
    ret.fBottom = std::max(std::max(std::max(tl[1], tr[1]), bl[1]), br[1]);
    return ret;
  }

  void drawBitmap(const GBitmap &bm, float x, float y, const GPaint &paint) {
    const GBitmap &ctxbm = GetInternalBitmap();
    GRect ctxRect = GRect::MakeXYWH(0, 0, ctxbm.width(), ctxbm.height());
    GRect bmRect = GRect::MakeXYWH(x, y, bm.width(), bm.height());
    GRect pixelRect = GetTransformedBoundingBox(bmRect);

    GRect rect;
    if(!(rect.setIntersection(ctxRect, pixelRect))) {
      return;
    }

    // Rein everything back into integer land
    GIRect dstRect = GIRect::MakeLTRB(
      static_cast<int32_t>(rect.fLeft + 0.5f),
      static_cast<int32_t>(rect.fTop + 0.5f),
      static_cast<int32_t>(rect.fRight + 0.5f),
      static_cast<int32_t>(rect.fBottom + 0.5f)
    );

    if(dstRect.isEmpty()) {
      return;
    }

    // If the alpha value is above this value, then it will round to
    // an opaque pixel during quantization.
    const float kOpaqueAlpha = (254.5f / 255.0f);
    float alpha = paint.getAlpha();

    if(alpha >= kOpaqueAlpha) {
      for(uint32_t j = 0; j < dstRect.height(); j++) {
        for(uint32_t i = 0; i < dstRect.width(); i++) {

          GVec3f ctxPt;
          ctxPt[0] = static_cast<float>(dstRect.fLeft + i) + 0.5f;
          ctxPt[1] = static_cast<float>(dstRect.fTop + j) + 0.5f;
          ctxPt[2] = 1.0f;

          ctxPt = m_CTMInv * ctxPt;

          if(bmRect.contains(ctxPt[0], ctxPt[1])) {
            uint32_t xx = static_cast<uint32_t>(ctxPt[0] - bmRect.fLeft);
            uint32_t yy = static_cast<uint32_t>(ctxPt[1] - bmRect.fTop);

            GPixel *srcRow = GetRow(bm, yy);
            GPixel *dstRow = GetRow(ctxbm, j+dstRect.fTop) + dstRect.fLeft;
            dstRow[i] = blend(dstRow[i], srcRow[xx], eBlendOp_SrcOver);
          }
        }
      }
    } else {
      const uint32_t alphaVal = static_cast<uint32_t>((alpha * 255.0f) + 0.5f);
      for(uint32_t j = 0; j < dstRect.height(); j++) {
        for(uint32_t i = 0; i < dstRect.width(); i++) {
          GVec3f ctxPt;
          ctxPt[0] = static_cast<float>(dstRect.fLeft + i) + 0.5f;
          ctxPt[1] = static_cast<float>(dstRect.fTop + j) + 0.5f;
          ctxPt[2] = 1.0f;

          ctxPt = m_CTMInv * ctxPt;

          if(bmRect.contains(ctxPt[0], ctxPt[1])) {
            uint32_t x = static_cast<uint32_t>(ctxPt[0] - bmRect.fLeft);
            uint32_t y = static_cast<uint32_t>(ctxPt[1] - bmRect.fTop);

            GPixel *srcRow = GetRow(bm, y);
            GPixel *dstRow = GetRow(ctxbm, j+dstRect.fTop) + dstRect.fLeft;

            uint32_t srcA = fixed_multiply(GPixel_GetA(srcRow[x]), alphaVal);
            uint32_t srcR = fixed_multiply(GPixel_GetR(srcRow[x]), alphaVal);
            uint32_t srcG = fixed_multiply(GPixel_GetG(srcRow[x]), alphaVal);
            uint32_t srcB = fixed_multiply(GPixel_GetB(srcRow[x]), alphaVal);
            GPixel src = GPixel_PackARGB(srcA, srcR, srcG, srcB);
            dstRow[i] = blend(dstRow[i], src, eBlendOp_SrcOver);
          }
        }
      }
    }
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

    GRect trRect;
    if(!(trRect.setIntersection(ctxRect, pixelRect))) {
      return;
    }

    // Rein everything back into integer land
    GIRect dstRect = GIRect::MakeLTRB(
      static_cast<int32_t>(trRect.fLeft + 0.5f),
      static_cast<int32_t>(trRect.fTop + 0.5f),
      static_cast<int32_t>(trRect.fRight + 0.5f),
      static_cast<int32_t>(trRect.fBottom + 0.5f)
    );

    if(dstRect.isEmpty()) {
      return;
    }

    GPixel clearValue = ColorToPixel(p.getColor());

    // If the alpha value is above this value, then it will round to
    // an opaque pixel during quantization.
    const float kOpaqueAlpha = (254.5f / 255.0f);
    float alpha = p.getAlpha();

    for(uint32_t j = 0; j < dstRect.height(); j++) {
      for(uint32_t i = 0; i < dstRect.width(); i++) {

        GVec3f ctxPt;
        ctxPt[0] = static_cast<float>(dstRect.fLeft + i) + 0.5f;
        ctxPt[1] = static_cast<float>(dstRect.fTop + j) + 0.5f;
        ctxPt[2] = 1.0f;

        ctxPt = m_CTMInv * ctxPt;

        if(rect.contains(ctxPt[0], ctxPt[1])) {
          uint32_t x = static_cast<uint32_t>(ctxPt[0] - rect.fLeft);
          uint32_t y = static_cast<uint32_t>(ctxPt[1] - rect.fTop);

          GPixel *dstRow = GetRow(ctxbm, j+dstRect.fTop) + dstRect.fLeft;
          dstRow[i] = blend(dstRow[i], clearValue, eBlendOp_SrcOver);
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

    // premultiply alpha ...
    GPixel clearValue = ColorToPixel(c);

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
        GPixel *rowPixels = GetRow(bitmap, j) + bmRect.fLeft;

        switch(op) {
          case eBlendOp_Src: {
            memsetPixel(rowPixels, clearValue, w);
            break;
          }

          default: {
            GPixel oldP = rowPixels[0];
            GPixel newP = blend(oldP, clearValue, op);
            rowPixels[0] = newP;
            
            for(uint32_t i = 1; i < w; i++) {
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

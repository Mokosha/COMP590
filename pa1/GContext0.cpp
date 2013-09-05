#include "GContext.h"

#include <cstring>
#include <cassert>
#include <algorithm>

#include "GBitmap.h"
#include "GColor.h"

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
  GDeferredContext() : m_NumCommands(0) { }

  virtual void getBitmap(GBitmap *bm) const {
    Flush();
    if(bm)
      *bm = GetInternalBitmap();
  }

  virtual void clear(const GColor &c) {
    if (m_Commands[m_NumCommands - 1] != eCommand_Clear) {
      if (m_NumCommands == kMaxNumCommands) {
        Flush();
      }
      m_Commands[m_NumCommands++] = eCommand_Clear;
    }
    m_ClearColor = c;
  }

 protected:
  virtual const GBitmap &GetInternalBitmap() const = 0;

 private:
  enum ECommand {
    eCommand_Clear,

    kNumCommands
  };

  // We only have one possible command that works on the entire
  // frame buffer, so we really only need to keep track of the
  // latest one.
  static const int kMaxNumCommands = 1;
  ECommand m_Commands[kMaxNumCommands];
  mutable int m_NumCommands;

  void Flush() const {
    for(int i = 0; i < m_NumCommands; i++) {
      switch(m_Commands[i]) {
        case eCommand_Clear:
          ClearOp();
          break;

        default:
          // Do nothing?
          assert(!"Unknown command!");
          break;
      }
    }
    m_NumCommands = 0;
  }

 protected:
  GColor m_ClearColor;
  void ClearOp() const {
    const GColor &c = m_ClearColor;
    const GBitmap &bitmap = GetInternalBitmap();

    const unsigned int h = bitmap.fHeight;
    const unsigned int w = bitmap.fWidth;

    const unsigned int rb = bitmap.fRowBytes;
    unsigned char *pixels = reinterpret_cast<unsigned char *>(bitmap.fPixels);

    // premultiply alpha ...
    GColor dc = ClampColor(c);
    dc.fR *= dc.fA;
    dc.fG *= dc.fA;
    dc.fB *= dc.fA;

    GPixel clearValue =
      (int((dc.fA * 255.0f) + 0.5f) << GPIXEL_SHIFT_A) |
      (int((dc.fR * 255.0f) + 0.5f) << GPIXEL_SHIFT_R) |
      (int((dc.fG * 255.0f) + 0.5f) << GPIXEL_SHIFT_G) |
      (int((dc.fB * 255.0f) + 0.5f) << GPIXEL_SHIFT_B);

    for(unsigned int j = 0; j < h; j++) {
      for(unsigned int i = 0; i < w; i++) {
        // Pack into a pixel
        const unsigned int offset = ((j * rb) + (i * sizeof(GPixel)));
        *(reinterpret_cast<GPixel *>(pixels + offset)) = clearValue;
      }
    }
  }
};

class GContextProxy : public GDeferredContext {
 public:
  GContextProxy(const GBitmap &bm): GDeferredContext(), m_Bitmap(bm) { }

  virtual ~GContextProxy() { }

  virtual void clear(const GColor &c) {
    m_ClearColor = c;
    ClearOp();
  }

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
  
  virtual void clear(const GColor &c) {
    m_ClearColor = c;
    ClearOp();
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
  if(static_cast<unsigned int>(bm.fRowBytes) % sizeof(unsigned int))
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
  if(!ctx || !ctx->Valid())
    return NULL;

  // Guess it did...
  return ctx;
}

#include "GContext.h"

#include <cstring>
#include <cassert>

#include "GBitmap.h"
#include "GColor.h"

class GDeferredContext : public GContext {
 public:
  GDeferredContext() : m_NumCommands(0) { }

  virtual void getBitmap(GBitmap *bm) const {
    Flush();
    memcpy(bm, GetInternalBitmap(), sizeof(*bm));
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
  virtual GBitmap *GetInternalBitmap() const = 0;

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
    GBitmap *bitmap = GetInternalBitmap();

    const unsigned int h = bitmap->fHeight;
    const unsigned int w = bitmap->fWidth;

    const unsigned int rb = bitmap->fRowBytes;
    unsigned char *pixels = reinterpret_cast<unsigned char *>(bitmap->fPixels);

    for(unsigned int j = 0; j < h; j++) {
      for(unsigned int i = 0; i < w; i++) {
        // premultiply alpha ...
        GColor dc;
        dc.fA = c.fA;
        dc.fR = c.fR * c.fA;
        dc.fG = c.fG * c.fA;
        dc.fB = c.fB * c.fA;

        // Pack into a pixel
        const unsigned int offset = ((j * rb) + (i * sizeof(GPixel)));
        GPixel &p = *(reinterpret_cast<GPixel *>(pixels + offset));
        p =
          (int((dc.fA * 255.0f) + 0.5f) << GPIXEL_SHIFT_A) |
          (int((dc.fR * 255.0f) + 0.5f) << GPIXEL_SHIFT_R) |
          (int((dc.fG * 255.0f) + 0.5f) << GPIXEL_SHIFT_G) |
          (int((dc.fB * 255.0f) + 0.5f) << GPIXEL_SHIFT_B);
      }
    }
  }
};

class GContextProxy : public GDeferredContext {
 public:
  GContextProxy(const GBitmap &bm)
    : GDeferredContext(), m_Bitmap(new GBitmap) {
    memcpy(m_Bitmap, &bm, sizeof(GBitmap));
  }

  virtual ~GContextProxy() {
    delete m_Bitmap;
  }

  virtual void clear(const GColor &c) {
    m_ClearColor = c;
    ClearOp();
  }

 private:
  virtual GBitmap *GetInternalBitmap() const {
    return m_Bitmap;
  };

  GBitmap *m_Bitmap;
};

class GContextLocal : public GDeferredContext {
 public:
  GContextLocal(int width, int height)
    : GDeferredContext(), m_Bitmap(new GBitmap) {
    m_Bitmap->fWidth = width;
    m_Bitmap->fHeight = height;
    m_Bitmap->fPixels = new GPixel[width * height];
    m_Bitmap->fRowBytes = width * sizeof(GPixel);
  }

  virtual ~GContextLocal() {
    delete [] m_Bitmap->fPixels;
    delete m_Bitmap;
  }
  
  // !FIXME! What do we do here? Reference count the bitmap?
  /* 
  const GDeferredContext &operator=(const GDeferredContext &other) {
  }

  GDeferredContext(const GDeferredContext &other) {
  }
  */
 private:
  virtual GBitmap *GetInternalBitmap() const {
    return m_Bitmap;
  };

  GBitmap *m_Bitmap;
};

/**
 *  Create a new context that will draw into the specified bitmap. The
 *  caller is responsible for managing the lifetime of the pixel memory.
 *  If the new context cannot be created, return NULL.
 */
GContext* GContext::Create(const GBitmap &bm) {
  return new GContextProxy(bm);
}

/**
 *  Create a new context is sized to match the requested dimensions. The
 *  context is responsible for managing the lifetime of the pixel memory.
 *  If the new context cannot be created, return NULL.
 */
GContext* GContext::Create(int width, int height) {
  return new GContextLocal(width, height);
}

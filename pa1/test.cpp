/**
 *  Copyright 2013 Mike Reed
 *
 *  COMP 590 -- Fall 2013
 */

#include <string.h>

#include "GContext.h"
#include "GBitmap.h"
#include "GColor.h"
#include "GRandom.h"

static bool gVerbose;

typedef void (*MakeColorProc)(GRandom&, GColor*);

struct Stats {
    int             fTests;
    int             fFailures;
    MakeColorProc   fMakeColor;
    
    double percent() const {
        return 100.0 * (fTests - fFailures) / fTests;
    }
};

static void assert_unit_float(float x) {
    GASSERT(x >= 0 && x <= 1);
}

static int unit_float_to_byte(float x) {
    GASSERT(x >= 0 && x <= 1);
    return (int)(x * 255 + 0.5f);
}

/*
 *  Pins each float value to be [0...1]
 *  Then scales them to bytes, and packs them into a GPixel
 */
static GPixel color_to_pixel(const GColor& c) {
    assert_unit_float(c.fA);
    assert_unit_float(c.fR);
    assert_unit_float(c.fG);
    assert_unit_float(c.fB);
    int a = unit_float_to_byte(c.fA);
    int r = unit_float_to_byte(c.fR * c.fA);
    int g = unit_float_to_byte(c.fG * c.fA);
    int b = unit_float_to_byte(c.fB * c.fA);
    
    return GPixel_PackARGB(a, r, g, b);
}

struct Size {
    int fW, fH;
};

template <typename T> class AutoDelete {
public:
    AutoDelete(T* obj) : fObj(obj) {}
    ~AutoDelete() { delete fObj; }

private:
    T* fObj;
};

static int max(int a, int b) { return a > b ? a : b; }

static int pixel_max_diff(uint32_t p0, uint32_t p1) {
    int da = abs(GPixel_GetA(p0) - GPixel_GetA(p1));
    int dr = abs(GPixel_GetR(p0) - GPixel_GetR(p1));
    int dg = abs(GPixel_GetG(p0) - GPixel_GetG(p1));
    int db = abs(GPixel_GetB(p0) - GPixel_GetB(p1));
    
    return max(da, max(dr, max(dg, db)));
}

static int check_pixels(const GBitmap& bm, GPixel expected) {
    GPixel pixel;

    const GPixel* row = bm.fPixels;
    for (int y = 0; y < bm.fHeight; ++y) {
        for (int x = 0; x < bm.fWidth; ++x) {
            pixel = row[x];
            if (pixel == expected) {
                continue;
            }

            // since pixels wre computed from floats, give a slop of
            // +- 1 for the resulting byte value in each component.
            if (pixel_max_diff(pixel, expected) > 1) {
                if (gVerbose) {
                    fprintf(stderr, "at (%d, %d) expected %x but got %x",
                            x, y, expected, pixel);
                }
                return -1;
            }
        }
        // skip to the next row
        row = (const GPixel*)((const char*)row + bm.fRowBytes);
    }
    return 0;
}

static void test_context(Stats* stats, GContext* ctx, const Size& size) {
    if (!ctx) {
        fprintf(stderr, "GContext::Create failed\n");
        exit(-1);
    }

    AutoDelete<GContext> ad(ctx);

    GBitmap bitmap;
    // memset() not required, but makes it easier to detect errors in the
    // getBitmap implementation.
    memset(&bitmap, 0, sizeof(GBitmap));
    ctx->getBitmap(&bitmap);

    if (!bitmap.fPixels) {
        fprintf(stderr, "did not get valid fPixels from getBitmap\n");
        exit(-1);
    }

    if (bitmap.fRowBytes < bitmap.fWidth * 4) {
        fprintf(stderr, "fRowBytes too small from getBitmap [%zu]\n",
                bitmap.fRowBytes);
        exit(-1);
    }

    if (bitmap.fWidth != size.fW || bitmap.fHeight != size.fH) {
        fprintf(stderr,
                "mismatch on dimensions: expected [%d %d] but got [%d %d]",
                size.fW, size.fH, bitmap.fWidth, bitmap.fHeight);
        exit(-1);
    }

    GRandom rand;
    for (int i = 0; i < 100; ++i) {
        GColor color;
        stats->fMakeColor(rand, &color);
        const GPixel pixel = color_to_pixel(color);

        ctx->clear(color);
        int err = check_pixels(bitmap, pixel);
        if (err) {
            if (gVerbose) {
                fprintf(stderr, " for color(%g %g %g %g)\n",
                        color.fA, color.fR, color.fG, color.fB);
            }
            stats->fFailures += 1;
        }
        stats->fTests += 1;
    }
}

class BitmapAlloc {
public:
    BitmapAlloc(GBitmap* bitmap, int width, int height) {
        const size_t rb_slop = 17 * sizeof(GPixel);

        bitmap->fWidth = width;
        bitmap->fHeight = height;
        bitmap->fRowBytes = width * sizeof(GPixel) + rb_slop;

        fPixels = malloc(bitmap->fHeight * bitmap->fRowBytes);
        bitmap->fPixels = (GPixel*)fPixels;
    }

    ~BitmapAlloc() {
        free(fPixels);
    }

private:
    void* fPixels;
};

static void run_tests(Stats* stats) {
    static const int gDim[] = {
        1, 2, 3, 5, 10, 25, 200, 1001
    };
    
    for (int wi = 0; wi < GARRAY_COUNT(gDim); ++wi) {
        for (int hi = 0; hi < GARRAY_COUNT(gDim); ++hi) {
            const int w = gDim[wi];
            const int h = gDim[hi];
            Size size = { w, h };
            
            GBitmap bitmap;
            BitmapAlloc alloc(&bitmap, w, h);
            
            if (gVerbose) {
                fprintf(stderr, "testing [%d %d]\n", w, h);
            }
            
            test_context(stats, GContext::Create(bitmap), size);
            test_context(stats, GContext::Create(w, h), size);
        }
    }
}

static void make_opaque_color(GRandom& rand, GColor* color) {
    color->fA = 1;
    color->fR = rand.nextF();
    color->fG = rand.nextF();
    color->fB = rand.nextF();
}

static void make_translucent_color(GRandom& rand, GColor* color) {
    color->fA = rand.nextF();
    color->fR = rand.nextF();
    color->fG = rand.nextF();
    color->fB = rand.nextF();
}

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose")) {
            gVerbose = true;
        }
    }

    Stats opaqueStats = { 0, 0, make_opaque_color };
    run_tests(&opaqueStats);
    printf("Opaque      %g%%\n", opaqueStats.percent());

    Stats translucentStats = { 0, 0, make_translucent_color };
    run_tests(&translucentStats);
    printf("Translucent %g%%\n", translucentStats.percent());

    return 0;
}


/**
 *  Copyright 2013 Mike Reed
 *
 *  COMP 590 -- Fall 2013
 */

#include <string.h>
#include <string>

#include "GContext.h"
#include "GBitmap.h"
#include "GColor.h"
#include "GRandom.h"
#include "GIRect.h"

static const GColor gGColor_TRANSPARENT_BLACK = { 0, 0, 0, 0 };
static const GColor gGColor_BLACK = { 1, 0, 0, 0 };
static const GColor gGColor_WHITE = { 1, 1, 1, 1 };

static void make_filename(std::string* str, const char path[], const char name[]) {
    str->append(path);
    if ('/' != (*str)[str->size() - 1]) {
        str->append("/");
    }
    str->append(name);
    str->append(".png");
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

static void translate(GIRect* r, int dx, int dy) {
    r->fLeft += dx;
    r->fTop += dy;
    r->fRight += dx;
    r->fBottom += dy;
}

static void make_rand_rect(GRandom& rand, GIRect* r, int w, int h) {
    int cx = rand.nextRange(0, w);
    int cy = rand.nextRange(0, h);
    int cw = rand.nextRange(1, w/4);
    int ch = rand.nextRange(1, h/4);
    r->setXYWH(cx - cw/2, cy - ch/2, cw, ch);
}

typedef GContext* (*ImageProc)(const char**);

// Draw a grid of primary colors
static GContext* image_primaries(const char** name) {
    const int W = 64;
    const int H = 64;
    const GColor colors[] = {
        { 1, 1, 0, 0 }, { 1, 0, 1, 0 },          { 1, 0, 0, 1 },
        { 1, 1, 1, 0 }, { 1, 1, 0, 1 },          { 1, 0, 1, 1 },
        { 1, 0, 0, 0 }, { 1, 0.5f, 0.5f, 0.5f }, { 1, 1, 1, 1 },
    };
    
    GContext* ctx = GContext::Create(W*3, H*3);
    ctx->clear(gGColor_TRANSPARENT_BLACK);
    
    const GColor* colorPtr = colors;
    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 3; ++x) {
            ctx->fillIRect(GIRect::MakeXYWH(x * W, y * H, W, H), *colorPtr++);
        }
    }
    
    *name = "primaries";
    return ctx;
}

static float lerp(float x0, float x1, float percent) {
    return x0 + (x1 - x0) * percent;
}

static void lerp(const GColor& c0, const GColor& c1, float percent, GColor* result) {
    result->fA = lerp(c0.fA, c1.fA, percent);
    result->fR = lerp(c0.fR, c1.fR, percent);
    result->fG = lerp(c0.fG, c1.fG, percent);
    result->fB = lerp(c0.fB, c1.fB, percent);
}

static GContext* image_ramp(const char** name) {
    const int W = 200;
    const int H = 100;
    const GColor c0 = { 1, 1, 0, 0 };
    const GColor c1 = { 1, 0, 1, 1 };

    GContext* ctx = GContext::Create(W, H);
    ctx->clear(gGColor_TRANSPARENT_BLACK);

    GIRect r = GIRect::MakeWH(1, H);
    for (int x = 0; x < W; ++x) {
        GColor color;
        lerp(c0, c1, x * 1.0f / W, &color);
        ctx->fillIRect(r, color);
        translate(&r, 1, 0);
    }
    *name = "ramp";
    return ctx;
}

static GContext* image_rand(const char** name) {
    const int N = 8;
    const int W = N * 40;
    const int H = N * 40;
    
    GContext* ctx = GContext::Create(W, H);
    ctx->clear(gGColor_TRANSPARENT_BLACK);
    
    GRandom rand;
    for (int y = 0; y < H; y += N) {
        for (int x = 0; x < W; x += N) {
            GColor color;
            make_opaque_color(rand, &color);
            ctx->fillIRect(GIRect::MakeXYWH(x, y, N, N), color);
        }
    }
    *name = "rand";
    return ctx;
}

static GContext* image_blend(const char** name) {
    const int W = 500;
    const int H = 500;
    
    GContext* ctx = GContext::Create(W, H);
    ctx->clear(gGColor_BLACK);
    
    GRandom rand;
    for (int i = 0; i < 1000; ++i) {
        GColor color;
        make_translucent_color(rand, &color);
        color.fA /= 2;

        GIRect r;
        make_rand_rect(rand, &r, W, H);

        ctx->fillIRect(r, color);
    }
    *name = "blend";
    return ctx;
}

static const ImageProc gProcs[] = {
    image_primaries, image_ramp, image_rand, image_blend,
};

int main(int argc, char** argv) {
    const char* writePath = NULL;

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--help")) {
            printf("generates a series of test images.\n"
                   "--write foo (or -w foo) writes the images as *.png files to foo directory\n");
            exit(0);
        }
        if (!strcmp(argv[i], "-w") || !strcmp(argv[i], "--write")) {
            if (i == argc - 1) {
                fprintf(stderr, "need path following -w or --write\n");
                exit(-1);
            }
            writePath = argv[++i];
        }
    }

    for (int i = 0; i < GARRAY_COUNT(gProcs); ++i) {
        const char* name = NULL;
        GContext* ctx = gProcs[i](&name);
        printf("drawing... %s\n", name);
        if (writePath) {
            std::string path;
            make_filename(&path, writePath, name);
            remove(path.c_str());

            GBitmap bm;
            ctx->getBitmap(&bm);
            if (!GWriteBitmapToFile(bm, path.c_str())) {
                fprintf(stderr, "failed to write image to %s\n", path.c_str());
            }
        }
        delete ctx;
    }
    return 0;
}


/**
 *  Copyright 2013 Mike Reed
 *
 *  COMP 590 -- Fall 2013
 */

#include "GSlide.h"
#include "GXWindow.h"
#include "GBitmap.h"
#include "GPaint.h"
#include "GContext.h"
#include "GRect.h"
#include "GRandom.h"
#include "GTime.h"
#include "app_utils.h"
#include <string>
#include <math.h>

static const float gMaxScale = 32;
static const float gMinScale = 1 / gMaxScale;
static const GMSec gSlideDuration = 7 * 1000;

class SlideWindow : public GXWindow {
    const GBitmap*  fBitmaps;
    int             fBitmapCount;

    float           fScale;

    GSlide*         fSlide;
    GSlide::Pair*   fSlideArray;
    int             fSlideCount;
    int             fSlideIndex;
    
    GMSec           fNextSlideChangeMSec;
    bool            fAnimating;
    int             fShift;
    GRandom         fRand;

    void updateTitle() {
        char buffer[100];
        sprintf(buffer, "%s : scale=%g", fSlide->name(), fScale);
        this->setTitle(buffer);
    }

    void initSlide() {
        delete fSlide;
        fSlide = fSlideArray[fSlideIndex].fFact(fSlideArray[fSlideIndex].fRefCon);
        fSlide->initWithBitmaps(fBitmaps, fBitmapCount);
        this->updateTitle();
    }

    void scaleAboutCenter(GContext* ctx) {
        GBitmap bm;
        ctx->getBitmap(&bm);
        
        float cx = bm.width() * 0.5f;
        float cy = bm.height() * 0.5f;
        
        ctx->translate(cx, cy);
        ctx->scale(fScale, fScale);
        ctx->translate(-cx, -cy);
    }

    void prevSlide() {
        if (--fSlideIndex < 0) {
            fSlideIndex = fSlideCount - 1;
        }
        this->initSlide();
    }
    
    void nextSlide() {
        if (++fSlideIndex >= fSlideCount) {
            fSlideIndex = 0;
        }
        this->initSlide();
    }

public:
    SlideWindow(int w, int h,
               const GBitmap bitmaps[], int bitmapCount) : GXWindow(w, h) {
        fScale = 1;

        fBitmaps = bitmaps;
        fBitmapCount = bitmapCount;

        fNextSlideChangeMSec = 0;
        fAnimating = true;
        fShift = 0;

        fSlide = NULL;
        fSlideArray = GSlide::CopyPairArray(&fSlideCount);
        fSlideIndex = 0;
        this->initSlide();
    }

    virtual ~SlideWindow() {
        for (int i = 0; i < fBitmapCount; ++i) {
            free(fBitmaps[i].fPixels);
        }
        delete fSlide;
        delete[] fSlideArray;
    }

    static void Scale(GContext* ctx, const GBitmap& numer, const GBitmap& denom) {
        ctx->scale(numer.fWidth * 1.0 / denom.fWidth,
                   numer.fHeight * 1.0 / denom.fHeight);
    }

    void drawSlide(GSlide* slide, GContext* ctx) {
        const int shift = fShift;

        if (0 == shift) {
            slide->draw(ctx);
            return;
        }
        GBitmap dst;
        ctx->getBitmap(&dst);

        GAutoDelete<GContext> ctx2(GContext::Create(dst.fWidth, dst.fHeight));
        GBitmap src;
        ctx2->getBitmap(&src);
        src.fWidth >>= shift;
        src.fHeight >>= shift;

        if (false) {
            slide->draw(ctx2);
            GPaint paint;
            paint.setAlpha(0.5);
            ctx->drawBitmap(src, 0, 0, paint);
            return;
        }
        Scale(ctx2, src, dst);
        slide->draw(ctx2);
        
        ctx->save();
        Scale(ctx, dst, src);
        ctx->drawBitmap(src, 0, 0, GPaint());
        ctx->restore();
    }

protected:
    virtual void onDraw(GContext* ctx) {
        ctx->clear(GColor::Make(1, 1, 1, 1));

        GAutoRestoreToCount artc(ctx);
        ctx->save();
        this->scaleAboutCenter(ctx);

        this->drawSlide(fSlide, ctx);

        if (fAnimating) {
            this->requestDraw();
        }

        if (fNextSlideChangeMSec) {
            GMSec now = GTime::GetMSec();
            if (now >= fNextSlideChangeMSec) {
                this->nextSlide();
                fNextSlideChangeMSec = now + gSlideDuration;
                
                fShift = 0;//(fRand.nextU() >> 13) & 3;
            }
        } else {
            fShift = 0;
        }
    }
    
    virtual bool onKeyPress(const XEvent& evt, KeySym sym) {
        if (!fAnimating) {
            fAnimating = true;
            this->requestDraw();
        }

        switch (sym) {
            case XK_Return:
                if (fNextSlideChangeMSec) {
                    fNextSlideChangeMSec = 0;
                } else {
                    fNextSlideChangeMSec = GTime::GetMSec();
                }
                return true;
            case XK_Up:
                if (fScale < gMaxScale) {
                    fScale *= 2;
                    this->updateTitle();
                }
                return true;
            case XK_Down:
                if (fScale > gMinScale) {
                    fScale /= 2;
                    this->updateTitle();
                }
                return true;
            case XK_Left:
                this->prevSlide();
                return true;
            case XK_Right:
                this->nextSlide();
                return true;
        }
        if ((sym <= 0x7F) && fSlide->handleKey(sym)) {
            return true;
        }
        return this->INHERITED::onKeyPress(evt, sym);
    }

    virtual void onResize(int w, int h) {
        fAnimating = false;
        this->INHERITED::onResize(w, h);
    }

private:
    typedef GXWindow INHERITED;
};

int main(int argc, char const* const* argv) {
    int fileCount = argc - 1;
    int bitmapCount = 0;
    GBitmap* bitmaps = new GBitmap[fileCount];
    for (int i = 0; i < fileCount; ++i) {
        if (GReadBitmapFromFile(argv[i + 1], &bitmaps[i])) {
            bitmapCount += 1;
        }
    }

    return SlideWindow(640, 480, bitmaps, bitmapCount).run();
}


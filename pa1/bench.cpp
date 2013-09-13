#include "GContext.h"
#include "GBitmap.h"
#include "GColor.h"
#include "GTime.h"

#include <string.h>
#include <stdlib.h>

static bool gVerbose;

static double time_erase(GContext* ctx, const GColor& color, int repeatCount) {
    GBitmap bm;
    ctx->getBitmap(&bm);

    int loop = 10 * 1000 * repeatCount;
    
    GMSec before = GTime::GetMSec();
    
    for (int i = 0; i < loop; ++i) {
        ctx->clear(color);
    }
    
    GMSec dur = GTime::GetMSec() - before;
    
    return dur * 1000.0 / (bm.fWidth * bm.fHeight) / repeatCount;
}

struct Size {
    int fW, fH;
};

const int DIM = 1 << 8;

static const Size gSizes[] = {
    { DIM * DIM, 1 },
    { 1, DIM * DIM },
    { DIM, DIM },
};

int main(int argc, char** argv) {
    int repeatCount = 1;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose")) {
            gVerbose = true;
        } else if (!strcmp(argv[i], "--repeat")) {
            if (i == argc - 1) {
                fprintf(stderr, "need valid repeat_count # after --repeat\n");
                exit(-1);
            }
            int n = (int)atol(argv[i + 1]);
            if (n > 0) {
                repeatCount = n;
            } else {
                fprintf(stderr, "repeat value needs to be > 0\n");
                exit(-1);
            }
        }
    }
    
    const GColor color = { 0.5, 1, 0.5, 0 };
    double total = 0;

    for (int i = 0; i < GARRAY_COUNT(gSizes); ++i) {
        const int w = gSizes[i].fW;
        const int h = gSizes[i].fH;
        
        GContext* ctx = GContext::Create(w, h);
        if (!ctx) {
            fprintf(stderr, "GContext::Create failed [%d %d]\n", w, h);
            return -1;
        }

        double dur = time_erase(ctx, color, repeatCount);
        if (gVerbose) {
            printf("[%5d, %5d] %8.4f per-pixel\n", w, h, dur);
        }
        delete ctx;
        
        total += dur;
    }
    printf("Average time   %8.4f per-pixel\n", total / GARRAY_COUNT(gSizes));
    return 0;
}


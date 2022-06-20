#include "bm.h"
void Wrblk2bm(uint32_t* bm, uint32_t blk, int o) {
    int r, off;
    r = blk / 32;
    off = blk % 32;
    if (o) {
        bm[r] |= (1 << (31 - off));
    } else {
        bm[r] &= ~(1 << (31 - off));
    }
}
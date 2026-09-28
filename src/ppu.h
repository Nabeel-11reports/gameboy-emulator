#ifndef GB_PPU_H
#define GB_PPU_H

#include "common.h"
#include "bus.h"

typedef struct {
    int line_cycles;
    uint32_t frame[GB_SCREEN_W * GB_SCREEN_H];
    bool frame_ready;
} PPU;

void ppu_init(PPU *ppu);
void ppu_step(PPU *ppu, Bus *bus, int cycles);

#endif

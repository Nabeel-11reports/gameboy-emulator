#ifndef GB_APU_H
#define GB_APU_H

#include "common.h"
#include <SDL2/SDL.h>

typedef struct {
    SDL_AudioDeviceID device;
    SDL_AudioSpec spec;
    volatile bool enabled;
    volatile float freq1;
    volatile float freq2;
    volatile float vol1;
    volatile float vol2;
    float phase1;
    float phase2;
} APU;

bool apu_init(APU *apu);
void apu_shutdown(APU *apu);
void apu_write(APU *apu, uint16_t address, uint8_t value);

#endif

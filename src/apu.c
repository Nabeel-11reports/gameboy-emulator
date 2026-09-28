#include "apu.h"
#include <math.h>
#include <string.h>

static float freq_from_period(uint16_t raw) {
    if (raw >= 2048) raw = 2047;
    double hz = 131072.0 / (2048.0 - (double)raw);
    if (hz < 20.0) hz = 20.0;
    if (hz > 12000.0) hz = 12000.0;
    return (float)hz;
}

static void audio_callback(void *userdata, Uint8 *stream, int len) {
    APU *apu = (APU *)userdata;
    float *out = (float *)stream;
    int frames = len / (int)(sizeof(float) * 2u);
    float rate = (float)apu->spec.freq;

    for (int i = 0; i < frames; ++i) {
        float s = 0.0f;
        if (apu->enabled && apu->vol1 > 0.0f) {
            s += (apu->phase1 < 0.5f ? 1.0f : -1.0f) * apu->vol1;
            apu->phase1 += apu->freq1 / rate;
            if (apu->phase1 >= 1.0f) apu->phase1 -= floorf(apu->phase1);
        }
        if (apu->enabled && apu->vol2 > 0.0f) {
            s += (apu->phase2 < 0.5f ? 1.0f : -1.0f) * apu->vol2;
            apu->phase2 += apu->freq2 / rate;
            if (apu->phase2 >= 1.0f) apu->phase2 -= floorf(apu->phase2);
        }
        if (s > 1.0f) s = 1.0f;
        if (s < -1.0f) s = -1.0f;
        out[i * 2] = s;
        out[i * 2 + 1] = s;
    }
}

bool apu_init(APU *apu) {
    memset(apu, 0, sizeof(*apu));
    SDL_AudioSpec want;
    SDL_zero(want);
    want.freq = 48000;
    want.format = AUDIO_F32SYS;
    want.channels = 2;
    want.samples = 1024;
    want.callback = audio_callback;
    want.userdata = apu;

    apu->device = SDL_OpenAudioDevice(NULL, 0, &want, &apu->spec, 0);
    if (!apu->device) return false;
    apu->freq1 = 440.0f;
    apu->freq2 = 660.0f;
    SDL_PauseAudioDevice(apu->device, 0);
    return true;
}

void apu_shutdown(APU *apu) {
    if (apu->device) SDL_CloseAudioDevice(apu->device);
    apu->device = 0;
}

void apu_write(APU *apu, uint16_t address, uint8_t value) {
    static uint8_t regs[0x30];
    uint16_t idx = (uint16_t)(address - 0xFF10u);
    if (idx < sizeof(regs)) regs[idx] = value;

    if (address == 0xFF12u) {
        apu->vol1 = (float)((value >> 4) & 0x0Fu) / 15.0f * 0.15f;
    } else if (address == 0xFF17u) {
        apu->vol2 = (float)((value >> 4) & 0x0Fu) / 15.0f * 0.15f;
    } else if (address == 0xFF14u && (value & 0x80u)) {
        uint16_t raw = (uint16_t)(regs[0x03] | ((regs[0x04] & 0x07u) << 8));
        apu->freq1 = freq_from_period(raw);
        apu->enabled = true;
    } else if (address == 0xFF19u && (value & 0x80u)) {
        uint16_t raw = (uint16_t)(regs[0x08] | ((regs[0x09] & 0x07u) << 8));
        apu->freq2 = freq_from_period(raw);
        apu->enabled = true;
    } else if (address == 0xFF26u) {
        apu->enabled = (value & 0x80u) != 0;
        if (!apu->enabled) {
            apu->vol1 = 0.0f;
            apu->vol2 = 0.0f;
        }
    }
}

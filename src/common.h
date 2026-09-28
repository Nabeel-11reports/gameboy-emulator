#ifndef GB_COMMON_H
#define GB_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define GB_SCREEN_W 160
#define GB_SCREEN_H 144
#define GB_RAM_SIZE 0x10000u

static inline uint16_t make16(uint8_t hi, uint8_t lo) {
    return (uint16_t)(((uint16_t)hi << 8) | lo);
}

#endif

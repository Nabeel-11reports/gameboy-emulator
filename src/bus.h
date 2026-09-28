#ifndef GB_BUS_H
#define GB_BUS_H

#include "common.h"
#include "cartridge.h"
#include "input.h"
#include "apu.h"

typedef struct {
    Cartridge *cart;
    Input *input;
    APU *apu;
    uint8_t mem[GB_RAM_SIZE];
} Bus;

void bus_init(Bus *bus, Cartridge *cart, Input *input, APU *apu);
uint8_t bus_read8(Bus *bus, uint16_t address);
void bus_write8(Bus *bus, uint16_t address, uint8_t value);
uint16_t bus_read16(Bus *bus, uint16_t address);
void bus_write16(Bus *bus, uint16_t address, uint16_t value);

#endif

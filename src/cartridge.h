#ifndef GB_CARTRIDGE_H
#define GB_CARTRIDGE_H

#include "common.h"

typedef enum {
    CART_MBC0,
    CART_MBC1
} CartType;

typedef struct {
    uint8_t *rom;
    size_t rom_size;
    uint8_t ram[0x8000];
    size_t ram_size;
    CartType type;
    bool ram_enable;
    uint8_t rom_bank_low5;
    uint8_t bank_high2;
    uint8_t banking_mode;
} Cartridge;

bool cartridge_load(Cartridge *cart, const char *path);
void cartridge_free(Cartridge *cart);
uint8_t cartridge_read(Cartridge *cart, uint16_t address);
void cartridge_write(Cartridge *cart, uint16_t address, uint8_t value);
const char *cartridge_title(const Cartridge *cart);

#endif

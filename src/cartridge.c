#include "cartridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t decode_ram_size(uint8_t code) {
    switch (code) {
        case 0x00: return 0;
        case 0x01: return 0x800;
        case 0x02: return 0x2000;
        case 0x03: return 0x8000;
        case 0x04: return 0x20000;
        case 0x05: return 0x10000;
        default: return 0;
    }
}

bool cartridge_load(Cartridge *cart, const char *path) {
    memset(cart, 0, sizeof(*cart));

    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Could not open ROM: %s\n", path);
        return false;
    }

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return false; }
    long n = ftell(f);
    if (n <= 0) { fclose(f); return false; }
    rewind(f);

    cart->rom_size = (size_t)n;
    cart->rom = (uint8_t *)malloc(cart->rom_size);
    if (!cart->rom) {
        fclose(f);
        fprintf(stderr, "Out of memory reading ROM.\n");
        return false;
    }

    if (fread(cart->rom, 1, cart->rom_size, f) != cart->rom_size) {
        fclose(f);
        cartridge_free(cart);
        fprintf(stderr, "Could not read ROM.\n");
        return false;
    }
    fclose(f);

    uint8_t type = cart->rom_size > 0x147 ? cart->rom[0x147] : 0;
    cart->ram_size = cart->rom_size > 0x149 ? decode_ram_size(cart->rom[0x149]) : 0;
    if (cart->ram_size > sizeof(cart->ram)) cart->ram_size = sizeof(cart->ram);

    switch (type) {
        case 0x01: case 0x02: case 0x03:
            cart->type = CART_MBC1;
            break;
        default:
            cart->type = CART_MBC0;
            if (type != 0x00) {
                fprintf(stderr, "Warning: cartridge type 0x%02X is not implemented; treating as MBC0.\n", type);
            }
            break;
    }

    cart->rom_bank_low5 = 1;
    return true;
}

void cartridge_free(Cartridge *cart) {
    free(cart->rom);
    cart->rom = NULL;
    cart->rom_size = 0;
}

static size_t rom_index(const Cartridge *cart, size_t index) {
    if (cart->rom_size == 0) return 0;
    return index % cart->rom_size;
}

uint8_t cartridge_read(Cartridge *cart, uint16_t address) {
    if (address < 0x4000) {
        size_t bank = 0;
        if (cart->type == CART_MBC1 && cart->banking_mode == 1) {
            bank = (size_t)(cart->bank_high2 & 0x03u) << 5;
        }
        return cart->rom[rom_index(cart, bank * 0x4000u + address)];
    }

    if (address < 0x8000) {
        size_t bank;
        if (cart->type == CART_MBC1) {
            bank = cart->rom_bank_low5 & 0x1Fu;
            if (bank == 0) bank = 1;
            if (cart->banking_mode == 0) bank |= (size_t)(cart->bank_high2 & 0x03u) << 5;
        } else {
            bank = 1;
        }
        return cart->rom[rom_index(cart, bank * 0x4000u + (address - 0x4000u))];
    }

    if (address >= 0xA000 && address < 0xC000 && cart->ram_size > 0 && cart->ram_enable) {
        size_t bank = 0;
        if (cart->type == CART_MBC1 && cart->banking_mode == 1) {
            bank = cart->bank_high2 & 0x03u;
        }
        size_t index = bank * 0x2000u + (address - 0xA000u);
        if (index < cart->ram_size) return cart->ram[index];
    }

    return 0xFF;
}

void cartridge_write(Cartridge *cart, uint16_t address, uint8_t value) {
    if (cart->type == CART_MBC1) {
        if (address < 0x2000) {
            cart->ram_enable = (value & 0x0Fu) == 0x0Au;
        } else if (address < 0x4000) {
            cart->rom_bank_low5 = value & 0x1Fu;
            if (cart->rom_bank_low5 == 0) cart->rom_bank_low5 = 1;
        } else if (address < 0x6000) {
            cart->bank_high2 = value & 0x03u;
        } else if (address < 0x8000) {
            cart->banking_mode = value & 1u;
        } else if (address >= 0xA000 && address < 0xC000 && cart->ram_size > 0 && cart->ram_enable) {
            size_t bank = cart->banking_mode ? (cart->bank_high2 & 0x03u) : 0;
            size_t index = bank * 0x2000u + (address - 0xA000u);
            if (index < cart->ram_size) cart->ram[index] = value;
        }
        return;
    }

    if (address >= 0xA000 && address < 0xC000 && cart->ram_size > 0) {
        cart->ram[address - 0xA000u] = value;
    }
}

const char *cartridge_title(const Cartridge *cart) {
    static char title[17];
    memset(title, 0, sizeof(title));
    if (cart->rom_size < 0x144) return "(unknown)";
    memcpy(title, cart->rom + 0x134, 16);
    for (int i = 15; i >= 0; --i) {
        if ((unsigned char)title[i] < 0x20 || title[i] == ' ') title[i] = '\0';
        else break;
    }
    return title;
}

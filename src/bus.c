#include "bus.h"
#include <string.h>

void bus_init(Bus *bus, Cartridge *cart, Input *input, APU *apu) {
    memset(bus, 0, sizeof(*bus));
    bus->cart = cart;
    bus->input = input;
    bus->apu = apu;
    bus->mem[0xFF00] = 0xCF;
    bus->mem[0xFF40] = 0x00;
    bus->mem[0xFF41] = 0x85;
    bus->mem[0xFF44] = 0;
    bus->mem[0xFF47] = 0xFC;
    bus->mem[0xFF48] = 0xFF;
    bus->mem[0xFF49] = 0xFF;
    bus->mem[0xFF50] = 0x01;
    bus->mem[0xFF26] = 0xF1;
}

uint8_t bus_read8(Bus *bus, uint16_t address) {
    if (address < 0x8000u || (address >= 0xA000u && address < 0xC000u)) {
        return cartridge_read(bus->cart, address);
    }

    if (address == 0xFF00u) return input_read_joyp(bus->input, bus->mem[0xFF00]);
    if (address == 0xFF04u) return bus->mem[0xFF04];
    return bus->mem[address];
}

void bus_write8(Bus *bus, uint16_t address, uint8_t value) {
    if (address < 0x8000u || (address >= 0xA000u && address < 0xC000u)) {
        cartridge_write(bus->cart, address, value);
        return;
    }

    if (address == 0xFF00u) {
        bus->mem[0xFF00] = (uint8_t)((value & 0x30u) | 0xCFu);
        return;
    }

    if (address == 0xFF04u) {
        bus->mem[0xFF04] = 0;
        return;
    }

    if (address >= 0xFF10u && address <= 0xFF26u && bus->apu) {
        bus->mem[address] = value;
        apu_write(bus->apu, address, value);
        return;
    }

    if (address == 0xFF44u) {
        bus->mem[0xFF44] = 0;
        return;
    }

    /* Echo RAM: C000-DDFF mirrors E000-FDFF. */
    if (address >= 0xE000u && address < 0xFE00u) {
        uint16_t mirror = (uint16_t)(address - 0x2000u);
        bus->mem[address] = value;
        bus->mem[mirror] = value;
        return;
    }
    if (address >= 0xC000u && address < 0xDE00u) {
        uint16_t mirror = (uint16_t)(address + 0x2000u);
        bus->mem[address] = value;
        if (mirror < 0xFE00u) bus->mem[mirror] = value;
        return;
    }

    bus->mem[address] = value;
}

uint16_t bus_read16(Bus *bus, uint16_t address) {
    uint8_t lo = bus_read8(bus, address);
    uint8_t hi = bus_read8(bus, (uint16_t)(address + 1u));
    return make16(hi, lo);
}

void bus_write16(Bus *bus, uint16_t address, uint16_t value) {
    bus_write8(bus, address, (uint8_t)(value & 0xFFu));
    bus_write8(bus, (uint16_t)(address + 1u), (uint8_t)(value >> 8));
}

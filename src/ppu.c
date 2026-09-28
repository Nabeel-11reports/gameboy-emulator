#include "ppu.h"
#include <string.h>

static uint32_t shade_color(int shade) {
    static const uint32_t colors[4] = {
        0xFFE0F8D0u, /* lightest */
        0xFF88C070u,
        0xFF346856u,
        0xFF081820u  /* darkest */
    };
    return colors[shade & 3];
}

static int pal_lookup(uint8_t pal, int color_id) {
    return (pal >> (color_id * 2)) & 3;
}

static uint16_t tile_address(Bus *bus, uint8_t tile_index) {
    uint8_t lcdc = bus->mem[0xFF40];
    if (lcdc & 0x10u) return (uint16_t)(0x8000u + tile_index * 16u);
    int8_t signed_index = (int8_t)tile_index;
    return (uint16_t)(0x9000 + signed_index * 16);
}

static void render_line(PPU *ppu, Bus *bus, int ly) {
    uint8_t lcdc = bus->mem[0xFF40];
    uint8_t scy = bus->mem[0xFF42];
    uint8_t scx = bus->mem[0xFF43];
    uint8_t bgp = bus->mem[0xFF47];

    bool bg_on = (lcdc & 0x01u) != 0;
    uint16_t bg_map = (lcdc & 0x08u) ? 0x9C00u : 0x9800u;

    for (int x = 0; x < GB_SCREEN_W; ++x) {
        int color = 0;
        if (bg_on) {
            int bx = (x + scx) & 0xFF;
            int by = (ly + scy) & 0xFF;
            uint16_t tile_map = (uint16_t)(bg_map + (by / 8) * 32 + (bx / 8));
            uint8_t tile = bus_read8(bus, tile_map);
            uint16_t addr = tile_address(bus, tile);
            int row = by & 7;
            int col = bx & 7;
            uint8_t lo = bus_read8(bus, (uint16_t)(addr + row * 2));
            uint8_t hi = bus_read8(bus, (uint16_t)(addr + row * 2 + 1));
            int bit = 7 - col;
            int raw = ((hi >> bit) & 1) * 2 + ((lo >> bit) & 1);
            color = pal_lookup(bgp, raw);
        }
        ppu->frame[ly * GB_SCREEN_W + x] = shade_color(color);
    }

    if ((lcdc & 0x20u) && ly >= bus->mem[0xFF4A]) {
        int wx = (int)bus->mem[0xFF4B] - 7;
        int wy = bus->mem[0xFF4A];
        uint16_t win_map = (lcdc & 0x40u) ? 0x9C00u : 0x9800u;
        int window_line = ly - wy;
        if (window_line >= 0) {
            for (int x = 0; x < GB_SCREEN_W; ++x) {
                if (x < wx) continue;
                int wxp = x - wx;
                int wyp = window_line;
                int bx = wxp & 0xFF;
                int by = wyp & 0xFF;
                uint16_t map_addr = (uint16_t)(win_map + (by / 8) * 32 + (bx / 8));
                uint8_t tile = bus_read8(bus, map_addr);
                int row = by & 7;
                int col = bx & 7;
                uint16_t addr = tile_address(bus, tile);
                uint8_t lo = bus_read8(bus, (uint16_t)(addr + row * 2));
                uint8_t hi = bus_read8(bus, (uint16_t)(addr + row * 2 + 1));
                int bit = 7 - col;
                int raw = ((hi >> bit) & 1) * 2 + ((lo >> bit) & 1);
                ppu->frame[ly * GB_SCREEN_W + x] = shade_color(pal_lookup(bus->mem[0xFF47], raw));
            }
        }
    }

    /* Simple DMG sprite pass: enough for ordinary 8x8 sprite use. */
    if (lcdc & 0x02u) {
        int sprite_h = (lcdc & 0x04u) ? 16 : 8;
        for (int i = 0; i < 40; ++i) {
            uint16_t o = (uint16_t)(0xFE00u + i * 4);
            int sy = (int)bus->mem[o] - 16;
            int sx = (int)bus->mem[o + 1] - 8;
            uint8_t tile = bus->mem[o + 2];
            uint8_t attr = bus->mem[o + 3];
            if (ly < sy || ly >= sy + sprite_h) continue;

            int row = ly - sy;
            if (attr & 0x40u) row = sprite_h - 1 - row;
            if (sprite_h == 16) {
                tile &= 0xFEu;
                if (row >= 8) tile = (uint8_t)(tile + 1);
            }
            uint16_t addr = (uint16_t)(0x8000u + tile * 16u + (row & 7) * 2);
            uint8_t lo = bus_read8(bus, addr);
            uint8_t hi = bus_read8(bus, (uint16_t)(addr + 1));
            uint8_t pal = (attr & 0x10u) ? bus->mem[0xFF49] : bus->mem[0xFF48];

            for (int px = 0; px < 8; ++px) {
                int screen_x = sx + px;
                if (screen_x < 0 || screen_x >= GB_SCREEN_W) continue;
                int bit = (attr & 0x20u) ? px : (7 - px);
                int raw = ((hi >> bit) & 1) * 2 + ((lo >> bit) & 1);
                if (raw == 0) continue;
                if ((attr & 0x80u) && ((ppu->frame[ly * GB_SCREEN_W + screen_x] != shade_color(0)))) continue;
                int shade = pal_lookup(pal, raw);
                ppu->frame[ly * GB_SCREEN_W + screen_x] = shade_color(shade);
            }
        }
    }
}

static void update_stat(PPU *ppu, Bus *bus) {
    uint8_t lcdc = bus->mem[0xFF40];
    if (!(lcdc & 0x80u)) {
        bus->mem[0xFF41] = (uint8_t)((bus->mem[0xFF41] & 0xF8u) | 0u);
        return;
    }
    int mode;
    uint8_t ly = bus->mem[0xFF44];
    if (ly >= 144) mode = 1;
    else if (ppu->line_cycles < 80) mode = 2;
    else if (ppu->line_cycles < 252) mode = 3;
    else mode = 0;
    bus->mem[0xFF41] = (uint8_t)((bus->mem[0xFF41] & 0xF8u) | mode);
    if (bus->mem[0xFF45] == ly) bus->mem[0xFF41] |= 0x04u;
    else bus->mem[0xFF41] &= (uint8_t)~0x04u;
}

void ppu_init(PPU *ppu) {
    memset(ppu, 0, sizeof(*ppu));
    for (size_t i = 0; i < sizeof(ppu->frame) / sizeof(ppu->frame[0]); ++i) ppu->frame[i] = shade_color(0);
}

void ppu_step(PPU *ppu, Bus *bus, int cycles) {
    if (!(bus->mem[0xFF40] & 0x80u)) {
        bus->mem[0xFF44] = 0;
        ppu->line_cycles = 0;
        update_stat(ppu, bus);
        return;
    }

    while (cycles > 0) {
        int until_line = 456 - ppu->line_cycles;
        int step = cycles < until_line ? cycles : until_line;
        ppu->line_cycles += step;
        cycles -= step;
        update_stat(ppu, bus);

        if (ppu->line_cycles >= 456) {
            ppu->line_cycles -= 456;
            uint8_t ly = bus->mem[0xFF44];
            if (ly < 144) render_line(ppu, bus, ly);
            ly++;
            if (ly == 144) {
                bus->mem[0xFF0F] |= 0x01u;
                ppu->frame_ready = true;
            }
            if (ly > 153) ly = 0;
            bus->mem[0xFF44] = ly;
        }
    }
    update_stat(ppu, bus);
}

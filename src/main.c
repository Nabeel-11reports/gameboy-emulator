#include "common.h"
#include "cartridge.h"
#include "bus.h"
#include "cpu.h"
#include "ppu.h"
#include "timer.h"
#include "input.h"
#include "apu.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void make_demo_rom(uint8_t *rom, size_t size) {
    memset(rom, 0, size);
    /* A tiny educational ROM that writes tile 0, enables LCD, then loops. */
    size_t p = 0x100;
    const uint8_t program[] = {
        0x21,0x00,0x80,             /* LD HL,$8000 */
        0x3E,0x81, 0x22,            /* LD A,$81 / LD (HL+),A */
        0x3E,0x42, 0x22,
        0x3E,0x24, 0x22,
        0x3E,0x18, 0x22,
        0x3E,0x24, 0x22,
        0x3E,0x42, 0x22,
        0x3E,0x81, 0x22,
        0x3E,0x00, 0x22,
        0x21,0x47,0xFF,             /* LD HL,$FF47 */
        0x3E,0xE4,0x77,             /* LD A,$E4 / LD (HL),A */
        0x21,0x40,0xFF,
        0x3E,0x91,0x77,             /* LCDC = on + BG + tile data */
        0x18,0xFE                    /* JR -2 */
    };
    memcpy(rom + p, program, sizeof(program));
}

static bool load_or_demo(Cartridge *cart, const char *path) {
    if (path) return cartridge_load(cart, path);
    cart->rom_size = 0x8000;
    cart->rom = (uint8_t*)malloc(cart->rom_size);
    if (!cart->rom) return false;
    cart->type = CART_MBC0;
    make_demo_rom(cart->rom, cart->rom_size);
    return true;
}

int main(int argc, char **argv) {
    const char *rom_path = argc >= 2 ? argv[1] : NULL;
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [game.gb]\n", argv[0]);
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    Cartridge cart;
    if (!load_or_demo(&cart, rom_path)) {
        SDL_Quit();
        return 1;
    }

    Input input; input_init(&input);
    APU apu;
    if (!apu_init(&apu)) fprintf(stderr, "Warning: audio unavailable: %s\n", SDL_GetError());
    Bus bus; bus_init(&bus, &cart, &input, &apu);
    CPU cpu; cpu_init(&cpu);
    PPU ppu; ppu_init(&ppu);
    Timer timer; timer_init(&timer);

    if (rom_path) printf("Loaded ROM: %s (%zu bytes), title: %s\n", rom_path, cart.rom_size, cartridge_title(&cart));
    else printf("No ROM supplied. Running the built-in CPU/PPU demonstration ROM.\n");
    printf("Controls: arrows/WASD=d-pad, Z=B, X=A, Backspace=Select, Enter=Start. Escape=quit.\n");

    SDL_Window *window = SDL_CreateWindow("Educational Game Boy Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          GB_SCREEN_W*4, GB_SCREEN_H*4, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, GB_SCREEN_W, GB_SCREEN_H);
    if (!window || !renderer || !texture) {
        fprintf(stderr, "SDL graphics init failed: %s\n", SDL_GetError());
        if (texture) SDL_DestroyTexture(texture);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        apu_shutdown(&apu); cartridge_free(&cart); SDL_Quit(); return 1;
    }

    bool running = true;
    uint64_t total_cycles = 0;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) running = false;
            input_handle_event(&input, &event, &bus.mem[0xFF00], &bus.mem[0xFF0F]);
        }

        int cycles = cpu_step(&cpu, &bus);
        if (cycles <= 0) cycles = 4;
        timer_step(&timer, &bus, cycles);
        ppu_step(&ppu, &bus, cycles);
        total_cycles += (uint64_t)cycles;

        if (ppu.frame_ready) {
            ppu.frame_ready = false;
            SDL_UpdateTexture(texture, NULL, ppu.frame, GB_SCREEN_W * (int)sizeof(uint32_t));
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }

        if ((total_cycles & 0xFFFFFFu) == 0) cpu_dump(&cpu);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    apu_shutdown(&apu);
    cartridge_free(&cart);
    SDL_Quit();
    return 0;
}

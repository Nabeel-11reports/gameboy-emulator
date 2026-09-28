# Presentation Notes — Game Boy Emulator

## Slide 1 — Problem

Goal: emulate the behavior of a Game Boy in software using C.

Core idea:

ROM -> Memory -> CPU -> Video / Input / Timer / Audio

## Slide 2 — Game Boy architecture

- CPU: Sharp SM83-style 8-bit core.
- Registers: A, F, B, C, D, E, H, L plus SP and PC.
- Flags in F: Z (zero), N (subtract), H (half-carry), C (carry).
- Address space: 16-bit, so the CPU can address `0x0000` through `0xFFFF`.
- Graphics: tile-based monochrome display, 160x144 pixels.
- Input: joypad register at `0xFF00`.
- Timing: CPU, PPU and timer must advance together.

Source references: Pan Docs and the gbdev opcode tables.

## Slide 3 — CPU implementation

Explain fetch/decode/execute:

```text
PC -> read opcode -> decode -> execute -> update registers/memory -> repeat
```

The C structure is the virtual CPU state:

```c
CPU cpu;
```

Important fields:

```c
uint8_t a,b,c,d,e,f,h,l;
uint16_t sp,pc;
```

## Slide 4 — Memory / bus

The emulator uses the CPU's address to decide what hardware is being accessed.

Examples:

```text
0x0000-0x7FFF  cartridge ROM
0x8000-0x9FFF  VRAM
0xC000-0xDFFF  work RAM
0xFE00-0xFE9F  sprite/OAM
0xFF00         joypad
0xFF04-0xFF07  timer
0xFF40-0xFF4B  LCD/PPU registers
0xFFFF         interrupt enable
```

This is a classic memory-mapped-I/O design.

## Slide 5 — Graphics

The Game Boy graphics system stores tile data in VRAM.

A background is a tile map; each tile is 8x8 pixels.

The emulator converts tile bits into a 160x144 framebuffer and sends that image to SDL2.

## Slide 6 — Input and sound

Input:

SDL keyboard events update the virtual joypad and `0xFF00`.

Sound:

This project uses a deliberately simplified square-wave APU model and SDL audio output. A production emulator would need the full four-channel Game Boy APU behavior and many timing details.

## Slide 7 — Why emulator development is hard

- Every instruction changes hidden CPU state.
- Flags must be correct, not merely the final value.
- Memory addresses can represent hardware registers.
- Timing matters because video and timers run alongside the CPU.
- Small errors can cause an entire game to fail much later.
- Debugging needs trace logs, register dumps and test ROMs.

## Slide 8 — Cybersecurity relevance

- Reverse engineering: interpreting binary instructions and state.
- Low-level programming: registers, pointers, bit operations and memory.
- System security: understanding trust boundaries between CPU, memory and devices.
- Debugging: tracing unexpected state changes.
- Binary analysis: reading hex and mapping bytes to semantics.

## Slide 9 — Learning outcomes

By the end of the project I can explain and implement:

- basic C program structure
- structs, arrays, pointers and fixed-width integers
- CPU registers and flags
- instruction decoding
- memory-mapped I/O
- timers and interrupts
- tile-based graphics
- keyboard input and audio output
- modular systems programming

## Slide 10 — Limitations / future work

This educational implementation intentionally omits or simplifies:

- full boot ROM behavior
- complete cartridge-controller coverage
- cycle-perfect PPU quirks
- complete APU channel behavior
- many undocumented edge cases
- comprehensive ROM compatibility testing

Future work: add test ROM suites, more cartridge types, exact PPU timing, complete APU emulation and debugging tools.

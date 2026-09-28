# Architecture Walkthrough

## 1. CPU

The original Game Boy uses a Sharp SM83-style 8-bit CPU core. The CPU has 8-bit registers A, F, B, C, D, E, H, L and the 16-bit program counter (PC) and stack pointer (SP). AF, BC, DE and HL can also be treated as register pairs.

In this project, the CPU state is represented by:

```c
typedef struct {
    uint8_t a, f, b, c, d, e, h, l;
    uint16_t sp, pc;
    bool ime;
    bool halted;
    bool stopped;
} CPU;
```

The important idea is that the struct represents **state**, while `cpu_step()` represents the next chunk of **behavior**.

### Fetch/decode/execute

For each instruction:

```text
PC
 |
 | read a byte
 v
opcode
 |
 | decode
 v
operation
 |
 | modify CPU and/or memory
 v
new state
```

Example:

```text
3E 42
```

`0x3E` means `LD A,n8`, and `0x42` is the immediate byte. The CPU therefore ends with `A = 0x42`.

The implementation in `cpu.c` uses the opcode bit fields (`x`, `y`, `z`) to group similar instructions rather than creating 256 completely unrelated functions.

## 2. Memory / bus

The Game Boy uses a 16-bit address space. That means the CPU can generate addresses from `0x0000` through `0xFFFF`.

The important lesson is that an address is not always ordinary RAM. Many addresses are **memory-mapped hardware registers**.

Conceptual map:

```text
0000-7FFF  Cartridge ROM / banked ROM
8000-9FFF  Video RAM
A000-BFFF  Cartridge RAM
C000-DFFF  Work RAM
E000-FDFF  Echo of work RAM
FE00-FE9F  OAM / sprite attributes
FF00-FF7F  I/O registers
FF80-FFFE  High RAM
FFFF       Interrupt Enable
```

`bus_read8()` and `bus_write8()` are therefore the hardware boundary. CPU code does not need to know whether an address is ROM, RAM, VRAM or an I/O register.

## 3. Cartridge and banking

A small Game Boy cartridge can fit directly into the address space. Larger cartridges use a Memory Bank Controller (MBC) to select which 16 KiB ROM bank appears in the `0x4000-0x7FFF` window.

This project implements basic MBC0 and MBC1 behavior.

A bank switch is an important emulator lesson because the CPU may write to an address that is not “normal RAM” at all; the write changes how future reads from a memory window are interpreted.

## 4. PPU / graphics

The display is 160x144 pixels. Game Boy graphics are tile based: an 8x8 tile stores 2 bits per pixel, giving four color indices. Background and window layers refer to tiles through tile maps, while objects/sprites live in OAM and can move independently.

The project models this with:

```text
VRAM -> tile data + tile maps
OAM  -> sprite position/tile/attributes
PPU  -> converts those bits into framebuffer pixels
SDL2 -> presents framebuffer to a Windows window
```

The PPU advances from the same CPU cycles as the CPU. At the end of visible scanlines it produces a frame and requests the VBlank interrupt.

## 5. Timer

The Game Boy has a divider register and a programmable timer. In the emulator they advance from CPU cycles so software that uses the timer sees a changing value.

This is an example of why emulators need a common notion of time.

## 6. Input

SDL2 reports Windows keyboard events. The input layer translates those events into Game Boy joypad state.

Mapping used by this project:

```text
Arrow keys / WASD -> D-pad
Z -> B
X -> A
Backspace -> Select
Enter -> Start
```

The virtual joypad appears to the emulated CPU at `0xFF00`.

## 7. Audio

The original hardware has four audio channels. This project intentionally does not attempt cycle-accurate reproduction of all four channels. Instead it watches the main sound registers and feeds simplified square waves to an SDL2 audio device.

This is useful pedagogically because it shows the path:

```text
ROM writes sound register
        |
        v
memory-mapped I/O
        |
        v
APU model
        |
        v
PCM samples
        |
        v
SDL2 audio device
```

For a fully accurate emulator, this component should be replaced with a real four-channel APU implementation.

## 8. Main loop

`main.c` is the scheduler and integration point:

```c
while (running) {
    handle_input();
    cycles = cpu_step();
    timer_step(cycles);
    ppu_step(cycles);
}
```

There is no real Game Boy CPU executing inside the PC. Your C program is simulating the state transitions that the Game Boy hardware would make.

## 9. Why this maps to cybersecurity

The emulator is a controlled exercise in reading and reproducing machine behavior. That overlaps with several security tasks:

- Binary/reverse engineering: bytes become instructions and state changes.
- Memory analysis: addresses are mapped to different regions and devices.
- Low-level programming: fixed-width integers, bit fields, stacks and registers.
- Debugging: unexpected state must be traced to a specific instruction or memory access.
- Systems security: software interacts with components through defined interfaces and state machines.

The project does not by itself implement a security mechanism; the cybersecurity value is the low-level understanding gained by implementing and debugging a CPU-like system.

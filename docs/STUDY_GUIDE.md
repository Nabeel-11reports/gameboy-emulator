# One-Week Study Guide

## Day 1 — C and tools

Learn: variables, functions, structs, arrays, pointers, fixed-width integers, compile/link/run.

Exercise: change values in `main.c`, compile with `gcc`, and observe output.

## Day 2 — Hardware model

Learn: CPU, register, memory address, stack, flags, program counter.

Read `src/cpu.h` and `src/bus.h`.

## Day 3 — CPU instructions

Read `src/cpu.c` in this order:

1. `rd8` / `rd16`
2. register access helpers
3. stack helpers
4. flag helpers
5. `cpu_step`

Trace one simple instruction by hand: `LD A,n`.

## Day 4 — Memory and cartridge

Read `src/cartridge.c` and `src/bus.c`.

Trace an address such as `0x8000` and explain why it is VRAM.

## Day 5 — PPU and timer

Read `src/ppu.c` and `src/timer.c`.

Understand why the same CPU cycles are fed to both components.

## Day 6 — Input / audio / integration

Read `input.c`, `apu.c` and `main.c`.

Draw an architecture diagram from memory.

## Day 7 — Test + presentation

Demonstrate:

- clean build
- built-in demo ROM
- keyboard input
- source structure
- one CPU instruction traced step by step
- known limitations

Be able to answer: “What happens when the CPU executes one instruction?”

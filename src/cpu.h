#ifndef GB_CPU_H
#define GB_CPU_H

#include "common.h"
#include "bus.h"

typedef struct {
    uint8_t a, f, b, c, d, e, h, l;
    uint16_t sp, pc;
    bool ime;
    bool halted;
    bool stopped;
    int ei_delay;
} CPU;

void cpu_init(CPU *cpu);
int cpu_step(CPU *cpu, Bus *bus);
void cpu_dump(const CPU *cpu);

#endif

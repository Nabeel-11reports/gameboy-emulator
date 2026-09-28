#include "timer.h"

void timer_init(Timer *timer) {
    timer->div_counter = 0;
    timer->tima_counter = 0;
}

static int tima_period(uint8_t tac) {
    switch (tac & 3u) {
        case 0: return 1024;
        case 1: return 16;
        case 2: return 64;
        default: return 256;
    }
}

void timer_step(Timer *timer, Bus *bus, int cycles) {
    timer->div_counter += cycles;
    while (timer->div_counter >= 256) {
        timer->div_counter -= 256;
        bus->mem[0xFF04]++;
    }

    if (!(bus->mem[0xFF07] & 0x04u)) return;
    timer->tima_counter += cycles;
    int period = tima_period(bus->mem[0xFF07]);
    while (timer->tima_counter >= period) {
        timer->tima_counter -= period;
        if (bus->mem[0xFF05] == 0xFF) {
            bus->mem[0xFF05] = bus->mem[0xFF06];
            bus->mem[0xFF0F] |= 0x04u;
        } else {
            bus->mem[0xFF05]++;
        }
    }
}

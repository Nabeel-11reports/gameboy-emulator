#ifndef GB_TIMER_H
#define GB_TIMER_H

#include "common.h"
#include "bus.h"

typedef struct {
    int div_counter;
    int tima_counter;
} Timer;

void timer_init(Timer *timer);
void timer_step(Timer *timer, Bus *bus, int cycles);

#endif

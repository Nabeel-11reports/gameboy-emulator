#ifndef GB_INPUT_H
#define GB_INPUT_H

#include "common.h"
#include <SDL2/SDL.h>

typedef struct {
    bool right, left, up, down;
    bool a, b, select, start;
} Input;

void input_init(Input *in);
void input_handle_event(Input *in, const SDL_Event *event, uint8_t *joyp, uint8_t *if_reg);
uint8_t input_read_joyp(const Input *in, uint8_t current);

#endif

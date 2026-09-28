#include "input.h"
#include <string.h>

void input_init(Input *in) {
    memset(in, 0, sizeof(*in));
}

static void set_key(Input *in, SDL_Keycode key, bool down) {
    switch (key) {
        case SDLK_RIGHT: case SDLK_d: in->right = down; break;
        case SDLK_LEFT:  case SDLK_a: in->left  = down; break;
        case SDLK_UP:    case SDLK_w: in->up    = down; break;
        case SDLK_DOWN:  case SDLK_s: in->down  = down; break;
        case SDLK_x: in->a = down; break;
        case SDLK_z: in->b = down; break;
        case SDLK_BACKSPACE: in->select = down; break;
        case SDLK_RETURN: in->start = down; break;
        default: break;
    }
}

void input_handle_event(Input *in, const SDL_Event *event, uint8_t *joyp, uint8_t *if_reg) {
    if (event->type != SDL_KEYDOWN && event->type != SDL_KEYUP) return;
    if (event->type == SDL_KEYDOWN && event->key.repeat) return;

    bool before = false;
    bool recognized = false;
    SDL_Keycode key = event->key.keysym.sym;
    switch (key) {
        case SDLK_RIGHT: case SDLK_d: before = in->right; recognized = true; break;
        case SDLK_LEFT:  case SDLK_a: before = in->left; recognized = true; break;
        case SDLK_UP:    case SDLK_w: before = in->up; recognized = true; break;
        case SDLK_DOWN:  case SDLK_s: before = in->down; recognized = true; break;
        case SDLK_x: before = in->a; recognized = true; break;
        case SDLK_z: before = in->b; recognized = true; break;
        case SDLK_BACKSPACE: before = in->select; recognized = true; break;
        case SDLK_RETURN: before = in->start; recognized = true; break;
        default: break;
    }

    bool down = event->type == SDL_KEYDOWN;
    set_key(in, key, down);

    if (recognized && down && !before) *if_reg |= 0x10u;
    if (joyp) *joyp = input_read_joyp(in, *joyp);
}

uint8_t input_read_joyp(const Input *in, uint8_t current) {
    uint8_t result = (uint8_t)(current | 0x0Fu);

    if (!(current & 0x10u)) {
        if (in->right)  result &= (uint8_t)~0x01u;
        if (in->left)   result &= (uint8_t)~0x02u;
        if (in->up)     result &= (uint8_t)~0x04u;
        if (in->down)   result &= (uint8_t)~0x08u;
    }
    if (!(current & 0x20u)) {
        if (in->a)      result &= (uint8_t)~0x01u;
        if (in->b)      result &= (uint8_t)~0x02u;
        if (in->select) result &= (uint8_t)~0x04u;
        if (in->start)  result &= (uint8_t)~0x08u;
    }
    return result;
}

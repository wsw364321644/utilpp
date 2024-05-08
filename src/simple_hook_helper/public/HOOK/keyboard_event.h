#pragma once
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_events.h>
#include <simple_os_defs.h>
#include <stdbool.h>
#include <stdint.h>
#include "simple_hook_helper_common.h"

typedef enum EPressedState
{
    Down= SDL_PRESSED,
    Up= SDL_RELEASED,
}EPressedState;

typedef struct keyboard_event_t
{
    EPressedState state:1;
    SDL_Keycode key_code;
} keyboard_event_t;


#pragma once
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_events.h>
#include <simple_os_defs.h>
#include <stdbool.h>
#include "simple_hook_helper_common.h"
#include "keyboard_event.h"
typedef struct mouse_motion_event_t
{
    int32_t x;
    int32_t y;
    int32_t xrel;
    int32_t yrel;
} mouse_motion_event_t;

typedef struct mouse_button_event_t
{
    EPressedState state:1;
    uint8_t button;
    uint8_t clicks;
    int32_t x;
    int32_t y;
} mouse_button_event_t;

typedef struct mouse_wheel_event_t
{
    int32_t x;
    int32_t y;
    float preciseX;
    float preciseY;
} mouse_wheel_event_t;

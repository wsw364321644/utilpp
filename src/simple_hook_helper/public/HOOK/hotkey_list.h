#pragma once
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_events.h>
#include <simple_os_defs.h>
#include <stdbool.h>
#include "simple_hook_helper_common.h"

typedef struct keyboard_event_t
{
    SDL_EventType type;
    SDL_Keycode key_code;
} keyboard_event_t;





typedef uintptr_t hot_key_list_handle_t;

typedef struct key_with_modifier_t
{
    SDL_Keycode sym;            /**< SDL virtual key code - see ::SDL_Keycode for details */
    Uint16 mod;                 /**< current key modifiers */
} key_with_modifier_t;



HOOK_HELPER_API hot_key_list_handle_t new_hot_key_list();
HOOK_HELPER_API bool add_hot_key(hot_key_list_handle_t handle, const char* name, key_with_modifier_t key);
HOOK_HELPER_API bool remove_hot_key(hot_key_list_handle_t handle, const char* name);
HOOK_HELPER_API void free_hot_key_list(hot_key_list_handle_t handle);

HOOK_HELPER_API void  get_hot_key_list_buf(hot_key_list_handle_t handle, uint8_t** buf,   uint64_t* buf_size);
HOOK_HELPER_API hot_key_list_handle_t parse_hot_key_list_buf(const uint8_t* buf);
HOOK_HELPER_API uint64_t get_hot_key_list_len(hot_key_list_handle_t handle);
HOOK_HELPER_API const char* get_hot_key_name(hot_key_list_handle_t handle,uint64_t index);
HOOK_HELPER_API key_with_modifier_t get_hot_key(hot_key_list_handle_t handle,uint64_t index);
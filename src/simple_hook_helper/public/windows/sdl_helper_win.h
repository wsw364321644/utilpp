#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <simple_os_defs.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_keycode.h>
#include "simple_hook_helper_common.h"
typedef SHORT(WINAPI* GetKeyState_t)(_In_ int vKey);
HOOK_HELPER_API SDL_Scancode WindowsScanCodeToSDLScanCode(LPARAM lParam, WPARAM wParam);
HOOK_HELPER_API bool WindowsCheckModifier(Uint16 mod, GetKeyState_t fn);
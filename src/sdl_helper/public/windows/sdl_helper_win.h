#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <simple_os_defs.h>
#include <SDL2/SDL_scancode.h>


SDL_Scancode WindowsScanCodeToSDLScanCode(LPARAM lParam, WPARAM wParam);
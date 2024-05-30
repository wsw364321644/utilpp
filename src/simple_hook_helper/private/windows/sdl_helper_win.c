#include "windows/sdl_helper_win.h"

static const SDL_Scancode windows_scancode_table[] =
{
    /*	0						1							2							3							4						5							6							7 */
    /*	8						9							A							B							C						D							E							F */
    SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_ESCAPE,		SDL_SCANCODE_1,				SDL_SCANCODE_2,				SDL_SCANCODE_3,			SDL_SCANCODE_4,				SDL_SCANCODE_5,				SDL_SCANCODE_6,			/* 0 */
    SDL_SCANCODE_7,				SDL_SCANCODE_8,				SDL_SCANCODE_9,				SDL_SCANCODE_0,				SDL_SCANCODE_MINUS,		SDL_SCANCODE_EQUALS,		SDL_SCANCODE_BACKSPACE,		SDL_SCANCODE_TAB,		/* 0 */

    SDL_SCANCODE_Q,				SDL_SCANCODE_W,				SDL_SCANCODE_E,				SDL_SCANCODE_R,				SDL_SCANCODE_T,			SDL_SCANCODE_Y,				SDL_SCANCODE_U,				SDL_SCANCODE_I,			/* 1 */
    SDL_SCANCODE_O,				SDL_SCANCODE_P,				SDL_SCANCODE_LEFTBRACKET,	SDL_SCANCODE_RIGHTBRACKET,	SDL_SCANCODE_RETURN,	SDL_SCANCODE_LCTRL,			SDL_SCANCODE_A,				SDL_SCANCODE_S,			/* 1 */

    SDL_SCANCODE_D,				SDL_SCANCODE_F,				SDL_SCANCODE_G,				SDL_SCANCODE_H,				SDL_SCANCODE_J,			SDL_SCANCODE_K,				SDL_SCANCODE_L,				SDL_SCANCODE_SEMICOLON,	/* 2 */
    SDL_SCANCODE_APOSTROPHE,	SDL_SCANCODE_GRAVE,			SDL_SCANCODE_LSHIFT,		SDL_SCANCODE_BACKSLASH,		SDL_SCANCODE_Z,			SDL_SCANCODE_X,				SDL_SCANCODE_C,				SDL_SCANCODE_V,			/* 2 */

    SDL_SCANCODE_B,				SDL_SCANCODE_N,				SDL_SCANCODE_M,				SDL_SCANCODE_COMMA,			SDL_SCANCODE_PERIOD,	SDL_SCANCODE_SLASH,			SDL_SCANCODE_RSHIFT,		SDL_SCANCODE_PRINTSCREEN,/* 3 */
    SDL_SCANCODE_LALT,			SDL_SCANCODE_SPACE,			SDL_SCANCODE_CAPSLOCK,		SDL_SCANCODE_F1,			SDL_SCANCODE_F2,		SDL_SCANCODE_F3,			SDL_SCANCODE_F4,			SDL_SCANCODE_F5,		/* 3 */

    SDL_SCANCODE_F6,			SDL_SCANCODE_F7,			SDL_SCANCODE_F8,			SDL_SCANCODE_F9,			SDL_SCANCODE_F10,		SDL_SCANCODE_NUMLOCKCLEAR,	SDL_SCANCODE_SCROLLLOCK,	SDL_SCANCODE_HOME,		/* 4 */
    SDL_SCANCODE_UP,			SDL_SCANCODE_PAGEUP,		SDL_SCANCODE_KP_MINUS,		SDL_SCANCODE_LEFT,			SDL_SCANCODE_KP_5,		SDL_SCANCODE_RIGHT,			SDL_SCANCODE_KP_PLUS,		SDL_SCANCODE_END,		/* 4 */

    SDL_SCANCODE_DOWN,			SDL_SCANCODE_PAGEDOWN,		SDL_SCANCODE_INSERT,		SDL_SCANCODE_DELETE,		SDL_SCANCODE_UNKNOWN,	SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_NONUSBACKSLASH,SDL_SCANCODE_F11,		/* 5 */
    SDL_SCANCODE_F12,			SDL_SCANCODE_PAUSE,			SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_LGUI,			SDL_SCANCODE_RGUI,		SDL_SCANCODE_APPLICATION,	SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_UNKNOWN,	/* 5 */

    SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_F13,		SDL_SCANCODE_F14,			SDL_SCANCODE_F15,			SDL_SCANCODE_F16,		/* 6 */
    SDL_SCANCODE_F17,			SDL_SCANCODE_F18,			SDL_SCANCODE_F19,			SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_UNKNOWN,	SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_UNKNOWN,	/* 6 */

    SDL_SCANCODE_INTERNATIONAL2,		SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_INTERNATIONAL1,		SDL_SCANCODE_UNKNOWN,	SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_UNKNOWN,	/* 7 */
    SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_INTERNATIONAL4,		SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_INTERNATIONAL5,		SDL_SCANCODE_UNKNOWN,	SDL_SCANCODE_INTERNATIONAL3,		SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_UNKNOWN	/* 7 */
};

static SDL_Scancode VKeytoScancode(WPARAM vkey)
{
    switch (vkey) {
    case VK_MODECHANGE:
        return SDL_SCANCODE_MODE;
    case VK_SELECT:
        return SDL_SCANCODE_SELECT;
    case VK_EXECUTE:
        return SDL_SCANCODE_EXECUTE;
    case VK_HELP:
        return SDL_SCANCODE_HELP;
    case VK_PAUSE:
        return SDL_SCANCODE_PAUSE;
    case VK_NUMLOCK:
        return SDL_SCANCODE_NUMLOCKCLEAR;

    case VK_F13:
        return SDL_SCANCODE_F13;
    case VK_F14:
        return SDL_SCANCODE_F14;
    case VK_F15:
        return SDL_SCANCODE_F15;
    case VK_F16:
        return SDL_SCANCODE_F16;
    case VK_F17:
        return SDL_SCANCODE_F17;
    case VK_F18:
        return SDL_SCANCODE_F18;
    case VK_F19:
        return SDL_SCANCODE_F19;
    case VK_F20:
        return SDL_SCANCODE_F20;
    case VK_F21:
        return SDL_SCANCODE_F21;
    case VK_F22:
        return SDL_SCANCODE_F22;
    case VK_F23:
        return SDL_SCANCODE_F23;
    case VK_F24:
        return SDL_SCANCODE_F24;

    case VK_OEM_NEC_EQUAL:
        return SDL_SCANCODE_KP_EQUALS;
    case VK_BROWSER_BACK:
        return SDL_SCANCODE_AC_BACK;
    case VK_BROWSER_FORWARD:
        return SDL_SCANCODE_AC_FORWARD;
    case VK_BROWSER_REFRESH:
        return SDL_SCANCODE_AC_REFRESH;
    case VK_BROWSER_STOP:
        return SDL_SCANCODE_AC_STOP;
    case VK_BROWSER_SEARCH:
        return SDL_SCANCODE_AC_SEARCH;
    case VK_BROWSER_FAVORITES:
        return SDL_SCANCODE_AC_BOOKMARKS;
    case VK_BROWSER_HOME:
        return SDL_SCANCODE_AC_HOME;
    case VK_VOLUME_MUTE:
        return SDL_SCANCODE_MUTE;
    case VK_VOLUME_DOWN:
        return SDL_SCANCODE_VOLUMEDOWN;
    case VK_VOLUME_UP:
        return SDL_SCANCODE_VOLUMEUP;

    case VK_MEDIA_NEXT_TRACK:
        return SDL_SCANCODE_AUDIONEXT;
    case VK_MEDIA_PREV_TRACK:
        return SDL_SCANCODE_AUDIOPREV;
    case VK_MEDIA_STOP:
        return SDL_SCANCODE_AUDIOSTOP;
    case VK_MEDIA_PLAY_PAUSE:
        return SDL_SCANCODE_AUDIOPLAY;
    case VK_LAUNCH_MAIL:
        return SDL_SCANCODE_MAIL;
    case VK_LAUNCH_MEDIA_SELECT:
        return SDL_SCANCODE_MEDIASELECT;

    case VK_OEM_102:
        return SDL_SCANCODE_NONUSBACKSLASH;

    case VK_ATTN:
        return SDL_SCANCODE_SYSREQ;
    case VK_CRSEL:
        return SDL_SCANCODE_CRSEL;
    case VK_EXSEL:
        return SDL_SCANCODE_EXSEL;
    case VK_OEM_CLEAR:
        return SDL_SCANCODE_CLEAR;

    case VK_LAUNCH_APP1:
        return SDL_SCANCODE_APP1;
    case VK_LAUNCH_APP2:
        return SDL_SCANCODE_APP2;

    default:
        return SDL_SCANCODE_UNKNOWN;
    }
}


static SDL_Scancode VKeytoScancodeFallback(WPARAM vkey)
{
    switch (vkey) {
    case VK_LEFT:
        return SDL_SCANCODE_LEFT;
    case VK_UP:
        return SDL_SCANCODE_UP;
    case VK_RIGHT:
        return SDL_SCANCODE_RIGHT;
    case VK_DOWN:
        return SDL_SCANCODE_DOWN;

    default:
        return SDL_SCANCODE_UNKNOWN;
    }
}

SDL_Scancode WindowsScanCodeToSDLScanCode(LPARAM lParam, WPARAM wParam)
{
    SDL_Scancode code;
    int nScanCode = (lParam >> 16) & 0xFF;
    bool bIsExtended = (lParam & (1 << 24)) != 0;

    code = VKeytoScancode(wParam);

    if (code == SDL_SCANCODE_UNKNOWN && nScanCode <= 127) {
        code = windows_scancode_table[nScanCode];

        if (bIsExtended) {
            switch (code) {
            case SDL_SCANCODE_RETURN:
                code = SDL_SCANCODE_KP_ENTER;
                break;
            case SDL_SCANCODE_LALT:
                code = SDL_SCANCODE_RALT;
                break;
            case SDL_SCANCODE_LCTRL:
                code = SDL_SCANCODE_RCTRL;
                break;
            case SDL_SCANCODE_SLASH:
                code = SDL_SCANCODE_KP_DIVIDE;
                break;
            case SDL_SCANCODE_CAPSLOCK:
                code = SDL_SCANCODE_KP_PLUS;
                break;
            default:
                break;
            }
        }
        else {
            switch (code) {
            case SDL_SCANCODE_HOME:
                code = SDL_SCANCODE_KP_7;
                break;
            case SDL_SCANCODE_UP:
                code = SDL_SCANCODE_KP_8;
                break;
            case SDL_SCANCODE_PAGEUP:
                code = SDL_SCANCODE_KP_9;
                break;
            case SDL_SCANCODE_LEFT:
                code = SDL_SCANCODE_KP_4;
                break;
            case SDL_SCANCODE_RIGHT:
                code = SDL_SCANCODE_KP_6;
                break;
            case SDL_SCANCODE_END:
                code = SDL_SCANCODE_KP_1;
                break;
            case SDL_SCANCODE_DOWN:
                code = SDL_SCANCODE_KP_2;
                break;
            case SDL_SCANCODE_PAGEDOWN:
                code = SDL_SCANCODE_KP_3;
                break;
            case SDL_SCANCODE_INSERT:
                code = SDL_SCANCODE_KP_0;
                break;
            case SDL_SCANCODE_DELETE:
                code = SDL_SCANCODE_KP_PERIOD;
                break;
            case SDL_SCANCODE_PRINTSCREEN:
                code = SDL_SCANCODE_KP_MULTIPLY;
                break;
            default:
                break;
            }
        }
    }

    /* The on-screen keyboard can generate VK_LEFT and VK_RIGHT events without a scancode
     * value set, however we cannot simply map these in VKeytoScancode() or we will be
     * incorrectly handling the arrow keys on the number pad when NumLock is disabled
     * (which also generate VK_LEFT, VK_RIGHT, etc in that scenario). Instead, we'll only
     * map them if none of the above special number pad mappings applied. */
    if (code == SDL_SCANCODE_UNKNOWN) {
        code = VKeytoScancodeFallback(wParam);
    }

    return code;
}

static bool WindowsCheckModifierLRSub(Uint16 mod, SDL_Keymod lmodekey, SDL_Keymod rmodekey, uint32_t lvkcode, uint32_t rvkcode) {
    if (mod & lmodekey) {
        if (mod & rmodekey) {
            if (!(GetKeyState(lvkcode) & 0x8000)&& !(GetKeyState(rvkcode) & 0x8000)) {
                return false;
            }
        }
        else {
            if (!(GetKeyState(lvkcode) & 0x8000)) {
                return false;
            }
        }
    }
    if (mod & rmodekey) {
        if (!(GetKeyState(rvkcode) & 0x8000)) {
            return false;
        }
    }
    return true;
}
bool WindowsCheckModifier(Uint16 mod)
{
    if (!WindowsCheckModifierLRSub(mod,KMOD_LSHIFT, KMOD_RSHIFT,  VK_LSHIFT, VK_RSHIFT)) {
       return false;
    }
    if (!WindowsCheckModifierLRSub(mod, KMOD_LCTRL, KMOD_RCTRL,  VK_LCONTROL, VK_RCONTROL)) {
        return false;
    }
    if (!WindowsCheckModifierLRSub(mod, KMOD_LALT, KMOD_RALT,  VK_LMENU, VK_RMENU)) {
        return false;
    }
    if (!WindowsCheckModifierLRSub(mod, KMOD_LGUI, KMOD_RGUI,  VK_LWIN, VK_RWIN)) {
        return false;
    }
    if (mod & KMOD_NUM) {
        if (!(GetKeyState(VK_NUMLOCK) & 0x0001)) {
            return false;
        }
    }
    if (mod & KMOD_CAPS) {
        if (!(GetKeyState(VK_CAPITAL) & 0x0001)) {
            return false;
        }
    }
    if (mod & KMOD_SCROLL) {
        if (!(GetKeyState(VK_SCROLL) & 0x0001)) {
            return false;
        }
    }
    return true;
}

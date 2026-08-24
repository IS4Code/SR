#include <string.h>
#include "Game_defs.h"
#include "Game_vars.h"
#include "Albion-mobile.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#define GAME_MOBILE_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define GAME_MOBILE_EXPORT
#endif

int Game_SelectionMode = 0;

void Game_InjectClick(Uint8 button, int x, int y)
{
    SDL_Event event;

    memset(&event, 0, sizeof(event));
    event.button.button = button;
    event.button.x = x;
    event.button.y = y;
    event.button.clicks = 1;

    event.type = SDL_MOUSEBUTTONDOWN;
    event.button.state = SDL_PRESSED;
    SDL_PushEvent(&event);

    event.type = SDL_MOUSEBUTTONUP;
    event.button.state = SDL_RELEASED;
    SDL_PushEvent(&event);
}

GAME_MOBILE_EXPORT
void Game_InjectKey(int sdl_keycode)
{
    SDL_Event event;

    memset(&event, 0, sizeof(event));
    event.key.keysym.sym = sdl_keycode;

    event.type = SDL_KEYDOWN;
    event.key.state = SDL_PRESSED;
    SDL_PushEvent(&event);

    event.type = SDL_KEYUP;
    event.key.state = SDL_RELEASED;
    SDL_PushEvent(&event);
}

GAME_MOBILE_EXPORT
int Game_SelectionMode_Toggle(void)
{
    Game_SelectionMode = !Game_SelectionMode;
    return Game_SelectionMode;
}

extern uint16_t loc_13CF64; // Cheat_keys_activated (DIAGNOST.C): enables the low-level diagnostic keys
extern uint16_t loc_147130; // Cheat_mode (GAME.C): "god mode", read independently throughout the game

void Game_ApplyDeveloperGodMode(void)
{
    if (Game_DeveloperMode)
    {
        loc_13CF64 = 1;
    }

    if (Game_GodMode)
    {
        loc_147130 = 1;
    }
}

static int Game_InEnteringTextLast = 0;

void Game_MobileKeyboard_Poll(void)
{
    int visible = (int) Game_InEnteringText ? 1 : 0;

    // keep polling while visible
    if (visible == Game_InEnteringTextLast && !visible) return;
    Game_InEnteringTextLast = visible;

#if defined(__EMSCRIPTEN__)
    MAIN_THREAD_EM_ASM({
        var visible = $0;
        if (typeof Game_IsPhone !== "function" || !Game_IsPhone()) return;
        var el = document.getElementById("mobile-kbd-trigger");
        if (!el) return;
        if (visible) {
            if (document.activeElement !== el) el.focus();
        } else {
            el.blur();
        }
    }, visible);
#endif
}

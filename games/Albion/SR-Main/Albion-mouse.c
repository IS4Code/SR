/**
 *
 *  Copyright (C) 2016-2026 Roman Pauer
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of
 *  this software and associated documentation files (the "Software"), to deal in
 *  the Software without restriction, including without limitation the rights to
 *  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 *  of the Software, and to permit persons to whom the Software is furnished to do
 *  so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 */

#include <math.h>
#include "Game_defs.h"
#include "Game_vars.h"
#include "Albion-engine.h"

// 3D mouse look

#define GAME_3D_ANGLE_STEPS 16384

// balanced X/Y sensitivity (0.3 deg/px)
#define GAME_3D_TURN_SENSITIVITY 14
#define GAME_3D_PITCH_SENSITIVITY (0.3 * M_PI / 180.0)

extern uint32_t loc_14A48C; // I3DM.Camera_angle (16:16 fixed-point, wraps over GAME_3D_ANGLE_STEPS)
extern int16_t loc_14A492; // I3DM.Horizon_Y (offset in pixels from horizon)
extern uint16_t loc_14A49C; // I3DM.Window_3D_height
extern int32_t loc_140008; // vertical focal length

static int Game_MouseLookActive = 0;

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>

// Emscripten bug fix - pointerlockchange does not update relative mouse mode
static volatile int Game_MouseLook_PointerLockLost = 0;

static void Game_MouseLook_InitPointerLockListener(void)
{
    static int registered = 0;
    if (registered) return;
    registered = 1;

    MAIN_THREAD_EM_ASM({
        var flagPtr = $0;
        document.addEventListener("pointerlockchange", function () {
            if (document.pointerLockElement == null)
            {
                Atomics.store(HEAP32, flagPtr >> 2, 1);
            }
        });
    }, &Game_MouseLook_PointerLockLost);
}

static void Game_MouseLook_SendEsc(void)
{
    SDL_Event event;

    memset(&event, 0, sizeof(event));
    event.key.keysym.sym = SDLK_ESCAPE;
    event.key.keysym.scancode = SDL_SCANCODE_ESCAPE;

    event.type = SDL_KEYDOWN;
    event.key.state = SDL_PRESSED;
    SDL_PushEvent(&event);

    event.type = SDL_KEYUP;
    event.key.state = SDL_RELEASED;
    SDL_PushEvent(&event);
}
#endif

int Game_MouseLook_Active(void)
{
    return Game_MouseLookActive;
}

static void Game_MouseLook_SetActive(int active)
{
    if (active == Game_MouseLookActive) return;

    Game_MouseLookActive = active;
    SDL_SetRelativeMouseMode(active ? SDL_TRUE : SDL_FALSE);
}

void Game_MouseLook_Update(void)
{
    if (!Game_MouseLookEnabled)
    {
        // not enabled
        Game_MouseLook_SetActive(0);

        // ignore any lock updates
        Game_MouseLook_PointerLockLost = 0;
        return;
    }

#if defined(__EMSCRIPTEN__)
    Game_MouseLook_InitPointerLockListener();

    if (Game_MouseLook_PointerLockLost)
    {
        // lock lost signal
        Game_MouseLook_PointerLockLost = 0;

        if (Game_MouseLookActive)
        {
            // still active, lock lost due to ESC - sync and synthesize ESC
            Game_MouseLook_SetActive(0);
            Game_MouseLook_SendEsc();
            return;
        }
    }
#endif

    Game_MouseLook_SetActive(Game_ScreenType() == GAME_SCREEN_MAP_3D);
}

void Game_MouseLook_Toggle(void)
{
    // switch mouse look control
    Game_MouseLookEnabled = !Game_MouseLookEnabled;
    Game_MouseLook_SetActive(Game_MouseLookEnabled);
}

// Rotate_3D(dAlpha) analogue
static void Game_MouseLook_Turn(int32_t xrel)
{
    int32_t d_angle_steps = (-xrel * GAME_3D_TURN_SENSITIVITY * Game_MouseLookSensitivity) / 100;
    int64_t angle = (int64_t) loc_14A48C + ((int64_t) d_angle_steps << 16);

    loc_14A48C = (uint32_t) angle & ((GAME_3D_ANGLE_STEPS * 65536u) - 1);
}

static void Game_MouseLook_Pitch(int32_t yrel)
{
    double half_height = ((int16_t) loc_14A49C) / 2.0;
    double focal_length_y = (double) loc_140008;

    // convert Y offset to pitch
    double angle = atan2((double) loc_14A492, focal_length_y);
    
    // move
    angle -= (double) yrel * GAME_3D_PITCH_SENSITIVITY * Game_MouseLookSensitivity / 100.0;

    // update offset
    double new_offset = focal_length_y * tan(angle);

    if (new_offset > half_height) new_offset = half_height;
    if (new_offset < -half_height) new_offset = -half_height;

    loc_14A492 = (int16_t) new_offset;
}

void Game_MouseLook_Move(int32_t xrel, int32_t yrel)
{
    if (xrel) Game_MouseLook_Turn(xrel);
    if (yrel) Game_MouseLook_Pitch(yrel);
}

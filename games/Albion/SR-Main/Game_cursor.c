#include "Game_defs.h"
#include "Game_vars.h"

#include <stddef.h>
#include <stdlib.h>
#include <SDL.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

#pragma pack(1)
typedef struct PACKED {
    uint16_t hotspot_x;
    uint16_t hotspot_y;
    uint16_t width;
    uint16_t height;
    PTR32(uint8_t) sprite;
} Game_CursorEntry;
#pragma pack()

extern PTR32(Game_CursorEntry) loc_17966C[]; // stack of GAME_CURSOR_STACK_MAX cursor pointers
extern uint16_t loc_13EEDC; // stack top
extern Game_CursorEntry loc_137BCC[]; // cursor entries

#define GAME_CURSOR_STACK_MAX 8
#define GAME_CURSOR_COUNT 27
#define GAME_CURSOR_DEFAULT_SCALE 2

static SDL_Cursor *Game_SDL_Cursors[GAME_CURSOR_COUNT];
static Game_CursorEntry Game_Cursor_Original[GAME_CURSOR_COUNT];
static uint8_t Game_Cursor_EmptySprite[4 * 4];
static int Game_Cursor_Hidden = 0;
static int Game_Cursor_Loaded = 0;
static int Game_Cursor_LastIndex = -1;

#if defined(__EMSCRIPTEN__)
static int Game_Cursor_AutoScale(void)
{
    int scale = MAIN_THREAD_EM_ASM_INT({
        var w = window.innerWidth;
        var h = window.innerHeight;
        if (w * 2 > h * 3)
        {
            w = h * 3 / 2;
        }
        else
        {
            h = w * 2 / 3;
        }
        return Math.round(w / 360);
    });

    return (scale < 1) ? 1 : scale;
}
#endif

static int Game_Cursor_GetScale(void)
{
    if (Game_MouseCursorScale != 0)
    {
        return Game_MouseCursorScale;
    }

#if defined(__EMSCRIPTEN__)
    return Game_Cursor_AutoScale();
#else
    return GAME_CURSOR_DEFAULT_SCALE;
#endif
}

static SDL_Cursor *Game_Cursor_Render(const Game_CursorEntry *entry, int scale)
{
    int width = entry->width, height = entry->height;
    if (!entry->sprite || !width || !height || width > 64 || height > 64)
    {
        return NULL;
    }

    int count = width * height * (scale * scale);
    uint32_t *pixels = (uint32_t *) malloc(count * sizeof(uint32_t));
    if (pixels == NULL)
    {
        return NULL;
    }

    int dst_stride = width * scale;
    for (int y = 0; y < height; y++)
    {
        uint8_t *src_row = &entry->sprite[y * entry->width];
        for (int x = 0; x < width; x++)
        {
            // source pixel color
            uint8_t idx = src_row[x];
            pixel_format_orig col = Game_Palette_Or[idx];
            uint32_t pixel = (idx == 0)
                ? 0
                : ((uint32_t)0xFF << 24)
                | ((uint32_t)col.s.b << 16)
                | ((uint32_t)col.s.g << 8)
                | ((uint32_t)col.s.r);

            // set the destination scaled pixel
            uint32_t *dst = &pixels[(y * dst_stride + x) * scale];
            for (int dy = 0; dy < scale; dy++)
            {
                uint32_t *dst_row = &dst[dy * dst_stride];
                for (int dx = 0; dx < scale; dx++)
                {
                    dst_row[dx] = pixel;
                }
            }
        }
    }

    SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(
        pixels, width * scale, height * scale, 32,
        dst_stride * sizeof(uint32_t), 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
    );

    SDL_Cursor* cursor = NULL;
    if (surface)
    {
        cursor = SDL_CreateColorCursor(surface, entry->hotspot_x * scale, entry->hotspot_y * scale);
        SDL_FreeSurface(surface);
    }

    free(pixels);
    return cursor;
}

void Game_Cursor_Hide(void)
{
    if (Game_MouseCursor != 3 || Game_Cursor_Hidden)
    {
        return;
    }

    // replicate Empty_mouse_pointer behaviour
    // with a new empty sprite
    Game_CursorEntry empty_cursor = {
        0, 0,
        4, 4,
        Game_Cursor_EmptySprite
    };

    for (int i = 0; i < GAME_CURSOR_COUNT; i++)
    {
        Game_CursorEntry *cursor = &loc_137BCC[i];

        // preserve the original entry for later rendering
        Game_Cursor_Original[i] = *cursor;

        // overwrite with empty data
        *cursor = empty_cursor;
    }

    Game_Cursor_Hidden = 1;
}

static void Game_Cursor_Load(void)
{
    // hide first just in case
    Game_Cursor_Hide();

    int scale = Game_Cursor_GetScale();

    for (int i = 0; i < GAME_CURSOR_COUNT; i++)
    {
        // render to SDL cursor from the original sprite
        Game_SDL_Cursors[i] = Game_Cursor_Render(&Game_Cursor_Original[i], scale);
    }

    Game_Cursor_Loaded = 1;
}

void Game_Cursor_Update(void)
{
    if (Game_MouseCursor != 3)
    {
        // game cursor not enabled
        return;
    }

    if (!Game_Cursor_Loaded)
    {
        if (!Game_Palette_Or[255].pix)
        {
            // wait until palette is available
            return;
        }
        Game_Cursor_Load();
    }

    if (loc_13EEDC >= GAME_CURSOR_STACK_MAX)
    {
        // no cursor assigned
        return;
    }

    Game_CursorEntry *entry = loc_17966C[loc_13EEDC];
    if (entry == NULL)
    {
        return;
    }

    ptrdiff_t index = entry - &loc_137BCC[0];
    if (index < 0 || index >= GAME_CURSOR_COUNT)
    {
        // hidden cursor
        if (Game_Cursor_LastIndex != -1)
        {
            SDL_ShowCursor(SDL_DISABLE);
            Game_Cursor_LastIndex = -1;
        }
        return;
    }

    if ((int)index != Game_Cursor_LastIndex && Game_SDL_Cursors[index])
    {
        // update the cursor
        SDL_SetCursor(Game_SDL_Cursors[index]);
        if (Game_Cursor_LastIndex == -1)
        {
            SDL_ShowCursor(SDL_ENABLE);
        }
        Game_Cursor_LastIndex = (int)index;
    }
}

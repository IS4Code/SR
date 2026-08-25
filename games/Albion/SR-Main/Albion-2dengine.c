#include "Game_defs.h"
#include "Game_vars.h"
#include "Albion-engine.h"
#include "Albion-BBOPM.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Scroll_data
#pragma pack(1)
typedef struct PACKED {
    uint16_t Flags;
    PTR32(void) Update_unit;
    uint16_t Unit_width, Unit_height;
    uint16_t Viewport_width, Viewport_height;
    uint16_t sbWidth, sbHeight;
    uint32_t Playfield_width, Playfield_height;
    uint16_t Unit_X, Unit_Y;
    uint16_t Viewport_X, Viewport_Y;
    int32_t Playfield_X, Playfield_Y;
    int16_t Vector_X, Vector_Y;
    PTR32(void) Base_OPM_handle;
    OPM_Struct Base_OPM;
    OPM_Struct Scroll_OPMs[4];
} Game_ScrollData;
#pragma pack()

extern Game_ScrollData loc_14A334; // Scroll_2D
extern uint32_t loc_14A328; // Camera_2D_X
extern uint32_t loc_14A32C; // Camera_2D_Y

extern uint16_t loc_134BD8; // Map_changed

extern uint32_t loc_1482AC; // Party_object_X
extern uint32_t loc_1482A8; // Party_object_Y
extern uint16_t loc_14A44A; // Mapbuf_X
extern uint16_t loc_14A44C; // Mapbuf_Y

extern uint16_t loc_151246; // Current_map_selection_data.Map_X
extern uint16_t loc_151248; // Current_map_selection_data.Map_Y
extern uint16_t loc_1479B0; // Big_guy

#define GAME_ENH2D_QUAD_W 360
#define GAME_ENH2D_QUAD_H 192

// bounds memory and per-frame draw passes
#define GAME_ENH2D_MAX_ZOOM_INT 4
#define GAME_ENH2D_COMP_W (GAME_ENH2D_QUAD_W * GAME_ENH2D_MAX_ZOOM_INT)
#define GAME_ENH2D_COMP_H (GAME_ENH2D_QUAD_H * GAME_ENH2D_MAX_ZOOM_INT)

static OPM_Struct Game_Enh2D_OPM;
static int Game_Enh2D_Initialized = 0;

static int Game_Enh2D_GridN = 1;
static int Game_Enh2D_Tiles = 1;

static int32_t Game_Enh2D_CropOrigX, Game_Enh2D_CropOrigY;

// floors to whole screens, since the capture grid always renders ceil(zoom) full screens
static double Game_Enh2D_MapZoomCap(void)
{
    double by_w = floor((double) loc_14A334.Playfield_width / (double) GAME_ENH2D_QUAD_W);
    double by_h = floor((double) loc_14A334.Playfield_height / (double) GAME_ENH2D_QUAD_H);
    double cap = (by_w < by_h) ? by_w : by_h;

    if (cap < 1.0) cap = 1.0; // never cap below native

    return cap;
}

static double Game_Enh2D_ComputeZoom(void)
{
    double zoom = Game_2DZoomFactor;
    double cap = Game_Enh2D_MapZoomCap();

    if (cap > (double) GAME_ENH2D_MAX_ZOOM_INT) cap = (double) GAME_ENH2D_MAX_ZOOM_INT;

    if (zoom < GAME_2DZOOM_MIN) zoom = GAME_2DZOOM_MIN;
    if (zoom > cap) zoom = cap;

    return zoom;
}

void Game_2DZoomFactor_Adjust(double delta)
{
    double zoom = Game_2DZoomFactor + delta;
    double cap = Game_Enh2D_MapZoomCap();

    if (cap > (double) GAME_ENH2D_MAX_ZOOM_INT) cap = (double) GAME_ENH2D_MAX_ZOOM_INT;

    if (zoom < GAME_2DZOOM_MIN) zoom = GAME_2DZOOM_MIN;
    if (zoom > cap) zoom = cap;

    Game_2DZoomFactor = zoom;
}

static void Game_Enh2D_Init(void)
{
    if (Game_Enh2D_Initialized)
    {
        return;
    }

    Game_Enh2DBuffer[0] = (uint8_t *) malloc(GAME_ENH2D_COMP_W * GAME_ENH2D_COMP_H);
    Game_Enh2DBuffer[1] = (uint8_t *) malloc(GAME_ENH2D_COMP_W * GAME_ENH2D_COMP_H);
    Game_Enh2DRefBuffer[0] = (uint8_t *) malloc(GAME_ENH2D_QUAD_W * GAME_ENH2D_QUAD_H);
    Game_Enh2DRefBuffer[1] = (uint8_t *) malloc(GAME_ENH2D_QUAD_W * GAME_ENH2D_QUAD_H);
    {
        for (int i = 0; i < GAME_ENH2D_SLOTS; i++)
        {
            Game_Enh2DScreenBuffer[i] = (uint8_t *) malloc(360 * 240);
            Game_Enh2DRefCopy[i] = (uint8_t *) malloc(GAME_ENH2D_QUAD_W * GAME_ENH2D_QUAD_H);
            Game_Enh2DMaskBuf[i] = (uint8_t *) malloc(GAME_ENH2D_QUAD_W * GAME_ENH2D_QUAD_H);
            if (Game_Enh2DPrevRef == NULL) Game_Enh2DPrevRef = (uint8_t *) malloc(GAME_ENH2D_QUAD_W * GAME_ENH2D_QUAD_H);
            if ((Game_Enh2DScreenBuffer[i] == NULL) || (Game_Enh2DRefCopy[i] == NULL) ||
                (Game_Enh2DMaskBuf[i] == NULL)) return;
        }
    }
    if ((Game_Enh2DBuffer[0] == NULL) || (Game_Enh2DBuffer[1] == NULL) ||
        (Game_Enh2DRefBuffer[0] == NULL) || (Game_Enh2DRefBuffer[1] == NULL) ||
        0)
    {
        return;
    }

    Game_Enh2DDraw.Overlay = Game_Enh2DBuffer[0];
    Game_Enh2DDraw.Reference = Game_Enh2DRefBuffer[0];
    Game_Enh2DDraw.Active = 0;
    Game_Enh2DDisplay.Overlay = Game_Enh2DBuffer[0];
    Game_Enh2DDisplay.Reference = Game_Enh2DRefBuffer[0];
    Game_Enh2DDisplay.Active = 0;

    OPM_New(GAME_ENH2D_COMP_W, GAME_ENH2D_COMP_H, 1, &Game_Enh2D_OPM, Game_Enh2DDraw.Overlay);

    Game_Enh2D_Initialized = 1;
}

static int32_t Game_Enh2D_TrueX, Game_Enh2D_TrueY;
static int32_t Game_Enh2D_OrigX, Game_Enh2D_OrigY;

#define GAME_ENH2D_MARGIN_X 0
#define GAME_ENH2D_MARGIN_Y 0
#define GAME_ENH2D_STRIDE_W GAME_ENH2D_QUAD_W
#define GAME_ENH2D_STRIDE_H GAME_ENH2D_QUAD_H

static int Game_Enh2D_Tile;

static void Game_Enh2D_SetFullRedraw(void)
{
    loc_134BD8 = 1;
}

static void Game_Enh2D_SetCamera(int32_t x, int32_t y)
{
    loc_14A328 = (uint32_t) ((x < 0) ? 0 : x);
    loc_14A32C = (uint32_t) ((y < 0) ? 0 : y);

    Game_Enh2D_SetFullRedraw();
}

// Set_scroll_position camera clamp analogue
static int32_t Game_Enh2D_ClampOrigin(int32_t want, int32_t playfield, int32_t margin, int32_t last_cam, int32_t viewport)
{
    int32_t lo = margin;
    int32_t hi = playfield - (last_cam - margin) - viewport;

    if (hi < lo) hi = lo;
    if (want < lo) want = lo;
    if (want > hi) want = hi;

    return want;
}

typedef void (*Game_Enh2D_RectFunc)(int bx, int by, int w, int h, int ox, int oy, void *arg);

static void Game_Enh2D_ForEachRect(int vx_off, int vy_off, int width, int height, Game_Enh2D_RectFunc func, void *arg)
{
    const int bw = loc_14A334.Base_OPM.width;
    const int bh = loc_14A334.Base_OPM.height;

    if (bw <= 0 || bh <= 0) return;

    int vx = (loc_14A334.Viewport_X + vx_off) % bw;
    int vy = (loc_14A334.Viewport_Y + vy_off) % bh;

    int w0 = bw - vx;
    int h0 = bh - vy;
    if (w0 > width)  w0 = width;
    if (h0 > height) h0 = height;

    if (w0 > 0 && h0 > 0)           func(vx, vy, w0, h0, 0, 0, arg);
    if (w0 < width && h0 > 0)       func(0, vy, width - w0, h0, w0, 0, arg);
    if (w0 > 0 && h0 < height)      func(vx, 0, w0, height - h0, 0, h0, arg);
    if (w0 < width && h0 < height)  func(0, 0, width - w0, height - h0, w0, h0, arg);
}

struct Game_Enh2D_CaptureArg { int dst_x, dst_y; };

static void Game_Enh2D_CaptureRect(int bx, int by, int w, int h, int ox, int oy, void *arg)
{
    struct Game_Enh2D_CaptureArg *a = (struct Game_Enh2D_CaptureArg *) arg;

    OPM_CopyOPMOPM(&loc_14A334.Base_OPM, &Game_Enh2D_OPM, bx, by, w, h, a->dst_x + ox, a->dst_y + oy);
}

static void Game_Enh2D_Capture(int tile)
{
    struct Game_Enh2D_CaptureArg arg;

    if (!Game_Enh2D_Initialized) return;

    loc_14A334.Base_OPM.clip_x = 0;
    loc_14A334.Base_OPM.clip_y = 0;
    loc_14A334.Base_OPM.clip_width = loc_14A334.Base_OPM.width;
    loc_14A334.Base_OPM.clip_height = loc_14A334.Base_OPM.height;

    arg.dst_x = (tile % Game_Enh2D_GridN) * GAME_ENH2D_STRIDE_W;
    arg.dst_y = (tile / Game_Enh2D_GridN) * GAME_ENH2D_STRIDE_H;

    Game_Enh2D_ForEachRect(GAME_ENH2D_MARGIN_X, GAME_ENH2D_MARGIN_Y,
                           GAME_ENH2D_STRIDE_W, GAME_ENH2D_STRIDE_H,
                           &Game_Enh2D_CaptureRect, &arg);
}

static void Game_Enh2D_FocusTile(int tile)
{
    Game_Enh2D_SetCamera(
        Game_Enh2D_OrigX + (tile % Game_Enh2D_GridN) * GAME_ENH2D_STRIDE_W - GAME_ENH2D_MARGIN_X,
        Game_Enh2D_OrigY + (tile / Game_Enh2D_GridN) * GAME_ENH2D_STRIDE_H - GAME_ENH2D_MARGIN_Y);
}

// downscales the crop rect into Base_OPM, nearest-neighbour
static void Game_Enh2D_PresentRect(int bx, int by, int w, int h, int ox, int oy, void *arg)
{
    uint8_t *const base = loc_14A334.Base_OPM.buffer;
    const int stride = loc_14A334.Base_OPM.stride;
    const int comp_stride = Game_Enh2DDraw.CompW;
    const int crop_w = Game_Enh2DDraw.CropW;
    const int crop_h = Game_Enh2DDraw.CropH;
    const int crop_off_x = Game_Enh2DDraw.CropOffX;
    const int crop_off_y = Game_Enh2DDraw.CropOffY;

    (void) arg;

    for (int y = 0; y < h; y++)
    {
        uint8_t *dst_row = base + ((by + y) * stride) + bx;
        uint8_t *ref_row = &Game_Enh2DDraw.Reference[(oy + y) * GAME_ENH2D_QUAD_W + ox];
        int comp_y = crop_off_y + (int) (((int64_t) (oy + y) * crop_h) / GAME_ENH2D_QUAD_H);
        const uint8_t *src_row = &Game_Enh2DDraw.Overlay[comp_y * comp_stride];

        for (int x = 0; x < w; x++)
        {
            int comp_x = crop_off_x + (int) (((int64_t) (ox + x) * crop_w) / GAME_ENH2D_QUAD_W);
            uint8_t p = src_row[comp_x];

            dst_row[x] = p;
            ref_row[x] = p;
        }
    }
}

static void Game_Enh2D_Present(void)
{
    if (!Game_Enh2D_Initialized) return;

    Game_Enh2D_ForEachRect(0, 0, GAME_ENH2D_QUAD_W, GAME_ENH2D_QUAD_H,
                           &Game_Enh2D_PresentRect, NULL);
}

// no zoom==1 bypass - one caused correctness bugs before
void CCALL Game_Enh2D_BeginFrame(void)
{
    // called before pass 1
    Game_Enh2D_SuppressScrollDraw = 1;

    Game_Enh2DDraw.SelectorActive = 0;
}

void CCALL Game_Enh2D_BeginQuadrants(void)
{
    double zoom = Game_Enh2D_ComputeZoom();
    int n = (int) ceil(zoom - 1e-9);
    int comp_w, comp_h, crop_w, crop_h, crop_off_x, crop_off_y;

    // turns off downstream hi-res paths when exactly native (zoom==1)
    Game_Enh2D_CompositeActive = (zoom != 1.0) ? 1 : 0;

    if (n < 1) n = 1;
    if (n > GAME_ENH2D_MAX_ZOOM_INT) n = GAME_ENH2D_MAX_ZOOM_INT;

    Game_Enh2D_Init();

    Game_Enh2D_GridN = n;
    Game_Enh2D_Tiles = n * n;

    comp_w = n * GAME_ENH2D_QUAD_W;
    comp_h = n * GAME_ENH2D_QUAD_H;

    Game_Enh2D_OPM.buffer = Game_Enh2DDraw.Overlay;
    Game_Enh2D_OPM.width = (int16_t) comp_w;
    Game_Enh2D_OPM.height = (int16_t) comp_h;
    Game_Enh2D_OPM.stride = (int16_t) comp_w;
    Game_Enh2D_OPM.clip_x = 0;
    Game_Enh2D_OPM.clip_y = 0;
    Game_Enh2D_OPM.clip_width = (int16_t) comp_w;
    Game_Enh2D_OPM.clip_height = (int16_t) comp_h;

    Game_Enh2D_TrueX = (int32_t) loc_14A328;
    Game_Enh2D_TrueY = (int32_t) loc_14A32C;

    Game_SyntheticQuadrantPass = 1;

    {
        int32_t playfield_w = (int32_t) loc_14A334.Playfield_width;
        int32_t playfield_h = (int32_t) loc_14A334.Playfield_height;
        int32_t crop_want_x, crop_want_y;

        // crop size, computed before the clamps below since both need it
        crop_w = (int) llround(zoom * GAME_ENH2D_QUAD_W);
        crop_h = (int) llround(zoom * GAME_ENH2D_QUAD_H);
        if (crop_w > comp_w) crop_w = comp_w;
        if (crop_h > comp_h) crop_h = comp_h;
        if (crop_w < 1) crop_w = 1;
        if (crop_h < 1) crop_h = 1;

        Game_Enh2D_OrigX = Game_Enh2D_ClampOrigin(
            Game_Enh2D_TrueX + (GAME_ENH2D_QUAD_W / 2) - (comp_w / 2),
            playfield_w, GAME_ENH2D_MARGIN_X,
            (n - 1) * GAME_ENH2D_STRIDE_W, GAME_ENH2D_QUAD_W);
        Game_Enh2D_OrigY = Game_Enh2D_ClampOrigin(
            Game_Enh2D_TrueY + (GAME_ENH2D_QUAD_H / 2) - (comp_h / 2),
            playfield_h, GAME_ENH2D_MARGIN_Y,
            (n - 1) * GAME_ENH2D_STRIDE_H, GAME_ENH2D_QUAD_H);

        // anti-moire lattice snap
        if (!Game_Enh2D_HiresEnabled)
        {
            Game_Enh2D_OrigX -= (((Game_Enh2D_OrigX % n) + n) % n);
            Game_Enh2D_OrigY -= (((Game_Enh2D_OrigY % n) + n) % n);
        }

        // crop clamp, kept independent of the wider grid's own clamp above
        crop_want_x = Game_Enh2D_ClampOrigin(
            Game_Enh2D_TrueX + (GAME_ENH2D_QUAD_W / 2) - (crop_w / 2),
            playfield_w, 0, crop_w - GAME_ENH2D_QUAD_W, GAME_ENH2D_QUAD_W);
        crop_want_y = Game_Enh2D_ClampOrigin(
            Game_Enh2D_TrueY + (GAME_ENH2D_QUAD_H / 2) - (crop_h / 2),
            playfield_h, 0, crop_h - GAME_ENH2D_QUAD_H, GAME_ENH2D_QUAD_H);

        crop_off_x = crop_want_x - Game_Enh2D_OrigX;
        crop_off_y = crop_want_y - Game_Enh2D_OrigY;

        // pulls the crop back inside the captured grid, for maps smaller than one screen
        if (crop_off_x < 0) crop_off_x = 0;
        if (crop_off_y < 0) crop_off_y = 0;
        if (crop_off_x + crop_w > comp_w) crop_off_x = comp_w - crop_w;
        if (crop_off_y + crop_h > comp_h) crop_off_y = comp_h - crop_h;
    }

    Game_Enh2D_CropOrigX = Game_Enh2D_OrigX + crop_off_x;
    Game_Enh2D_CropOrigY = Game_Enh2D_OrigY + crop_off_y;

    Game_Enh2DDraw.CompW = comp_w;
    Game_Enh2DDraw.CompH = comp_h;
    Game_Enh2DDraw.CropW = crop_w;
    Game_Enh2DDraw.CropH = crop_h;
    Game_Enh2DDraw.CropOffX = crop_off_x;
    Game_Enh2DDraw.CropOffY = crop_off_y;

    Game_Enh2D_Tile = 0;
    Game_Enh2D_FocusTile(0);
}

// aligns the camera to the party's position at the crop's scale, not the grid's
static void Game_Enh2D_AlignCamera(void)
{
    int32_t px = (int32_t) loc_1482AC;
    int32_t py = (int32_t) loc_1482A8;
    int32_t crop_w = Game_Enh2DDraw.CropW;
    int32_t crop_h = Game_Enh2DDraw.CropH;

    int32_t cam_x = px - (int32_t) (((int64_t) (px - Game_Enh2D_CropOrigX) * GAME_ENH2D_QUAD_W) / crop_w);
    int32_t cam_y = py - (int32_t) (((int64_t) (py - Game_Enh2D_CropOrigY) * GAME_ENH2D_QUAD_H) / crop_h);

    Game_Enh2D_SetCamera(cam_x, cam_y);
}

// returns 1 while more capture passes remain, 0 once done and the camera is realigned
int CCALL Game_Enh2D_NextTile(void)
{
    Game_Enh2D_Capture(Game_Enh2D_Tile);

    Game_Enh2D_Tile++;

    if (Game_Enh2D_Tile < Game_Enh2D_Tiles)
    {
        Game_Enh2D_FocusTile(Game_Enh2D_Tile);
        return 1;
    }

    Game_Enh2D_AlignCamera();
    return 0;
}

extern uint8_t loc_17D95C[256]; // &Recolour_tables[7][0] - same recolor table SamplePixel_2D uses
extern uint8_t loc_13B726[];    // Select_2D_cursor - the 18x18 masked border sprite, same as SamplePixel_2D

// draws the highlight (mirrors SamplePixel_2D); must go through ForEachRect since Base_OPM wraps
static void Game_Enh2D_SelectorOverlayRect(int bx, int by, int w, int h, int ox, int oy, void *arg)
{
    const Game_Enh2DInfo *const info = (const Game_Enh2DInfo *) arg;
    uint8_t *const base = loc_14A334.Base_OPM.buffer;
    const int stride = loc_14A334.Base_OPM.stride;
    const int crop_w = info->CropW;
    const int crop_h = info->CropH;
    const int crop_off_x = info->CropOffX;
    const int crop_off_y = info->CropOffY;

    for (int y = 0; y < h; y++)
    {
        uint8_t *dst_row = base + ((by + y) * stride) + bx;
        int comp_y = crop_off_y + (int) (((int64_t) (oy + y) * crop_h) / GAME_ENH2D_QUAD_H);
        int ly = comp_y - info->SelectorY;

        if ((ly < -1) || (ly >= 17)) continue;

        for (int x = 0; x < w; x++)
        {
            int comp_x = crop_off_x + (int) (((int64_t) (ox + x) * crop_w) / GAME_ENH2D_QUAD_W);
            int lx = comp_x - info->SelectorX;

            // frame drawn before box, since its corners reach inward
            if ((lx >= -1) && (lx < 17))
            {
                uint8_t p = loc_13B726[(ly + 1) * 18 + (lx + 1)];
                if (p != 0)
                {
                    dst_row[x] = p;
                    continue;
                }
            }
            if ((lx >= 0) && (lx < 16) && (ly >= 0) && (ly < 16))
            {
                dst_row[x] = loc_17D95C[dst_row[x]];
            }
        }
    }
}

// must be called with Game_Enh2DDisplay, not Game_Enh2DDraw
static void Game_Enh2D_DrawSelectorLowres(const Game_Enh2DInfo *info)
{
    Game_Enh2D_ForEachRect(0, 0, GAME_ENH2D_QUAD_W, GAME_ENH2D_QUAD_H, &Game_Enh2D_SelectorOverlayRect, (void *) info);
}

void CCALL Game_Enh2D_Finish(void)
{
    Game_SyntheticQuadrantPass = 0;

    Game_Enh2D_Present();

    Game_Enh2DDraw.Active = Game_Enh2D_CompositeActive ? 1 : 0;

    // after Present() so it isn't overwritten
    if (!Game_Enh2D_HiresEnabled && Game_Enh2DDisplay.SelectorActive &&
        (Game_Enh2DDisplay.CropW > 0) && (Game_Enh2DDisplay.CropH > 0))
    {
        Game_Enh2D_DrawSelectorLowres(&Game_Enh2DDisplay);
    }

    loc_14A328 = (uint32_t) Game_Enh2D_TrueX;
    loc_14A32C = (uint32_t) Game_Enh2D_TrueY;

    Game_Enh2D_SuppressScrollDraw = 0;
}

// needed for edges to resolve to the correct tile
static int32_t Game_Enh2D_ScaleCeil(int32_t n_val, int32_t num, int32_t den)
{
    int64_t n = (int64_t) n_val * num;

    if (n >= 0) return (int32_t) ((n + den - 1) / den);

    return (int32_t) (-((-n) / den));
}

// records position only; SamplePixel_2D shows it in hi-res mode, Game_Enh2D_DrawSelectorLowres in lowres mode
void CCALL Game_Enh2D_SelectDraw(uint32_t X, uint32_t Y)
{
    int32_t mx = (int32_t) (X & 0xFFFF);
    int32_t my = (int32_t) (Y & 0xFFFF);

    if ((Game_Enh2DDraw.CropW <= 0) || (Game_Enh2DDraw.CropH <= 0)) return;

    int32_t comp_x = (int32_t) (((int64_t) mx * Game_Enh2DDraw.CropW) / GAME_ENH2D_QUAD_W);
    int32_t comp_y = (int32_t) (((int64_t) my * Game_Enh2DDraw.CropH) / GAME_ENH2D_QUAD_H);

    int32_t tx = (Game_Enh2D_CropOrigX + comp_x) >> 4;
    int32_t ty = (Game_Enh2D_CropOrigY + comp_y) >> 4;

    loc_151246 = (uint16_t) (tx + 1);
    loc_151248 = (uint16_t) (ty + 1);

    Game_Enh2DDraw.SelectorX = (tx << 4) - Game_Enh2D_OrigX;
    Game_Enh2DDraw.SelectorY = (ty << 4) - Game_Enh2D_OrigY;
    Game_Enh2DDraw.SelectorActive = 1;
}

// Select_M2_ModInit analogue
void CCALL Game_Enh2D_SelectArea(void)
{
    int32_t pw, ph;
    int32_t left_world, right_excl_world, top_world, bottom_excl_world;
    int32_t sel_x, sel_y, right_incl_view, bottom_incl_view;

    if (loc_1479B0)
    {
        pw = 32;
        ph = 48;
    }
    else
    {
        pw = 16;
        ph = 32;
    }

    int32_t px = (int32_t) loc_1482AC;
    int32_t py = (int32_t) loc_1482A8;

    int32_t sx = px & ~15;
    int32_t sy = (py - ph + 1) & ~15;

    if ((Game_Enh2DDraw.CropW <= 0) || (Game_Enh2DDraw.CropH <= 0)) return;

    // width/height are derived as the difference between two edges, not scaled directly
    left_world = sx - 2 * 16;             // 2 tiles left of the (tile-aligned) sprite
    right_excl_world = sx + pw + 2 * 16;  // one tile-column past the 2-tile right margin
    top_world = sy - 1 * 16;              // 1 tile above
    bottom_excl_world = sy + ph + 1 * 16; // one tile-row past the 1-tile bottom margin

    sel_x = Game_Enh2D_ScaleCeil(left_world - Game_Enh2D_CropOrigX, GAME_ENH2D_QUAD_W, Game_Enh2DDraw.CropW);
    right_incl_view = Game_Enh2D_ScaleCeil(right_excl_world - Game_Enh2D_CropOrigX, GAME_ENH2D_QUAD_W, Game_Enh2DDraw.CropW) - 1;

    sel_y = Game_Enh2D_ScaleCeil(top_world - Game_Enh2D_CropOrigY, GAME_ENH2D_QUAD_H, Game_Enh2DDraw.CropH);
    bottom_incl_view = Game_Enh2D_ScaleCeil(bottom_excl_world - Game_Enh2D_CropOrigY, GAME_ENH2D_QUAD_H, Game_Enh2DDraw.CropH) - 1;

    Game_Enh2D_SelX = (uint32_t) sel_x;
    Game_Enh2D_SelY = (uint32_t) sel_y;
    Game_Enh2D_SelW = (uint32_t) (right_incl_view - sel_x);
    Game_Enh2D_SelH = (uint32_t) (bottom_incl_view - sel_y);
}

// Get_2D_mouse_state analogue - view-space size of the party sprite for cursor placement
void CCALL Game_Enh2D_PartyBox(uint32_t big)
{
    int32_t pw = big ? 32 : 16;
    int32_t ph = big ? 48 : 32;
    int32_t crop_w = Game_Enh2DDraw.CropW;
    int32_t crop_h = Game_Enh2DDraw.CropH;

    if (crop_w < 1) crop_w = GAME_ENH2D_QUAD_W;
    if (crop_h < 1) crop_h = GAME_ENH2D_QUAD_H;

    Game_Enh2D_PartyBoxW = (uint32_t) (((int64_t) pw * GAME_ENH2D_QUAD_W) / crop_w);
    Game_Enh2D_PartyBoxH = (uint32_t) (((int64_t) ph * GAME_ENH2D_QUAD_H) / crop_h);
}

void Game_Enh2D_Snapshot(void)
{
    if (Game_Enh2DDraw.Active > 0)
    {
        Game_Enh2DDisplay = Game_Enh2DDraw;
        Game_Enh2DDraw.Active = 0;
        if (Game_Enh2DDraw.Overlay == Game_Enh2DBuffer[0])
        {
            Game_Enh2DDraw.Overlay = Game_Enh2DBuffer[1];
            Game_Enh2DDraw.Reference = Game_Enh2DRefBuffer[1];
        }
        else
        {
            Game_Enh2DDraw.Overlay = Game_Enh2DBuffer[0];
            Game_Enh2DDraw.Reference = Game_Enh2DRefBuffer[0];
        }
    }
    else
    {
        // Draw.Active also reads 0 between ordinary draws, not just when truly done; Game_Enh2D_CompositeActive stays accurate through such gaps
        if (!Game_SceneVisible(GAME_SCREEN_MAP_2D) || !Game_Enh2D_CompositeActive)
        {
            Game_Enh2DDisplay.SelectorActive = 0;
            Game_Enh2DDisplay.Active = 0;

            if (Game_Enh2D_Initialized)
            {
                uint32_t cur = __atomic_load_n(&Game_Enh2DSnapshotIndex, __ATOMIC_ACQUIRE) % GAME_ENH2D_SLOTS;

                if (Game_Enh2DSnapshot[cur].Active)
                {
                    uint32_t next = (Game_Enh2DSnapshotIndex + 1) % GAME_ENH2D_SLOTS;
                    Game_Enh2DSnapshot[next].Active = 0;
                    __atomic_store_n(&Game_Enh2DSnapshotIndex, next, __ATOMIC_RELEASE);
                }
            }
        }
    }

    if (Game_Enh2DDisplay.Active && Game_Enh2D_Initialized && (Game_FrameBuffer != NULL) && (Game_Enh2DDisplay.Reference != NULL))
    {
        uint32_t next = (Game_Enh2DSnapshotIndex + 1) % GAME_ENH2D_SLOTS;
        Game_Enh2DInfo snap = Game_Enh2DDisplay;

        memcpy(Game_Enh2DScreenBuffer[next], &Game_FrameBuffer[Game_DisplayStart * 360], 360 * 240);
        memcpy(Game_Enh2DRefCopy[next], Game_Enh2DDisplay.Reference,
               GAME_ENH2D_QUAD_W * GAME_ENH2D_QUAD_H);

        snap.Screen = Game_Enh2DScreenBuffer[next];
        snap.Reference = Game_Enh2DRefCopy[next];
        snap.Mask = Game_Enh2DMaskBuf[next];

        {
            const uint8_t *scr = snap.Screen;
            const uint8_t *ref = snap.Reference;
            uint8_t *msk = snap.Mask;
            int i, n = GAME_ENH2D_QUAD_W * GAME_ENH2D_QUAD_H;

            int miss_cur = 0, miss_prev = 0;

            const uint8_t *last = (Game_Enh2DPrevRef != NULL) ? Game_Enh2DPrevRef : ref;
            static int skips = 0;
            int lagging;

            for (i = 0; i < n; i++)
            {
                miss_cur += (scr[i] != ref[i]);
                miss_prev += (scr[i] != last[i]);
            }

            lagging = (miss_prev < miss_cur);

            if (lagging && (skips < 3))
            {
                // keep snapshot on screen
                skips++;
                return;
            }

            skips = 0;

            for (i = 0; i < n; i++)
            {
                msk[i] = (uint8_t) (scr[i] != ref[i]);
            }

            if (Game_Enh2DPrevRef != NULL) memcpy(Game_Enh2DPrevRef, ref, n);
        }

        Game_Enh2DDisplay.Screen = snap.Screen;

        Game_Enh2DSnapshot[next] = snap;

        __atomic_store_n(&Game_Enh2DSnapshotIndex, next, __ATOMIC_RELEASE);
    }
}

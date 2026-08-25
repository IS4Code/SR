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

#include "../Game_defs.h"
#include "../Game_vars.h"
#include "../Albion-engine.h"
#include "palette32bgra.h"
#include "overlay.h"
#include <memory.h>
#include <string.h>

static int DisplayMode;

static int ScaledWidth, ScaledHeight, Fullscreen;

static pixel_format_disp Game_PaletteAlpha[256];

static void Set_Palette_Value2(uint32_t index, uint32_t r, uint32_t g, uint32_t b)
{
    Game_PaletteAlpha[index].s.r = r;
    Game_PaletteAlpha[index].s.g = g;
    Game_PaletteAlpha[index].s.b = b;
    Game_PaletteAlpha[index].s.a = 255;
}

static void Blit_Paletted(uint32_t *dst, const uint8_t *src, uint32_t count, const pixel_format_disp *palette)
{
    while (count >= 8)
    {
        dst[0] = palette[src[0]].pix;
        dst[1] = palette[src[1]].pix;
        dst[2] = palette[src[2]].pix;
        dst[3] = palette[src[3]].pix;
        dst[4] = palette[src[4]].pix;
        dst[5] = palette[src[5]].pix;
        dst[6] = palette[src[6]].pix;
        dst[7] = palette[src[7]].pix;

        src += 8;
        dst += 8;
        count -= 8;
    }

    while (count != 0)
    {
        dst[0] = palette[src[0]].pix;

        src++;
        dst++;
        count--;
    }
}

extern uint8_t loc_17D95C[256]; // &Recolour_tables[7][0]
extern uint8_t loc_13B726[]; // Select_2D_cursor

static uint8_t SamplePixel_2D(const Game_Enh2DInfo *snap, int comp_x, int comp_y)
{
    uint8_t base = snap->Overlay[comp_y * snap->CompW + comp_x];

    if (snap->SelectorActive)
    {
        int lx = comp_x - snap->SelectorX;
        int ly = comp_y - snap->SelectorY;

        // Put_masked_block(&Main_OPM, X - 1, Y - 1, 18, 18, Select_2D_cursor);
        if ((lx >= -1) && (lx < 17) && (ly >= -1) && (ly < 17))
        {
            uint8_t p = loc_13B726[(ly + 1) * 18 + (lx + 1)];
            if (p != 0) return p;
        }

        // Put_recoloured_box(&Main_OPM, X, Y, 16, 16, &Recolour_tables[7][0]);
        if ((lx >= 0) && (lx < 16) && (ly >= 0) && (ly < 16))
        {
            return loc_17D95C[base];
        }
    }

    return base;
}

// DrawOverlay analogue, for the whole 360x192 screen
static int SnapshotReady_2D(void)
{
    const uint32_t i = __atomic_load_n(&Game_Enh2DSnapshotIndex, __ATOMIC_ACQUIRE) % GAME_ENH2D_SLOTS;
    const Game_Enh2DInfo *const s = &Game_Enh2DSnapshot[i];

    return s->Active && (s->Overlay != NULL) && (s->Screen != NULL) && (s->Mask != NULL);
}

static void Flip_2D_composite(uint32_t *dst1, uint32_t *dst2)
{
    // load snapshot, written from the game thread
    const uint32_t snap_idx = __atomic_load_n(&Game_Enh2DSnapshotIndex, __ATOMIC_ACQUIRE) % GAME_ENH2D_SLOTS;
    const Game_Enh2DInfo snap = Game_Enh2DSnapshot[snap_idx];
    const uint8_t *mask = snap.Mask; // 360x192
    const uint8_t *const src = snap.Screen;

    // mask out Game_PaletteAlpha pixels if unchanged
    for (int y = 0; y < 192; y++)
    {
        for (int x = 0; x < 360; x++)
        {
            const int i = y * 360 + x;

            dst1[i] = mask[i] ? Game_PaletteAlpha[src[i]].pix : 0;
        }
    }

    // UI
    const uint8_t *s = src + 360 * 192;
    uint32_t *d = dst1 + 360 * 192;
    for (int counter = 360 * (240 - 192); counter != 0; counter--)
    {
        d[0] = Game_PaletteAlpha[s[0]].pix;
        s++;
        d++;
    }

    // resamples the crop rect into the scaled viewport
    const int crop_w = snap.CropW;
    const int crop_h = snap.CropH;
    const int crop_off_x = snap.CropOffX;
    const int crop_off_y = snap.CropOffY;
    int dst_w = Scaler_ScaleFactor * 360;
    int dst_h_total = Scaler_ScaleFactor * 240;
    int dst_h_view = Scaler_ScaleFactor * 192;
    int xdelta = (crop_w << 16) / dst_w;
    int ydelta = (crop_h << 16) / dst_h_view;
    int xpos = 0;
    int ypos = 0;

    for (int y = 0; y < dst_h_total; y++)
    {
        int local_y, comp_y, view_y, in_viewport;

        if (y >= dst_h_view)
        {
            for (int x = 0; x < dst_w; x++)
            {
                dst2[y * dst_w + x] = 0;
            }
            continue;
        }

        local_y = ypos >> 16;
        comp_y = crop_off_y + local_y;
        view_y = (int) (((int64_t) local_y * 192) / crop_h);
        in_viewport = (local_y < crop_h) && (view_y < 192);

        xpos = 0;
        for (int x = 0; x < dst_w; x++)
        {
            int local_x = xpos >> 16;
            int comp_x = crop_off_x + local_x;
            int show = 0;

            if (in_viewport && (local_x < crop_w))
            {
                int view_x = (int) (((int64_t) local_x * 360) / crop_w);

                if ((view_x < 360) && (dst1[view_y * 360 + view_x] == 0))
                {
                    show = 1;
                }
            }

            dst2[y * dst_w + x] = show ? Game_PaletteAlpha[SamplePixel_2D(&snap, comp_x, comp_y)].pix : 0;

            xpos += xdelta;
        }

        ypos += ydelta;
    }
}

static void Flip_360x240x8_to_360x240x32_advanced(uint8_t *src, uint32_t *dst1, uint32_t *dst2, int *dst2_used)
{
    int counter, DrawOverlay;
    Game_OverlayInfo OverlayInfo;

    OverlayInfo = Game_OverlayDisplay;
    DrawOverlay = Get_DrawOverlay(src, &OverlayInfo);
    *dst2_used = DrawOverlay;

    if (DrawOverlay)
    {
        uint8_t *zalsrc, *src2, *orig;
        uint32_t *zaldst1, *zaldst2;
        int x, y, ViewportX2;
        uint8_t *line0, *line1, *line2, lines[364*3], *linetemp;

        // display part above the viewport
        if (OverlayInfo.ViewportY != 0)
        {
            counter = 360 * OverlayInfo.ViewportY;
            Blit_Paletted(dst1, src, (uint32_t) counter, Game_PaletteAlpha);
            src += counter;
            dst1 += counter;

            for (counter = Scaler_ScaleFactor * 360 * Scaler_ScaleFactor * OverlayInfo.ViewportY; counter != 0; counter -= 8)
            {
                dst2[0] = 0;
                dst2[1] = 0;
                dst2[2] = 0;
                dst2[3] = 0;
                dst2[4] = 0;
                dst2[5] = 0;
                dst2[6] = 0;
                dst2[7] = 0;

                dst2 += 8;
            }
        }

        zalsrc = src;
        zaldst1 = dst1;
        zaldst2 = dst2;

        // display part left of the viewport
        if (OverlayInfo.ViewportX != 0)
        {
            for (y = OverlayInfo.ViewportHeight; y != 0; y--)
            {
                Blit_Paletted(dst1, src, OverlayInfo.ViewportX, Game_PaletteAlpha);
                src += 360;
                dst1 += 360;
            }

            for (y = Scaler_ScaleFactor * OverlayInfo.ViewportHeight; y != 0; y--)
            {
                for (x = Scaler_ScaleFactor * OverlayInfo.ViewportX; x >= 8; x -= 8)
                {
                    dst2[0] = 0;
                    dst2[1] = 0;
                    dst2[2] = 0;
                    dst2[3] = 0;
                    dst2[4] = 0;
                    dst2[5] = 0;
                    dst2[6] = 0;
                    dst2[7] = 0;

                    dst2 += 8;
                }

                for (; x != 0; x--)
                {
                    dst2[0] = 0;

                    dst2++;
                }

                dst2 += Scaler_ScaleFactor * (360 - OverlayInfo.ViewportX);
            }
        }

        // display part right of the viewport
        if ((OverlayInfo.ViewportX + OverlayInfo.ViewportWidth) != 360)
        {
            src = zalsrc + (OverlayInfo.ViewportX + OverlayInfo.ViewportWidth);
            dst1 = zaldst1 + (OverlayInfo.ViewportX + OverlayInfo.ViewportWidth);
            dst2 = zaldst2 + Scaler_ScaleFactor * (OverlayInfo.ViewportX + OverlayInfo.ViewportWidth);
            ViewportX2 = 360 - (OverlayInfo.ViewportX + OverlayInfo.ViewportWidth);

            for (y = OverlayInfo.ViewportHeight; y != 0; y--)
            {
                Blit_Paletted(dst1, src, (uint32_t) ViewportX2, Game_PaletteAlpha);
                src += 360;
                dst1 += 360;
            }

            for (y = Scaler_ScaleFactor * OverlayInfo.ViewportHeight; y != 0; y--)
            {
                for (x = Scaler_ScaleFactor * ViewportX2; x >= 8; x -= 8)
                {
                    dst2[0] = 0;
                    dst2[1] = 0;
                    dst2[2] = 0;
                    dst2[3] = 0;
                    dst2[4] = 0;
                    dst2[5] = 0;
                    dst2[6] = 0;
                    dst2[7] = 0;

                    dst2 += 8;
                }

                for (; x != 0; x--)
                {
                    dst2[0] = 0;

                    dst2++;
                }

                dst2 += Scaler_ScaleFactor * (360 - ViewportX2);
            }
        }

        // display part below the viewport
        src = zalsrc + (360 * OverlayInfo.ViewportHeight);
        dst1 = zaldst1 + (360 * OverlayInfo.ViewportHeight);
        dst2 = zaldst2 + Scaler_ScaleFactor * Scaler_ScaleFactor * (360 * OverlayInfo.ViewportHeight);

        counter = 360 * (240 - (OverlayInfo.ViewportY + OverlayInfo.ViewportHeight));
        Blit_Paletted(dst1, src, (uint32_t) counter, Game_PaletteAlpha);
        src += counter;
        dst1 += counter;

        for (counter = Scaler_ScaleFactor * 360 * Scaler_ScaleFactor * (240 - (OverlayInfo.ViewportY + OverlayInfo.ViewportHeight)); counter != 0; counter -= 8)
        {
            dst2[0] = 0;
            dst2[1] = 0;
            dst2[2] = 0;
            dst2[3] = 0;
            dst2[4] = 0;
            dst2[5] = 0;
            dst2[6] = 0;
            dst2[7] = 0;

            dst2 += 8;
        }

        // the viewport
        src = zalsrc + OverlayInfo.ViewportX;
        dst1 = zaldst1 + OverlayInfo.ViewportX;
        orig = OverlayInfo.ScreenViewpartOriginal + 360 * OverlayInfo.ViewportY + OverlayInfo.ViewportX;

        line0 = lines + 2;
        line1 = line0 + 364;
        line2 = line1 + 364;
        line0[-1] = line0[OverlayInfo.ViewportWidth] = 0;
        line1[-1] = line1[OverlayInfo.ViewportWidth] = 0;
        line2[-1] = line2[OverlayInfo.ViewportWidth] = 0;
        for (x = 0; (unsigned int)x < OverlayInfo.ViewportWidth; x++) line0[x] = 0;
        for (x = 0; (unsigned int)x < OverlayInfo.ViewportWidth; x++) line1[x] = src[x] - orig[x];

        for (y = OverlayInfo.ViewportHeight; y != 0; y--)
        {
            if (y != 1)
            {
                for (x = 0; (unsigned int)x < OverlayInfo.ViewportWidth; x++) line2[x] = src[x + 360] - orig[x + 360];
            }
            else
            {
                for (x = 0; (unsigned int)x < OverlayInfo.ViewportWidth; x++) line2[x] = 0;
            }

            for (x = 0; (unsigned int)x < OverlayInfo.ViewportWidth; x++)
            {
                int diff;

                if (line1[x] == 0)
                {
                    diff = 0;
                    if (line1[x - 1]) diff++;
                    if (line1[x + 1]) diff++;
                    if (line0[x]) diff++;
                    if (line2[x]) diff++;
                }
                else
                {
                    diff = 2;
                }

                diff--;
                dst1[x] = (diff <= 0)?0:Game_PaletteAlpha[src[x]].pix;
            }

            src += 360;
            orig += 360;
            dst1 += 360;

            linetemp = line0;
            line0 = line1;
            line1 = line2;
            line2 = linetemp;
        }

        dst1 = zaldst1 + OverlayInfo.ViewportX;
        dst2 = zaldst2 + Scaler_ScaleFactor * OverlayInfo.ViewportX;
        src2 = OverlayInfo.ScreenViewpartOverlay + Scaler_ScaleFactor * 360 * Scaler_ScaleFactor * OverlayInfo.ViewportY + Scaler_ScaleFactor * OverlayInfo.ViewportX;

        counter = Scaler_ScaleFactor;
        for (y = Scaler_ScaleFactor * OverlayInfo.ViewportHeight; y != 0; y--)
        {
            int counter2, same;

            counter2 = Scaler_ScaleFactor;
            same = (dst1[0] == 0)?1:0;

            for (x = Scaler_ScaleFactor * OverlayInfo.ViewportWidth; x != 0; x--)
            {
                dst2[0] = (same)?Game_PaletteAlpha[src2[0]].pix:0;

                src2++;
                dst2++;

                counter2--;
                if (counter2 == 0)
                {
                    counter2 = Scaler_ScaleFactor;
                    dst1++;
                    same = (dst1[0] == 0)?1:0;
                }
            }

            src2 += Scaler_ScaleFactor * (360 - OverlayInfo.ViewportWidth);
            dst2 += Scaler_ScaleFactor * (360 - OverlayInfo.ViewportWidth);

            counter--;
            if (counter == 0)
            {
                counter = Scaler_ScaleFactor;
                dst1 += (360 - OverlayInfo.ViewportWidth);
            }
            else
            {
                dst1 -= OverlayInfo.ViewportWidth;
            }
        }

        // markers
        if (DrawOverlay & 1)
        {
            dst1 = zaldst1 + 360 * (OverlayInfo.ViewportHeight - 2) + (OverlayInfo.ViewportX + 1);

            for (x = 8; x != 0; x--)
            {
                dst1[0] = 0;
                dst1++;
            }

            dst2 = zaldst2 + Scaler_ScaleFactor * 360 * Scaler_ScaleFactor * (OverlayInfo.ViewportHeight - 2) + Scaler_ScaleFactor * (OverlayInfo.ViewportX + 1);
            src2 = OverlayInfo.ScreenViewpartOverlay + Scaler_ScaleFactor * 360 * Scaler_ScaleFactor * (OverlayInfo.ViewportY + OverlayInfo.ViewportHeight - 2) + Scaler_ScaleFactor * (OverlayInfo.ViewportX + 1);

            for (y = Scaler_ScaleFactor; y != 0; y--)
            {
                for (x = Scaler_ScaleFactor * 8; x != 0; x--)
                {
                    dst2[0] = Game_PaletteAlpha[src2[0]].pix;
                    src2++;
                    dst2++;
                }

                src2 += Scaler_ScaleFactor * 352;
                dst2 += Scaler_ScaleFactor * 352;
            }
        }

        if (DrawOverlay & 2)
        {
            dst1 = zaldst1 + 360 * (OverlayInfo.ViewportHeight - 2) + (OverlayInfo.ViewportX + OverlayInfo.ViewportWidth - 10);

            for (x = 8; x != 0; x--)
            {
                dst1[0] = 0;
                dst1++;
            }

            dst2 = zaldst2 + Scaler_ScaleFactor * 360 * Scaler_ScaleFactor * (OverlayInfo.ViewportHeight - 2) + Scaler_ScaleFactor * (OverlayInfo.ViewportX + OverlayInfo.ViewportWidth - 10);
            src2 = OverlayInfo.ScreenViewpartOverlay + Scaler_ScaleFactor * 360 * Scaler_ScaleFactor * (OverlayInfo.ViewportY + OverlayInfo.ViewportHeight - 2) + Scaler_ScaleFactor * (OverlayInfo.ViewportX + OverlayInfo.ViewportWidth - 10);

            for (y = Scaler_ScaleFactor; y != 0; y--)
            {
                for (x = Scaler_ScaleFactor * 8; x != 0; x--)
                {
                    dst2[0] = Game_PaletteAlpha[src2[0]].pix;
                    src2++;
                    dst2++;
                }

                src2 += Scaler_ScaleFactor * 352;
                dst2 += Scaler_ScaleFactor * 352;
            }
        }
    }
    else if (Game_Enh2D_HiresEnabled && SnapshotReady_2D() && Game_SceneVisible(GAME_SCREEN_MAP_2D))
    {
        *dst2_used = 1;
        Flip_2D_composite(dst1, dst2);
    }
    else
    {
        Blit_Paletted(dst1, src, 360 * 240, Game_Palette);
    }
}

static void Flip_360x240x8_to_720x480x32(uint8_t *src, uint32_t *dst)
{
    int x, y, DrawOverlay, ViewportX2;
    Game_OverlayInfo OverlayInfo;

#define WRITE_PIXEL2(_x) dst[2 * (_x)] = dst[1 + (2 * (_x))] = dst[720 + (2 * (_x))] = dst[721 + (2 * (_x))] = Game_Palette[src[(_x)]].pix;

    OverlayInfo = Game_OverlayDisplay;
    DrawOverlay = Get_DrawOverlay(src, &OverlayInfo);

    if (DrawOverlay)
    {
        uint8_t *zalsrc, *src2, *orig;
        uint32_t *zaldst;
        uint8_t *line0, *line1, *line2, lines[364*3], *linetemp;

        // display part above the viewport
        for (y = OverlayInfo.ViewportY; y != 0; y--)
        {
            for (x = 360; x != 0; x-=9)
            {
                WRITE_PIXEL2(0)
                WRITE_PIXEL2(1)
                WRITE_PIXEL2(2)
                WRITE_PIXEL2(3)
                WRITE_PIXEL2(4)
                WRITE_PIXEL2(5)
                WRITE_PIXEL2(6)
                WRITE_PIXEL2(7)
                WRITE_PIXEL2(8)

                src+=9;
                dst+=18;
            }
            dst+=720;
        }

        zalsrc = src;
        zaldst = dst;

        // display part left of the viewport
        if (OverlayInfo.ViewportX != 0)
        {
            for (y = OverlayInfo.ViewportHeight; y != 0; y--)
            {
                for (x = OverlayInfo.ViewportX; x != 0; x--)
                {
                    WRITE_PIXEL2(0)

                    src++;
                    dst+=2;
                }
                src+=(360-OverlayInfo.ViewportX);
                dst+=720+(720-2*OverlayInfo.ViewportX);
            }
        }

        // display part right of the viewport
        if ((OverlayInfo.ViewportX + OverlayInfo.ViewportWidth) != 360)
        {
            src = zalsrc + OverlayInfo.ViewportX + OverlayInfo.ViewportWidth;
            dst = zaldst + 2 * (OverlayInfo.ViewportX + OverlayInfo.ViewportWidth);
            ViewportX2 = 360 - (OverlayInfo.ViewportX + OverlayInfo.ViewportWidth);
            for (y = OverlayInfo.ViewportHeight; y != 0; y--)
            {
                for (x = ViewportX2; x != 0; x--)
                {
                    WRITE_PIXEL2(0)

                    src++;
                    dst+=2;
                }
                src+=(360-ViewportX2);
                dst+=720+(720-2*ViewportX2);
            }
        }

        // display part below the viewport
        src = zalsrc + 360 * OverlayInfo.ViewportHeight;
        dst = zaldst + 720 * 2 * OverlayInfo.ViewportHeight;
        for (y = 240 - (OverlayInfo.ViewportY + OverlayInfo.ViewportHeight); y != 0; y--)
        {
            for (x = 360; x != 0; x-=9)
            {
                WRITE_PIXEL2(0)
                WRITE_PIXEL2(1)
                WRITE_PIXEL2(2)
                WRITE_PIXEL2(3)
                WRITE_PIXEL2(4)
                WRITE_PIXEL2(5)
                WRITE_PIXEL2(6)
                WRITE_PIXEL2(7)
                WRITE_PIXEL2(8)

                src+=9;
                dst+=18;
            }
            dst+=720;
        }

        // the viewport
        src = zalsrc + OverlayInfo.ViewportX;
        orig = OverlayInfo.ScreenViewpartOriginal + 360 * OverlayInfo.ViewportY + OverlayInfo.ViewportX;
        dst = zaldst + 2 * OverlayInfo.ViewportX;
        src2 = OverlayInfo.ScreenViewpartOverlay + OverlayInfo.ViewportY*2 * 720 + OverlayInfo.ViewportX*2;

        line0 = lines + 2;
        line1 = line0 + 364;
        line2 = line1 + 364;
        line0[-1] = line0[OverlayInfo.ViewportWidth] = 0;
        line1[-1] = line1[OverlayInfo.ViewportWidth] = 0;
        line2[-1] = line2[OverlayInfo.ViewportWidth] = 0;
        for (x = 0; (unsigned int)x < OverlayInfo.ViewportWidth; x++) line0[x] = 0;
        for (x = 0; (unsigned int)x < OverlayInfo.ViewportWidth; x++) line1[x] = src[x] - orig[x];

        for (y = OverlayInfo.ViewportHeight; y != 0; y--)
        {
            if (y != 1)
            {
                for (x = 0; (unsigned int)x < OverlayInfo.ViewportWidth; x++) line2[x] = src[x + 360] - orig[x + 360];
            }
            else
            {
                for (x = 0; (unsigned int)x < OverlayInfo.ViewportWidth; x++) line2[x] = 0;
            }

            for (x = 0; (unsigned int)x < OverlayInfo.ViewportWidth; x++)
            {
                int diff;

                if (line1[x] == 0)
                {
                    diff = 0;
                    if (line1[x - 1]) diff++;
                    if (line1[x + 1]) diff++;
                    if (line0[x]) diff++;
                    if (line2[x]) diff++;
                }
                else
                {
                    diff = 2;
                }

                diff--;
                if (diff <= 0)
                {
                    dst[2 * x] = Game_Palette[src2[2 * x]].pix;
                    dst[2 * x + 1] = Game_Palette[src2[2 * x + 1]].pix;
                    dst[2 * x + 720] = Game_Palette[src2[2 * x + 720]].pix;
                    dst[2 * x + 721] = Game_Palette[src2[2 * x + 721]].pix;
                }
                else
                {
                    WRITE_PIXEL2(x)
                }
            }

            src += 360;
            orig += 360;
            src2 += 2*720;
            dst += 2*720;

            linetemp = line0;
            line0 = line1;
            line1 = line2;
            line2 = linetemp;
        }

        if (DrawOverlay & 1)
        {
            dst = zaldst + 720 * 2 * (OverlayInfo.ViewportHeight - 2) + 2 * (OverlayInfo.ViewportX + 1);
            src2 = OverlayInfo.ScreenViewpartOverlay + (OverlayInfo.ViewportY + OverlayInfo.ViewportHeight - 2)*2 * 720 + (OverlayInfo.ViewportX + 1)*2;
            for (x = 8; x != 0; x--)
            {
                dst[0] = Game_Palette[src2[0]].pix;
                dst[1] = Game_Palette[src2[1]].pix;
                dst[720] = Game_Palette[src2[720]].pix;
                dst[721] = Game_Palette[src2[721]].pix;

                src2+=2;
                dst+=2;
            }
        }

        if (DrawOverlay & 2)
        {
            dst = zaldst + 720 * 2 * (OverlayInfo.ViewportHeight - 2) + 2 * OverlayInfo.ViewportX + 2 * (OverlayInfo.ViewportWidth - 10);
            src2 = OverlayInfo.ScreenViewpartOverlay + (OverlayInfo.ViewportY + OverlayInfo.ViewportHeight - 2)*2 * 720 + OverlayInfo.ViewportX*2 + 2*(OverlayInfo.ViewportWidth - 10);
            for (x = 8; x != 0; x--)
            {
                dst[0] = Game_Palette[src2[0]].pix;
                dst[1] = Game_Palette[src2[1]].pix;
                dst[720] = Game_Palette[src2[720]].pix;
                dst[721] = Game_Palette[src2[721]].pix;

                src2+=2;
                dst+=2;
            }
        }
    }
    else
    {
        for (y = 240; y != 0; y--)
        {
            for (x = 360; x != 0; x-=9)
            {
                WRITE_PIXEL2(0)
                WRITE_PIXEL2(1)
                WRITE_PIXEL2(2)
                WRITE_PIXEL2(3)
                WRITE_PIXEL2(4)
                WRITE_PIXEL2(5)
                WRITE_PIXEL2(6)
                WRITE_PIXEL2(7)
                WRITE_PIXEL2(8)

                src+=9;
                dst+=18;
            }
            dst+=720;
        }
    }

#undef WRITE_PIXEL2
}

void Init_Display(void)
{
    Display_FSType = 1;
    Game_UseEnhanced3DEngineNewValue = 1;

    ScaledWidth = 720;
    ScaledHeight = 480;
    Fullscreen = 0;
}

void Init_Display2(void)
{
    Init_Palette();

    if (Fullscreen && Display_FSType)
    {
        if ((ScaledWidth == 0) || (ScaledHeight == 0))
        {
            Display_FSType = 2;
            ScaledWidth = 640;
            ScaledHeight = 480;
        }
    }
    else
    {
        if (ScaledWidth < 640) ScaledWidth = 640;
        if (ScaledHeight < 480) ScaledHeight = 480;
    }

    DisplayMode = 0;
    Display_Width = ScaledWidth;
    Display_Height = ScaledHeight;
    Display_Bitsperpixel = 32;
    Display_Fullscreen = Fullscreen;
    Display_MouseLocked = 0;
    Render_Width = 720;
    Render_Height = 480;
    Picture_Width = ScaledWidth;
    Picture_Height = ScaledHeight;
    Picture_Position_UL_X = 0;
    Picture_Position_UL_Y = 0;
    Picture_Position_BR_X = ScaledWidth - 1;
    Picture_Position_BR_Y = ScaledHeight - 1;
    if (Game_AdvancedScaling)
    {
        Render_Width = 360;
        Render_Height = 240;
        Display_Advanced_Flip_Procedure = (Game_Advanced_Flip_Procedure) &Flip_360x240x8_to_360x240x32_advanced;
    }
    else
    {
        Display_Flip_Procedure = (Game_Flip_Procedure) &Flip_360x240x8_to_720x480x32;
    }
}

int Config_Display(char *str, char *param)
{
    int num_int;

    if ( strncasecmp(str, "Display_", 8) == 0 ) // str begins with "Display_"
    {
        str += 8;

        if ( strcasecmp(str, "ScaledWidth") == 0)    // str equals "ScaledWidth"
        {
            num_int = 0;
            if (sscanf(param, "%i", &num_int) > 0)
            {
                ScaledWidth = num_int;
                if (ScaledWidth < 0) ScaledWidth = 0;
            }

            return 1;
        }
        else if ( strcasecmp(str, "ScaledHeight") == 0)    // str equals "ScaledHeight"
        {
            num_int = 0;
            if (sscanf(param, "%i", &num_int) > 0)
            {
                ScaledHeight = num_int;
                if (ScaledHeight < 0) ScaledHeight = 0;
            }

            return 1;
        }
        else if ( strcasecmp(str, "Fullscreen") == 0)    // str equals "Fullscreen"
        {
            if ( strcasecmp(param, "yes") == 0 ) // param equals "yes"
            {
                Fullscreen = 1;
            }
            else if ( strcasecmp(param, "no") == 0 ) // param equals "no"
            {
                Fullscreen = 0;
            }

            return 1;
        }
    }

    return 0;
}

void Cleanup_Display(void)
{
}


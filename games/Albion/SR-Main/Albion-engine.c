#include <stdio.h>
#include <stdlib.h>
#include "Game_defs.h"
#include "Game_vars.h"
#include "Albion-engine.h"

#pragma pack(1)
typedef struct PACKED {
    uint16_t Flags;
    uint16_t Type;
    uint16_t Screen_type;
    PTR32(void) MainLoop_function;
    PTR32(void) ModInit_function;
    PTR32(void) ModExit_function;
    PTR32(void) DisInit_function;
    PTR32(void) DisExit_function;
    PTR32(void) DisUpd_function;
} Game_Module;
#pragma pack()

extern Game_Module loc_179164[8]; // stack of screen modules
extern uint16_t loc_13EEEE; // stack top

uint16_t Game_ScreenType(void)
{
    return loc_179164[loc_13EEEE].Screen_type;
}

uint16_t Game_RootScreenType(void)
{
    for (int i = (int) loc_13EEEE; i >= 0; i--)
    {
        if (loc_179164[i].Type == 0) /* SCREEN_MOD */
        {
            return loc_179164[i].Screen_type;
        }
    }

    return GAME_SCREEN_NO_SCREEN;
}

int Game_SceneVisible(int scene_type)
{
    uint16_t screen_type = Game_RootScreenType();

    return (screen_type == (uint16_t) scene_type) || (screen_type == GAME_SCREEN_DIALOGUE);
}

extern uint16_t loc_153B28; // PARTY_DATA.Year
extern uint16_t loc_153B2A; // PARTY_DATA.Month
extern uint16_t loc_153B2C; // PARTY_DATA.Day
extern uint16_t loc_153B2E; // PARTY_DATA.Hour
extern uint16_t loc_153B30; // PARTY_DATA.Minute

char *Game_FormatDate(const char *format)
{
    int len = snprintf(NULL, 0, format, loc_153B28, loc_153B2A, loc_153B2C, loc_153B2E, loc_153B30);
    if (len < 0) return NULL;

    char *result = malloc((size_t) len + 1);
    if (result != NULL) snprintf(result, (size_t) len + 1, format, loc_153B28, loc_153B2A, loc_153B2C, loc_153B2E, loc_153B30);
    return result;
}

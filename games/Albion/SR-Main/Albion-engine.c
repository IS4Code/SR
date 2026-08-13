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

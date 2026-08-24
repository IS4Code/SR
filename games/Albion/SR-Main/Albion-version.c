#include <stdint.h>
#include <string.h>
#include "Albion-version.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

extern uint16_t loc_1344B8; // Albion_version_nr
extern uint16_t loc_1344BA; // Albion_subversion_nr
extern char loc_130004[9]; // __TIME__
extern char loc_13000D[12]; // __DATE__

#define Game_VersionMajor loc_1344B8
#define Game_VersionMinor loc_1344BA
#define Game_BuildTime loc_130004
#define Game_BuildDate loc_13000D

#if defined(__EMSCRIPTEN__)
EMSCRIPTEN_KEEPALIVE
#endif
void Game_SetVersion(int major, int minor)
{
    Game_VersionMajor = (uint16_t) major;
    Game_VersionMinor = (uint16_t) minor;
}

void Game_GetVersion(int *major, int *minor)
{
    if (major) *major = Game_VersionMajor;
    if (minor) *minor = Game_VersionMinor;
}

#if defined(__EMSCRIPTEN__)
EMSCRIPTEN_KEEPALIVE
#endif
void Game_SetBuildDate(const char *date, const char *time)
{
    if (date)
    {
        strncpy(Game_BuildDate, date, sizeof(Game_BuildDate) - 1);
        Game_BuildDate[sizeof(Game_BuildDate) - 1] = 0;
    }

    if (time)
    {
        strncpy(Game_BuildTime, time, sizeof(Game_BuildTime) - 1);
        Game_BuildTime[sizeof(Game_BuildTime) - 1] = 0;
    }
}

void Game_InitBuildInfo(void)
{
    Game_SetBuildDate(__DATE__, __TIME__);
}

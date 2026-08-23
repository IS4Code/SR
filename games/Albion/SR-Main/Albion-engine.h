#if !defined(_ALBION_ENGINE_H_INCLUDED_)
#define _ALBION_ENGINE_H_INCLUDED_

#define GAME_SCREEN_NO_SCREEN 0
#define GAME_SCREEN_MAP_2D 1
#define GAME_SCREEN_MAP_3D 2
#define GAME_SCREEN_DIALOGUE 7

uint16_t Game_ScreenType(void);
uint16_t Game_RootScreenType(void);

void Game_Fov_Adjust(double coef);

#define GAME_CAPTURE_DATE_SUFFIX " %d%02d%02d%02d%02d"
char *Game_FormatDate(const char *format);

#endif /* _ALBION_ENGINE_H_INCLUDED_ */

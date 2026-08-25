#if !defined(_ALBION_ENGINE_H_INCLUDED_)
#define _ALBION_ENGINE_H_INCLUDED_

#define GAME_SCREEN_NO_SCREEN 0
#define GAME_SCREEN_MAP_2D 1
#define GAME_SCREEN_MAP_3D 2
#define GAME_SCREEN_DIALOGUE 7

uint16_t Game_ScreenType(void);
uint16_t Game_RootScreenType(void);
int Game_SceneVisible(int scene_type);

void Game_Fov_Adjust(double coef);

// 1.0 = native/off, below 1.0 magnifies; upper bound is map-size/memory dependent, see Game_Enh2D_MapZoomCap
#define GAME_2DZOOM_MIN 0.5
#define GAME_2DZOOM_STEP 0.25
void Game_2DZoomFactor_Adjust(double coef);

#define GAME_CAPTURE_DATE_SUFFIX " %d%02d%02d%02d%02d"
char *Game_FormatDate(const char *format);

#endif /* _ALBION_ENGINE_H_INCLUDED_ */

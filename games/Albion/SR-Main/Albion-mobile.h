#if !defined(_ALBION_MOBILE_H_INCLUDED_)
#define _ALBION_MOBILE_H_INCLUDED_

extern int Game_SelectionMode;

void Game_InjectClick(Uint8 button, int x, int y);

// call once, after Game_ReadConfig()
void Game_ApplyDeveloperGodMode(void);

// call every event pump
void Game_MobileKeyboard_Poll(void);

#endif /* _ALBION_MOBILE_H_INCLUDED_ */

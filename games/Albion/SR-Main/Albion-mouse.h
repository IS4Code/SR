#if !defined(_ALBION_MOUSE_H_INCLUDED_)
#define _ALBION_MOUSE_H_INCLUDED_

int Game_MouseLook_Active(void);
void Game_MouseLook_Update(void);
void Game_MouseLook_Toggle(void);
void Game_MouseLook_Move(int32_t xrel, int32_t yrel);
void Game_MouseWheel_Move(int32_t y);

#endif /* _ALBION_MOUSE_H_INCLUDED_ */

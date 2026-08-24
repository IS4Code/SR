#if !defined(_ALBION_VERSION_H_INCLUDED_)
#define _ALBION_VERSION_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

void Game_SetVersion(int major, int minor);
void Game_GetVersion(int *major, int *minor);
void Game_SetBuildDate(const char *date, const char *time);
void Game_InitBuildInfo(void);

#ifdef __cplusplus
}
#endif

#endif /* _ALBION_VERSION_H_INCLUDED_ */

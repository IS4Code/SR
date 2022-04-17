/**
 *
 *  Copyright (C) 2016-2022 Roman Pauer
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

#if !defined(_INTRO_PROC_H_INCLUDED_)
#define _INTRO_PROC_H_INCLUDED_

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int Game_errno(void);
extern int Game_checkch(void);
extern int Game_getch(void);
extern int Game_dlseek(int32_t fd, int32_t offset, int32_t whence);
extern int Game_dread(void *buf, int32_t count, int32_t fd);
extern void Game_dclose(int32_t fd);

#ifdef __cplusplus
}
#endif

extern void Game_FlipScreen(void);

#endif /* _INTRO_PROC_H_INCLUDED_ */

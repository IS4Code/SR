/**
 *
 *  Copyright (C) 2018-2026 Roman Pauer
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

#include "Albion-BBERROR.h"
#include "Game_vars.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif


#define BBERROR_MAXERRORSONSTACK 20

typedef struct {
    ERROR_PrintErrorPtr PrintError;
    // todo: remove
    ERROR_PrintErrorPtr PrintErrorDOS;
    const char *prefix;
    uint8_t data[16];
} ERROR_ErrorStruct;

static ERROR_ErrorStruct ERROR_errors[BBERROR_MAXERRORSONSTACK];
static char ERROR_string_buffer[400];
static ERROR_OutputFuncPtr ERROR_OutputFunc;

#ifdef __cplusplus
extern "C" {
#endif
// todo: make static
extern uint16_t ERROR_num_errors;

// todo: remove
extern uint32_t CCALL Game_RunProcReg1_Asm(void *proc_addr, const char *proc_param1);
extern uint32_t CCALL Game_RunProcReg2_Asm(void *proc_addr, const char *proc_param1, const uint8_t *proc_param2);
#ifdef __cplusplus
}
#endif


static void ERROR_SetOutputFuncPtr(ERROR_OutputFuncPtr output_func_ptr);
static void BBERROR_LocalPrintError(char *buffer, const uint8_t *data);

#if defined(__EMSCRIPTEN__)
static void ERROR_WebOutputLine(const char *message);
#endif

void CCALL ERROR_Init(ERROR_OutputFuncPtr output_func_ptr)
{
    ERROR_ClearStack();
    ERROR_SetOutputFuncPtr(output_func_ptr);
}

void CCALL ERROR_ClearStack(void)
{
    ERROR_num_errors = 0;
}

static void ERROR_SetOutputFuncPtr(ERROR_OutputFuncPtr output_func_ptr)
{
    ERROR_OutputFunc = output_func_ptr;
}

int32_t CCALL ERROR_PushError(ERROR_PrintErrorPtr error_print_error_ptr, const char *error_prefix, int32_t error_data_len, const uint8_t *error_data)
{
    int data_index;

    if (ERROR_num_errors == BBERROR_MAXERRORSONSTACK)
    {
        ERROR_num_errors--;
        ERROR_PushError(BBERROR_LocalPrintError, "BBERROR Library", (int32_t)strlen("STACK FULL") + 1, (const uint8_t *) "STACK FULL");

        return 1;
    }

    if (error_data_len > 16)
    {
        ERROR_PushError(BBERROR_LocalPrintError, "BBERROR Library", (int32_t)strlen("TOO MUCH DATA") + 1, (const uint8_t *) "TOO MUCH DATA");

        return 2;
    }

    ERROR_errors[ERROR_num_errors].PrintError = error_print_error_ptr;
    ERROR_errors[ERROR_num_errors].PrintErrorDOS = NULL;
    ERROR_errors[ERROR_num_errors].prefix = error_prefix;
    for (data_index = 0; data_index < error_data_len; data_index++)
    {
        ERROR_errors[ERROR_num_errors].data[data_index] = error_data[data_index];
    }
    ERROR_num_errors++;

    return 0;
}

// todo: remove
int32_t CCALL ERROR_PushErrorDOS(ERROR_PrintErrorPtr error_print_error_ptr, const char *error_prefix, int32_t error_data_len, const uint8_t *error_data)
{
    int data_index;

    if (ERROR_num_errors == BBERROR_MAXERRORSONSTACK)
    {
        ERROR_num_errors--;
        ERROR_PushError(BBERROR_LocalPrintError, "BBERROR Library", (int32_t)strlen("STACK FULL") + 1, (const uint8_t *) "STACK FULL");

        return 1;
    }

    if (error_data_len > 16)
    {
        ERROR_PushError(BBERROR_LocalPrintError, "BBERROR Library", (int32_t)strlen("TOO MUCH DATA") + 1, (const uint8_t *) "TOO MUCH DATA");

        return 2;
    }

    ERROR_errors[ERROR_num_errors].PrintError = NULL;
    ERROR_errors[ERROR_num_errors].PrintErrorDOS = error_print_error_ptr;
    ERROR_errors[ERROR_num_errors].prefix = error_prefix;
    for (data_index = 0; data_index < error_data_len; data_index++)
    {
        ERROR_errors[ERROR_num_errors].data[data_index] = error_data[data_index];
    }
    ERROR_num_errors++;

    return 0;
}

void CCALL ERROR_PopError(void)
{
    if (ERROR_num_errors)
    {
        ERROR_num_errors--;
    }
}

int32_t CCALL ERROR_IsStackEmpty(void)
{
    return (ERROR_num_errors == 0)?1:0;
}

void CCALL ERROR_PrintAllErrors(uint32_t flags)
{
    int index;
    int buflen;

    if (ERROR_num_errors == 0) return;

    if ((flags & 0x20) && (flags & 0x08))
    {
#if !defined(__EMSCRIPTEN__)
        if (ERROR_OutputFunc != NULL)
        {
            //ERROR_OutputFunc("BBERROR: ERRORSTACK START:--------------\n");
            // todo: remove
            Game_RunProcReg1_Asm((void *)ERROR_OutputFunc, "BBERROR: ERRORSTACK START:--------------\n");
        }
#endif
    }

    for (index = 0; index < ERROR_num_errors; index++)
    {
        ERROR_string_buffer[0] = 0;
        buflen = 0;

        if (flags & 0x02)
        {
            if (ERROR_errors[index].prefix != NULL)
            {
                snprintf(ERROR_string_buffer + buflen, sizeof(ERROR_string_buffer) - buflen, "%s: ", ERROR_errors[index].prefix);
                buflen = (int)strlen(ERROR_string_buffer);
            }
        }

        if (flags & 0x01)
        {
            if (ERROR_errors[index].PrintError != NULL)
            {
                ERROR_errors[index].PrintError(ERROR_string_buffer + buflen, ERROR_errors[index].data);
                buflen = (int)strlen(ERROR_string_buffer);
            }
            // todo: remove
            else if (ERROR_errors[index].PrintErrorDOS != NULL)
            {
                Game_RunProcReg2_Asm((void *)ERROR_errors[index].PrintErrorDOS, ERROR_string_buffer + buflen, ERROR_errors[index].data);
                buflen = (int)strlen(ERROR_string_buffer);
            }
        }

        if (flags & 0x04)
        {
            snprintf(ERROR_string_buffer + buflen, sizeof(ERROR_string_buffer) - buflen, "\n");
            buflen = (int)strlen(ERROR_string_buffer);
        }

        if (flags & 0x20)
        {
#if defined(__EMSCRIPTEN__)
            ERROR_WebOutputLine(ERROR_string_buffer);
#else
            if (ERROR_OutputFunc != NULL)
            {
                //ERROR_OutputFunc(ERROR_string_buffer);
                // todo: remove
                Game_RunProcReg1_Asm((void *)ERROR_OutputFunc, ERROR_string_buffer);
            }
#endif
        }
    }

    if ((flags & 0x20) && (flags & 0x10))
    {
#if !defined(__EMSCRIPTEN__)
        if (ERROR_OutputFunc != NULL)
        {
            //ERROR_OutputFunc("BBERROR: ERRORSTACK END-----------------\n");
            // todo: remove
            Game_RunProcReg1_Asm((void *)ERROR_OutputFunc, "BBERROR: ERRORSTACK END-----------------\n");
        }
#endif
    }

    ERROR_num_errors = 0;
}

static void BBERROR_LocalPrintError(char *buffer, const uint8_t *data)
{
    sprintf(buffer, "INTERNAL ERROR: %s", (const char *)data);
}

#if defined(__EMSCRIPTEN__)
static int ERROR_web_reports = 0;
static int ERROR_web_disabled = 0;

static void ERROR_WebOutputLine(const char *message)
{
    char line[sizeof(ERROR_string_buffer) + 64];

    if (ERROR_web_disabled) return;

    ERROR_web_reports++;

    if (ERROR_web_reports > BBERROR_MAXERRORSONSTACK)
    {
        ERROR_web_disabled = 1;
        snprintf(line, sizeof(line), "%s Further errors will be disabled.", message);
        message = line;
    }

    warnprint(message);
}
#endif

static int WebPrint(const char *message, int show_panel)
{
#if defined(__EMSCRIPTEN__)
    return MAIN_THREAD_EM_ASM_INT({
        if (!Module.print) return 0;
        var text = UTF8ToString($0);
        Module.print(text);
        if ($1 && typeof showPanel === 'function') showPanel(true);
        return 1;
    }, message, show_panel);
#else
    return 0;
#endif
}

void logprint(const char *message)
{
    if (!WebPrint(message, 0)) fprintf(stderr, "%s\n", message);
}

void warnprint(const char *message)
{
    if (!WebPrint(message, 1)) fprintf(stderr, "%s\n", message);
}

void logprintf(const char *format, ...)
{
    char buffer[512];
    va_list args;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    logprint(buffer);
}

void warnprintf(const char *format, ...)
{
    char buffer[512];
    va_list args;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    warnprint(buffer);
}


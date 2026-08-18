#include <stdio.h>
#include <string.h>
#include "Game_defs.h"
#include "Albion-websettings.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>

static void Game_WebSetupOptionsChanged(const char *payload)
{
    MAIN_THREAD_EM_ASM({
        var payload = UTF8ToString($0);
        if (window.Game_SaveSetupOptions) window.Game_SaveSetupOptions(payload);
    }, payload);
}
#endif

void Game_SyncSetupOptions(const char *setup_ini_real_path)
{
#if defined(__EMSCRIPTEN__)
    FILE *f;
    char line[256];
    char section[64];
    char payload[2048];
    size_t payload_len;
    int in_albion;

    f = fopen(setup_ini_real_path, "rt");
    if (f == NULL) return;

    section[0] = 0;
    in_albion = 0;
    payload_len = 0;
    payload[0] = 0;

    while (fgets(line, sizeof(line), f) != NULL)
    {
        char *s;
        size_t len;

        s = line;
        len = strlen(s);
        while (len != 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) s[--len] = 0;
        while (*s == ' ' || *s == '\t') s++;
        len = strlen(s);
        while (len != 0 && (s[len - 1] == ' ' || s[len - 1] == '\t')) s[--len] = 0;

        if (len == 0) continue;

        if (s[0] == '[' && s[len - 1] == ']')
        {
            size_t name_len;

            name_len = len - 2;
            if (name_len >= sizeof(section)) name_len = sizeof(section) - 1;
            memcpy(section, s + 1, name_len);
            section[name_len] = 0;

            in_albion = (strcasecmp(section, "ALBION") == 0);
            continue;
        }

        if (in_albion)
        {
            size_t add_len;

            add_len = strlen(s);
            // +2: the '\n' this appends, plus the terminator
            if (payload_len + add_len + 2 <= sizeof(payload))
            {
                memcpy(payload + payload_len, s, add_len);
                payload_len += add_len;
                payload[payload_len] = '\n';
                payload_len++;
                payload[payload_len] = 0;
            }
        }
    }

    fclose(f);

    if (payload_len != 0)
    {
        Game_WebSetupOptionsChanged(payload);
    }
#else
    (void) setup_ini_real_path;
#endif
}

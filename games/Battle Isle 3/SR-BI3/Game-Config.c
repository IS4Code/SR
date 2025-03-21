/**
 *
 *  Copyright (C) 2021 Roman Pauer
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

#define GAME_CONFIG_DEFINE_VARIABLES
#include "Game-Config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define CONFIG_FILE "BI3.cfg"


static char *trim_string(char *buf)
{
    int len;

    while (*buf == ' ') buf++;

    len = strlen(buf);

    while (len != 0 && buf[len - 1] == ' ')
    {
        len--;
        buf[len] = 0;
    }

    return buf;
}


void ReadConfiguration(void) __attribute__((noinline));
void ReadConfiguration(void)
{
    FILE *f;
    char buf[8192];
    char *str, *param;
    int items, num_int;

    Intro_Play = 1;
    Outro_Play = 1;

#if defined(__WINE__)
    Audio_MidiSubsystem = 3;
#else
    Audio_MidiSubsystem = 11;
#endif
    Audio_MidiVolume = 100;
    Audio_SoundFontPath = NULL;
    Audio_MidiDevice = NULL;
    Audio_OPL3BankNumber = 59;

    f = fopen(CONFIG_FILE, "rt");
    if (f == NULL) return;


    while (!feof(f))
    {
        // skip empty lines
        items = fscanf(f, "%8192[\n\r]", buf);

        // read line
        buf[0] = 0;
        items = fscanf(f, "%8192[^\n^\r]", buf);
        if (items <= 0) continue;

        // trim line
        str = trim_string(buf);

        if (str[0] == 0) continue;
        if (str[0] == '#') continue;

        // find parameter (after '=')
        param = strchr(str, '=');

        if (param == NULL) continue;

        // split string into two strings
        *param = 0;
        param++;

        // trim them
        str = trim_string(str);
        param = trim_string(param);

        if ( strncasecmp(str, "Intro_", 6) == 0 ) // str begins with "Intro_"
        {
            // intro settings

            str += 6;

            if ( strcasecmp(str, "Play") == 0 ) // str equals "Play"
            {
                if ( strcasecmp(param, "yes") == 0 ) // param equals "yes"
                {
                    Intro_Play = 1;
                }
                else if ( strcasecmp(param, "no") == 0 ) // param equals "no"
                {
                    Intro_Play = 0;
                }
            }

        }
        else if ( strncasecmp(str, "Outro_", 6) == 0 ) // str begins with "Outro_"
        {
            // intro settings

            str += 6;

            if ( strcasecmp(str, "Play") == 0 ) // str equals "Play"
            {
                if ( strcasecmp(param, "yes") == 0 ) // param equals "yes"
                {
                    Outro_Play = 1;
                }
                else if ( strcasecmp(param, "no") == 0 ) // param equals "no"
                {
                    Outro_Play = 0;
                }
            }

        }
        else if ( strncasecmp(str, "Audio_", 6) == 0 ) // str begins with "Audio_"
        {
            // audio settings

            str += 6;

            if ( strcasecmp(str, "MidiSubsystem") == 0 ) // str equals "MidiSubsystem"
            {
                // MIDI subsystem

                if ( strcasecmp(param, "wildmidi") == 0 ) // param equals "wildmidi"
                {
                    Audio_MidiSubsystem = 1;
                }
                else if ( strcasecmp(param, "bassmidi") == 0 ) // param equals "bassmidi"
                {
                    Audio_MidiSubsystem = 2;
                }
                else if ( strcasecmp(param, "adlmidi") == 0 ) // param equals "adlmidi"
                {
                    Audio_MidiSubsystem = 3;
                }
                else if ( strcasecmp(param, "nativewindows") == 0 ) // param equals "nativewindows"
                {
                    Audio_MidiSubsystem = 11;
                }
                else if ( strcasecmp(param, "alsa") == 0 ) // param equals "alsa"
                {
                    Audio_MidiSubsystem = 12;
                }
                else if ( strcasecmp(param, "original") == 0 ) // param equals "original"
                {
                    Audio_MidiSubsystem = 0;
                }
            }
            else if ( strcasecmp(str, "MidiVolume") == 0 ) // str equals "MidiVolume"
            {
                num_int = 0;
                sscanf(param, "%i", &num_int);
                if (num_int > 0)
                {
                    Audio_MidiVolume = num_int;
                }
            }
            else if ( strcasecmp(str, "SoundFont_Path") == 0 ) // str equals "SoundFont_Path"
            {
                // path to SoundFont file

                if (*param != 0)
                {
                    Audio_SoundFontPath = strdup(param);
                }
            }
            else if ( strcasecmp(str, "MidiDevice") == 0 ) // str equals "MidiDevice"
            {
                // MIDI Device name

                if (*param != 0)
                {
                    Audio_MidiDevice = strdup(param);
                }
            }
            else if ( strcasecmp(str, "OPL3BankNumber") == 0 ) // str equals "OPL3BankNumber"
            {
                num_int = 0;
                sscanf(param, "%i", &num_int);
                if (num_int >= 0)
                {
                    Audio_OPL3BankNumber = num_int;
                }
            }

        }

    }

    fclose(f);
}

#if (defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__))
    #define WIN32_LEAN_AND_MEAN

    #include <windows.h>
#else
    #define _FILE_OFFSET_BITS 64
    #include <unistd.h>
    #include <sys/types.h>
    #include <dirent.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "midi-plugins.h"
#include "fluidsynth.h"

#ifdef _MSC_VER
    #define EXPORT __declspec(dllexport)
    #define strdup _strdup
#elif defined __GNUC__
    #define EXPORT __attribute__ ((visibility ("default")))
#else
    #define EXPORT
#endif

#define FLUIDSYNTH_FULL_GAIN 0.2f
#define MAX_FLUIDSYNTH_STREAMS 8

// owns all members
typedef struct _fluidsynth_stream_
{
    fluid_settings_t *settings;
    fluid_synth_t *synth;
    fluid_player_t *player;
    // copied from input
    char *midi_file;
    void *midi_buffer;
    size_t midi_buffer_size;
} fluidsynth_stream;

static fluidsynth_stream *open_streams[MAX_FLUIDSYNTH_STREAMS];
static char *soundfont_path = NULL;
static unsigned int stream_sample_rate = 44100;
static unsigned char master_volume = 127;

static int file_exists(char const *filename)
{
    if (filename == NULL) return 0;
    if (*filename == 0) return 0;

#if (defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__))
    DWORD dwAttrib = GetFileAttributesA(filename);
    if ((dwAttrib == INVALID_FILE_ATTRIBUTES) || (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
#else
    if (access(filename, F_OK))
#endif
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

static int MIDI_PLUGIN_API set_master_volume(unsigned char new_master_volume) // 0 - 127
{
    if (new_master_volume > 127) new_master_volume = 127;

    master_volume = new_master_volume;

    for (int index = 0; index < MAX_FLUIDSYNTH_STREAMS; index++)
    {
        if (open_streams[index])
        {
            fluid_synth_set_gain(open_streams[index]->synth, FLUIDSYNTH_FULL_GAIN * ((float) master_volume / 127.0f));
        }
    }

    return 0;
}

static void destroy_stream(fluidsynth_stream *stream)
{
    if (stream == NULL) return;

    for (int index = 0; index < MAX_FLUIDSYNTH_STREAMS; index++)
    {
        if (open_streams[index] == stream)
        {
            open_streams[index] = NULL;
            break;
        }
    }

    if (stream->player)
    {
        fluid_player_stop(stream->player);
        delete_fluid_player(stream->player);
    }
    if (stream->synth) delete_fluid_synth(stream->synth);
    if (stream->settings) delete_fluid_settings(stream->settings);
    if (stream->midi_file) free(stream->midi_file);
    if (stream->midi_buffer) free(stream->midi_buffer);

    free(stream);
}

static fluidsynth_stream *create_stream(void)
{
    int index;
    for (index = 0; index < MAX_FLUIDSYNTH_STREAMS; index++)
    {
        if (open_streams[index] == NULL) break;
    }
    if (index >= MAX_FLUIDSYNTH_STREAMS) return NULL;

    fluidsynth_stream *stream = (fluidsynth_stream *) calloc(1, sizeof(fluidsynth_stream));
    if (stream == NULL) return NULL;

    stream->settings = new_fluid_settings();
    if (stream->settings == NULL)
    {
        free(stream);
        return NULL;
    }

    fluid_settings_setnum(stream->settings, "synth.sample-rate", (double) stream_sample_rate);
    fluid_settings_setint(stream->settings, "synth.midi-channels", 16);
    fluid_settings_setint(stream->settings, "synth.threadsafe-api", 0);

    stream->synth = new_fluid_synth(stream->settings);
    if (stream->synth == NULL)
    {
        delete_fluid_settings(stream->settings);
        free(stream);
        return NULL;
    }

    fluid_synth_set_gain(stream->synth, FLUIDSYNTH_FULL_GAIN * ((float) master_volume / 127.0f));

    if (fluid_synth_sfload(stream->synth, soundfont_path, 1) == FLUID_FAILED)
    {
        delete_fluid_synth(stream->synth);
        delete_fluid_settings(stream->settings);
        free(stream);
        return NULL;
    }

    stream->player = new_fluid_player(stream->synth);
    if (stream->player == NULL)
    {
        delete_fluid_synth(stream->synth);
        delete_fluid_settings(stream->settings);
        free(stream);
        return NULL;
    }

    open_streams[index] = stream;

    return stream;
}

static void * MIDI_PLUGIN_API open_file(char const *midifile)
{
    if (midifile == NULL) return NULL;

    fluidsynth_stream *stream = create_stream();
    if (stream == NULL) return NULL;

    stream->midi_file = strdup(midifile);

    if ((stream->midi_file == NULL) ||
        (fluid_player_add(stream->player, stream->midi_file) != FLUID_OK) ||
        (fluid_player_play(stream->player) != FLUID_OK))
    {
        destroy_stream(stream);
        return NULL;
    }

    return (void *) stream;
}

static void * MIDI_PLUGIN_API open_buffer(void const *midibuffer, long int size)
{
    if (midibuffer == NULL) return NULL;
    if (size <= 0) return NULL;

    fluidsynth_stream *stream = create_stream();
    if (stream == NULL) return NULL;

    // clone the MIDI
    stream->midi_buffer = malloc((size_t) size);

    if (stream->midi_buffer == NULL)
    {
        destroy_stream(stream);
        return NULL;
    }
    memcpy(stream->midi_buffer, midibuffer, (size_t) size);
    stream->midi_buffer_size = (size_t) size;

    if ((fluid_player_add_mem(stream->player, stream->midi_buffer, stream->midi_buffer_size) != FLUID_OK) ||
        (fluid_player_play(stream->player) != FLUID_OK))
    {
        destroy_stream(stream);
        return NULL;
    }

    return (void *) stream;
}

static long int MIDI_PLUGIN_API get_data(void *handle, void *buffer, long int size)
{
    if (handle == NULL) return -2;
    if (buffer == NULL) return -3;
    if (size < 0) return -4;
    if (size < 4) return 0;

    fluidsynth_stream *stream = (fluidsynth_stream *) handle;

    int frames = size / 4; // 16-bit stereo

    // looping starts when the MIDI reaches the end (not when the sound goes quiet)
    int total_ticks = fluid_player_get_total_ticks(stream->player);
    if ((total_ticks > 0) && (fluid_player_get_current_tick(stream->player) >= total_ticks))
    {
        // render less to indicate the end
        frames--;
    }

    if (fluid_synth_write_s16(stream->synth, frames, buffer, 0, 2, buffer, 1, 2) != FLUID_OK)
    {
        return -1;
    }

    if (frames != (size / 4))
    {
        // reset before playback finishes
        fluid_player_seek(stream->player, 0);
    }

    return ((long int) frames) << 2;
}

static int MIDI_PLUGIN_API rewind_midi(void *handle)
{
    if (handle == NULL) return -2;

    fluidsynth_stream *stream = (fluidsynth_stream *) handle;

    if (fluid_player_get_status(stream->player) == FLUID_PLAYER_PLAYING)
    {
        fluid_player_seek(stream->player, 0);
        return 0;
    }

    // construct a replacement player for the old dead one
    fluid_player_t *new_player = new_fluid_player(stream->synth);
    if (new_player == NULL) return -1;

    if (stream->midi_file != NULL)
    {
        if (fluid_player_add(new_player, stream->midi_file) != FLUID_OK)
        {
            delete_fluid_player(new_player);
            return -1;
        }
    }
    else if (stream->midi_buffer != NULL)
    {
        if (fluid_player_add_mem(new_player, stream->midi_buffer, stream->midi_buffer_size) != FLUID_OK)
        {
            delete_fluid_player(new_player);
            return -1;
        }
    }
    else
    {
        delete_fluid_player(new_player);
        return -1;
    }

    if (fluid_player_play(new_player) != FLUID_OK)
    {
        delete_fluid_player(new_player);
        return -1;
    }

    fluid_player_stop(stream->player);
    delete_fluid_player(stream->player);
    stream->player = new_player;

    return 0;
}

static int MIDI_PLUGIN_API close_midi(void *handle)
{
    if (handle == NULL) return -2;

    destroy_stream((fluidsynth_stream *) handle);

    return 0;
}

static void MIDI_PLUGIN_API shutdown_plugin(void)
{
    for (int index = 0; index < MAX_FLUIDSYNTH_STREAMS; index++)
    {
        if (open_streams[index] != NULL)
        {
            destroy_stream(open_streams[index]);
        }
    }

    if (soundfont_path != NULL)
    {
        free(soundfont_path);
        soundfont_path = NULL;
    }
}


EXPORT
int MIDI_PLUGIN_API initialize_midi_plugin(unsigned short int rate, midi_plugin_parameters const *parameters, midi_plugin_functions *functions)
{
    char const *soundfont_sf2 = NULL;
    stream_sample_rate = rate;
    if (parameters)
    {
        soundfont_sf2 = parameters->soundfont_path;
        if (stream_sample_rate == 0)
        {
            stream_sample_rate = parameters->sampling_rate;
        }
    }

    if ((stream_sample_rate < 8000) || (stream_sample_rate > 96000)) return -2;
    if (functions == NULL) return -3;

    if (soundfont_sf2)
    {
        if (!file_exists(soundfont_sf2))
        {
            soundfont_sf2 = NULL;
        }
    }

    if (soundfont_sf2)
    {
        soundfont_path = strdup(soundfont_sf2);
    }
    else
    {
#if (defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__))
        WIN32_FIND_DATA finddata;
        HANDLE findhandle = FindFirstFile("./*.sf2", &finddata);
        if (findhandle != INVALID_HANDLE_VALUE)
        {
            do
            {
                if ((finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                {
                    soundfont_path = strdup(&(finddata.cFileName[0]));
                    break;
                }
            } while (FindNextFile(findhandle, &finddata));

            FindClose(findhandle);
        }
#else
        DIR *curdir = opendir(".");
        if (curdir)
        {
            struct dirent *curentry;
            while ((curentry = readdir(curdir)) != NULL)
            {
                int len;
                len = strlen(&(curentry->d_name[0]));
                if (len < 5) continue;
                if (strcasecmp(&(curentry->d_name[len - 4]), ".sf2")) continue;

                if (file_exists(&(curentry->d_name[0])))
                {
                    soundfont_path = strdup(&(curentry->d_name[0]));
                    break;
                }
            }
            closedir(curdir);
        }
#endif
    }

    if (soundfont_path == NULL) return -4;

    memset(&(open_streams[0]), 0, sizeof(open_streams));
    master_volume = 127;

    functions->set_master_volume = &set_master_volume;
    functions->open_file = &open_file;
    functions->open_buffer = &open_buffer;
    functions->get_data = &get_data;
    functions->rewind_midi = &rewind_midi;
    functions->close_midi = &close_midi;
    functions->shutdown_plugin = &shutdown_plugin;

    return 0;
}

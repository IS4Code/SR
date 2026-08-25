#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#include "Game_defs.h"
#include "Game_vars.h"
#include "Albion-webshare.h"
#include "Albion-engine.h"
#include "virtualfs.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#include "brotli/encode.h"
#include "brotli/decode.h"
#endif

extern uint32_t CCALL Game_Share_TriggerSaveGameState(uint16_t saved_game_nr, const char *saved_game_name);

#define MAX_XFTYPES 49

extern char loc_1351C5[MAX_XFTYPES][13];
#define Game_Share_Filenames loc_1351C5

#define XFT_LANGUAGE_DEPENDENT (1u << 0)
#define XFT_SAVE_FILE          (1u << 1)

static const uint8_t share_xft_flags[MAX_XFTYPES] = {
    /*  0 Map data            */ 0,
    /*  1 Icon data           */ 0,
    /*  2 Icon graphics       */ 0,
    /*  3 Palette             */ 0,
    /*  4 Base palette        */ 0,
    /*  5 Slab                */ 0,
    /*  6 Big party graphics  */ 0,
    /*  7 Small party graphics*/ 0,
    /*  8 Lab data            */ 0,
    /*  9 3D wall             */ 0,
    /* 10 3D object           */ 0,
    /* 11 3D overlay          */ 0,
    /* 12 3D floor            */ 0,
    /* 13 Big NPC graphics    */ 0,
    /* 14 Background graphics */ 0,
    /* 15 Font                */ 0,
    /* 16 Block list          */ 0,
    /* 17 Party character data*/ XFT_SAVE_FILE,
    /* 18 Small portrait      */ 0,
    /* 19 System texts        */ XFT_LANGUAGE_DEPENDENT,
    /* 20 Event set           */ 0,
    /* 21 Event texts         */ XFT_LANGUAGE_DEPENDENT,
    /* 22 Map texts           */ XFT_LANGUAGE_DEPENDENT,
    /* 23 Item list           */ 0,
    /* 24 Item names          */ 0,
    /* 25 Item graphics       */ 0,
    /* 26 Full-body picture   */ 0,
    /* 27 Automap             */ XFT_SAVE_FILE,
    /* 28 Automap graphics    */ 0,
    /* 29 Song                */ 0,
    /* 30 Sample              */ 0,
    /* 31 Wave library        */ 0,
    /* 32 (unused)            */ 0,
    /* 33 Chest data          */ XFT_SAVE_FILE,
    /* 34 Merchant data       */ XFT_SAVE_FILE,
    /* 35 NPC character data  */ XFT_SAVE_FILE,
    /* 36 Monster group       */ 0,
    /* 37 Monster character   */ 0,
    /* 38 Monster graphics    */ 0,
    /* 39 Combat background   */ 0,
    /* 40 Combat graphics     */ 0,
    /* 41 Tactical icon       */ 0,
    /* 42 Spell data          */ 0,
    /* 43 Small NPC graphics  */ 0,
    /* 44 Flic                */ 0,
    /* 45 Dictionary          */ XFT_LANGUAGE_DEPENDENT,
    /* 46 Script              */ 0,
    /* 47 Picture             */ 0,
    /* 48 Transparency tables */ 0,
};

#if defined(__EMSCRIPTEN__)

#define SHARE_QUICKSAVE_SLOT 100
#define SHARE_TEMP_SLOT 101
#define SHARE_SAVE_MAGIC 0x25051971u
#define SHARE_SAVE_VERSION 138
#define SHARE_TEMP_SAVE_PATH "./SAVES/save.101"

#define SHARE_XLD_MAGIC1 0x30444C58u // XLD0
#define SHARE_XLD_MAGIC2 0x0049u     // I\0

static const char *const share_language_dirs[3] = { "GERMAN", "ENGLISH", "FRENCH" };
#define SHARE_DEFAULT_LANGUAGE 1

static void Game_Share_BuildXldPath(char *out, size_t outsz, uint16_t type, uint16_t group, const char *dir)
{
    if (type >= MAX_XFTYPES) { out[0] = 0; return; }

    char name[13];
    memcpy(name, Game_Share_Filenames[type], 13);
    name[12] = 0;
    size_t len = strlen(name);

    if (len >= 5 && name[len - 5] >= '0' && name[len - 5] <= '9')
    {
        name[len - 5] = (char) ('0' + (group % 10));
    }

    if (share_xft_flags[type] & XFT_LANGUAGE_DEPENDENT)
    {
        snprintf(out, outsz, "XLDLIBS\\%s\\%s", share_language_dirs[SHARE_DEFAULT_LANGUAGE], name);
    }
    else if ((share_xft_flags[type] & XFT_SAVE_FILE) && dir)
    {
        snprintf(out, outsz, "XLDLIBS\\%s\\%s", dir, name);
    }
    else
    {
        snprintf(out, outsz, "XLDLIBS\\%s", name);
    }
}

static int Game_Share_ReadAt(int fd, off_t off, void *buf, size_t len)
{
    if (lseek(fd, off, SEEK_SET) != off) return 0;
    return read(fd, buf, len) == (ssize_t) len;
}

static int Game_Share_WriteAt(int fd, off_t off, const void *buf, size_t len)
{
    if (lseek(fd, off, SEEK_SET) != off) return 0;
    return write(fd, buf, len) == (ssize_t) len;
}

static uint32_t Game_Share_GetU32(const uint8_t *p) { uint32_t v; memcpy(&v, p, 4); return v; }
static uint16_t Game_Share_GetU16(const uint8_t *p) { uint16_t v; memcpy(&v, p, 2); return v; }
static void Game_Share_SetU32(uint8_t *p, uint32_t v) { memcpy(p, &v, 4); }

#define SHARE_MAX_SUBFILES 100

static void Game_Share_XorData(int save_fd, off_t save_off, uint32_t len, int base_fd, off_t base_off, uint32_t base_avail)
{
    if (base_fd == -1) return;

    uint64_t save_buf[1024];
    uint64_t base_buf[sizeof(save_buf) / sizeof(save_buf[0])];
    uint32_t remaining = (len < base_avail) ? len : base_avail;

    while (remaining > 0)
    {
        uint32_t chunk = (remaining < sizeof(save_buf)) ? remaining : sizeof(save_buf);
        uint32_t chunk_ints = (chunk + 7) / 8;
        if ((chunk % 8) > 0)
        {
            // Prevent XORing uninitialized memory
            save_buf[chunk_ints - 1] = base_buf[chunk_ints - 1] = 0;
        }

        if (!Game_Share_ReadAt(save_fd, save_off, save_buf, chunk)) return;
        if (!Game_Share_ReadAt(base_fd, base_off, base_buf, chunk)) return;

        for (uint32_t i = 0; i < chunk_ints; i++) save_buf[i] ^= base_buf[i];

        if (!Game_Share_WriteAt(save_fd, save_off, save_buf, chunk)) return;

        save_off += chunk;
        base_off += chunk;
        remaining -= chunk;
    }
}

static void Game_Share_XorXld(int save_fd, off_t entry_off, uint32_t length, int base_fd, uint32_t baseline_len, const uint8_t base_hdr[8], int encoding)
{
    if (length < 8 || baseline_len < 8) return;

    uint8_t save_hdr[8];
    if (!Game_Share_ReadAt(save_fd, entry_off, save_hdr, 8)) return;
    uint32_t magic1 = Game_Share_GetU32(save_hdr);
    uint16_t magic2 = Game_Share_GetU16(save_hdr + 4);
    uint32_t saved_count = Game_Share_GetU16(save_hdr + 6);

    {
        uint8_t out[6];
        uint16_t nm2 = (uint16_t) (magic2 ^ SHARE_XLD_MAGIC2);
        Game_Share_SetU32(out, magic1 ^ SHARE_XLD_MAGIC1);
        memcpy(out + 4, &nm2, 2);
        if (!Game_Share_WriteAt(save_fd, entry_off, out, 6)) return;
    }
    // Check pre- or post-XOR depending on encoding
    if (encoding ? (magic1 != SHARE_XLD_MAGIC1) : (magic1 != 0)) return;
    if (encoding ? (magic2 != SHARE_XLD_MAGIC2) : (magic2 != 0)) return;

    if (8 + (off_t) saved_count * 4 > length) return;
    if (saved_count > SHARE_MAX_SUBFILES) return; 

    uint32_t baseline_count = Game_Share_GetU16(base_hdr + 6);
    if (baseline_count > SHARE_MAX_SUBFILES) baseline_count = SHARE_MAX_SUBFILES;
    if (8 + (uint32_t) baseline_count * 4 > baseline_len) baseline_count = (baseline_len - 8) / 4;

    uint32_t baseline_sizes[SHARE_MAX_SUBFILES];
    if (baseline_count > 0 && !Game_Share_ReadAt(base_fd, 8, baseline_sizes, (size_t) baseline_count * 4)) baseline_count = 0;

    off_t off = entry_off + 8;
    uint32_t saved_sizes[SHARE_MAX_SUBFILES];
    if (saved_count > 0 && !Game_Share_ReadAt(save_fd, off, saved_sizes, (size_t) saved_count * 4)) return;

    uint32_t sizes[SHARE_MAX_SUBFILES];
    for (uint32_t i = 0; i < saved_count; i++)
    {
        uint32_t v = saved_sizes[i];
        if (i < baseline_count)
        {
            uint32_t live_len = v ^ baseline_sizes[i];
            saved_sizes[i] = live_len;
            sizes[i] = encoding ? v : live_len;
        }
        else
        {
            sizes[i] = v;
        }
    }
    if (!Game_Share_WriteAt(save_fd, off, saved_sizes, (size_t) saved_count * 4)) return;
    off += (off_t) saved_count * 4;

    off_t base_data_off = 8 + (off_t) baseline_count * 4;
    for (uint32_t i = 0; i < saved_count; i++)
    {
        uint32_t sublen = sizes[i];
        if (off + sublen > entry_off + length) return;

        if (i < baseline_count)
        {
            uint32_t base_avail = 0;
            if (base_data_off <= (off_t) baseline_len)
            {
                base_avail = (base_data_off + baseline_sizes[i] <= baseline_len)
                    ? baseline_sizes[i] : (uint32_t) (baseline_len - base_data_off);
            }
            Game_Share_XorData(save_fd, off, sublen, base_fd, base_data_off, base_avail);
            base_data_off += baseline_sizes[i];
        }

        off += sublen;
    }
}

static void Game_Share_XorDirectoryEntry(int save_fd, off_t entry_off, uint32_t length, uint16_t type, uint16_t group, int encoding)
{
    if (type >= MAX_XFTYPES || !Game_Share_Filenames[type][0]) return;

    char dospath[64], realpath[MAX_PATH];
    Game_Share_BuildXldPath(dospath, sizeof(dospath), type, group, "INITIAL");
    realpath[0] = 0;
    file_entry *realdir;
    vfs_get_real_name(dospath, realpath, &realdir);
    vfs_fetch(realpath, 0);

    int base_fd = open(realpath, O_RDONLY);
    if (base_fd == -1) return;

    struct stat st;
    if (fstat(base_fd, &st) != 0) { close(base_fd); return; }
    uint32_t baseline_len = (uint32_t) st.st_size;

    uint8_t base_hdr[8];
    if (baseline_len >= 8 && Game_Share_ReadAt(base_fd, 0, base_hdr, 8) && Game_Share_GetU32(base_hdr) == SHARE_XLD_MAGIC1)
    {
        Game_Share_XorXld(save_fd, entry_off, length, base_fd, baseline_len, base_hdr, encoding);
    }
    else
    {
        Game_Share_XorData(save_fd, entry_off, length, base_fd, 0, baseline_len);
    }

    close(base_fd);
}

static void Game_Share_ProcessSaveData(int save_fd, off_t file_len, off_t pos, int encoding)
{
    static const uint32_t fixed_data_len =
        10 + 8 + 4 * (1 + 12 * 3) + 6 + 20 + 12 + 200 + 30 + 6 * 32 +
        (1024 + 7) / 8 + (512 * 250 + 7) / 8 + (512 * 96 + 7) / 8 + (256 + 7) / 8 +
        (999 + 7) / 8 + (999 + 7) / 8 + 300 // Party_data
        + 96 * 128 // VNPC_data[NPCS_PER_MAP]
        + 64 * 35; // located sound effect table

    if (pos + fixed_data_len > file_len) return;
    pos += fixed_data_len;

    for (int block = 0; block < 3; block++)
    {
        if (pos + 4 > file_len) return;
        uint32_t blocklen;
        if (!Game_Share_ReadAt(save_fd, pos, &blocklen, 4)) return;
        pos += 4;
        if (pos + blocklen > file_len) return;
        pos += blocklen;
    }

    for (;;)
    {
        if (pos + 4 > file_len) return;
        uint32_t entrylen;
        if (!Game_Share_ReadAt(save_fd, pos, &entrylen, 4)) return;
        pos += 4;
        if (entrylen == 0) return;

        if (pos + 4 > file_len) return;
        uint16_t type, group;
        if (!Game_Share_ReadAt(save_fd, pos, &type, 2)) return;
        if (!Game_Share_ReadAt(save_fd, pos + 2, &group, 2)) return;
        pos += 4;

        if (pos + entrylen > file_len) return;
        Game_Share_XorDirectoryEntry(save_fd, pos, entrylen, type, group, encoding);
        pos += entrylen;
    }
}

#define SHARE_DECOMPRESS_CHUNK 16384
#define SHARE_COMPRESS_CHUNK 8192

static uint8_t *Game_Share_BrotliCompressFromFile(int fd, off_t in_off, size_t in_size, size_t *out_size)
{
    size_t cap = BrotliEncoderMaxCompressedSize(in_size);
    if (cap == 0) cap = in_size + 1024;

    uint8_t *out = (uint8_t *) malloc(cap);
    if (!out) return NULL;

    BrotliEncoderState *state = BrotliEncoderCreateInstance(NULL, NULL, NULL);
    if (!state) { free(out); return NULL; }

    uint8_t *next_out = out;
    size_t avail_out = cap;

    uint8_t in_chunk[SHARE_COMPRESS_CHUNK];
    const uint8_t *next_in = in_chunk;
    
    size_t avail_in = 0;
    size_t remaining_in = in_size;
    int is_eof = 0;
    int ok = 1;
    
    for (;;)
    {
        BrotliEncoderOperation op;

        if (avail_in == 0 && !is_eof)
        {
            size_t want = (remaining_in < sizeof(in_chunk)) ? remaining_in : sizeof(in_chunk);
            if (want > 0 && !Game_Share_ReadAt(fd, in_off, in_chunk, want)) { ok = 0; break; }
            in_off += (off_t) want;
            remaining_in -= want;
            next_in = in_chunk;
            avail_in = want;
            is_eof = (remaining_in == 0);
        }
        op = is_eof ? BROTLI_OPERATION_FINISH : BROTLI_OPERATION_PROCESS;

        if (!BrotliEncoderCompressStream(state, op, &avail_in, &next_in, &avail_out, &next_out, NULL)) { ok = 0; break; }

        if (is_eof && BrotliEncoderIsFinished(state)) break;
    }

    BrotliEncoderDestroyInstance(state);
    if (!ok) { free(out); return NULL; }

    *out_size = cap - avail_out;
    return out;
}

static int Game_Share_BrotliDecompressToFile(int fd, off_t write_off, const uint8_t *in, size_t in_size)
{
    BrotliDecoderState *state = BrotliDecoderCreateInstance(NULL, NULL, NULL);
    if (!state) return 0;

    uint8_t chunk[SHARE_DECOMPRESS_CHUNK];

    const uint8_t *next_in = in;
    size_t avail_in = in_size;
    int ok = 1;

    for (;;)
    {
        uint8_t *next_out = chunk;
        size_t avail_out = sizeof(chunk);
        BrotliDecoderResult result = BrotliDecoderDecompressStream(state, &avail_in, &next_in, &avail_out, &next_out, NULL);
        size_t produced = sizeof(chunk) - avail_out;

        if (produced > 0)
        {
            if (!Game_Share_WriteAt(fd, write_off, chunk, produced)) { ok = 0; break; }
            write_off += (off_t) produced;
        }

        if (result == BROTLI_DECODER_RESULT_SUCCESS) break;
        if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) continue;

        ok = 0;
        break;
    }

    BrotliDecoderDestroyInstance(state);
    return ok;
}

static void Game_Share_DeleteTempSave(void)
{
    unlink(SHARE_TEMP_SAVE_PATH);
}

static void Game_Share_CopyLink(const char *name, uint32_t version, uint32_t data_offset)
{
    int handled = MAIN_THREAD_EM_ASM_INT({
        var name = UTF8ToString($0);
        var version = $1;
        var dataOffset = $2;
        if (typeof Game_Share_BuildAndCopyLink !== 'function')
        {
            if (Module.print) Module.print("share.js did not load correctly.");
            try { Module.FS.unlink(Game_Share_TempSavePath); } catch (e) {}
            return 1;
        }
        return Game_Share_BuildAndCopyLink(name, version, dataOffset, null, false) ? 1 : 0;
    }, name, version, (int) data_offset);

    if (handled) return;

    int fd = open(SHARE_TEMP_SAVE_PATH, O_RDONLY);
    struct stat st;
    if (fd == -1 || fstat(fd, &st) != 0 || (uint32_t) st.st_size < data_offset)
    {
        if (fd != -1) close(fd);
        logprint("Could not reopen the save for compression.");
        Game_Share_DeleteTempSave();
        return;
    }
    size_t in_size = (size_t) (st.st_size - data_offset);

    size_t compressed_size = 0;
    uint8_t *compressed = Game_Share_BrotliCompressFromFile(fd, (off_t) data_offset, in_size, &compressed_size);
    close(fd);
    if (!compressed)
    {
        logprint("Brotli compression failed.");
        Game_Share_DeleteTempSave();
        return;
    }

    MAIN_THREAD_EM_ASM({
        var name = UTF8ToString($0);
        var version = $1;
        var ptr = $2;
        var len = $3;
        // copy from shared buffer
        var bytes = HEAPU8.slice(ptr, ptr + len);
        if (typeof Game_Share_BuildAndCopyLink === 'function')
        {
            Game_Share_BuildAndCopyLink(name, version, 0, bytes, true);
        }
        else
        {
            if (Module.print) Module.print("share.js did not load correctly.");
            try { Module.FS.unlink(Game_Share_TempSavePath); } catch (e) {}
        }
    }, name, version, compressed, (uint32_t) compressed_size);

    free(compressed);
}

static void Game_Share_CreateLink(void)
{
    int fd = open(SHARE_TEMP_SAVE_PATH, O_RDWR);
    if (fd == -1)
    {
        int err = errno;
        logprintf("Could not open the quicksave, errno %d.", err);
        return;
    }
    struct stat st;
    if (fstat(fd, &st) != 0) { close(fd); Game_Share_DeleteTempSave(); return; }
    off_t file_len = st.st_size;

    if (file_len < 12)
    {
        close(fd);
        logprint("The save file is too small.");
        Game_Share_DeleteTempSave();
        return;
    }

    uint32_t namelen;
    if (!Game_Share_ReadAt(fd, 0, &namelen, 4)) { close(fd); Game_Share_DeleteTempSave(); return; }

    char name[128];
    if (namelen > sizeof(name) - 1 || 4 + (off_t) namelen + 8 > file_len)
    {
        close(fd);
        logprint("Unexpected savegame format.");
        Game_Share_DeleteTempSave();
        return;
    }
    if (namelen > 0 && !Game_Share_ReadAt(fd, 4, name, namelen)) { close(fd); Game_Share_DeleteTempSave(); return; }
    name[namelen] = 0;
    off_t pos = 4 + namelen;

    uint32_t magic;
    if (!Game_Share_ReadAt(fd, pos, &magic, 4)) { close(fd); Game_Share_DeleteTempSave(); return; }
    if (magic != SHARE_SAVE_MAGIC)
    {
        close(fd);
        logprint("unexpected savegame format.");
        Game_Share_DeleteTempSave();
        return;
    }
    pos += 4;

    uint32_t version;
    if (!Game_Share_ReadAt(fd, pos, &version, 4)) { close(fd); Game_Share_DeleteTempSave(); return; }
    pos += 4;

    Game_Share_ProcessSaveData(fd, file_len, pos, 1);

    close(fd);

    Game_Share_CopyLink(name, version, (uint32_t) pos);
}

static volatile int Game_WebShare_Pending = 0;

void Game_QuickSave_KeyTriggered(void)
{
    int is_share = Game_WebShare_Pending;
    Game_WebShare_Pending = 0;

    // allow saving in context menus
    uint16_t screen = Game_RootScreenType();
    if (screen != GAME_SCREEN_MAP_2D && screen != GAME_SCREEN_MAP_3D)
    {
        logprint("Save allowed only on 2D/3D map screen.");
        return;
    }

    uint16_t slot = is_share ? SHARE_TEMP_SLOT : SHARE_QUICKSAVE_SLOT;
    char *save_name = Game_FormatDate(is_share ? "Sharesave" GAME_CAPTURE_DATE_SUFFIX : "Quicksave" GAME_CAPTURE_DATE_SUFFIX);
    uint32_t saved = Game_Share_TriggerSaveGameState(slot, save_name);
    free(save_name);
    if (!saved)
    {
        logprint("Saving the game is not currently possible.");
        return;
    }

    if (is_share)
    {
        Game_Share_CreateLink();
    }
}

EMSCRIPTEN_KEEPALIVE
void Game_Share_RequestLink(void)
{
    // Set link request and trigger quicksave
    Game_WebShare_Pending = 1;

    SDL_Event event;
    memset(&event, 0, sizeof(event));
    event.key.keysym.sym = SDLK_F9;
    event.key.keysym.scancode = SDL_SCANCODE_F9;

    event.type = SDL_KEYDOWN;
    event.key.state = SDL_PRESSED;
    SDL_PushEvent(&event);

    event.type = SDL_KEYUP;
    event.key.state = SDL_RELEASED;
    SDL_PushEvent(&event);
}

EMSCRIPTEN_KEEPALIVE
void Game_Share_UnxorSaveFile(int data_offset)
{
    int fd = open(SHARE_TEMP_SAVE_PATH, O_RDWR);
    if (fd == -1)
    {
        int err = errno;
        logprintf("Could not reopen the loaded save, errno %d.", err);
        return;
    }
    struct stat st;
    if (fstat(fd, &st) != 0) { close(fd); return; }

    Game_Share_ProcessSaveData(fd, st.st_size, (off_t) data_offset, 0);

    close(fd);

    logprint("Shared game loaded successfully.");
}

EM_JS(uint8_t *, Game_Share_ReadLinkDataJs, (int *out_len, uint32_t magic, uint32_t version), {
    setValue(out_len, 0, 'i32');

    var params = (typeof Game_ParseFragmentParams === 'function') ? Game_ParseFragmentParams() : new URLSearchParams();
    var name = params.get('Save_Name') || "";
    var dataBase64 = params.get('Save_Data');
    if (dataBase64 == null) return 0;

    try
    {
        var compressed = Game_Share_FromBase64Url(dataBase64);

        var nameBytes = new TextEncoder().encode(name);
        var preamble = new Uint8Array(4 + nameBytes.length + 8);
        var dv = new DataView(preamble.buffer);
        dv.setUint32(0, nameBytes.length, true);
        preamble.set(nameBytes, 4);
        dv.setUint32(4 + nameBytes.length, magic, true);
        dv.setUint32(4 + nameBytes.length + 4, version, true);

        if (!Module.FS) throw new Error("filesystem not ready");
        Module.FS.mkdirTree('/SAVES');
        Module.FS.writeFile(Game_Share_TempSavePath, preamble);

        var decompressionStream;
        try
        {
            decompressionStream = new DecompressionStream('brotli');
        }
        catch (e)
        {
            var ptr = _malloc(compressed.length || 1);
            HEAPU8.set(compressed, ptr);
            setValue(out_len, compressed.length, 'i32');
            return ptr;
        }
        (async function () {
            try
            {
                var stream = new Blob([compressed]).stream().pipeThrough(decompressionStream);
                var reader = stream.getReader();
                var fd = Module.FS.open(Game_Share_TempSavePath, 'a');
                for (;;)
                {
                    var next = await reader.read();
                    if (next.done) break;
                    Module.FS.write(fd, next.value, 0, next.value.length);
                }
                Module.FS.close(fd);
                Module.ccall('Game_Share_UnxorSaveFile', null, [ 'number' ], [ preamble.length ]);
            }
            catch (e)
            {
                if (Module.print) Module.print("could not decompress the loaded save (" + e + ").");
            }
        })();
        return 0;
    }
    catch (e)
    {
        if (Module.print) Module.print("Could not load the shared savegame (" + e + ").");
        return 0;
    }
});

void Game_Share_LoadFromLink(void)
{
    int compressed_len = 0;
    uint8_t *compressed = Game_Share_ReadLinkDataJs(&compressed_len, SHARE_SAVE_MAGIC, SHARE_SAVE_VERSION);
    if (!compressed) return; // Handled

    int fd = open(SHARE_TEMP_SAVE_PATH, O_WRONLY);
    struct stat st;
    if (fd == -1 || fstat(fd, &st) != 0)
    {
        if (fd != -1) close(fd);
        free(compressed);
        logprint("Could not open the loaded save.");
        return;
    }
    off_t preamble_len = st.st_size;

    if (!Game_Share_BrotliDecompressToFile(fd, preamble_len, compressed, (size_t) compressed_len))
    {
        close(fd);
        free(compressed);
        logprint("Could not decompress the loaded save.");
        return;
    }

    close(fd);
    free(compressed);

    Game_Share_UnxorSaveFile((int) preamble_len);
}

#else /* !__EMSCRIPTEN__ */

void Game_QuickSave_KeyTriggered(void)
{
}

void Game_Share_RequestLink(void)
{
}

void Game_Share_LoadFromLink(void)
{
}

#endif

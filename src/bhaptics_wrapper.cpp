/*
 * bhaptics_wrapper.cpp
 *
 * Full Lua 5.4 C extension wrapping all functions exported by bhaptics_library.dll.
 * Function signatures are taken directly from library.h (extern "C", so export
 * names match 1:1 — no mangling to worry about).
 *
 * Compile as x64 DLL against Lua 5.4 headers + lua54.lib.
 * No bhaptics headers or import lib needed — everything is resolved at runtime
 * via LoadLibrary / GetProcAddress.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

// ---------------------------------------------------------------------------
// Function pointer types  (mirrors library.h exactly)
// ---------------------------------------------------------------------------
using fn_registryAndInit        = bool        (*)(const char*, const char*, const char*);
using fn_registryAndInitHost    = bool        (*)(const char*, const char*, const char*, const char*);
using fn_wsIsConnected          = bool        (*)();
using fn_wsClose                = void        (*)();
using fn_reInitMessage          = bool        (*)(const char*, const char*, const char*);

using fn_play                   = int         (*)(const char*);
using fn_playParam              = int         (*)(const char*, int, float, float, float, float);
using fn_playWithoutResult      = void        (*)(const char*, int, float, float, float, float);
using fn_playWithStartTime      = int         (*)(const char*, int, int, float, float, float, float);
using fn_playDot                = int         (*)(int, int, int, int*, int);
using fn_playWaveform           = int         (*)(int, int, int*, int*, int*, int, int);
using fn_playPath               = int         (*)(int, int, int, float*, float*, int*, int);
using fn_playLoop               = int         (*)(const char*, int, float, float, float, float, int, int);

using fn_getEventTime           = int         (*)(const char*);
using fn_pause                  = int         (*)(const char*);
using fn_resume                 = bool        (*)(const char*);

using fn_stop                   = bool        (*)(int);
using fn_stopByEventId          = bool        (*)(const char*);
using fn_stopAll                = bool        (*)();

using fn_isbHapticsConnected    = bool        (*)(int);
using fn_isPlaying              = bool        (*)();
using fn_isPlayingByRequestId   = bool        (*)(int);
using fn_isPlayingByEventId     = bool        (*)(const char*);

using fn_bHapticsGetHapticMessage  = const char* (*)(const char*, const char*, int, int&);
using fn_bHapticsGetHapticMappings = const char* (*)(const char*, const char*, int, int&);

using fn_isPlayerRunning        = bool        (*)();
using fn_isPlayerInstalled      = bool        (*)();
using fn_launchPlayer           = bool        (*)(bool);

using fn_getDeviceInfoJson      = const char* (*)();
using fn_getHapticMappingsJson  = const char* (*)();

using fn_ping                   = bool        (*)(const char*);
using fn_pingAll                = bool        (*)();
using fn_swapPosition           = bool        (*)(const char*);
using fn_setDeviceVsm           = bool        (*)(const char*, int);

// ---------------------------------------------------------------------------
// Module-level state
// ---------------------------------------------------------------------------
static HMODULE g_hLib = nullptr;

static fn_registryAndInit           g_registryAndInit           = nullptr;
static fn_registryAndInitHost       g_registryAndInitHost       = nullptr;
static fn_wsIsConnected             g_wsIsConnected             = nullptr;
static fn_wsClose                   g_wsClose                   = nullptr;
static fn_reInitMessage             g_reInitMessage             = nullptr;
static fn_play                      g_play                      = nullptr;
static fn_playParam                 g_playParam                 = nullptr;
static fn_playWithoutResult         g_playWithoutResult         = nullptr;
static fn_playWithStartTime         g_playWithStartTime         = nullptr;
static fn_playDot                   g_playDot                   = nullptr;
static fn_playWaveform              g_playWaveform              = nullptr;
static fn_playPath                  g_playPath                  = nullptr;
static fn_playLoop                  g_playLoop                  = nullptr;
static fn_getEventTime              g_getEventTime              = nullptr;
static fn_pause                     g_pause                     = nullptr;
static fn_resume                    g_resume                    = nullptr;
static fn_stop                      g_stop                      = nullptr;
static fn_stopByEventId             g_stopByEventId             = nullptr;
static fn_stopAll                   g_stopAll                   = nullptr;
static fn_isbHapticsConnected       g_isbHapticsConnected       = nullptr;
static fn_isPlaying                 g_isPlaying                 = nullptr;
static fn_isPlayingByRequestId      g_isPlayingByRequestId      = nullptr;
static fn_isPlayingByEventId        g_isPlayingByEventId        = nullptr;
static fn_bHapticsGetHapticMessage  g_bHapticsGetHapticMessage  = nullptr;
static fn_bHapticsGetHapticMappings g_bHapticsGetHapticMappings = nullptr;
static fn_isPlayerRunning           g_isPlayerRunning           = nullptr;
static fn_isPlayerInstalled         g_isPlayerInstalled         = nullptr;
static fn_launchPlayer              g_launchPlayer              = nullptr;
static fn_getDeviceInfoJson         g_getDeviceInfoJson         = nullptr;
static fn_getHapticMappingsJson     g_getHapticMappingsJson     = nullptr;
static fn_ping                      g_ping                      = nullptr;
static fn_pingAll                   g_pingAll                   = nullptr;
static fn_swapPosition              g_swapPosition              = nullptr;
static fn_setDeviceVsm              g_setDeviceVsm              = nullptr;

// ---------------------------------------------------------------------------
// DLL loader — called once from l_registryAndInit
// ---------------------------------------------------------------------------
static std::string LoadBhapticsLibrary()
{
    if (g_hLib) return "";

    g_hLib = LoadLibraryA("bhaptics_library.dll");
    if (!g_hLib) {
        char buf[256];
        snprintf(buf, sizeof(buf),
            "LoadLibrary(\"bhaptics_library.dll\") failed (Windows error %lu). "
            "Make sure bhaptics_library.dll is in the same scripts/ folder.",
            GetLastError());
        return buf;
    }

    // Resolve every export. Missing symbols are soft-warned but not fatal so
    // that a future SDK version with a subset of functions still loads.
    auto resolve = [&](auto& ptr, const char* name) {
        ptr = reinterpret_cast<std::remove_reference_t<decltype(ptr)>>(
                  GetProcAddress(g_hLib, name));
        if (!ptr)
            OutputDebugStringA(("bhaptics_wrapper: symbol not found: " + std::string(name) + "\n").c_str());
    };

    resolve(g_registryAndInit,           "registryAndInit");
    resolve(g_registryAndInitHost,       "registryAndInitHost");
    resolve(g_wsIsConnected,             "wsIsConnected");
    resolve(g_wsClose,                   "wsClose");
    resolve(g_reInitMessage,             "reInitMessage");
    resolve(g_play,                      "play");
    resolve(g_playParam,                 "playParam");
    resolve(g_playWithoutResult,         "playWithoutResult");
    resolve(g_playWithStartTime,         "playWithStartTime");
    resolve(g_playDot,                   "playDot");
    resolve(g_playWaveform,              "playWaveform");
    resolve(g_playPath,                  "playPath");
    resolve(g_playLoop,                  "playLoop");
    resolve(g_getEventTime,              "getEventTime");
    resolve(g_pause,                     "pause");
    resolve(g_resume,                    "resume");
    resolve(g_stop,                      "stop");
    resolve(g_stopByEventId,             "stopByEventId");
    resolve(g_stopAll,                   "stopAll");
    resolve(g_isbHapticsConnected,       "isbHapticsConnected");
    resolve(g_isPlaying,                 "isPlaying");
    resolve(g_isPlayingByRequestId,      "isPlayingByRequestId");
    resolve(g_isPlayingByEventId,        "isPlayingByEventId");
    resolve(g_bHapticsGetHapticMessage,  "bHapticsGetHapticMessage");
    resolve(g_bHapticsGetHapticMappings, "bHapticsGetHapticMappings");
    resolve(g_isPlayerRunning,           "isPlayerRunning");
    resolve(g_isPlayerInstalled,         "isPlayerInstalled");
    resolve(g_launchPlayer,              "launchPlayer");
    resolve(g_getDeviceInfoJson,         "getDeviceInfoJson");
    resolve(g_getHapticMappingsJson,     "getHapticMappingsJson");
    resolve(g_ping,                      "ping");
    resolve(g_pingAll,                   "pingAll");
    resolve(g_swapPosition,              "swapPosition");
    resolve(g_setDeviceVsm,              "setDeviceVsm");

    // registryAndInit is the one truly critical export
    if (!g_registryAndInit) {
        FreeLibrary(g_hLib);
        g_hLib = nullptr;
        return "bhaptics_library.dll loaded but 'registryAndInit' not found — wrong DLL version?";
    }

    return "";
}

// ---------------------------------------------------------------------------
// Convenience macros for guards used in every Lua binding
// ---------------------------------------------------------------------------
#define REQUIRE_LIB(L)                                          \
    if (!g_hLib) {                                              \
        lua_pushnil(L);                                         \
        lua_pushstring(L, "bhaptics_library.dll not loaded — call bhaptics.registry_and_init() first"); \
        return 2;                                               \
    }

#define REQUIRE_FN(L, fn)                                       \
    if (!(fn)) {                                                \
        lua_pushnil(L);                                         \
        lua_pushstring(L, "function '" #fn "' not found in bhaptics_library.dll"); \
        return 2;                                               \
    }

// ---------------------------------------------------------------------------
// Lua bindings
// ---------------------------------------------------------------------------

// bhaptics.registry_and_init(apiKey, workspaceId, initJson)  -> bool [, errmsg]
// This is the main initialisation call — also triggers DLL loading.
static int l_registryAndInit(lua_State* L)
{
    const char* apiKey      = luaL_checkstring(L, 1);
    const char* workspaceId = luaL_checkstring(L, 2);
    const char* initJson    = luaL_optstring(L, 3, "{}");

    std::string err = LoadBhapticsLibrary();
    if (!err.empty()) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, err.c_str());
        return 2;
    }

    bool ok = g_registryAndInit(apiKey, workspaceId, initJson);
    lua_pushboolean(L, ok);
    if (!ok) lua_pushstring(L, "registryAndInit returned false — is bHaptics Player running?");
    return ok ? 1 : 2;
}

// bhaptics.registry_and_init_host(apiKey, workspaceId, initJson, url)  -> bool
static int l_registryAndInitHost(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_registryAndInitHost)
    lua_pushboolean(L, g_registryAndInitHost(
        luaL_checkstring(L, 1), luaL_checkstring(L, 2),
        luaL_optstring(L, 3, "{}"), luaL_checkstring(L, 4)));
    return 1;
}

// bhaptics.ws_is_connected()  -> bool
static int l_wsIsConnected(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_wsIsConnected)
    lua_pushboolean(L, g_wsIsConnected() ? 1 : 0);
    return 1;
}

// bhaptics.ws_close()
static int l_wsClose(lua_State* L)
{
    if (g_wsClose) g_wsClose();
    return 0;
}

// bhaptics.re_init_message(apiKey, workspaceId, initJson)  -> bool
static int l_reInitMessage(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_reInitMessage)
    lua_pushboolean(L, g_reInitMessage(
        luaL_checkstring(L, 1), luaL_checkstring(L, 2),
        luaL_optstring(L, 3, "{}")));
    return 1;
}

// bhaptics.play(key)  -> requestId
static int l_play(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_play)
    lua_pushinteger(L, g_play(luaL_checkstring(L, 1)));
    return 1;
}

// bhaptics.play_param(key [, requestId=0, intensity=1, duration=1, angleX=0, offsetY=0])  -> requestId
static int l_playParam(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_playParam)
    lua_pushinteger(L, g_playParam(
        luaL_checkstring(L, 1),
        (int)luaL_optinteger(L, 2, 0),
        (float)luaL_optnumber(L, 3, 1.0),
        (float)luaL_optnumber(L, 4, 1.0),
        (float)luaL_optnumber(L, 5, 0.0),
        (float)luaL_optnumber(L, 6, 0.0)));
    return 1;
}

// bhaptics.play_without_result(key [, requestId=0, intensity=1, duration=1, angleX=0, offsetY=0])
static int l_playWithoutResult(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_playWithoutResult)
    g_playWithoutResult(
        luaL_checkstring(L, 1),
        (int)luaL_optinteger(L, 2, 0),
        (float)luaL_optnumber(L, 3, 1.0),
        (float)luaL_optnumber(L, 4, 1.0),
        (float)luaL_optnumber(L, 5, 0.0),
        (float)luaL_optnumber(L, 6, 0.0));
    return 0;
}

// bhaptics.play_with_start_time(key [, requestId=0, startMillis=0, intensity=1, duration=1, angleX=0, offsetY=0])  -> requestId
static int l_playWithStartTime(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_playWithStartTime)
    lua_pushinteger(L, g_playWithStartTime(
        luaL_checkstring(L, 1),
        (int)luaL_optinteger(L, 2, 0),
        (int)luaL_optinteger(L, 3, 0),
        (float)luaL_optnumber(L, 4, 1.0),
        (float)luaL_optnumber(L, 5, 1.0),
        (float)luaL_optnumber(L, 6, 0.0),
        (float)luaL_optnumber(L, 7, 0.0)));
    return 1;
}

// bhaptics.play_dot(requestId, position, duration, motorValues_table)  -> requestId
// motorValues_table: Lua array of integers 0-100, up to 32 entries
static int l_playDot(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_playDot)
    int requestId = (int)luaL_checkinteger(L, 1);
    int position  = (int)luaL_checkinteger(L, 2);
    int duration  = (int)luaL_checkinteger(L, 3);
    luaL_checktype(L, 4, LUA_TTABLE);

    int motorValues[32] = {};
    int len = (int)lua_rawlen(L, 4);
    if (len > 32) len = 32;
    for (int i = 0; i < len; i++) {
        lua_rawgeti(L, 4, i + 1);
        motorValues[i] = (int)lua_tointeger(L, -1);
        lua_pop(L, 1);
    }
    lua_pushinteger(L, g_playDot(requestId, position, duration, motorValues, len));
    return 1;
}

// bhaptics.play_waveform(requestId, position, motorValues_t, playTimes_t, shapes_t, repeatCount)  -> requestId
// All three tables must be the same length (== motorLen).
static int l_playWaveform(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_playWaveform)
    int requestId   = (int)luaL_checkinteger(L, 1);
    int position    = (int)luaL_checkinteger(L, 2);
    luaL_checktype(L, 3, LUA_TTABLE);
    luaL_checktype(L, 4, LUA_TTABLE);
    luaL_checktype(L, 5, LUA_TTABLE);
    int repeatCount = (int)luaL_checkinteger(L, 6);

    int motorValues[6] = {}, playTimes[6] = {}, shapes[6] = {};
    int len = (int)lua_rawlen(L, 3);
    if (len > 6) len = 6;
    for (int i = 0; i < len; i++) {
        lua_rawgeti(L, 3, i + 1); motorValues[i] = (int)lua_tointeger(L, -1); lua_pop(L, 1);
        lua_rawgeti(L, 4, i + 1); playTimes[i]   = (int)lua_tointeger(L, -1); lua_pop(L, 1);
        lua_rawgeti(L, 5, i + 1); shapes[i]      = (int)lua_tointeger(L, -1); lua_pop(L, 1);
    }
    lua_pushinteger(L, g_playWaveform(requestId, position, motorValues, playTimes, shapes, repeatCount, len));
    return 1;
}

// bhaptics.play_path(requestId, position, durationMillis, xValues_t, yValues_t, intensities_t)  -> requestId
static int l_playPath(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_playPath)
    int requestId       = (int)luaL_checkinteger(L, 1);
    int position        = (int)luaL_checkinteger(L, 2);
    int durationMillis  = (int)luaL_checkinteger(L, 3);
    luaL_checktype(L, 4, LUA_TTABLE);
    luaL_checktype(L, 5, LUA_TTABLE);
    luaL_checktype(L, 6, LUA_TTABLE);

    float xValues[32] = {}, yValues[32] = {};
    int   intensities[32] = {};
    int len = (int)lua_rawlen(L, 4);
    if (len > 32) len = 32;
    for (int i = 0; i < len; i++) {
        lua_rawgeti(L, 4, i + 1); xValues[i]    = (float)lua_tonumber(L, -1); lua_pop(L, 1);
        lua_rawgeti(L, 5, i + 1); yValues[i]    = (float)lua_tonumber(L, -1); lua_pop(L, 1);
        lua_rawgeti(L, 6, i + 1); intensities[i] = (int)lua_tointeger(L, -1); lua_pop(L, 1);
    }
    lua_pushinteger(L, g_playPath(requestId, position, durationMillis, xValues, yValues, intensities, len));
    return 1;
}

// bhaptics.play_loop(key [, requestId=0, intensity=1, duration=1, angleX=0, offsetY=0, interval=0, maxCount=0])  -> requestId
static int l_playLoop(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_playLoop)
    lua_pushinteger(L, g_playLoop(
        luaL_checkstring(L, 1),
        (int)luaL_optinteger(L, 2, 0),
        (float)luaL_optnumber(L, 3, 1.0),
        (float)luaL_optnumber(L, 4, 1.0),
        (float)luaL_optnumber(L, 5, 0.0),
        (float)luaL_optnumber(L, 6, 0.0),
        (int)luaL_optinteger(L, 7, 0),
        (int)luaL_optinteger(L, 8, 0)));
    return 1;
}

// bhaptics.get_event_time(eventId)  -> ms (int)
static int l_getEventTime(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_getEventTime)
    lua_pushinteger(L, g_getEventTime(luaL_checkstring(L, 1)));
    return 1;
}

// bhaptics.pause(eventId)  -> ms at pause point (int)
static int l_pause(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_pause)
    lua_pushinteger(L, g_pause(luaL_checkstring(L, 1)));
    return 1;
}

// bhaptics.resume(eventId)  -> bool
static int l_resume(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_resume)
    lua_pushboolean(L, g_resume(luaL_checkstring(L, 1)) ? 1 : 0);
    return 1;
}

// bhaptics.stop(requestId)  -> bool
static int l_stop(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_stop)
    lua_pushboolean(L, g_stop((int)luaL_checkinteger(L, 1)) ? 1 : 0);
    return 1;
}

// bhaptics.stop_by_event_id(eventId)  -> bool
static int l_stopByEventId(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_stopByEventId)
    lua_pushboolean(L, g_stopByEventId(luaL_checkstring(L, 1)) ? 1 : 0);
    return 1;
}

// bhaptics.stop_all()  -> bool
static int l_stopAll(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_stopAll)
    lua_pushboolean(L, g_stopAll() ? 1 : 0);
    return 1;
}

// bhaptics.is_bhaptics_connected(position)  -> bool
static int l_isbHapticsConnected(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_isbHapticsConnected)
    lua_pushboolean(L, g_isbHapticsConnected((int)luaL_checkinteger(L, 1)) ? 1 : 0);
    return 1;
}

// bhaptics.is_playing()  -> bool
static int l_isPlaying(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_isPlaying)
    lua_pushboolean(L, g_isPlaying() ? 1 : 0);
    return 1;
}

// bhaptics.is_playing_by_request_id(requestId)  -> bool
static int l_isPlayingByRequestId(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_isPlayingByRequestId)
    lua_pushboolean(L, g_isPlayingByRequestId((int)luaL_checkinteger(L, 1)) ? 1 : 0);
    return 1;
}

// bhaptics.is_playing_by_event_id(eventId)  -> bool
static int l_isPlayingByEventId(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_isPlayingByEventId)
    lua_pushboolean(L, g_isPlayingByEventId(luaL_checkstring(L, 1)) ? 1 : 0);
    return 1;
}

// bhaptics.get_haptic_message(appKey, workspaceId, lastVersion)  -> json_string, status_int
static int l_bHapticsGetHapticMessage(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_bHapticsGetHapticMessage)
    int status = 0;
    const char* result = g_bHapticsGetHapticMessage(
        luaL_checkstring(L, 1), luaL_checkstring(L, 2),
        (int)luaL_checkinteger(L, 3), status);
    lua_pushstring(L, result ? result : "");
    lua_pushinteger(L, status);
    return 2;
}

// bhaptics.get_haptic_mappings(appKey, workspaceId, lastVersion)  -> json_string, status_int
static int l_bHapticsGetHapticMappings(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_bHapticsGetHapticMappings)
    int status = 0;
    const char* result = g_bHapticsGetHapticMappings(
        luaL_checkstring(L, 1), luaL_checkstring(L, 2),
        (int)luaL_checkinteger(L, 3), status);
    lua_pushstring(L, result ? result : "");
    lua_pushinteger(L, status);
    return 2;
}

// bhaptics.is_player_running()  -> bool
static int l_isPlayerRunning(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_isPlayerRunning)
    lua_pushboolean(L, g_isPlayerRunning() ? 1 : 0);
    return 1;
}

// bhaptics.is_player_installed()  -> bool
static int l_isPlayerInstalled(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_isPlayerInstalled)
    lua_pushboolean(L, g_isPlayerInstalled() ? 1 : 0);
    return 1;
}

// bhaptics.launch_player(tryLaunch)  -> bool
static int l_launchPlayer(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_launchPlayer)
    luaL_checktype(L, 1, LUA_TBOOLEAN);
    lua_pushboolean(L, g_launchPlayer(lua_toboolean(L, 1) != 0) ? 1 : 0);
    return 1;
}

// bhaptics.get_device_info_json()  -> json_string
static int l_getDeviceInfoJson(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_getDeviceInfoJson)
    const char* r = g_getDeviceInfoJson();
    lua_pushstring(L, r ? r : "");
    return 1;
}

// bhaptics.get_haptic_mappings_json()  -> json_string
static int l_getHapticMappingsJson(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_getHapticMappingsJson)
    const char* r = g_getHapticMappingsJson();
    lua_pushstring(L, r ? r : "");
    return 1;
}

// bhaptics.ping(macAddress)  -> bool
static int l_ping(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_ping)
    lua_pushboolean(L, g_ping(luaL_checkstring(L, 1)) ? 1 : 0);
    return 1;
}

// bhaptics.ping_all()  -> bool
static int l_pingAll(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_pingAll)
    lua_pushboolean(L, g_pingAll() ? 1 : 0);
    return 1;
}

// bhaptics.swap_position(macAddress)  -> bool
static int l_swapPosition(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_swapPosition)
    lua_pushboolean(L, g_swapPosition(luaL_checkstring(L, 1)) ? 1 : 0);
    return 1;
}

// bhaptics.set_device_vsm(macAddress, vsm)  -> bool   (vsm: 0-400)
static int l_setDeviceVsm(lua_State* L)
{
    REQUIRE_LIB(L) REQUIRE_FN(L, g_setDeviceVsm)
    lua_pushboolean(L, g_setDeviceVsm(
        luaL_checkstring(L, 1), (int)luaL_checkinteger(L, 2)) ? 1 : 0);
    return 1;
}

// ---------------------------------------------------------------------------
// Module registration
// ---------------------------------------------------------------------------
static const luaL_Reg bhaptics_funcs[] = {
    // Initialisation
    { "registry_and_init",          l_registryAndInit          },
    { "registry_and_init_host",     l_registryAndInitHost      },
    { "ws_is_connected",            l_wsIsConnected            },
    { "ws_close",                   l_wsClose                  },
    { "re_init_message",            l_reInitMessage            },
    // Playback
    { "play",                       l_play                     },
    { "play_param",                 l_playParam                },
    { "play_without_result",        l_playWithoutResult        },
    { "play_with_start_time",       l_playWithStartTime        },
    { "play_dot",                   l_playDot                  },
    { "play_waveform",              l_playWaveform             },
    { "play_path",                  l_playPath                 },
    { "play_loop",                  l_playLoop                 },
    // Control
    { "get_event_time",             l_getEventTime             },
    { "pause",                      l_pause                    },
    { "resume",                     l_resume                   },
    { "stop",                       l_stop                     },
    { "stop_by_event_id",           l_stopByEventId            },
    { "stop_all",                   l_stopAll                  },
    // Status queries
    { "is_bhaptics_connected",      l_isbHapticsConnected      },
    { "is_playing",                 l_isPlaying                },
    { "is_playing_by_request_id",   l_isPlayingByRequestId     },
    { "is_playing_by_event_id",     l_isPlayingByEventId       },
    // Portal / metadata
    { "get_haptic_message",         l_bHapticsGetHapticMessage },
    { "get_haptic_mappings",        l_bHapticsGetHapticMappings},
    // Player management
    { "is_player_running",          l_isPlayerRunning          },
    { "is_player_installed",        l_isPlayerInstalled        },
    { "launch_player",              l_launchPlayer             },
    // Device info
    { "get_device_info_json",       l_getDeviceInfoJson        },
    { "get_haptic_mappings_json",   l_getHapticMappingsJson    },
    // Device control
    { "ping",                       l_ping                     },
    { "ping_all",                   l_pingAll                  },
    { "swap_position",              l_swapPosition             },
    { "set_device_vsm",             l_setDeviceVsm             },
    { nullptr, nullptr }
};

extern "C" __declspec(dllexport)
int luaopen_bhaptics_wrapper(lua_State* L)
{
    luaL_newlib(L, bhaptics_funcs);
    return 1;
}

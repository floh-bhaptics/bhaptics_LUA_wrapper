/*
 * bhaptics_wrapper.cpp
 *
 * A thin Lua 5.4 C extension (bhaptics_wrapper.dll) that bridges UE4SS Lua
 * scripts to bhaptics_library.dll via runtime dynamic loading.
 *
 * Neither BhapticsSDK2.h nor bhaptics_library.lib are needed to compile this.
 * All bhaptics functions are resolved at runtime with GetProcAddress, so the
 * only compile-time dependency is Lua 5.4.
 *
 * Place in the mod's /scripts/ folder at runtime:
 *   - bhaptics_wrapper.dll   (this project's output)
 *   - bhaptics_library.dll   (from the bhaptics SDK — see lib\bhaptics\)
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
// bhaptics_library.dll function pointer types
//
// These are the plain-C exports of bhaptics_library.dll (SDK2, Windows x64).
// If the DLL was built as C++ with mangled names you'll see load failures —
// run `dumpbin /EXPORTS bhaptics_library.dll` and update the name strings in
// the LoadProc calls below to match whatever dumpbin shows.
// ---------------------------------------------------------------------------
using fn_Initialize  = bool (*)(const char* appId, const char* apiKey);
using fn_Destroy     = void (*)();
using fn_Play        = int  (*)(const char* eventId);
using fn_PlayParam   = int  (*)(const char* eventId, float intensity,
                                float duration, float angleX, float offsetY);
using fn_IsConnected = bool (*)();
using fn_IsPlaying   = bool (*)(const char* eventId);
using fn_Stop        = void (*)(const char* eventId);

// ---------------------------------------------------------------------------
// Module-level state — loaded once, kept for the lifetime of the DLL
// ---------------------------------------------------------------------------
static HMODULE        g_hLib        = nullptr;
static fn_Initialize  g_Initialize  = nullptr;
static fn_Destroy     g_Destroy     = nullptr;
static fn_Play        g_Play        = nullptr;
static fn_PlayParam   g_PlayParam   = nullptr;
static fn_IsConnected g_IsConnected = nullptr;
static fn_IsPlaying   g_IsPlaying   = nullptr;
static fn_Stop        g_Stop        = nullptr;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// Resolve one export by name. Returns false (and sets g_hLib = nullptr on
// critical failure) so the caller can decide how to handle partial loads.
template<typename T>
static bool LoadProc(T& out, const char* name)
{
    out = reinterpret_cast<T>(GetProcAddress(g_hLib, name));
    return out != nullptr;
}

// Load bhaptics_library.dll and bind all function pointers.
// Returns an empty string on success, or an error message on failure.
static std::string LoadBhapticsLibrary()
{
    if (g_hLib)
        return "";   // already loaded

    g_hLib = LoadLibraryA("bhaptics_library.dll");
    if (!g_hLib) {
        DWORD err = GetLastError();
        char buf[256];
        snprintf(buf, sizeof(buf),
            "LoadLibrary(\"bhaptics_library.dll\") failed, Windows error %lu. "
            "Make sure bhaptics_library.dll is in the same scripts/ folder as "
            "bhaptics_wrapper.dll.", err);
        return std::string(buf);
    }

    // Bind each export. We treat missing symbols as a hard error so the Lua
    // side sees a clear message rather than a crash later.
    const char* missing = nullptr;
    if (!LoadProc(g_Initialize,  "Initialize"))  missing = "Initialize";
    if (!LoadProc(g_Destroy,     "Destroy"))     missing = missing ? missing : "Destroy";
    if (!LoadProc(g_Play,        "Play"))         missing = missing ? missing : "Play";
    if (!LoadProc(g_PlayParam,   "PlayParam"))   missing = missing ? missing : "PlayParam";
    if (!LoadProc(g_IsConnected, "IsConnected")) missing = missing ? missing : "IsConnected";
    if (!LoadProc(g_IsPlaying,   "IsPlaying"))   missing = missing ? missing : "IsPlaying";
    if (!LoadProc(g_Stop,        "Stop"))        missing = missing ? missing : "Stop";

    if (missing) {
        FreeLibrary(g_hLib);
        g_hLib = nullptr;
        char buf[512];
        snprintf(buf, sizeof(buf),
            "bhaptics_library.dll was loaded but the export \"%s\" was not found. "
            "Run:  dumpbin /EXPORTS bhaptics_library.dll  and check that the "
            "function names match those expected by bhaptics_wrapper.cpp.", missing);
        return std::string(buf);
    }

    return "";
}

// ---------------------------------------------------------------------------
// Lua-callable API
// ---------------------------------------------------------------------------

// bhaptics.initialize(appId, apiKey)  ->  true | false, errmsg
static int l_initialize(lua_State* L)
{
    const char* appId  = luaL_checkstring(L, 1);
    const char* apiKey = luaL_checkstring(L, 2);

    std::string err = LoadBhapticsLibrary();
    if (!err.empty()) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, err.c_str());
        return 2;
    }

    bool ok = g_Initialize(appId, apiKey);
    lua_pushboolean(L, ok ? 1 : 0);
    if (!ok)
        lua_pushstring(L, "bhaptics Initialize() returned false — is bHaptics Player running?");
    return ok ? 1 : 2;
}

// bhaptics.destroy()
static int l_destroy(lua_State* L)
{
    if (g_Destroy)
        g_Destroy();
    return 0;
}

// bhaptics.play(eventId)  ->  requestId  (-1 on error)
static int l_play(lua_State* L)
{
    const char* eventId = luaL_checkstring(L, 1);
    lua_pushinteger(L, g_Play ? g_Play(eventId) : -1);
    return 1;
}

// bhaptics.play_param(eventId [, intensity [, duration [, angleX [, offsetY]]]])
//   intensity : 0.0 – 1.0+  (default 1.0, multiplier on the pattern's own intensity)
//   duration  : 0.0 – 1.0+  (default 1.0, multiplier on the pattern's own duration)
//   angleX    : 0.0 – 360.0 (default 0.0, rotates the pattern around the vertical axis)
//   offsetY   : -0.5 – 0.5  (default 0.0, shifts the pattern up or down)
static int l_play_param(lua_State* L)
{
    const char* eventId = luaL_checkstring(L, 1);
    float intensity     = (float)luaL_optnumber(L, 2, 1.0);
    float duration      = (float)luaL_optnumber(L, 3, 1.0);
    float angleX        = (float)luaL_optnumber(L, 4, 0.0);
    float offsetY       = (float)luaL_optnumber(L, 5, 0.0);
    lua_pushinteger(L, g_PlayParam ? g_PlayParam(eventId, intensity, duration, angleX, offsetY) : -1);
    return 1;
}

// bhaptics.is_connected()  ->  bool
static int l_is_connected(lua_State* L)
{
    lua_pushboolean(L, (g_IsConnected && g_IsConnected()) ? 1 : 0);
    return 1;
}

// bhaptics.is_playing(eventId)  ->  bool
static int l_is_playing(lua_State* L)
{
    const char* eventId = luaL_checkstring(L, 1);
    lua_pushboolean(L, (g_IsPlaying && g_IsPlaying(eventId)) ? 1 : 0);
    return 1;
}

// bhaptics.stop(eventId)
static int l_stop(lua_State* L)
{
    const char* eventId = luaL_checkstring(L, 1);
    if (g_Stop)
        g_Stop(eventId);
    return 0;
}

// ---------------------------------------------------------------------------
// Module registration  (Lua 5.4 luaL_newlib style)
// ---------------------------------------------------------------------------
static const luaL_Reg bhaptics_funcs[] = {
    { "initialize",   l_initialize   },
    { "destroy",      l_destroy      },
    { "play",         l_play         },
    { "play_param",   l_play_param   },
    { "is_connected", l_is_connected },
    { "is_playing",   l_is_playing   },
    { "stop",         l_stop         },
    { nullptr, nullptr }
};

extern "C" __declspec(dllexport)
int luaopen_bhaptics_wrapper(lua_State* L)
{
    luaL_newlib(L, bhaptics_funcs);
    return 1;
}

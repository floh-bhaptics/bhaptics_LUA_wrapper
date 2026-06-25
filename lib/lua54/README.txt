# lib/lua54/

Place the Lua 5.4 development files here. You need two things:

## include/
The Lua 5.4 C headers:
  lua.h, luaconf.h, lualib.h, lauxlib.h

## lib/
The Lua 5.4 import library for MSVC:
  lua54.lib

---

## Where to get them

Download the **Windows x64 binaries** from https://luabinaries.sourceforge.net

Pick the release matching UE4SS's Lua version. At time of writing UE4SS ships
Lua 5.4.4, so look for a package like:
  lua-5.4.4_Win64_dllw6_lib.zip

Inside that zip you will find:
  lua54.dll   ← runtime (NOT needed to compile the wrapper)
  lua54.lib   ← import library  → copy to lib/lua54/lib/
  include/    ← headers         → copy contents to lib/lua54/include/

You do NOT need lua54.dll in this folder; it is already embedded inside UE4SS.

# bhaptics_wrapper

A thin Lua 5.4 C extension DLL that exposes the full `bhaptics_library.dll` API to UE4SS Lua mods. It loads `bhaptics_library.dll` at runtime via `LoadLibrary`, so no bhaptics headers or import library are needed to compile it.

## Setup

Before building, populate the two lib folders:

**`lib/lua54/`** — Lua 5.4 development files.
Download a Windows x64 package from https://luabinaries.sourceforge.net (e.g. `lua-5.4.4_Win64_dllw6_lib.zip`) and copy:
- `include/*.h` → `lib/lua54/include/`
- `lua54.lib` → `lib/lua54/lib/`

**`lib/bhaptics/`** — Drop your copy of `bhaptics_library.dll` here for safekeeping. It is not needed to compile, only at runtime.

## Build

1. Open `bhaptics_wrapper.sln` in Visual Studio 2019 or 2022.
2. Confirm the configuration is **Release | x64**.
3. Press **Ctrl+Shift+B**.

Output: `out/bhaptics_wrapper.dll`

## Deploy

Copy both DLLs into your UE4SS mod's `scripts/` folder:
- `out/bhaptics_wrapper.dll`
- `lib/bhaptics/bhaptics_library.dll`

Tip: change `<OutDir>` in `bhaptics_wrapper.vcxproj` to point directly at your mod's `scripts/` folder to skip the manual copy step after each build.

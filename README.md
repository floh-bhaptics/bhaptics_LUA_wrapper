# bhaptics_wrapper — Visual Studio Project

Builds `bhaptics_wrapper.dll`: a thin Lua 5.4 C extension that lets UE4SS Lua
mods call into `bhaptics_library.dll` without needing any bhaptics headers or
import libraries at compile time.

---

## Folder layout

```
bhaptics_wrapper/
  bhaptics_wrapper.sln          ← open this in Visual Studio
  bhaptics_wrapper.vcxproj
  src/
    bhaptics_wrapper.cpp        ← the only source file
  lib/
    lua54/
      README.txt                ← instructions for getting Lua headers + lib
      include/                  ← put lua.h, luaconf.h, lualib.h, lauxlib.h here
      lib/                      ← put lua54.lib here
    bhaptics/
      README.txt                ← instructions for the bhaptics DLL
                                   put bhaptics_library.dll here for safekeeping
  out/
    bhaptics_wrapper.dll        ← build output lands here automatically
```

---

## Setup (one-time)

### 1. Get the Lua 5.4 development files

Download the Windows x64 binaries from https://luabinaries.sourceforge.net
Look for a package such as `lua-5.4.4_Win64_dllw6_lib.zip`.

From the zip, copy:
- `include/*.h`  →  `lib/lua54/include/`
- `lua54.lib`    →  `lib/lua54/lib/`

You do NOT need `lua54.dll` — UE4SS already embeds Lua internally.

### 2. Drop in bhaptics_library.dll

Put a copy of `bhaptics_library.dll` (from your bhaptics SDK package) into
`lib/bhaptics/` for reference. You do not need it to compile.

---

## Build

1. Open `bhaptics_wrapper.sln` in Visual Studio 2019 or 2022.
2. Make sure the configuration is **Release | x64** (it's the only one defined).
3. Press **Ctrl+Shift+B** (Build Solution).
4. The output `bhaptics_wrapper.dll` will appear in `out/`.

---

## Deploy

Copy both DLLs into your mod's `scripts/` folder:

```
<Game>\Binaries\Win64\ue4ss\Mods\BehemothHaptics\scripts\
    main.lua
    bhaptics_wrapper.dll    ← from out/ after building
    bhaptics_library.dll    ← from lib/bhaptics/ (or wherever bhaptics gave it to you)
```

To skip the manual copy step, edit `bhaptics_wrapper.vcxproj` and change the
`<OutDir>` line to point directly at your mod's `scripts/` folder:

```xml
<OutDir>C:\path\to\Behemoth\Binaries\Win64\ue4ss\Mods\BehemothHaptics\scripts\</OutDir>
```

---

## Troubleshooting

| Error in UE4SS console | Cause | Fix |
|---|---|---|
| `Could not load bhaptics_wrapper.dll` | DLL not in `scripts/` or wrong Lua version | Rebuild against correct Lua version; confirm placement |
| `LoadLibrary("bhaptics_library.dll") failed` | bhaptics_library.dll missing from `scripts/` | Copy it next to bhaptics_wrapper.dll |
| `export "Initialize" was not found` | DLL has C++-mangled names | Run `dumpbin /EXPORTS bhaptics_library.dll`, update symbol names in the .cpp |
| `Initialize() returned false` | bHaptics Player not running | Start the bHaptics Player app before launching the game |
| Pattern plays but no vibration | Wrong event name | Event name must exactly match what's in your bHaptics Developer Portal app |

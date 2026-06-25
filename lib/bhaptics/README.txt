# lib/bhaptics/

Place the bhaptics SDK files here for reference / safekeeping.

## What goes here

  bhaptics_library.dll   ← the bhaptics runtime DLL (required at game runtime)

Optionally, if bhaptics supplied them with your SDK package:
  bhaptics_library.lib   ← import library  (NOT needed — we use LoadLibrary)
  BhapticsSDK2.h         ← C++ header      (NOT needed — we use GetProcAddress)

---

## Important

bhaptics_library.dll does NOT live here at runtime.
At runtime it must sit next to bhaptics_wrapper.dll, i.e.:

  <Game>\Binaries\Win64\ue4ss\Mods\BehemothHaptics\scripts\
      bhaptics_wrapper.dll   ← compiled output of this project
      bhaptics_library.dll   ← copy here from lib/bhaptics/

Keep a copy here in lib/bhaptics/ so the source tree is self-contained and
you always know which version you built against.

---

## Checking export names

If bhaptics.initialize() fails with "export not found", run this in a
Visual Studio Developer Command Prompt to see all exported symbols:

  dumpbin /EXPORTS bhaptics_library.dll

The wrapper expects these plain-C names:
  Initialize, Destroy, Play, PlayParam, IsConnected, IsPlaying, Stop

If the DLL was built as C++ the names will be mangled (e.g. ?Initialize@@...).
Update the matching strings in src/bhaptics_wrapper.cpp if so.

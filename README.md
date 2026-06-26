# bhaptics_wrapper — Lua wrapper for the bHaptics SDK

A thin Lua 5.4 extension DLL that exposes the full [bHaptics](https://www.bhaptics.com) SDK to UE4SS Lua mods. It wraps `bhaptics_library.dll` at runtime, so you can trigger haptic effects directly from your Lua scripts without writing any C++.

---

## Setup

1. Download the latest release and extract it. You will find two DLL files:
   - `bhaptics_wrapper.dll`
   - `bhaptics_library.dll`

2. Place **both** DLLs in the same `scripts/` folder as your `main.lua`.

That's it. No additional dependencies, no configuration files.

---

## Quick start

The following is a complete minimal mod that connects to the bHaptics Player and plays a heartbeat pattern on startup.

```lua
local APP_ID  = "your_workspace_id_here"
local API_KEY = "your_api_key_here"

-- Load the wrapper
local ok, bhaptics = pcall(require, "bhaptics_wrapper")
if not ok then
    print("[MyMod] Failed to load bhaptics_wrapper.dll: " .. tostring(bhaptics))
    return
end

-- Connect to the bHaptics Player
local connected, err = bhaptics.registry_and_init(API_KEY, APP_ID, "{}")
if not connected then
    print("[MyMod] Could not connect to bHaptics Player: " .. tostring(err))
    return
end

print("[MyMod] Connected to bHaptics Player!")

-- Play a pattern
bhaptics.play("heartbeat")
```

Your App ID and API key are found in the [bHaptics Developer Portal](https://developer.bhaptics.com) under your app's workspace settings.

> **Note:** Pattern names are automatically converted to lowercase by the wrapper, so `"HeartBeat"`, `"heartbeat"`, and `"HEARTBEAT"` all refer to the same pattern. You can use whichever spelling you prefer in your code.

---

## Function reference

### Initialisation

---

#### `bhaptics.registry_and_init(apiKey, workspaceId, initJson)` → `bool [, errmsg]`

Connects to the local bHaptics Player and registers your app. Must be called before any playback functions.

- `apiKey` — your workspace API key from the Developer Portal
- `workspaceId` — your workspace / App ID from the Developer Portal
- `initJson` — initial haptic message as a JSON string; pass `"{}"` if you have nothing to send

Returns `true` on success. Returns `false, errmsg` on failure.

---

#### `bhaptics.registry_and_init_host(apiKey, workspaceId, initJson, url)` → `bool`

Same as `registry_and_init`, but connects to a bHaptics Player running on a remote machine at `url` instead of localhost.

---

#### `bhaptics.ws_is_connected()` → `bool`

Returns `true` if the WebSocket connection to the bHaptics Player is currently open.

---

#### `bhaptics.ws_close()`

Closes the WebSocket connection. Call this on mod unload or game exit if you want a clean shutdown.

---

#### `bhaptics.re_init_message(apiKey, workspaceId, initJson)` → `bool`

Re-registers the app with the Player over an existing connection. Useful if the connection dropped and was re-established.

---

### Playback

---

#### `bhaptics.play(eventName)` → `requestId`

Plays a haptic pattern by name. The name must match an event defined in your bHaptics Developer Portal workspace.

Returns a `requestId` integer you can use to check status or stop the effect. Returns `-1` on failure.

```lua
local id = bhaptics.play("explosion")
```

---

#### `bhaptics.play_param(eventName [, requestId, intensity, duration, angleX, offsetY])` → `requestId`

Plays a pattern with additional parameters.

- `requestId` — pass `0` to let the SDK assign one automatically
- `intensity` — multiplier on the pattern's own intensity (default `1.0`)
- `duration` — multiplier on the pattern's own duration (default `1.0`)
- `angleX` — rotates the pattern around the vertical axis in degrees, `0`–`360` (default `0.0`). Useful for directional effects such as mapping where an attack came from.
- `offsetY` — shifts the pattern up or down, `-0.5`–`0.5` (default `0.0`)

```lua
-- Play "impact" at half intensity, rotated 90 degrees to the right
bhaptics.play_param("impact", 0, 0.5, 1.0, 90.0, 0.0)
```

---

#### `bhaptics.play_without_result(eventName [, requestId, intensity, duration, angleX, offsetY])`

Same as `play_param` but does not return a request ID. Slightly more efficient when you don't need to track the effect.

---

#### `bhaptics.play_with_start_time(eventName [, requestId, startMillis, intensity, duration, angleX, offsetY])` → `requestId`

Plays a pattern starting partway through. `startMillis` sets the offset into the pattern in milliseconds.

---

#### `bhaptics.play_loop(eventName [, requestId, intensity, duration, angleX, offsetY, interval, maxCount])` → `requestId`

Plays a pattern in a loop.

- `interval` — gap between repetitions in milliseconds (default `0`)
- `maxCount` — number of times to repeat; `0` means loop indefinitely (default `0`)

---

#### `bhaptics.play_dot(requestId, position, duration, motorValues)` → `requestId`

Plays a raw dot-mode pattern directly on individual motors.

- `position` — bHaptics device position index
- `duration` — duration in milliseconds
- `motorValues` — a Lua array of up to 32 integers, each `0`–`100`, representing per-motor intensity

```lua
bhaptics.play_dot(0, 1, 500, {100, 0, 50, 0, 100, 0, 50, 0})
```

---

#### `bhaptics.play_waveform(requestId, position, motorValues, playTimes, shapes, repeatCount)` → `requestId`

Plays a waveform pattern on up to 6 motors.

- `motorValues` — array of 6 integers `0`–`100`
- `playTimes` — array of 6 integers representing duration steps (1 = 5 ms, 2 = 10 ms, 4 = 20 ms, 6 = 30 ms, 8 = 40 ms)
- `shapes` — array of 6 integers: `0` = sustain, `1` = 50% linear fade out, `2` = 50% linear fade in

---

#### `bhaptics.play_path(requestId, position, durationMillis, xValues, yValues, intensities)` → `requestId`

Plays a path-mode pattern, tracing a line across the vest.

- `xValues`, `yValues` — arrays of up to 32 floats defining the path coordinates
- `intensities` — array of up to 32 integers `0`–`100`

---

### Playback control

---

#### `bhaptics.get_event_time(eventName)` → `milliseconds`

Returns the total duration of a pattern in milliseconds.

---

#### `bhaptics.pause(eventName)` → `milliseconds`

Pauses a currently playing pattern. Returns the playback position in milliseconds at the point it was paused.

---

#### `bhaptics.resume(eventName)` → `bool`

Resumes a paused pattern. Returns `true` on success.

---

#### `bhaptics.stop(requestId)` → `bool`

Stops a playing effect by its request ID. Returns `true` on success.

---

#### `bhaptics.stop_by_event_id(eventName)` → `bool`

Stops a playing effect by its pattern name. Returns `true` on success.

---

#### `bhaptics.stop_all()` → `bool`

Stops all currently playing effects. Returns `true` on success.

---

### Status queries

---

#### `bhaptics.is_playing()` → `bool`

Returns `true` if any effect is currently playing.

---

#### `bhaptics.is_playing_by_request_id(requestId)` → `bool`

Returns `true` if the effect with the given request ID is currently playing.

---

#### `bhaptics.is_playing_by_event_id(eventName)` → `bool`

Returns `true` if the named pattern is currently playing.

---

#### `bhaptics.is_bhaptics_connected(position)` → `bool`

Returns `true` if a bHaptics device at the given position index is connected.

---

### Player management

---

#### `bhaptics.is_player_running()` → `bool`

Returns `true` if the bHaptics Player application is currently running.

---

#### `bhaptics.is_player_installed()` → `bool`

Returns `true` if the bHaptics Player is installed on this machine.

---

#### `bhaptics.launch_player(tryLaunch)` → `bool`

Attempts to launch the bHaptics Player. Pass `true` to actually launch it. Returns `true` if the Player started successfully.

---

### Device information

---

#### `bhaptics.get_device_info_json()` → `string`

Returns a JSON string describing all currently connected devices.

---

#### `bhaptics.get_haptic_mappings_json()` → `string`

Returns a JSON string listing all haptic event mappings registered for your app.

---

#### `bhaptics.get_haptic_message(appKey, workspaceId, lastVersion)` → `string, statusCode`

Fetches haptic message data from the bHaptics portal. Returns the data as a JSON string and an integer status code.

---

#### `bhaptics.get_haptic_mappings(appKey, workspaceId, lastVersion)` → `string, statusCode`

Fetches haptic mapping data from the bHaptics portal. Returns the data as a JSON string and an integer status code.

---

### Device control

---

#### `bhaptics.ping(macAddress)` → `bool`

Sends a ping to a specific connected device by its MAC address. The device will briefly vibrate all motors so you can identify it.

---

#### `bhaptics.ping_all()` → `bool`

Pings all connected devices.

---

#### `bhaptics.swap_position(macAddress)` → `bool`

Swaps the left/right designation of a device that supports it (e.g. arm or glove trackers).

---

#### `bhaptics.set_device_vsm(macAddress, vsm)` → `bool`

Sets the VSM (vibration strength multiplier) for a specific device. Range is `0`–`400`.

## Build information

### Setup

Before building, populate the two lib folders:

**`lib/lua54/`** — Lua 5.4 development files.
Download a Windows x64 package from https://luabinaries.sourceforge.net (e.g. `lua-5.4.4_Win64_dllw6_lib.zip`) and copy:
- `include/*.h` → `lib/lua54/include/`
- `lua54.lib` → `lib/lua54/lib/`

**`lib/bhaptics/`** — Drop your copy of `bhaptics_library.dll` here for safekeeping. It is not needed to compile, only at runtime.

### Build

1. Open `bhaptics_wrapper.sln` in Visual Studio 2019 or 2022.
2. Confirm the configuration is **Release | x64**.
3. Press **Ctrl+Shift+B**.

Output: `out/bhaptics_wrapper.dll`

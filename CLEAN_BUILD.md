# Pandora Lite — Custom Edition

Windows x64 standalone executable, static C++ runtime. Open Roblox, join a game,
and launch `pandora_lite.exe`. INSERT toggles the menu; END exits.

## Features
- Hold-to-aim mouse assist: head/root, six hold keys, screen-center FOV, time-based
  smoothing, mouse gain, sticky target, FOV circle and target line.
- ESP: full/corner/rounded boxes, translucent fills, names, health, distances,
  tracers from top/center/bottom; teammate and maximum-range filters.
- North-up X/Z radar: character-centered, configurable size, range and position.
- Crosshair, FPS watermark, independent color pickers, four theme presets,
  live ESP preview, animated collapsible navigation, five profile slots.
- Automatic reconnect, client-version guard, settings validation and persistence.

Aim assist starts disabled. Enable it, close the menu and hold the chosen key
while Roblox is focused. Mouse gain must be tuned to game sensitivity. It does
not test line of sight. Distance units are Roblox studs. Range filtering needs
an available local character. Custom rigs may need a separate adapter.

Settings: `%LOCALAPPDATA%\PandoraLite\settings.ini`; profiles `profile1.ini` through
`profile5.ini`. Loading a profile leaves aim assist disabled. A missing profile
retains the current values. No script executor, movement modifications, silent
aim, anti-cheat bypass or driver is included.

## Interface attribution
Michael Conors' 1011 custom navigation and embedded font arrays, supplied via
KingsleydotDev/ImGui-Menus and the user's attached archive, are adapted to modern
Dear ImGui public APIs. Dark palette and collapsible icon navigation come from
that design; the feature panels are original. The renderer remains DirectX 11.
Dear ImGui is MIT licensed; upstream license is included below.

## Build
CMake 3.24+, Visual Studio 2022 C++ desktop tools, Windows SDK, internet for Dear
ImGui v1.91.9b. GitHub Actions compiles and smoke-tests the executable automatically.

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel
```

Offsets are specific to `version-02c37bc51a384b8f`. In-game validation of this
edition's new features remains necessary; the previous ESP build was confirmed
working by the user. CI checks compilation and startup, not Roblox behavior.

## Dear ImGui license

```text
The MIT License (MIT)

Copyright (c) 2014-2022 Omar Cornut

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

```

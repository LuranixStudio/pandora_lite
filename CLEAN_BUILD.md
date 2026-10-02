# Pandora Lite x64

CMake builds the new root-level `pandora_app.cpp` and `pandora_reader.hpp`. The original 2023 source remains in the repository as an archive; its injection and x86 backend are not compiled.

- Dear ImGui v1.91.9b, DirectX 11, static MSVC runtime.
- Read-only process access, 64-bit pointers, bounded reads, reconnect and client-version check.
- Universal player boxes, display names, health bars, team filter and persistent visual settings.
- Insert toggles the menu; End exits. The overlay follows the Roblox client rectangle and is click-through when the menu is closed.
- Offsets come from the user-supplied dump for `version-02c37bc51a384b8f` only. Unknown/zero dump fields are not used.
- No claim of in-game validation. Memory layouts, projection and name indirections must be verified against a running copy of that exact client. Custom character systems (including games advertised by the original README) are not validated.

Build: `cmake -S . -B build -A x64`, then `cmake --build build --config Release`.
Output: `build/Release/pandora_lite.exe`. No compiler or SDK is needed to run the built executable. DirectX 11 / Windows 10 or newer is required.

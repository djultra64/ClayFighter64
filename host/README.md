# Native host

ClayFighter 64 integration by `djultra64`. Previously named ClayTality 64. RT64, ultramodern, librecomp and their dependencies retain their authentic authors and licenses in the configured read-only SDK.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
./Jugar.sh
ctest --test-dir build --output-on-failure
```

Override the SDK with `-DCLAY_SDK_ROOT=/path/to/sdk`. Build products and generated runtime repairs stay in this project. Windows uses its separate project in `tools/windows`.

The normal launcher verifies both locally provided ROM archives, prepares private donor assets, and runs the original 63⅓ CPU/audio code with sixteen native selector slots. The additional right column contains High Five, Lady Liberty, Lockjaw and Zappa. `./Jugar.sh --original` selects the original twelve unlocked characters. The donor definitions and animation layouts are adapted only inside verified donor ranges. Native portraits are extended in both fresh and cached banks. Selector geometry preserves the game's mesh, colors and textures.

First launch defaults to 16:9. Native Options → Display controls aspect, resolution, filtering, target refresh, fullscreen and MSAA; the four original display settings follow. Preferences are saved under SDL's user directory for `djultra64/ClayTality64`; `--user-data PATH` isolates hidden tests. Target120 is clamped by RT64 to the available display refresh; simulation stays at60Hz. Fighter interpolation tags are bounded by the native display-list/vertex pools and cut at changed bitmap poses and teleports.

SDL input separates logical actions from physical positions. The first two controllers keep independent P1/P2 assignments and keyboard input stays on P1. All mapped controllers are opened so the last active device can drive prompts, with Auto/Xbox/PlayStation/Switch manual fallback. Nintendo's swapped labels are handled explicitly. The native game font renders keyboard prompts, while Kenney Input Prompts Pixel (CC0) supplies controller glyphs. Options → Button Remap affects combat only; menus and pause use fixed default bindings. Real controller verification is pending when hardware is available.

Hidden tests use `--hidden --test-input PATH --seconds N`; `tools/headless_test.py` uses a private Xvfb display and software Vulkan for visual inspection. Hardware validation runs on a hidden native window and records actual output dimensions, target/display refresh and real SDL input. Reports distinguish requested refresh from actual interpolation.

The original game has no cartridge or Controller Pak save requirement, as confirmed by its exact ROM identity in the Mupen64Plus cartridge catalog and absence of EEPROM/PFS guest calls. Unsupported RSP, privileged instructions and raw RDP submissions stop with explicit diagnostics. Host display and input preferences are persisted separately.

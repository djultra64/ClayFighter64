# ClayFighter 64 2.0 source and builds

Original port work by **djultra64**. The v2.0 source distribution includes the host source,
ROM extraction/generation scripts, configuration, tests, and the matching SDK
sources. Generated guest CPU/RSP translations and extracted game data are
produced locally from your own ROMs and are excluded from the public source.

## Source archives

The GitHub source contains the port. The separate `clayfighter64-2.0-sdk-source.tar.gz`
release asset contains the exact prepared N64ModernRuntime, N64Recomp, RT64 and
spimdisasm source trees, including their nested dependencies and authentic notices.
It also contains the SDK build records, SDL2-compat/SDL3/dbus source distributions,
and their installed Homebrew build recipes. Windows SDL2 2.26.3 has its own
official source archive, alongside the separate Linux SDL2-compat sources.
See `config/dependencies-2.0.json` for source URLs and verified hashes.
The SDK is read-only during the host build; generated repairs stay in this project.
The SDK archive also retains the full dependency notice inventory under
`sdk/licenses/`, including notices for its prebuilt DXC inputs.

## Private inputs and generation

Supply both exact USA ROMs described in INSTALL.md. Normalize them locally to
`probe-output/base.z64` and `probe-output/sc.z64`. Build N64Recomp and RSPRecomp
from the included SDK source using their upstream CMake instructions, and install
spimdisasm plus its Rabbitizer Python dependency. Then:

```sh
python3 tools/generate_cpu.py --sdk /path/to/sdk
python3 tools/generate_audio.py --rsp-recomp /path/to/RSPRecomp
python3 tools/release/prepare_decoder.py
```

The SDK layout must contain `deps/N64Recomp`, `deps/N64ModernRuntime`,
`deps/rt64` and `deps/spimdisasm`. `analysis/sdk_matches.json` is the function
binding map used by generation. Hook configurations describe the host integrations.

Compile the font extraction fixture with the same N64Recomp include revision:

```sh
c++ -O2 -I/path/to/sdk/deps/N64ModernRuntime/N64Recomp/include \
  -Igenerated/guest generated/release-decoder/guest.cpp \
  tools/release/font_decoder.cpp -o generated/release-decoder/font-decoder
```

On Windows, use MSVC x64 `/O2 /EHsc /std:c++20` and output `font-decoder.exe`.
The decoder executes ROM-generated codec code; its generated source is deliberately
not redistributed as independent authored port code.

## Linux host

Build RT64 as a static library with `RT64_STATIC=ON` and
`RT64_SDL_WINDOW_VULKAN=ON`; put its output at `sdk/rt64-build`.
The SDK preserves the original pinned RT64 source. `RT64-SOURCE-PATCH.json`
describes the CMake DXC invocation repair for paths containing spaces; its
modified hash is not the actual SDK CMakeLists hash. Apply the repair in a
build-local copy when required. The dependency record states this distinction.

The current Linux CMake recipe uses the project's existing Homebrew SDL include
and library paths at `/home/linuxbrew/.linuxbrew`, alongside system GCC/Clang,
CMake, Ninja, Vulkan drivers and desktop libraries. Adapt those include/library
paths to your local SDL installation if rebuilding outside that environment.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCLAY_SDK_ROOT=/path/to/sdk
cmake --build build --target claytality64 -j4
```

Targeted native fixture tests additionally need the locally generated probe
inputs used by their preparation scripts. Run the applicable `tools/verify_*.py`
scripts and `ctest --test-dir build --output-on-failure` with your local ROM data.
The published binaries derive from the recorded final builds.

## Windows host

Use Visual Studio 2022 Build Tools, MSVC x64, and matching pinned RT64/runtime
sources. Generate the same private CPU/RSP inputs on Linux, then prepare the
queue and graphics observer:

```sh
python3 tools/prepare_threadqueue_trace.py /path/to/sdk/deps/N64ModernRuntime generated/threadqueue_trace.cpp
python3 tools/graphics_interpolation_observer.py /path/to/sdk/deps/rt64 generated/graphics_workload_queue.cpp
```

Use `tools/windows/CMakeLists.txt` and supply `CLAY_RUNTIME`, `CLAY_RT64`,
`CLAY_RT64_BUILD` and `CLAY_SDL`. Build RT64 and its dependencies with the same
MSVC release ABI first. See [the Windows notes](../tools/windows/README.md)
for fixture preparation. Windows retains the original lifecycle wrappers while
renaming the two translated OS start/stop functions in a build-local copy.

## Packaging

`tools/release/package.py --version 2.0` stages versioned binary and authored
source packages. `tools/release/sdk_source.py --version 2.0` stages the matching
prepared dependency sources. See [PACKAGING.md](PACKAGING.md).
It consumes the latest local final binaries, the generated font decoder, the
user-supplied title announcement and CPython 3.13.9 embeddable runtime, and
copies applicable notices. Its artifact paths describe the release environment;
adapt those inputs when creating your own packages. It deliberately never copies
ROMs, extracted portraits, or biographies. Those are generated by the two-ROM
installer on the player's machine.

Upstream licenses remain in force. GPL-covered corresponding host/runtime sources,
regeneration scripts, and pinned dependency source are distributed with the release;
original cartridge material remains the player's private prerequisite.

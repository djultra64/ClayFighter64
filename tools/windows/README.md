ClayFighter 64 Windows native integration by djultra64.

Configure this directory with Visual Studio 2022 Build Tools, x64, and matching pinned RT64/N64ModernRuntime sources. The RT64 static libraries must come from the same source revision and MSVC runtime ABI. Pass `CLAY_RUNTIME`, `CLAY_RT64`, `CLAY_RT64_BUILD` and `CLAY_SDL` paths. This separate project leaves the Linux build untouched.

Prepare the generated guest/audio code and the repaired SDK queue/interpolation observer using the existing Linux generation tools, then run `python3 tools/windows/prepare_wait.py` and `python3 tools/windows/prepare_contracts.py` before staging. The Windows build copies and renames only the original OS Start/Stop translations in its build directory; the real lifecycle wrappers remain in host/runtime.cpp. Third-party sources are read-only.

For Training preparation, also run `python3 tools/windows/prepare_training.py` after
regenerating the current guest code. Transfer `generated/windows_training_guest.cpp`
together with the current `host/`, `tests/`, `tools/windows/` and normal private
fixtures. `build.ps1` rejects a missing Training guest fixture. This is private build
input and never belongs in a public binary or authored-source archive. Run
`test.ps1 -Version 2.0` only against the matching build; the result records the
requested version and actual test outcomes. Preparation does not validate Training.

All preferences and RT64 data belong under the installation's `local/` directory.
Use `--data-dir local/windows-tests/NAME` for isolated game profiles. Moving the
installation retains preferences; no AppData directory is used.

`build.ps1` builds the executable and hidden fixtures. `test.ps1` runs the native CTests and requires the actual original framebuffer caller (with only its known cooperative yield removed) to time out. `boot.ps1` runs a bounded hidden hardware boot; `--test-input` uses the project's real SDL virtual-controller driver. No scripts alter display settings or machine power. A profile requesting 120 FPS does not establish actual 120 FPS; inspect `graphics_effective` and interpolation evidence.

The generated `generated/windows_wait_guest.cpp` is an extracted fixture, not hand-authored replacement guest behavior. Regenerate it whenever the source poll/VI actor changes.

The final Windows suite contains 50 native CTests, including original/SC dispatch equality, actual sprite writers, two-controller routing and the two-virtual-pad script driver. `boot.ps1` captures the real retained-process ExitCode; do not infer success from a PowerShell script exit or a null Start-Process ExitCode. Release packaging must include config/controls.cfg, config/controller_glyphs.json and the licensed Kenney Tiles directory alongside the matching DLLs. Public packages must exclude ROMs and extracted game images; the two-ROM importer creates those locally. `Play ClayFighter 64.cmd` changes into its own directory and leaves the default profile/preferences intact.

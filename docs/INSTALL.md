# ClayFighter 64 1.0 installation

Project work by **djultra64**.

## Both USA ROMs are required

ClayFighter 64 combines the 63⅓ gameplay foundation with the four additional Sculptor's Cut characters. Installation therefore requires **both different games**, not two copies of one game. The installer does not obtain ROMs for you.

| Input | SHA-256 after normalization to big-endian z64 |
| --- | --- |
| ClayFighter 63⅓ (USA) | `357f26c6cd72559c5c05e8c2df46369624630dbcb1c02b3b6f3338f6ac79af07` |
| ClayFighter: Sculptor's Cut (USA) | `9ccebc317b7ee892be3bdc0d1c8ae46687995908ed51ebd58d6855e5e25abd7f` |

Other regions, revisions, translations, and modified ROMs are unsupported. `.z64`, byte-swapped `.v64`, word-swapped `.n64`, and ZIP archives containing exactly one of those files are supported. Paths may contain spaces. ROM identity is checked before any persistent game data is written.

## Linux

Extract `clayfighter64-1.0-linux-x86_64.tar.gz` and open a terminal in its folder:

```sh
./'Install Assets.sh'
./'Play ClayFighter 64.sh'
```

You can provide both paths directly, in either order:

```sh
./'Install Assets.sh' '/path/to/63-third.zip' '/path/to/sculptors-cut.z64'
```

Linux requires Python 3.10+, X11 or XWayland, Vulkan drivers, glibc 2.38+, libstdc++ providing GLIBCXX_3.4.31, libgcc, and the standard desktop/audio libraries used by SDL. SDL2-compat, SDL3, dbus and DXC are bundled; the launch script selects the local libraries.

## Windows

Extract `clayfighter64-1.0-windows-x86_64.zip`, then double-click **Install Assets.cmd**. Paste the two ROM paths when requested, then open **Play ClayFighter 64.cmd**. The Python runtime is included; you do not need to install Python.

From a command prompt you can also pass both paths:

```bat
"Install Assets.cmd" "C:\My ROMs\63-third.zip" "C:\My ROMs\sculptors-cut.z64"
```

Use Windows 10/11 x64, an SSSE3/SSE4.1 CPU and current Vulkan GPU drivers. Keep all DLLs beside the executable. If the Microsoft runtime is missing, install the [official x64 redistributable](https://aka.ms/vc14/vc_redist.x64.exe).

## Imported data, updates and recovery

The extractor creates `probe-output/base.z64`, `local/sc_roster/asset-image-roster.z64`, `local/sc_bios/bios.bin` and a local import manifest. These are private game data and are never part of a public package. Both input ROMs are needed for first installation and reinstallation; later launches use the verified imported files.

To update, retain these three generated files and replace the program, scripts, and bundled libraries. Keep your backup ROMs. If a generated file is absent or corrupt, Play opens the installer again; supply both ROMs to regenerate it.

Display, control and training settings stay in `local/settings/` inside the portable game folder. Keep `local/` when updating or moving the game. `--data-dir RELATIVE_PATH` selects an isolated profile within the game folder; paths outside it are rejected. `--controls PATH` imports controls, but subsequent edits save inside the game folder. Installation does not change your original ROMs or physical display configuration.

## Optional title announcement

The package includes the latest user-supplied replacement announcement, stored at `local/audio/title64.s16le`. Its provenance is documented in NOTICE. If absent, the executable falls back to the original announcement from your ROM.

## Help

The installer explains the two-ROM requirement in its prompt, `--help`, and failure messages. Run `python3 tools/release/import_assets.py --check` on Linux (or `python\python.exe tools\release\import_assets.py --check` on Windows) to verify installed game data.

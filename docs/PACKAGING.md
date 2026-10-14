# Portable release packaging

Own work: **djultra64**. Third-party notices remain in `licenses/`.

Package scripts accept an explicit version; preparing an archive does not certify
Training Mode or promote the published release. Build and validate the exact
Linux and Windows binaries first, then prepare fresh staging directories:

```sh
python3 tools/release/package.py --version 2.0 --sdk-root /path/to/prepared/sdk
python3 tools/release/sdk_source.py --version 2.0 --sdk-root /path/to/prepared/sdk
```

The SDK command requires `config/dependencies-2.0.json`, or an explicit
`--dependencies PATH` recording the actual dependency revisions used. Its selected source archives
include native Windows SDL2 2.26.3 as well as Linux SDL2-compat/SDL3/dbus.
Matching source SHA-256 values are checked before archiving. Do not
copy historical validation claims into a new version: `--validation PATH`
includes a completed report explicitly. `--linux-only` prepares just the Linux
binary package and authored source archive while Windows validation is pending.
The inputs still use the existing local final binary/decoder/Python paths and
Homebrew shared libraries; inspect these before staging on another build host.

Outputs go into `local/releases/VERSION/`; a custom `--output` must stay inside
the game folder. Existing staging directories and source archives are never
overwritten. Manifests record platform, version, author, per-file SHA-256 and
portable paths; they do not record unperformed tests. Archive owners use
`djultra64` rather than the build machine's account.

ROMs, extracted game data, analysis, generated translations, development build
outputs and saved settings are excluded. The only `local/` payload in a binary
package is the user-supplied replacement title announcement. Authored source is
selected from public code, recipes, configuration, documentation, assets and
license notices. Original game code regenerated from the player's ROMs remains
private. Prepared upstream source and authentic license notices ship separately.
Packaging rejects ROM suffixes, ROM headers and symlinks in the authored source
and binary staging trees.

Installation and updates keep player-owned ROM imports in `probe-output/` and
`local/sc_roster/` / `local/sc_bios/`. Installer scratch files stay in
`local/import-work/` and are removed after extraction. Preferences remain in
`local/settings/`, caches in `local/cache/`. To update, extract the new public
files over the installation and preserve its existing private folders. Moving
the whole installation carries preferences and imported assets with it.

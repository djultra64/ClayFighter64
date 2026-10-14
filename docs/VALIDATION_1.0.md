# ClayFighter 64 1.0 validation

Project work: **djultra64**. Release date: October 13, 2026 (America/Mexico_City).

## Native game checks

Fresh release verification passed **31/31 Linux CTests** and **50/50 Windows
CTests** on the latest game build. Windows uses MSVC x64. No gameplay changes
were made during release packaging.

The recorded final revision covers the sixteen-character selector, native
portrait animation, original/SC dispatch parity, biographies and original font,
options, input routing/remapping, framebuffer waits, thread lifecycle, title 64
across attract reloads, selector exit/reentry, and native Exit game.

## Public package checks

- **5/5 importer checks passed:** raw z64/v64/n64 and single-ROM ZIP normalization, ambiguous ZIP rejection, missing/duplicate/wrong ROM rejection before persistent writes, either ROM order, exact asset parity, and corruption detection.
- **Linux clean package:** installed both ZIP ROMs in a path containing spaces, created the square-artwork desktop launcher, and completed a 20-second bounded native run with exit code 0 and no stalled state (1,162 graphics submissions; software Vulkan on a private desktop).
- **Windows clean package:** used the bundled CPython runtime, rejected a single ROM, imported both raw ROMs in reverse order in a path containing spaces, verified imported hashes, created the square-artwork shortcut, and completed a 20-second bounded native run with exit code 0 and no stalled state (1,144 graphics submissions; RTX 2060).
- Both packaged game executables match the latest private final binaries. The extracted character/portrait image and biographies match the previously verified private outputs byte-for-byte.
- Archive audits found no ROMs or extracted original portraits/biographies. Applicable dependency notices and the separate SDK source archive accompany the release. Package manifests record per-file SHA-256 hashes; SHA256SUMS covers the final release archives.

## Limits

The Windows GPU is an NVIDIA GeForce RTX 2060 with a 60 Hz display; 120 fps is
a selectable request, not a verified physical presentation rate. Simulation stays
at 60 Hz. Real controllers from all three families and exhaustive mode/character
coverage remain outside this release's automated evidence. Biographies were tested
with selected IDs and authentic font/timer helpers, not every natural RNG sequence.
Linux packaged smoke tests use a private desktop; they do not establish performance
on every distribution, GPU or audio configuration. The source bundle is provided
with pinned dependencies and regeneration instructions, but a separate clean
rebuild from that public bundle has not been verified in this release task.

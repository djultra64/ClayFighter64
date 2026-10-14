# Training Mode 2.0 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking. Recommended execution: native in this session; no delegation is necessary for the dependent reverse-engineering tasks.

**Goal:** Implement native Training Mode with opponent selection, infinite practice, complete verified Move List and repeatable Claytality practice in ClayFighter 64 2.0 for Linux and Windows.

**Architecture:** A host-side TrainingSession owns preferences and practice lifecycle. Verified guest simulation hooks translate native state/events to the session, apply safe mutations and route synthetic dummy input; native-font menus and HUD render session snapshots. Recompiler changes remain reproducible in the CPU generator, and outside a training session all helpers preserve original behavior.

**Tech Stack:** C++20, Python 3, SDL2, N64Recomp-generated MIPS functions, N64ModernRuntime, RT64, CMake/Ninja and MSVC.

**Spec:** [Approved design](../specs/2026-10-13-training-mode-2.0-design.md).

**Status:** Design approved; implementation plan ready for review. No product code has been changed. The exact guest state fields beyond existing evidence are discovery deliverables of Task 1, not guessed constants.

## Global Constraints

- Attribute own work exclusively to `djultra64`; preserve authentic third-party notices. Verify Git author and committer before every commit/publication.
- Keep 63⅓ gameplay, simulation at 60 Hz and the integrated sixteen-character roster; preserve arcade, versus and attract behavior.
- Keep prompts device-aware with Xbox, PlayStation and Nintendo Switch layouts, manual family fallback, action remapping and native attack permutation.
- Normal practice recovers after 180 eligible ticks by default; 1/3/5 seconds or Manual are available; pause freezes timers.
- Claytality retries preserve selections/settings, finish animations before automatic reset and wait 60 active ticks afterward; extended finisher window is 1800 active ticks.
- Linux and Windows share source behavior. ROMs, extracted assets and raw guest fixtures remain private.
- Do not publish or mark binaries as 2.0 until release validation passes. Keep the current 1.0 release valid while developing.

## Review Focus

1. Changing graphics presentation between 60/120 targets must not accelerate recovery, dummy behavior or finisher windows (Tasks 1, 3 and 7).
2. Multi-hit lethal damage and transition between two life reserves must preserve combo damage and avoid ending normal practice (Task 3).
3. Finisher commands, native attack permutations and remapped physical buttons must agree on both facing directions; pause/restart cannot steal a required command (Tasks 4 and 6).
4. Canceling selection or leaving training for arcade/attract must clear session intent before the next match (Tasks 2 and 7).
5. Reloads, disconnected P2 and stage-changing finishers must not retain stale actor pointers, inputs, audio or interpolation resources (Tasks 5–7).

## Files and interfaces

- `host/training_session.{h,cpp}`: platform-independent rules/preferences; `host/training_guest.{h,cpp}`: sole guest simulation access; `host/training_options.{h,cpp}`: native pause pages; `host/training_dummy.{h,cpp}`: synthetic actions; `host/move_list.{h,cpp}`: validated catalog and device-aware notation; `host/training_hud.{h,cpp}`: display/telemetry snapshot.
- Modify `host/main_menu.*`, `host/input_mode.*`, `host/input.*`, `host/input_sdl.cpp`, `host/native_prompts.*`, `host/main.cpp`, `tools/generate_cpu.py`, both CMake projects and release tooling only where needed.
- `tools/probe_training_states.py` and private `analysis/training/` record guest findings. `config/training_hooks.json` records hash/instruction-guarded hook recipes without ROM payloads; `config/training_moves.json` holds authored names and logical commands with stable character IDs.
- `tests/training_session.cpp`, `tests/training_menu.cpp`, `tests/training_guest.cpp`, `tests/training_dummy.cpp`, `tests/training_moves.cpp`, `tests/training_options.cpp` and `tools/verify_training.py` provide isolated rules tests and real guest fixtures. CTest registrations use `training_session_rules`, `training_native_menu`, `training_guest_contract`, `training_dummy_rules`, `training_move_catalog`, `training_native_options`.
- Add actual SDL input scenarios under `tests/fixtures/training/`; extend `tools/headless_test.py` to retain training events in its report, without treating liveness as functional verification.

Core proposed interfaces, defined by Task 3 before their consumers:

```cpp
namespace clay::training {
enum class Preset { Normal, Claytalities };
enum class Phase { Inactive, Selecting, Preparing, Practicing, FinishWindow,
                   FinisherPlaying, RetryWait, ResetPending };
struct FrameState; // Complete value-only fields defined in Task 1's state contract.
struct FrameEffects; // Recovery/reset/finisher preparation effects; no guest pointers.
class Session {
public:
    void begin_selection();
    void leave();
    void request_reset();
    FrameEffects tick(const FrameState& frame);
};
void simulation_tick(uint8_t* rdram, recomp_context* ctx);
}
```

Define `FrameState` with active/paused/attract, actor-generation validity, safe-rest state, damage events, combo-end event, life-reserve values and finisher lifecycle events. Use native state translated by the verified bridge. Define `FrameEffects` as explicit recovery, reset and preparation requests consumed in the same simulation-owner thread. The structs must not expose unverified offsets or render-owned mutable memory.

## Task 1: Establish guest state and lifecycle contracts

**Files:** Create `tools/probe_training_states.py`, `config/training_hooks.json`, `tests/training_guest.cpp`, `tools/verify_training.py`; private evidence in `analysis/training/state-contract.json`.

**Consumes:** Current generated `func_8003C740`, `func_8003D940`, `func_8003ED2C`, `func_80061474`, existing combat evidence and user-provided local assets.
**Produces:** Verified fields/functions/instruction guards for selector ownership, simulation tick, all life reserves, KO, damage, combo/rest, dummy input, timers, finisher states and safe scene restart; exact `FrameState`/`FrameEffects` fields for Task 3.

- [ ] Record clean baseline with `ctest --test-dir build --output-on-failure`; report any existing failures before changes.
- [ ] Trace native single-player/versus selection, regular damage through both reserves, timeout, finisher success/failure and scene cleanup using private SDL scripts and debugger probes. Preserve actual function/register semantics; do not infer KO from HP alone.
- [ ] Extract bounded native fixtures using the existing fixture pattern. Add assertions for those observed transitions and instruction guards; execute `python3 tools/verify_training.py --contract` against private assets.
- [ ] Verify one rule update per unpaused 60 Hz combat tick. Trace both presentation targets; assert equal rule tick counts for the same simulation interval. Do not attach rules to render-thread entry/return.
- [ ] Verify restart reinitializes stage/camera/actors/resources and native selection can accept sequential P1-owned character choices. Record reset control latency and all required cleanup calls.
- [ ] Save the contract and hook recipe. If a required transition cannot be verified, report that specific blocker before implementing dependent guest mutation. Review new findings against the approved design.

## Task 2: Add native menu and sequential selection

**Files:** Modify `host/main_menu.*`, `tools/generate_cpu.py`, `tests/main_menu.cpp`, `tools/verify_main_menu.py`; create `tests/training_menu.cpp`; add both CMake targets.

**Consumes:** Task 1 selector/scene contract and current native roster helpers.
**Produces:** Menu confirm intent and explicit selection lifecycle used by `Session::begin_selection()`; P1/rival/stage IDs supplied to the native scene loader.

- [ ] Add failing original-menu fixture assertions for four visible rows, wrapping, original Game Start/Options behavior, Training confirmation and Exit on its new index. Include idle, hover, Back and animation-transition guards.
- [ ] Run `python3 tools/verify_main_menu.py` and observe the expected missing-fourth-row failure.
- [ ] Implement non-overlapping relocated tables/text and native border reflow; confirm training only at confirmed dispatch. Add instruction-checked generator hooks for sequential selection and cancellation.
- [ ] Add failing selection fixture scenarios for P1-owned rival selection without P2, mirror match, all sixteen slots, distinct colors and Back at each step; implement the verified selector routing.
- [ ] Run menu/selector fixtures and hidden GPU captures in 4:3/16:9 at 480p. Assert selected IDs at actual combat load; cancel then start arcade and prove training intent is cleared.
- [ ] Verify attribution and commit this tested task.

## Task 3: Implement simulation rules, resources and measured damage

**Files:** Create `host/training_session.*`, `host/training_guest.*`, `tests/training_session.cpp`; extend Task 1 fixtures, CPU generator, `host/input_mode.*` and both CMake projects.

**Consumes:** Task 1 state contract, Task 2 mode/selection intent.
**Produces:** The Session API above, normal practice and correctly calculated damage/combos; a read-only session view for UI consumers.

- [ ] Write failing deterministic assertions: no recovery at 179 eligible ticks, recovery at 180; pause/unsafe-rest consume zero ticks; a new hit restarts waiting; Manual never auto-recovers; no effect outside training.
- [ ] Register and run `ctest --test-dir build -R '^training_session_rules$' --output-on-failure` to observe missing behavior, then implement the value-only state machine with 60/180/300 tick recovery settings.
- [ ] Add native failing fixtures for crossing life reserves, lethal multi-hit, chip damage, super refill policies and KO suppression. Verify measured damage uses pre-protection damage events, not floor-clamped HP deltas.
- [ ] Implement instruction-checked damage/KO/timer hooks on the simulation owner, restoring all native reserve flags on recovery; validate actor lifetime/generation and bound all guest access.
- [ ] Run session/native-contract tests and a 30-minute actual training session. Assert no timeout/results, correct recovery, intact combos and unchanged behavior in non-training matches.
- [ ] Verify attribution and commit this tested task.

## Task 4: Build and verify the complete Move List

**Files:** Create `config/training_moves.json`, `host/move_list.*`, `tests/training_moves.cpp`; extend `tools/verify_training.py`, native prompt integration and release asset inclusion.

**Consumes:** Integrated CharDefs, actual input-recognition fixtures, host mapper and native attack permutation.
**Produces:** Catalog query by stable character ID; logical command rendering using current bindings/facing; verified finisher requirements and distance ranges for Task 6.

- [ ] Enumerate executable normal/special/super/combo-system/finisher records and variants for each of the sixteen integrated fighters. Build a coverage manifest distinguishing absence from unverified commands; record execution evidence privately.
- [ ] Write failing assertions for category coverage, unknown-ID safety, paging, simultaneous inputs, charge, both directions and native/host double remapping. Include all controller families and keyboard; distinguish direction Back from physical B.
- [ ] Implement catalog loading/validation and notation with `binding_marker`/`menu_marker`. Reject malformed counts/sequences and stale character selection; do not include raw game data.
- [ ] Execute every listed move/finisher under its required state and range, including donors. Explicitly document proven absent techniques. Catalog completeness remains false until every available record is covered.
- [ ] Run `ctest --test-dir build -R '^training_move_catalog$' --output-on-failure` and `python3 tools/verify_training.py --moves`; verify original native prompts/remapping tests remain green.
- [ ] Verify attribution and commit this tested task.

## Task 5: Add dummy behavior, native options and HUD

**Files:** Create `host/training_dummy.*`, `host/training_options.*`, `host/training_hud.*`, corresponding tests; modify input, native pause/render hooks, `host/main.cpp`, generator and both CMake projects.

**Consumes:** Session snapshots/requests from Task 3, move catalog from Task 4, legal actor/input opportunities from Task 1.
**Produces:** All approved pause pages/defaults/persistence, dummy settings, damage/history display, pinned move and independent optional remappable Restart Training action.

- [ ] Write failing fixtures for pause → Training Options → subpage → Back/Resume; scrolling long lists at 480p; invalid persisted values recover defaults; switching preset preserves normal settings.
- [ ] Write failing dummy tests for Quieto/Agachado/Salto/CPU/Player 2, AI ownership, first-hit-then-block at first legal opportunity, P2 disconnection and release of stale synthetic input.
- [ ] Implement value-only dummy requests delivered through verified native combat input. Route physical P2 exclusively in Player 2 mode; preserve two-controller assignments and family fallback.
- [ ] Implement paginated native-font options and versioned preference storage within the existing user-data directory; keep actors and transient state out of persisted data.
- [ ] Implement damage/combo display and 12-entry history from actual combat-delivered input; freeze navigation input out of history. Pin current-character moves and update orientation/glifos dynamically.
- [ ] Audit restart binding conflicts against verified moves. Add optional remapping and pause action, with release-before-resume protection; do not consume an existing finisher/attack command.
- [ ] Run the three new component CTests, native prompt/remapping suites and actual SDL/GPU scenarios; inspect 480p legibility and both sides.
- [ ] Verify attribution and commit this tested task.

## Task 6: Implement Claytality preparation and safe repeat/reset

**Files:** Extend Session, guest bridge, options, HUD, native contract/rules tests and private finisher scenarios.

**Consumes:** Task 1 verified finisher/reset states and Task 4 verified commands/distances.
**Produces:** Both Claytality preparations, original/1800-tick window, automatic/manual placement and native replay loop with counters.

- [ ] Add failing rules/native scenarios: final-hit preparation sets last reserve/minimum positive HP; direct preparation enters genuine finish state; recovery never fires during finishers; pause freezes window; no-available-finisher disables preset accurately.
- [ ] Implement preparation via verified native state transitions and selected-move range validation. Automatic assistance adjusts only when stable after final-hit displacement; Manual preserves real positioning.
- [ ] Add failing scenarios for finisher success, ordinary victory, timeout and aborted attempt; assert success only on a native finisher event, complete animation before auto reset, then exactly 60 active ticks of waiting.
- [ ] Implement safe restart preserving selections/options/pinned move; reset stage, actors, projectile/throw/stun/input/camera/audio state and interpolation identity. Use verified native scene restart rather than RAM snapshots.
- [ ] Run each available finisher in both preparations, both sides and both windows; perform at least 100 repeated representative resets including scene/camera-changing finishers. Record resource usage and reset latency.
- [ ] Run session/guest contract tests and verify leaving preset restores normal dummy/resource/recovery settings.
- [ ] Verify attribution and commit this tested task.

## Task 7: Cross-platform integration and regression verification

**Files:** Extend `tools/headless_test.py`, `tests/fixtures/training/`, `tools/verify_training.py`, Windows fixture preparation/CMake as necessary; create `docs/VALIDATION_2.0.md`.

**Consumes:** Implemented Tasks 1–6; current SDK and exact local game assets.
**Produces:** Evidence tied to exact binary/source revisions, explicit hardware limitations and release-ready functional assessment.

- [ ] Run `cmake --build build -j4` and `ctest --test-dir build --output-on-failure`; run private functional scripts using `tools/headless_test.py --input ... --seconds ...`. Functional assertions must inspect training events, not merely progressing gfx/polls.
- [ ] Test cancel/reentry/attract/arcade/versus, controller hotplug, remap/family changes, mirrors, all selected characters, long sessions, both presentation targets and all restart conditions from Review Focus.
- [ ] Use `maquinita-windows-recomps` for Windows reservation/power lifecycle; stage the same source/generated files and private assets. Run `tools/windows/build.ps1`, `tools/windows/test.ps1` and actual Windows training scripts with Vulkan/audio validation.
- [ ] Verify prompt/action agreement with real Xbox/PlayStation/Switch controllers when available; distinguish virtual-input coverage and untested hardware. Record test failures and functional coverage rather than reusing 1.0 counts.
- [ ] Measure startup/reset/resource behavior on both platforms; document any remaining unmet acceptance criterion and resolve it before release designation.
- [ ] Request independent review through the applicable review skill, correct findings with reproductions, then rerun affected checks and the required suites. Verify attribution before committing the validated integration.

## Task 8: Prepare concrete 2.0 release artifacts

**Files:** Modify `tools/release/package.py`, `tools/release/sdk_source.py`, `README.md`, `docs/INSTALL.md`; add dependency/source manifest for 2.0 and release notes; update packaging tests.

**Consumes:** Verified Linux/Windows binaries and reports from Task 7.
**Produces:** Locally staged 2.0 binaries/source archives/checksums and accurate release documentation. External publication is a separate final action governed by session authorization.

- [ ] Add failing package tests proving explicit 2.0 version/manifest/output paths, inclusion of authored training config, no overwriting 1.0 staging and exclusion of ROM/private extracted data.
- [ ] Implement version-aware packaging and corresponding-source archive for the validated binaries; preserve compatibility with installed ROMs/preferences and original notices.
- [ ] Document native Training flow/defaults, actual finishers/Move List support, device-aware controls and validation limits. Update public 2.0 labels only at the validated release freeze.
- [ ] Verify clean install, upgrade, paths with spaces, missing/wrong ROM diagnostics and packaged training startup on both platforms; use existing release tests plus training assertions.
- [ ] Generate checksums, verify matching binaries/source and Git identities; commit release preparation. Present artifact paths and completed checks to the user. Do not claim release publication from local packaging alone.

## Execution review

Recommended method: **native**, implementing tasks sequentially in this session. The critical interfaces depend on discoveries about one guest runtime, so separate implementers would repeatedly need the same state context. A fresh review is still required before release.

The approved design is covered by Tasks 1–8: entry/selection (2), infinite rules/resources (3), catalog and controls (4–5), dummy/options/feedback (5), Claytalities/reset (6), lifecycle/platform validation (7), release/update/attribution (8). Deferred advanced features remain outside this plan.

Before execution, review this written plan and confirm it captures the intended 2.0. Once approved, invoke executing-plans, choose the authorized workspace strategy, and begin Task 1. Do not interpret completion of this document as implementation of Training Mode.

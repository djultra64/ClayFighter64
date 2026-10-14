# Training Mode in v2.0

Own work: **djultra64**. v2.0 validation is recorded in
[the release validation record](releases/2.0-validation.md).

Choose **Training Mode** from the main menu, select P1 and the opponent, then
choose a stage in the original stage panel. Navigate with the logical D-pad
and confirm with the south face button; Back uses the east face button. Names
and glyphs follow the active Xbox, PlayStation or Nintendo Switch layout.
The stage waits for confirmation. Back returns to opponent selection.

Start pauses practice. The default shortcuts are **F5 / left-stick click** for
Restart Training and **Esc / right-stick click** for pause. They can be remapped
by action in Shortcut Bindings. The pause menu provides Resume, Training Options,
Restart Training, Change Fighters & Stage, and Main Menu. Training Options
includes the native move list, Normal and Claytality presets, opponent behavior,
resources, display options, restart setup, Claytality preparation and optional
shortcut bindings. The release validation record describes the tested binaries and scenarios.

Opponent settings include standing, crouching, jumping, CPU and a second player,
with guard settings Off, Always and After First Hit for the dummy. CPU and
Player 2 control their own guard; a disconnected second player stands still.
Resources configure recovery
and super meter behavior. Restart setup selects center or corner placement and
P1's side. Display options control damage/combo information, input history and
the pinned move. Restarts reuse the selected fighters and stage through the
reviewed native cleanup path and skip the intro. Linux measurements observed
about two seconds to resume under parallel software rendering; the one-second
target has not been fully achieved. Claytality practice uses the selected fighter's actual move
catalog; options cover preparation, original or extended 30-second window,
and automatic or manual distance. Fighters without a supported Claytality
cannot provide that practice preset.

Training and stage preferences save under the active profile inside
`local/settings/`. Keep the installation's `local/` folder when updating or
moving it. Restart and pause shortcuts are optional and reject conflicting
combat bindings. Real controller checks remain pending when hardware is
available; synthetic input tests do not establish physical controller behavior.

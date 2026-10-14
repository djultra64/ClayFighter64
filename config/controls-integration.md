# ClayFighter 64 input integration

Project work: djultra64.

`host/input.cpp` is the platform-independent logical action and prompt layer.
`host/input_sdl.cpp` is the SDL2 adapter. Add both files to the existing executable.

Create one `clay::input::SdlInput` on the SDL polling thread. Call
`clay::input::configure_sdl()` before `SDL_Init`; this selects position-based SDL
A/B/X/Y mapping. Send every polled SDL event to `input.event(event)`, then call
`input.poll()` and publish its `buttons/x/y` under the existing input mutex.
This replaces the old polling code; keeping both would produce conflicting
mappings and last-device detection. The instance must outlive the polling loop.

Load `config/controls.cfg` with `input.mapper().load(path,error)`. Report load
errors instead of silently replacing controls. Override the file's family when
`--family` is explicitly supplied:
`input.mapper().set_family(clay::input::parse_family(argument))`.
Unknown flags/families must produce a clear CLI error. `auto`, `xbox`,
`playstation`, and `switch` are valid manual options. A manual family changes
prompt names only, leaving physical actions stable. Keyboard use still shows
keyboard prompts even when a manual controller family is selected.

Optional remapping APIs are `remap(Action,Physical)` and
`remap_key(Action,SDL_scancode_name)`; `save(path,error)` persists these plus the
manual family. The supplied config is directly editable. A UI can call the same
APIs without rebuilding. Validate keyboard names using `SDL_GetScancodeFromName`
and reject `SDL_SCANCODE_UNKNOWN` when accepting text from a remapping UI.

`Mapper::prompt(Action)` returns the actual binding's glyph and name. For
Nintendo: physical south=B, east=A, west=Y, north=X. For PlayStation:
south=Cross/×, east=Circle/○, west=Square/□, north=Triangle/△. Xbox uses A/B/X/Y
respectively. An East remap of Primary action therefore shows Nintendo A and
Xbox B while still targeting the same physical East button.

The game now renders prompts through its original text measurement/drawing
routines. Keyboard names use the native font; controller icons use Kenney Input
Prompts Pixel (CC0) in original glyph descriptors. No modern overlay is used.
Options → Button Remap queues capture requests to the SDL polling thread,
persists combat bindings and manual family, and displays the original game's
current punch/kick names. A separate default mapper handles menus and pause;
the guest combat lifecycle publishes which mapper to use.

All mapped SDL controllers remain open. Activity selects the last device,
including a second connected controller. Captured input is suppressed until
released so confirming a binding cannot accidentally activate a menu item.

Verification completed:

- Pure core tests: physical layout names, mapping/prompt consistency, last-device
  persistence, keyboard arrows driving both stick and D-pad for menus, remapping,
  analog bounds, config save/load, atomic rejection of invalid config.
- SDL virtual controller tests: event order, manual family, key-repeat rejection,
  analog jitter rejection, held trigger not stealing keyboard prompts.
- Compilation with C++20 and `-Wall -Wextra -Werror`.

Physical Xbox/PlayStation/Switch controller verification remains pending. The
virtual controller checks do not establish a real device's SDL mapping or glyphs.

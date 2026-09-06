# Wii Classic Controller analog stick support for Canoe

This project provides a small wrapper library for the SNES Classic Mini's
Canoe emulator. It makes the **left analog stick** of a Wii Classic Controller
behave as an additional D-pad input in Canoe games.

The library is loaded alongside Canoe when it starts and adds this input
behavior without replacing the emulator or changing the original controller
mapping. On Linux, this loading technique is called `LD_PRELOAD`.

The original D-pad continues to work. Two controllers are handled independently, so controller 1's stick does not move player 2.

## Motivation

The Wii Classic Controller is an excellent official Nintendo controller for retro play: it has a familiar, compact layout, a proper D-pad, and useful analog sticks. Canoe recognizes it, but its SNES games normally only read the D-pad. The sticks therefore sit unused, even in games where using a stick as a comfortable way to express the same four directions feels natural today.

This project keeps the original game input model intact. It does **not** turn an SNES game into an analog-aware game; it maps the left stick to the same digital directions that a D-pad produces. That makes it useful for games that were never designed around analog movement while preserving their original controls, timing, menus, and behavior.

## What it changes

The library observes SDL controller-axis events and intercepts Canoe's calls to `SDL_GameControllerGetButton`. For D-pad buttons, it returns:

```
physical D-pad pressed OR virtual D-pad direction from left stick
```

It uses separate hysteresis thresholds to avoid flicker near the stick centre:

| Transition | Absolute axis value |
| --- | ---: |
| Press direction | > 18,000 |
| Release direction | < 12,000 |

The mapping is SDL axis 0 = horizontal and axis 1 = vertical. Diagonals work because horizontal and vertical directions are tracked independently.

## Included files

- `src/canoe_analog_state.c` — final C source.
- `dist/libcanoe_analog_state.so` — tested ARMv7 Linux shared library, also attached to the latest GitHub release.
- `scripts/build.ps1` — Windows build command using Zig.
- `scripts/install.sh` — installation script to run on the Mini over SSH.
- `scripts/uninstall.sh` — reversible removal script.

The binary's SHA-256 is:

```
faef005d9a1d8453f6df625b4456b93eba81bb88ad42ba535c16af0220aad098
```

The supplied binary is the exact one tested on the Mini. A later local rebuild
can legitimately have a different file hash because of toolchain/build metadata;
verify that it builds successfully before substituting it for the tested binary.

## License

This project is licensed under the [MIT License](LICENSE). If you use or
redistribute the project, including a modified version, keep the original
copyright and license notices.

## Requirements

- SNES Classic Mini with hakchi/SSH access.
- Canoe launcher at `/usr/bin/clover-canoe-shvc`.
- Wii Classic Controller(s) recognized by Canoe through SDL.

This does not remap controls system-wide and does not affect RetroArch. RetroArch already has its own analog input handling.

## Download

The tested ARMv7 binary is available in the [latest GitHub release](https://github.com/lincolnpomper/wii-classic-analog-on-canoe/releases/latest).

Direct download:

[`libcanoe_analog_state.so`](https://github.com/lincolnpomper/wii-classic-analog-on-canoe/releases/latest/download/libcanoe_analog_state.so)

The same tested binary is also kept in `dist/` in the repository.

## Install

1. Download `libcanoe_analog_state.so` from the latest release, or use the copy in `dist/`.
2. Copy it to `/var/lib/hakchi/libcanoe_analog_state.so` on the Mini.
3. Copy `scripts/install.sh` to the Mini and execute it as root.
4. Reboot the Mini.
5. Test one controller, then two controllers in a simultaneous two-player Canoe game.

`install.sh` makes a timestamped backup of the Canoe wrapper before changing it. It preserves any existing options or prior fixes in that wrapper.

## Verify on the Mini

```sh
sha256sum /var/lib/hakchi/libcanoe_analog_state.so
tail -n 8 /usr/bin/clover-canoe-shvc
```

The wrapper must export the library before it executes `canoe-shvc`:

```sh
if [ -r /var/lib/hakchi/libcanoe_analog_state.so ]; then
    export LD_PRELOAD=/var/lib/hakchi/libcanoe_analog_state.so
fi
exec canoe-shvc $options $log
```

## Reversal

Run `scripts/uninstall.sh` on the Mini. It restores the most recent backup created by the installer and removes the library.

## Development notes

The work began by observing Canoe's SDL input path with an ARM `LD_PRELOAD` probe. The probe established that the Wii Classic Controller uses SDL axis 0 for horizontal movement and axis 1 for vertical movement, while Canoe requests D-pad buttons 11–14 (up, down, left, right).

The implementation originally tried synthesizing SDL events, but Canoe polls `SDL_GameControllerGetButton` instead. The final version tracks stick state in `SDL_PollEvent` and merges it at the button-query boundary.

SDL assigns a joystick instance ID to each controller. State is keyed on this ID, not globally, so each controller's analog stick remains associated with its own player.

## Tested outcome

- Wii Classic Controller: all four analog directions, D-pad, face buttons, and diagonals work.
- Two identical Wii Classic Controllers: each analog stick controls only its own player.
- Original SNES controller: remains working.
- Persistence: works after an SNES Mini reboot.

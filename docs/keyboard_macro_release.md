# DMC4 Keyboard Macro + Battle Snapshot

This test build adds a new dmc4_hook control section:

```text
Keyboard Macro
```

The page contains two player-facing tools:

- Keyboard Macro: plays scripted inputs from text files.
- Battle Snapshot: captures and restores a stable training setup.

## Install

1. Close the game.
2. Back up the existing `dinput8.dll` next to `DevilMayCry4_DX9.exe`.
3. Copy the new `dinput8.dll` next to `DevilMayCry4_DX9.exe`.
4. Copy `keyboard_macro.txt` to the same folder, or choose a custom macro file in the GUI.
5. Launch the game and open the dmc4_hook GUI.
6. Find and enable `Keyboard Macro` in the dmc4_hook GUI. In current upstream builds it appears under `Training`.

## Macro Files

The GUI automatically lists text files in the game folder whose names match:

```text
keyboard_macro*.txt
```

Examples:

```text
keyboard_macro.txt
keyboard_macro1.txt
keyboard_macro_nero.txt
keyboard_macro_dante.txt
```

Use `Custom Macro File` if the file is somewhere else.

## Main Controls

- `Keyboard Macro`: master enable. When this is off, macro hotkeys and playback do not run.
- `Macro File`: selects one detected `keyboard_macro*.txt` file.
- `Custom Macro File`: manually enters a file path.
- `Macro Clip`: selects one section from the loaded file.
- `Hotkeys`: changes the reload, play, stop, capture, load, and load-then-play keys.
- `Reload Macro File`: reads the selected file again from disk.
- `Play Macro`: starts the selected clip.
- `Stop Macro`: stops playback and releases macro-held input.
- `Snapshot Play Delay Ticks`: delay between loading a Battle Snapshot and starting macro playback.
- `Load Snapshot + Play Macro`: loads the current Battle Snapshot, waits the configured delay, then starts the selected macro clip.

## Input Model

The macro system writes into the game's input layer instead of sending Windows keyboard events. This is why it is more stable than AHK-style automation:

- game focus matters less
- key timing is tied to game input updates
- held buttons and releases are represented explicitly
- your normal input can still be mixed with macro playback

Macro files do not press your physical keyboard keys. They describe the game's input buttons and movement directions. For example, use `Y` for melee, `A` for jump, and `EXCEED` for Nero Exceed.

Movement uses fixed macro direction tokens:

```text
MW = move forward / up
MS = move back / down
MA = move left
MD = move right
```

These names are virtual direction aliases. They are not your physical keyboard layout. This matters for AZERTY/ZQSD users: `MA` still means "move left" even if your real left key in DMC4 is `Q`. The short aliases `W`, `S`, `A`, and `D` may work as movement aliases, but shared scripts should prefer `MW/MS/MA/MD` so nobody confuses movement `A` with the game's `A` jump button.

`WAIT n` is not milliseconds. It means the current macro input state is held for `n` input ticks. An input tick is one pass through the game's input-reading path. In normal gameplay it is usually close to one rendered frame, so at 60 FPS it feels close to 1/60 second and at 120 FPS it feels close to 1/120 second, but scripts should be written in ticks instead of real-time milliseconds. If the game stalls, pauses, or changes update behavior, real time can drift; the macro is still counting game input updates.

Example:

```text
HOLD Y
WAIT 1
RELEASE Y
```

This means "hold melee through one game input update", not "hold Y for 1 ms".

## Character Notes

Common default pad labels:

```text
Y  = melee
X  = gun
A  = jump
B  = Nero Bringer / Dante style action
L1 = Devil Trigger
R1 = lock-on
L2 = Nero Exceed / Dante change gun
R2 = Dante change sword
```

These labels refer to the game's default in-game action mapping, not your physical keyboard keys. You can still change the game's own keyboard controls in the normal DMC4 options menu and keep playing with your personal layout. Macro scripts should describe the game action button (`Y` for melee, `A` for jump, `EXCEED` for Nero Exceed, etc.), and the game will resolve that through its current control settings.

For Nero Exceed, prefer:

```text
TAP EXCEED
```

`L2` is also treated as Exceed while playing as Nero. While playing as Dante, `L2` remains the normal change-gun button.

## Battle Snapshot

Battle Snapshot is for quickly restoring a training setup in the same room. It is not a full savestate.

`Load Snapshot + Play Macro` is a convenience action for repeatable setup practice. It does this:

```text
Load Battle Snapshot
wait Snapshot Play Delay Ticks
Play selected Macro Clip
```

Use it when you want one hotkey to reset player/enemy/camera position and immediately test the same macro again. If playback starts before the scene has visually settled, increase `Snapshot Play Delay Ticks`; values like 30 to 100 ticks are reasonable starting points.

The stable restore path covers:

- player position, facing, and velocity
- camera position and camera parameters
- matching active enemy position, velocity, and rotation
- regular enemy HP and stun data where safe

`Restore Resources` additionally restores selected combat resources:

- player HP
- DT amount and DT active flag
- Nero Exceed level/timer
- Dante style
- Royalguard/revenge/disaster-related gauges when available

Use Battle Snapshot in the same mission and room. Loading after changing rooms, restarting a stage, or changing the enemy set may fail or do nothing.

## Helper Command Quick Guide

`DIR direction button ticks` is for one-direction attacks. It temporarily holds the direction plus the button, then releases them. If `R1` or another input is already held, it stays held during the helper.

```text
HOLD R1
DIR MA Y 1
RELEASE R1
```

`BACK_FORWARD back forward button` is for moves that need a back-to-forward input before the attack button, such as Nero Calibur/Shuffle-style inputs. It is not a replacement for every backward move; if the move only needs back plus attack, use `DIR` instead.

```text
HOLD R1
BACK_FORWARD MD MA Y
RELEASE R1
```

## Testing Checklist

For first-time testing:

1. Enable `Keyboard Macro`.
2. Select `keyboard_macro.txt`.
3. Click `Reload Macro File`.
4. Select a clip.
5. Click `Play Macro`.
6. Test the hotkey for that clip.
7. Capture a Battle Snapshot.
8. Move the player or enemy.
9. Load the Battle Snapshot.

If something gets stuck, click `Stop Macro` or disable `Keyboard Macro`.

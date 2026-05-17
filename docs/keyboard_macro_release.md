# DMC4 Keyboard Macro + Battle Snapshot Player Guide

Read this first if you only want to install the DLL, play a macro, and understand the basic script language. For every accepted command and token, see `keyboard_macro_script_reference.md`.

## What This Adds

- Keyboard Macro plays scripted DMC4 inputs from `keyboard_macro*.txt`.
- Battle Snapshot restores a repeatable same-room training setup.

Keyboard Macro is not AHK. It writes into DMC4's input layer instead of sending Windows keyboard events. Timing is based on game input updates, and your normal player input can still be mixed with macro playback.

Battle Snapshot is not a full savestate. It is meant for practical training resets in the same room: player position, camera, and matching active enemies.

## Install

1. Close the game.
2. Back up the existing `dinput8.dll` next to `DevilMayCry4_DX9.exe`.
3. Copy the new `dinput8.dll` next to `DevilMayCry4_DX9.exe`.
4. Copy `keyboard_macro.txt` to the same folder.
5. Launch the game.
6. Open the dmc4_hook GUI.
7. Enable `Keyboard Macro` under `Training`.

The GUI automatically lists files next to the game executable whose names match `keyboard_macro*.txt`, such as:

```text
keyboard_macro.txt
keyboard_macro_nero.txt
keyboard_macro_boss_practice.txt
```

Use `Custom Macro File` if your file is somewhere else.

## First Test

Create or edit `keyboard_macro.txt`:

```text
[V] Jump test
TAP JUMP
```

In game:

1. Enable `Keyboard Macro`.
2. Select `keyboard_macro.txt`.
3. Click `Reload Macro File`.
4. Select `Jump test [V]`.
5. Press `V` or click `Play Macro`.

Your character should jump once.

## GUI Controls

- `Keyboard Macro`: master enable. When this is off, macro hotkeys and playback do not run.
- `Macro File`: selects one detected `keyboard_macro*.txt` file.
- `Custom Macro File`: lets you type or paste a custom path.
- `Macro Clip`: selects one section from the loaded file.
- `Reload Macro File`: reads the selected file again after you edit it.
- `Play Macro`: starts the selected clip.
- `Stop Macro`: stops playback and releases macro-held input.
- `Auto reload Macro file`: reloads the selected file after you save it, as long as playback is not currently running.
- `Stop Macro when game pauses`: stops playback when DMC4 opens the pause menu, so the macro will not continue after unpausing.
- `Hotkeys`: sets keys for reload, play, stop, capture snapshot, load snapshot, and load snapshot plus play.
- `Action Mapping`: tells action names such as `MELEE`, `JUMP`, and `LOCK_ON` which virtual controller buttons they should press.
- `Snapshot Play Delay Ticks`: delay between loading a snapshot and playing the selected macro.
- `Load Snapshot + Play Macro`: loads the snapshot, waits the delay, then starts the selected macro clip.

## Input Names

For readable scripts, write game action names:

```text
MELEE
GUN
JUMP
BRINGER
STYLE_ACTION
DEVIL_TRIGGER
LOCK_ON
EXCEED
CHANGE_GUN
CHANGE_SWORD
TAUNT
CHANGE_TARGET
RESET_CAMERA
```

Action names use `Action Mapping` in the GUI. Match this table to your DMC4 in-game controller settings. The table does not change DMC4's controls by itself; it only tells the macro which virtual controller button to output for each action name.

Example:

```text
DMC4 option: Lock-On = R2
Action Mapping: LOCK_ON = RT / R2
Script: HOLD LOCK_ON
```

Controller names are also valid:

```text
Xbox:         Y X A B LB RB LT RT LS RS START BACK
PlayStation: TRIANGLE SQUARE CROSS CIRCLE L1 R1 L2 R2 L3 R3 OPTIONS SHARE
```

Important:

- `X` means Xbox X, which is DMC4's default gun button. If you mean PlayStation Cross, write `CROSS`.
- `A` means Xbox A, which is DMC4's default jump button. If you mean move left, write `MA`.
- `LS` and `RS` mean physical stick clicks / L3 and R3. Prefer `CHANGE_TARGET` and `RESET_CAMERA` when you mean those actions.
- If you mean camera movement, write `CAM_LEFT`, `CAM_RIGHT`, `RS_LEFT`, or `RS_RIGHT`.
- `EXCEED` is the safest spelling for Nero Exceed.

## Movement, Camera, And Ticks

Movement uses virtual left-stick directions:

```text
MW = move forward / up
MS = move back / down
MA = move left
MD = move right
```

These names are not physical keyboard keys. On an AZERTY keyboard, your real left key might be `Q`, but macro `MA` still means move left.

Slow walk uses a light left-stick push:

```text
WALK_MW = slow walk forward / up
WALK_MS = slow walk back / down
WALK_MA = slow walk left
WALK_MD = slow walk right
```

`SLOW_MW`, `SLOW_MS`, `SLOW_MA`, and `SLOW_MD` are equivalent names. You can hold and release them like normal movement:

```text
HOLD WALK_MD
WAIT 30
RELEASE WALK_MD
```

Camera movement uses virtual right-stick directions:

```text
CAM_UP
CAM_DOWN
CAM_LEFT
CAM_RIGHT
```

`WAIT n` uses input ticks, not milliseconds. One tick is one pass through DMC4's input update path. In normal play it is usually close to one rendered frame:

```text
60 FPS  -> about 1/60 second per tick
120 FPS -> about 1/120 second per tick
```

Fastest normal tap in long form:

```text
HOLD MELEE
WAIT 1
RELEASE MELEE
```

If you pause the game while a macro is playing, DMC4 stops running the input updates that advance the macro. With `Stop Macro when game pauses` enabled, the macro is cancelled instead of waiting and continuing after you unpause.

## Common Script Helpers

`TAP` presses and releases an input:

```text
TAP JUMP
TAP EXCEED
TAP MELEE 3
```

`DIR direction action ticks` is for one-direction attacks:

```text
HOLD LOCK_ON
DIR MA MELEE 1
RELEASE LOCK_ON
```

`BACK_FORWARD back forward action` is for moves that need back, then forward, then attack:

```text
HOLD LOCK_ON
BACK_FORWARD MD MA MELEE
RELEASE LOCK_ON
```

Built-in Nero helpers:

```text
CALIBUR_RIGHT
CALIBUR_LEFT
SHUFFLE_RIGHT
SHUFFLE_LEFT
```

Dante style can be set directly:

```text
STYLE SM
STYLE GS
STYLE TS
STYLE RG
STYLE DS
```

If hook `Character Switcher` is already enabled and ready, a macro can request one swap:

```text
CHARACTER_SWITCH
```

One Hit Kill can be controlled from a macro:

```text
ONE_HIT_KILL ON
ONE_HIT_KILL OFF
ONE_HIT_KILL TOGGLE
```

For the full command list, see `keyboard_macro_script_reference.md`.

## Battle Snapshot

Battle Snapshot is for quickly restoring a training setup in the same mission and room.

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

It is not designed for changing rooms, changing enemy sets, restarting missions, or preserving every animation/model detail.

`Load Snapshot + Play Macro` performs:

```text
Load Battle Snapshot
wait Snapshot Play Delay Ticks
Play selected Macro Clip
```

Use it when you want one hotkey to reset position/camera/enemy setup and immediately test the same macro again. If playback starts too early after loading, increase `Snapshot Play Delay Ticks`.

## Troubleshooting

- Clip does nothing: enable `Keyboard Macro`, reload the file, and check the selected clip.
- Hotkey does nothing: make sure `Keyboard Macro` is enabled and the hotkey is set in the GUI.
- Input stays held: click `Stop Macro`.
- Exceed becomes Bringer: use `EXCEED` and update to the newest DLL from this release.
- Camera does not move: use `CAM_LEFT` or `RS_LEFT`, not plain `RS`.
- Target does not change: use `CHANGE_TARGET` while locked on, and make sure Action Mapping matches your in-game controller settings.
- Snapshot load looks wrong: recapture in the same room, with the same enemy setup, and avoid loading during room transitions.

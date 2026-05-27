# DMC4 Macro + Battle Snapshot Player Guide

Read this guide first if you want to install the DLL, play a macro, set up controller-aware action names, and use Battle Snapshot for repeatable practice.

For the complete script grammar, token list, aliases, and detailed `WAIT_UNTIL` templates, see `macro_script_reference.md`.

## What This Adds

- Macro plays scripted DMC4 inputs from `macro*.txt`.
- Battle Snapshot restores a repeatable same-room training setup.
- Animation Overlay shows runtime animation values on the game screen while you tune script timing.

Macro is not AHK. It writes into DMC4's input layer instead of sending Windows keyboard events. Timing is based on game input updates, and your normal player input can still be mixed with macro playback.

Battle Snapshot is not a full savestate. It is meant for practical training resets in the same room: player position, camera, and matching active enemies.

Animation Overlay is a read-only HUD for practice and script tuning. It does not pause, scrub, or edit animation playback.

## Install

1. Close the game.
2. Back up the existing `dinput8.dll` next to `DevilMayCry4_DX9.exe`.
3. Copy the new `dinput8.dll` next to `DevilMayCry4_DX9.exe`.
4. Copy `macro.txt` to the same folder.
5. Launch the game.
6. Open the dmc4_hook GUI.
7. Enable `Macro` under `Training`.

The GUI automatically lists files next to the game executable whose names match `macro*.txt`, such as:

```text
macro.txt
macro_nero.txt
macro_boss_practice.txt
```

Use `Custom Macro File` if your file is somewhere else.

## First Test

Create or edit `macro.txt`:

```text
[V][Nero] Jump test
TAP JUMP
```

In game:

1. Enable `Macro`.
2. Select `macro.txt`.
3. Select `Jump test [V] [Nero]`.
4. Play as Nero, then press `V` or click `Play Macro`.

Your character should jump once.

If the character does something other than jump, check `Action Mapping`. `JUMP` is an action name, so the macro presses whatever button the Nero or Dante mapping says is Jump. By default that is `A / CROSS`. If your in-game layout uses a different jump button, update the matching Action Mapping table.

Use `[Dante]` instead of `[Nero]` when testing as Dante. The mod will not start a clip whose character tag does not match the current player.

## Core Concepts

### Macro Clips

A macro file is split into clips. Each playable clip starts with a header, followed by one script command per line:

```text
[V][Nero] Nero clip name
TAP JUMP
WAIT 10
TAP MELEE
```

The first bracket is the keyboard hotkey. The last bracket is the character tag. The clip name after the brackets is what you see in the GUI.

The same keyboard hotkey can be used once for Nero and once for Dante:

```text
[V][Nero] Nero route
[V][Dante] Dante route
```

When you press `V`, the mod chooses the clip that matches the current character.

### Character Tags And Action Mapping

The character tag is required because Nero and Dante have separate controller settings in DMC4. The tag decides which `Action Mapping` table is used for action names.

Shared action names such as `MELEE`, `GUN`, `JUMP`, and `LOCK_ON` can point to different physical buttons for each character.

For example, if your DMC4 settings are:

```text
Nero MELEE  = Y / TRIANGLE
Dante MELEE = B / CIRCLE
```

then this script line:

```text
TAP MELEE
```

presses `Y / TRIANGLE` inside a `[Nero]` clip, but presses `B / CIRCLE` inside a `[Dante]` clip. The word stays the same because the action is the same; the output button changes because the character's controller table is different.

Use action names when you want the script to follow the player's layout:

```text
TAP JUMP
HOLD LOCK_ON
TAP MELEE
```

Action names also make scripts easier to read. `TAP MELEE` already says what the player is trying to do, so the script usually needs fewer comments than a button-only version such as `TAP Y`.

Use physical button names only when you really mean a specific controller button:

```text
TAP A
HOLD RB
TAP L2
```

### Gamepad Clip Hotkeys

You can add a gamepad clip hotkey between the keyboard hotkey and character tag:

```text
[V][X/SQUARE][Nero] Nero clip name
```

That example means: press `V` on keyboard, or hold `Back/Select` first and then press `X / Square` on controller, to play this clip while the current character is Nero.

Slash names must describe the same physical button in different controller naming styles. For example, `[L2/LT]`, `[LT/L2]`, `[LT]`, and `[L2]` all mean the left trigger, but `[L2/LB]` is invalid because `L2` and `LB` are different buttons.

All gamepad clip hotkeys use `Back/Select` as the fixed prefix. The order matters: hold `Back/Select` first, then press the second button.

## GUI Setup

### Macro

- `Macro`: master enable. When this is off, macro hotkeys and playback do not run.
- `Macro File`: selects and loads one detected `macro*.txt` file.
- `Custom Macro File`: lets you type or paste a custom path.
- `Macro Clip`: selects one section from the loaded file.
- `Play Macro`: starts the selected `Macro Clip` from the selected `Macro File` if its character tag matches the current player.
- `Stop Macro`: stops playback and releases macro-held input.
- The selected macro file reloads automatically after you save it, as long as playback is not currently running.
- `Stop Macro when game pauses`: stops playback when DMC4 opens the pause menu, so the macro will not continue after unpausing.
- `Snapshot Play Delay Ticks`: delay between loading a snapshot and playing the selected macro.
- `Load Snapshot + Play Macro`: loads the snapshot, waits the delay, then starts the selected `Macro Clip` from the selected `Macro File`.

`Play Macro` and `Load Snapshot + Play Macro` are ignored while macro playback is already running. `Stop Macro` remains available.

### Keyboard Hotkeys

`Keyboard Hotkeys` sets GUI action shortcuts such as:

```text
Play Macro
Stop Macro / Clear Input
Capture Snapshot
Load Snapshot
Load Snapshot + Play Macro
```

These are different from clip header hotkeys. GUI action hotkeys run the currently selected macro file, macro clip, and snapshot settings. Clip header hotkeys such as `[V][Nero]` belong to the macro file and start that specific clip.

### Action Mapping

`Action Mapping` tells action names such as `MELEE`, `JUMP`, `LOCK_ON`, and `DEVIL_TRIGGER` which virtual controller buttons they should press.

There are two tables:

- `Nero Action Mapping`
- `Dante Action Mapping`

Match both tables to your DMC4 in-game controller settings. Action Mapping does not change DMC4's controls by itself; it only changes what the macro outputs.

Action Mapping can use face buttons, bumpers, triggers, stick clicks, Back/Select, Start/Options, and D-pad directions. If you put a Nero or Dante action on D-pad in DMC4's settings, set the same D-pad direction here.

Shared action names are the names both characters understand, such as `MELEE`, `GUN`, `JUMP`, `DEVIL_TRIGGER`, `LOCK_ON`, `TAUNT`, `CHANGE_TARGET`, and `RESET_CAMERA`. These names are useful when you want one combo idea to read naturally for both characters.

Some action names belong to only one character. Nero has `EXCEED` and `BRINGER`; Dante has `STYLE_ACTION`, `CHANGE_GUN`, and `CHANGE_SWORD`. A Dante-only action will not work inside a `[Nero]` clip, and a Nero-only action will not work inside a `[Dante]` clip.

Use action names when you want the script to follow the player's layout:

```text
TAP JUMP
HOLD LOCK_ON
TAP MELEE
```

Use physical button names only when you really mean a specific controller button:

```text
TAP A
HOLD RB
TAP L2
```

Example:

```text
DMC4 option: Nero Lock-On = R2
Nero Action Mapping: LOCK_ON = RT / R2
Script: HOLD LOCK_ON
```

### Gamepad Hotkeys

`Gamepad Hotkeys` enables controller shortcuts. It covers both fixed GUI actions and optional per-clip gamepad hotkeys written in macro headers.

`Back/Select` is always the shortcut prefix. To set or use a shortcut, hold `Back/Select` first, then press the second button. Pressing the second button first and then pressing `Back/Select` does not trigger a macro.

While `Gamepad Hotkeys` is enabled, the mod consumes `Back/Select` and the second shortcut button while a shortcut is being detected, so the game will not also receive them. Manual `Back/Select` is reserved for shortcuts while this option is on, so the original action bound to Back/Select will not fire. In the default DMC4 layout, that original action is Taunt.

Default gamepad shortcuts:

```text
Back/Select + A  Play Macro
Back/Select + B  Stop Macro
Back/Select + X  Capture Snapshot
Back/Select + Y  Load Snapshot
Back/Select + RB Load Snapshot + Play Macro
```

Allowed second buttons include face buttons, bumpers, triggers, stick clicks, Start/Options, and D-pad directions. `Back/Select` itself cannot be the second button.

Macro clips can define their own controller shortcut:

```text
[F1][X/SQUARE][Nero] Air route
TAP JUMP
```

With `Gamepad Hotkeys` enabled, `Back/Select + X / Square` starts that Nero clip directly. If a clip shortcut uses the same second button as a fixed shortcut, the matching clip shortcut has priority for the current character.

Macro playback can still send Back/Select normally:

```text
HOLD SELECT
WAIT 100
RELEASE SELECT
```

### Animation Overlay

Enable `Animation Overlay` under `System -> Custom HUD elements` when you want to see timing values without keeping the main GUI open. Close the GUI after enabling it; the overlay stays visible on the game screen.

The overlay can show:

```text
Anim:   current animation ID
MoveID2: runtime action/state value
Part:   action phase value
Frame:  current animation frame / max frame
State:  grounded or airborne, plus hitstop and lock-on flags when active
```

Use `Frame` with `WAIT_UNTIL ANIM_FRAME` or `WAIT_UNTIL FRAME_REACHED_MAX`. Use `MoveID2` with `WAIT_UNTIL MOVEID2` or `WAIT_UNTIL MOVEID2_CHANGED`.

`Show Advanced Animation Fields` adds extra ground and hitstop timer values for players who want more detail while testing.

### Battle Snapshot

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

## Writing Basic Scripts

### Input Names And Commands

A script line usually starts with a command. Input names such as `MELEE`, `JUMP`, `LOCK_ON`, `MOVE_LEFT`, and `CAM_LEFT` are action, button, or stick names; they describe what input you want, but they do not play by themselves. Use them after commands such as `TAP`, `HOLD`, `RELEASE`, `SET`, `DIR`, or `BACK_FORWARD`.

Wrong:

```text
MELEE
LOCK_ON
```

Right:

```text
TAP MELEE
HOLD LOCK_ON
RELEASE LOCK_ON
```

Common script line types:

| Type | What it is | Examples |
| --- | --- | --- |
| Input names | Named actions, physical buttons, stick directions, camera directions, or D-pad directions. They need a command before them. | `MELEE`, `LOCK_ON`, `MOVE_LEFT`, `CAM_LEFT`, `CHANGE_STYLE_RG` |
| Complete commands | Full lines that run a direct macro or hook action by themselves. | `WAIT 10`, `WAIT_UNTIL CAN_EXCEED`, `STYLE RG`, `FREEZE`, `CHARACTER_SWITCH`, `ONE_HIT_KILL TOGGLE` |
| Commands with input arguments | Commands that perform, hold, release, or combine one or more input names. | `TAP MELEE`, `HOLD LOCK_ON`, `DIR MOVE_LEFT MELEE 1`, `BACK_FORWARD MOVE_RIGHT MOVE_LEFT MELEE` |

### Action Names

Use action names when you mean "the player's configured action":

| Action name | Meaning in game |
| --- | --- |
| `MELEE` | The current character's normal close-range attack button. |
| `GUN` | The current character's gun/shoot button. |
| `JUMP` | The current character's jump button. |
| `DEVIL_TRIGGER` | Devil Trigger action. |
| `LOCK_ON` | Lock-on/targeting button. Hold it for lock-on directional moves. |
| `TAUNT` | Taunt action. |
| `CHANGE_TARGET` | Change lock-on target. |
| `RESET_CAMERA` | Reset/center camera. |
| `EXCEED` | Nero's Exceed/rev action. Valid in `[Nero]` clips. |
| `BRINGER` | Nero's Devil Bringer/Buster action. Valid in `[Nero]` clips. |
| `STYLE_ACTION` | Dante's current-style action button, such as Trickster dash or Royalguard block. Valid in `[Dante]` clips. |
| `CHANGE_GUN` | Dante's gun weapon switch. Valid in `[Dante]` clips. |
| `CHANGE_SWORD` | Dante's melee weapon switch. Valid in `[Dante]` clips. |

Shared action names use the table selected by the clip header. `[V][Nero]` reads `MELEE` from Nero Action Mapping; `[V][Dante]` reads `MELEE` from Dante Action Mapping.

This is why shared scripts should usually use action names instead of physical button names. `TAP MELEE` means "press this character's melee button"; `TAP Y` always presses the physical `Y / TRIANGLE` button, even if you changed Nero or Dante melee to `B / CIRCLE` in DMC4.

Action names make scripts more self-explanatory. A line like `TAP DEVIL_TRIGGER` usually needs no extra note, while `TAP LB` only tells the reader which physical button was pressed.

### Dante Style Input

`STYLE RG` is a complete command. It changes Dante's current style directly:

```text
STYLE RG
```

`CHANGE_STYLE_RG` is an input name for D-pad Down. It does not set the style directly and it does not check Dante's controller settings:

```text
TAP CHANGE_STYLE_RG
TAP DOWN
```

Use `STYLE RG` when you want to force Royalguard directly. Use `TAP CHANGE_STYLE_RG` or `TAP DOWN` when you specifically want to press D-pad Down.

### Physical Button Names

Controller button names are also valid:

```text
Xbox:         Y X A B LB RB LT RT LS RS START BACK
PlayStation: TRIANGLE SQUARE CROSS CIRCLE L1 R1 L2 R2 L3 R3 OPTIONS SHARE
```

Important:

- `X` means Xbox X, which is DMC4's default gun button. If you mean PlayStation Cross, write `CROSS`.
- `A` means Xbox A, which is DMC4's default jump button. If you mean move left, write `MOVE_LEFT`.
- `BACK` / `SELECT` means the physical Back/Select button. If you mean move back, write `MOVE_BACK`.
- `L2` / `LT` means the physical controller trigger. If you mean Nero's Exceed action, write `EXCEED`.
- `LS` and `RS` mean physical stick clicks / L3 and R3. Prefer `CHANGE_TARGET` and `RESET_CAMERA` when you mean those actions.
- If you mean camera movement, write `CAM_LEFT`, `CAM_RIGHT`, `RS_LEFT`, or `RS_RIGHT`.

### Movement, Camera, And Ticks

Movement uses virtual left-stick directions:

| Name | Meaning |
| --- | --- |
| `MOVE_FORWARD` | Push the virtual left stick forward/up. |
| `MOVE_BACK` | Pull the virtual left stick back/down. |
| `MOVE_LEFT` | Push the virtual left stick left. |
| `MOVE_RIGHT` | Push the virtual left stick right. |

These names are not physical keyboard keys. They mean virtual left-stick directions, so they are the same no matter what keyboard layout or controller layout you use.

Slow walk uses a light left-stick push:

```text
WALK_FORWARD
WALK_BACK
WALK_LEFT
WALK_RIGHT
```

Camera movement uses virtual right-stick directions:

| Name | Meaning |
| --- | --- |
| `CAM_UP` | Push the virtual right stick up. |
| `CAM_DOWN` | Push the virtual right stick down. |
| `CAM_LEFT` | Push the virtual right stick left. |
| `CAM_RIGHT` | Push the virtual right stick right. |

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

## Simple Examples

Use action names when you want the script to follow the current character's Action Mapping:

```text
[V][X/SQUARE][Nero] Action names
TAP JUMP
WAIT 10
TAP MELEE
```

Use physical controller button names when you really want a specific button:

```text
[F1][Y/TRIANGLE][Nero] Physical buttons
TAP A
WAIT 10
TAP Y
```

Use complete commands when you want a built-in direct action:

```text
[F2][B/CIRCLE][Dante] Direct style set
STYLE RG
WAIT 1
TAP STYLE_ACTION
```

Another complete-command example:

```text
[F3][DPAD_UP][Nero] Short freeze
FREEZE
WAIT 30
UNFREEZE
```

Move, slow walk, and camera examples:

```text
[F4][RB/R1][Nero] Move and camera
HOLD MOVE_RIGHT
WAIT 20
RELEASE MOVE_RIGHT
HOLD WALK_LEFT
WAIT 20
RELEASE WALK_LEFT
HOLD CAM_LEFT
WAIT 20
RELEASE CAM_LEFT
```

For advanced recipes and full command syntax, see `macro_script_reference.md`.

## Troubleshooting

- Clip does nothing: enable `Macro`, save the macro file, and check the selected clip.
- Hotkey does nothing: make sure `Macro` is enabled and the hotkey is set in the script header or GUI.
- Selected clip refuses to play: make sure the clip's `[Nero]` or `[Dante]` tag matches the current character.
- Gamepad hotkey does nothing: enable `Macro` and `Gamepad Hotkeys`, then hold `Back/Select` and press the action button.
- Manual Back/Select no longer taunts: this is expected while `Gamepad Hotkeys` is enabled. Turn off `Gamepad Hotkeys`, or use `SELECT` / `BACK` in a macro file if you intentionally want the macro to send that input.
- Input stays held: click `Stop Macro`.
- Exceed becomes the wrong action: make sure the clip header is `[...][Nero]`, use `EXCEED` / `REV`, and set Nero Action Mapping `EXCEED` to your in-game Exceed button.
- `TAP L2` behaves differently from `TAP EXCEED`: `L2` is the physical trigger; `EXCEED` is the Nero action name.
- Camera does not move: use `CAM_LEFT` or `RS_LEFT`, not plain `RS`.
- Target does not change: use `CHANGE_TARGET` while locked on, and make sure the clip's Nero/Dante Action Mapping table matches your in-game controller settings.
- Snapshot load looks wrong: recapture in the same room, with the same enemy setup, and avoid loading during room transitions.

# keyboard_macro*.txt Script Reference

This is the command and token reference for Keyboard Macro scripts. For install steps, GUI controls, Action Mapping setup, and Battle Snapshot usage, read `keyboard_macro_release.md`.

## File Format

```text
# comments are ignored

[V] Optional clip hotkey and clip name
command
command

[Ctrl+1] Another clip
command
```

Rules:

- blank lines are ignored
- lines starting with `#` are comments
- write one command per line
- clip headers are optional, but recommended
- clip hotkeys start clips only; GUI hotkeys are configured in the GUI
- macro commands inject DMC4 game input, not Windows keyboard events

## Action Tokens

Action tokens are readable names for DMC4 actions. They use the GUI `Action Mapping` settings.

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

Use these names when possible:

```text
TAP JUMP
HOLD LOCK_ON
TAP MELEE
TAP EXCEED
```

`EXCEED` is the Nero Exceed action. Direct `L2` / `LT` is also accepted for default Nero controls, but `EXCEED` is clearer.

## Default Controller Layout

```text
Action             Xbox        PlayStation
MELEE             Y           TRIANGLE
GUN               X           SQUARE
JUMP              A           CROSS
BRINGER           B           CIRCLE
STYLE_ACTION      B           CIRCLE
DEVIL_TRIGGER     LB          L1
LOCK_ON           RB          R1
EXCEED            LT          L2
CHANGE_GUN        LT          L2
CHANGE_SWORD      RT          R2
TAUNT             BACK        SHARE
CHANGE_TARGET     LS          L3
RESET_CAMERA      RS          R3
```

If you changed DMC4's in-game controller settings, match the GUI `Action Mapping` table to your layout. Action Mapping only changes what the macro outputs; it does not change DMC4's own settings.

Important token differences:

- `X` means Xbox X / gun. PlayStation Cross is `CROSS`.
- `A` means Xbox A / jump. Movement left is `MA`.
- `LS` and `RS` mean physical stick clicks / L3 and R3. Prefer `CHANGE_TARGET` and `RESET_CAMERA` when you mean those actions.
- right-stick camera movement uses `CAM_LEFT`, `CAM_RIGHT`, `RS_LEFT`, or `RS_RIGHT`.

## Movement Tokens

Movement tokens are virtual left-stick directions, not physical keyboard keys.

```text
MW = move forward / up
MS = move back / down
MA = move left
MD = move right
```

Diagonals:

```text
MW_MD
MW_MA
MS_MD
MS_MA
```

Descriptive aliases:

```text
MOVE_UP
MOVE_DOWN
MOVE_LEFT
MOVE_RIGHT
MOVE_FORWARD
MOVE_BACK
MOVE_UP_RIGHT
MOVE_UP_LEFT
MOVE_DOWN_RIGHT
MOVE_DOWN_LEFT
```

Slow walk tokens use a light left-stick push:

```text
WALK_MW
WALK_MS
WALK_MA
WALK_MD
WALK_UP
WALK_DOWN
WALK_LEFT
WALK_RIGHT
```

`SLOW_*` is accepted as an equivalent spelling, for example `SLOW_MA`.

Slow diagonals:

```text
WALK_MW_MD
WALK_MW_MA
WALK_MS_MD
WALK_MS_MA
WALK_UP_RIGHT
WALK_UP_LEFT
WALK_DOWN_RIGHT
WALK_DOWN_LEFT
```

## Camera Tokens

Right-stick camera movement:

```text
CAM_UP
CAM_DOWN
CAM_LEFT
CAM_RIGHT
RS_UP
RS_DOWN
RS_LEFT
RS_RIGHT
RIGHT_STICK_UP
RIGHT_STICK_DOWN
RIGHT_STICK_LEFT
RIGHT_STICK_RIGHT
```

Diagonals:

```text
CAM_UP_LEFT
CAM_UP_RIGHT
CAM_DOWN_LEFT
CAM_DOWN_RIGHT
RS_UP_LEFT
RS_UP_RIGHT
RS_DOWN_LEFT
RS_DOWN_RIGHT
```

Right-stick click:

```text
TAP RESET_CAMERA
TAP RS
TAP R3
```

## D-pad And Style Tokens

```text
UP
DOWN
LEFT
RIGHT
STYLE_SM
STYLE_GS
STYLE_TS
STYLE_RG
STYLE_DS
```

For direct Dante style switching, prefer the `STYLE` command.

## Timing

`WAIT n` uses DMC4 input ticks, not milliseconds.

```text
HOLD MELEE
WAIT 1
RELEASE MELEE
```

This is the normal fastest long-form tap. Without `WAIT 1`, the game may not see the input as held.

## Basic Commands

### TAP

```text
TAP input
TAP input ticks
```

Examples:

```text
TAP MELEE
TAP JUMP
TAP EXCEED
TAP MELEE 3
```

### HOLD

```text
HOLD input
HOLD input+input
```

Examples:

```text
HOLD LOCK_ON
HOLD MA
HOLD LOCK_ON+MA
```

### RELEASE

```text
RELEASE input
RELEASE input+input
RELEASE ALL
```

Examples:

```text
RELEASE MELEE
RELEASE LOCK_ON+MA
RELEASE ALL
```

### WAIT

```text
WAIT ticks
```

Example:

```text
WAIT 10
```

### SET

Clears the current macro-held state, then holds the new input.

```text
SET LOCK_ON+MA
WAIT 5
RELEASE ALL
```

### CLEAR

Stops holding all macro-held input.

```text
CLEAR
```

## Helper Commands

### DIR

```text
DIR direction action ticks
```

Temporarily holds a direction and an action, then releases only those inputs.

```text
HOLD LOCK_ON
DIR MA MELEE 1
RELEASE LOCK_ON
```

Equivalent core input:

```text
HOLD MA
HOLD MELEE
WAIT 1
RELEASE MELEE
RELEASE MA
```

### BACK_FORWARD

```text
BACK_FORWARD back forward action
```

Performs back, then forward, then taps the action.

```text
HOLD LOCK_ON
BACK_FORWARD MD MA MELEE
RELEASE LOCK_ON
```

Equivalent core input:

```text
HOLD MD
WAIT 1
HOLD MA
RELEASE MD
WAIT 1
RELEASE MA
HOLD MELEE
WAIT 1
RELEASE MELEE
```

### Nero Helpers

```text
CALIBUR_RIGHT
CALIBUR_LEFT
SHUFFLE_RIGHT
SHUFFLE_LEFT
```

Use the version that matches the current facing/enemy direction.

## Dante Style Command

```text
STYLE SM
STYLE GS
STYLE TS
STYLE RG
STYLE DS
```

Full names:

```text
STYLE SWORDMASTER
STYLE GUNSLINGER
STYLE TRICKSTER
STYLE ROYALGUARD
STYLE DARKSLAYER
```

Numbers:

```text
0 SM
1 GS
2 TS
3 RG
4 DS
```

`STYLE RG` sets the style directly. To press the D-pad style input normally, use `TAP STYLE_RG` or `TAP DOWN`.

## Conditional Wait

```text
WAIT_UNTIL condition max_ticks
```

Supported conditions:

```text
WAIT_UNTIL ENEMY_STEP 120
WAIT_UNTIL GROUNDED 120
WAIT_UNTIL AIRBORNE 120
WAIT_UNTIL CAN_EXCEED 120
WAIT_UNTIL HITSTOP 120
WAIT_UNTIL HIT_CONFIRMED 120
WAIT_UNTIL LOCKED_ON 120
WAIT_UNTIL STYLE RG 120
```

If the condition becomes true earlier, playback continues immediately. If `max_ticks` is omitted, playback can wait indefinitely.

Examples:

```text
TAP MELEE
WAIT_UNTIL HIT_CONFIRMED 120
TAP JUMP
```

```text
CALIBUR_RIGHT
WAIT_UNTIL ENEMY_STEP 120
TAP JUMP
```

## Screen Freeze

```text
FREEZE
WAIT 30
UNFREEZE
```

Toggle:

```text
TOGGLE_FREEZE
```

## Character Switcher

```text
CHARACTER_SWITCH
SWITCH_CHARACTER
```

This requests one Character Switcher swap. It does not enable Character Switcher by itself.

Requirements:

- enable `Character Switcher` in the hook GUI before entering the stage
- wait until both characters have been created by Character Switcher
- do not use it in missions or moments where Character Switcher itself is unsafe

If Character Switcher is not enabled or ready, the command does nothing and the Macro status line reports it.

## Hook Actions

One Hit Kill:

```text
ONE_HIT_KILL ON
ONE_HIT_KILL OFF
ONE_HIT_KILL TOGGLE
OHK TOGGLE
```

`ON` enables One Hit Kill, `OFF` disables it, and `TOGGLE` flips the current setting.

## Short Examples

Nero Exceed:

```text
[F1] Exceed
TAP EXCEED
```

Nero Right Calibur:

```text
[V] Right Calibur
HOLD LOCK_ON
BACK_FORWARD MD MA MELEE
RELEASE LOCK_ON
```

Dante Split:

```text
[Ctrl+1] Split
HOLD LOCK_ON
DIR MA MELEE 1
RELEASE LOCK_ON
```

Dante Royalguard action:

```text
[F2] RG action
STYLE RG
WAIT 1
TAP STYLE_ACTION
```

Camera left:

```text
[F3] Camera left
HOLD CAM_LEFT
WAIT 20
RELEASE CAM_LEFT
```

## Script Troubleshooting

- Invalid token: check spelling and reload the macro file.
- Exceed becomes Bringer: use `EXCEED`.
- Camera does not move: use `CAM_LEFT` / `RS_LEFT`, not plain `RS`.
- Target does not change: use `CHANGE_TARGET` while locked on, and make sure Action Mapping matches your in-game controller settings.
- Directional move comes out on the wrong side: swap `MA` and `MD`, or swap `MW` and `MS`.

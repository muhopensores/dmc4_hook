# keyboard_macro*.txt Script Reference

Macro files are plain text. Blank lines are ignored. Lines starting with `#` are comments.

## Basic Format

The script is state-based:

```text
HOLD Y
WAIT 1
RELEASE Y
```

This means:

- press `Y`
- keep that state for 1 input tick
- release `Y`

Macro scripts describe game inputs, not physical keyboard keys. `Y` means the game's melee button label, `A` means jump, `R1` means lock-on, and so on. Movement should be written with the macro movement tokens explained below, especially `MW`, `MS`, `MA`, and `MD`.

`WAIT` uses input ticks, not milliseconds. One tick means one pass through the game's input update path. In normal play it is usually close to one rendered frame: about 1/60 second at 60 FPS, or about 1/120 second at 120 FPS. It is still better to think in "input updates" instead of real time, because this system is tied to the game's input layer.

## Clips and Hotkeys

A file can contain multiple clips. A clip starts with:

```text
[hotkey] clip name
```

Examples:

```text
[V] Right Calibur
CALIBUR_RIGHT

[Ctrl+1] Split
DIR MA Y 1

[Shift+F] Jump shot
TAP A
WAIT 1
TAP X
```

The clip name can use English, Chinese, or any text that helps you identify it.

## Buttons

Supported button names:

```text
A B X Y
L1 R1 L2 R2
START PAUSE MENU OPTIONS ESC ESCAPE
SELECT BACK
L3 R3
UP DOWN LEFT RIGHT
EXCEED EX REV MAX_ACT MAXACT
```

Default DMC4 meanings:

```text
Y  melee
X  gun
A  jump
B  Nero Bringer / Dante style action
L1 Devil Trigger
R1 lock-on
L2 Nero Exceed / Dante change gun
R2 Dante change sword
```

These are the game's default action labels. They are not meant to force you to use the default keyboard layout. If you changed DMC4's keyboard bindings in the game options, keep using your own layout; the macro file still names the in-game action buttons.

For Nero Exceed, `EXCEED` is the clearest spelling:

```text
TAP EXCEED
```

## Movement

Movement uses left-stick style directions. Do not write `HOLD W`, `HOLD S`, `HOLD A`, or `HOLD D` expecting physical keyboard keys. Use the explicit movement tokens below.

Recommended movement tokens:

```text
MW = move forward / up
MS = move back / down
MA = move left
MD = move right
```

Single directions:

```text
MW W FORWARD MOVE_UP
MS S BACK BACKWARD MOVE_DOWN
MA A MOVE_LEFT
MD D MOVE_RIGHT
```

The short aliases `W`, `S`, `A`, and `D` are accepted only as convenience aliases for movement tokens. They are not physical keyboard key presses, and they can be confusing next to the game button `A` for jump. For clarity, prefer `MW`, `MS`, `MA`, and `MD` in shared scripts and documentation.

Diagonal directions:

```text
MW_MD MOVE_UP_RIGHT
MW_MA MOVE_UP_LEFT
MS_MD MOVE_DOWN_RIGHT
MS_MA MOVE_DOWN_LEFT
```

These names are keyboard-like aliases. For lock-on attacks, use the direction relative to the enemy on screen, just like normal play.

## Commands

### HOLD

Holds one or more inputs until released.

```text
HOLD R1
HOLD MD
HOLD Y
WAIT 1
RELEASE Y
RELEASE MD
RELEASE R1
```

### RELEASE

Releases one or more inputs.

```text
RELEASE Y
RELEASE R1+MD
RELEASE ALL
```

### TAP

Presses an input for a short number of ticks, then releases it.

```text
TAP Y
TAP Y 3
TAP EXCEED
```

`TAP Y` is equivalent to a 1-tick tap.

### WAIT

Keeps the current held input state for a fixed number of input ticks.

```text
WAIT 10
```

`WAIT 10` means "keep the current macro state for 10 input updates". It does not mean 10 ms.

Examples:

```text
HOLD Y
RELEASE Y
```

Without a `WAIT`, the hold and release happen in the same macro step, so the game may never see a real held input.

```text
HOLD Y
WAIT 1
RELEASE Y
```

With `WAIT 1`, the game gets one input update where `Y` is held. This is the normal form for the fastest tap.

### CLEAR

Clears held macro input.

```text
CLEAR
```

## Helper Commands

### DIR

Direction plus button for a fixed number of ticks.

```text
DIR MA Y 1
```

This means hold left plus melee for 1 tick, then release both. For Dante, this can be used for Split when the direction matches your lock-on situation.

### BACK_FORWARD

Back, then forward, then button. This is useful for Nero Calibur and Shuffle style inputs.

```text
BACK_FORWARD MD MA Y
```

The first direction is held for 1 tick, then the second direction is held for 1 tick, then the button is tapped.

### Built-in Nero Helpers

```text
CALIBUR_RIGHT
CALIBUR_LEFT
SHUFFLE_RIGHT
SHUFFLE_LEFT
```

Use the right/left version that matches your enemy-facing situation.

## Dante Style

Use `STYLE` to force Dante's current style directly:

```text
STYLE SM
STYLE GS
STYLE TS
STYLE RG
STYLE DS
```

Aliases:

```text
FORCE_STYLE RG
SET_STYLE 3
```

Style numbers:

```text
0 SM
1 GS
2 TS
3 RG
4 DS
```

This avoids accidentally toggling into Dark Slayer by pressing the same D-pad style input twice.

## Conditional Wait

Conditional wait pauses the macro until a simple game-readable condition becomes true.

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

The final number is the maximum number of input ticks to wait. If the condition becomes true earlier, playback continues immediately. If the number is omitted, playback waits indefinitely.

Practical examples:

```text
TAP Y
WAIT_UNTIL HIT_CONFIRMED 120
TAP A
```

```text
CALIBUR_RIGHT
WAIT_UNTIL ENEMY_STEP 120
TAP A
```

## Screen Freeze

Screen freeze pauses the game simulation visually for observation.

```text
FREEZE
WAIT 30
UNFREEZE
```

Toggle form:

```text
TOGGLE_FREEZE
```

## Examples

### Nero Exceed Tap

```text
[F1] Exceed tap
TAP EXCEED
```

### Nero Right Calibur

```text
[V] Right Calibur
CALIBUR_RIGHT
```

### Dante Split

```text
[Ctrl+1] Split
HOLD R1
HOLD MA
HOLD Y
WAIT 1
RELEASE Y
RELEASE MA
RELEASE R1
```

### Dante Style Then Action

```text
[F2] RG action
STYLE RG
WAIT 1
TAP B
```

## Troubleshooting

- If a clip does nothing, click `Reload Macro File` and check the selected `Macro Clip`.
- If an input stays held, click `Stop Macro`.
- If hotkeys do nothing, make sure `Keyboard Macro` is enabled.
- If Nero Exceed behaves like Bringer, use a build where `EXCEED` and Nero `L2` use the dedicated Exceed path.

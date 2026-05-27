# macro*.txt Script Reference

This is the complete script reference for Macro. For install steps, GUI setup, Battle Snapshot usage, and first-time player flow, read `macro_release.md`.

Use this reference in this order:

1. `File Structure` explains how clips are declared and selected.
2. `Script Word Categories` explains the difference between input names, complete commands, and commands that take input arguments.
3. `Input Names` lists the words you can pass to `TAP`, `HOLD`, `RELEASE`, `SET`, `DIR`, and `BACK_FORWARD`.
4. `Commands` lists the script lines that do work.
5. `Timing And Conditional Wait` explains `WAIT`, `WAIT_UNTIL`, and overlay-based conditions.

## File Structure

```text
# comments are ignored
; semicolon comments are also ignored

[V][Nero] Nero clip
command
command

[V][X/SQUARE][Nero] Nero clip with controller shortcut
command
command

[Ctrl+1][Dante] Dante clip
command
```

Rules:

- blank lines are ignored
- text after `#` or `;` is ignored as a comment
- write one command per line
- commands must be inside a clip section
- every playable clip must start with `[KeyboardHotkey][Nero]` or `[KeyboardHotkey][Dante]`
- optional gamepad clip hotkeys use `[KeyboardHotkey][GamepadButton][Nero/Dante]`
- the character tag decides which Action Mapping table is used for action names
- clip hotkeys start the clip written in that header; GUI action hotkeys are configured in the GUI and run the currently selected file, clip, and snapshot action
- gamepad clip hotkeys always use `Back/Select` as the held prefix
- gamepad clip hotkeys consume `Back/Select` and the second shortcut button while `Gamepad Hotkeys` is enabled
- macro commands inject DMC4 game input, not Windows keyboard events

### Clip Headers

Header formats:

```text
[KeyboardHotkey][Character] Clip name
[KeyboardHotkey][GamepadButton][Character] Clip name
```

Examples:

```text
[V][Nero] Nero test
[V][X/SQUARE][Nero] Nero test with gamepad shortcut
[V][Dante] Dante test
[F1][RB/R1][Dante] Dante route with gamepad shortcut
[Ctrl+1][Nero] Boss route
[F8][Dante] RG punish
```

`Character` must be `Nero` or `Dante`. Short tags `N` and `D` are accepted, but the full names are clearer for shared files.

The same keyboard hotkey can be used once for Nero and once for Dante. Duplicate sections with the same keyboard hotkey and the same character are rejected.

`GamepadButton` is optional. It is the second button after the fixed `Back/Select` prefix, so `[V][X/SQUARE][Nero]` means holding `Back/Select` first, then pressing `X / Square`, starts that Nero clip. The keyboard hotkey `V` still works too. Duplicate sections with the same gamepad clip hotkey and the same character are rejected.

Accepted gamepad clip hotkey buttons include face buttons, bumpers, triggers, stick clicks, Start/Options, and D-pad directions. `Back/Select` itself is reserved as the prefix and cannot be the second button.

Slash names must describe the same physical button in different controller naming styles. For example, `[L2/LT]`, `[LT/L2]`, `[LT]`, and `[L2]` all mean the left trigger, but `[L2/LB]` is invalid because `L2` and `LB` are different buttons.

While `Gamepad Hotkeys` is enabled, detecting `Back/Select + GamepadButton` consumes the shortcut input so the game will not also receive it. The order matters: hold `Back/Select` first, then press the `GamepadButton`.

Keyboard hotkeys support:

| Hotkey type | Examples |
| --- | --- |
| Single letters or numbers | `V`, `1`, `Z` |
| Function keys | `F1` through `F24` |
| Numpad keys | `NUMPAD0` through `NUMPAD9` |
| Modifier combinations | `Ctrl+1`, `Shift+V`, `Alt+F2`, `Ctrl+Shift+F5` |
| Named keys | `Space`, `Tab`, `Enter`, `Esc`, `Backspace`, `Insert`, `Delete`, `Home`, `End`, `PageUp`, `PageDown`, arrow keys |

## Script Word Categories

Every non-header script line is one of these:

| Category | What it means | Examples |
| --- | --- | --- |
| Input names | Named actions, physical buttons, stick directions, camera directions, or D-pad directions. They describe an input, but do nothing by themselves. | `MELEE`, `LOCK_ON`, `MOVE_LEFT`, `CAM_LEFT`, `CHANGE_STYLE_RG` |
| Complete commands | Full script lines that run timing, direct macro behavior, or hook behavior by themselves. | `WAIT 10`, `WAIT_UNTIL CAN_EXCEED`, `STYLE RG`, `FREEZE`, `CHARACTER_SWITCH`, `ONE_HIT_KILL TOGGLE` |
| Commands with input arguments | Commands that perform, hold, release, or combine one or more input names. | `TAP MELEE`, `HOLD LOCK_ON`, `DIR MOVE_LEFT MELEE 1`, `BACK_FORWARD MOVE_RIGHT MOVE_LEFT MELEE` |

Wrong:

```text
MELEE
LOCK_ON
MOVE_LEFT
```

Right:

```text
TAP MELEE
HOLD LOCK_ON
HOLD MOVE_LEFT
```

## Quick Command Summary

### Input Names

| Input name type | Meaning | Examples | Correct use |
| --- | --- | --- | --- |
| Action names | Game actions read through Nero or Dante Action Mapping. Use these when the script should follow the player's layout. | `MELEE`, `JUMP`, `EXCEED`, `LOCK_ON`, `DEVIL_TRIGGER`, `CHANGE_TARGET` | `TAP MELEE`, `HOLD LOCK_ON`, `TAP DEVIL_TRIGGER` |
| Physical controller buttons | Exact virtual controller buttons. They ignore Action Mapping. | `Y`, `X`, `A`, `B`, `LB`, `RB`, `LT`, `RT`, `BACK`, `SELECT`, `TRIANGLE`, `CROSS` | `TAP Y`, `HOLD RB`, `TAP SELECT` |
| Movement | Virtual left-stick directions. Use these for walking, directional attacks, and lock-on directional inputs. | `MOVE_FORWARD`, `MOVE_BACK`, `MOVE_LEFT`, `MOVE_RIGHT` | `HOLD MOVE_LEFT`, `DIR MOVE_LEFT MELEE 1` |
| Slow movement | Light virtual left-stick directions. Use these when full stick movement is too strong. | `WALK_FORWARD`, `WALK_BACK`, `WALK_LEFT`, `WALK_RIGHT` | `HOLD WALK_RIGHT` |
| Camera | Virtual right-stick camera directions. These are not right-stick clicks. | `CAM_LEFT`, `CAM_RIGHT`, `RS_LEFT`, `RIGHT_STICK_RIGHT` | `HOLD CAM_LEFT`, `TAP RESET_CAMERA` |
| D-pad input | Exact virtual D-pad directions. `CHANGE_STYLE_*` names are readable aliases for those directions, not a separate Action Mapping system. | `UP`, `DOWN`, `LEFT`, `RIGHT`, `CHANGE_STYLE_SM`, `CHANGE_STYLE_RG` | `TAP CHANGE_STYLE_RG`, `TAP DOWN` |

### Complete Commands

| Command group | Forms to write | Example |
| --- | --- | --- |
| Timing | `WAIT ticks` | `WAIT 10` |
| Conditional wait | `WAIT_UNTIL condition [max_ticks]` | `WAIT_UNTIL CAN_EXCEED 120` |
| Dante style set | `STYLE style` | `STYLE RG` |
| Screen Freeze | `FREEZE`, `UNFREEZE`, `TOGGLE_FREEZE` | `FREEZE` |
| Character Switcher | `CHARACTER_SWITCH`, `SWITCH_CHARACTER` | `CHARACTER_SWITCH` |
| Hook actions | `ONE_HIT_KILL ON`, `ONE_HIT_KILL OFF`, `ONE_HIT_KILL TOGGLE`, `OHK TOGGLE` | `ONE_HIT_KILL TOGGLE` |
| Clear held macro input | `CLEAR` | `CLEAR` |

### Commands With Input Arguments

| Command form | What the input words mean | Example |
| --- | --- | --- |
| `TAP input [ticks]` | `input` is an action, button, movement, camera, or D-pad input name. | `TAP MELEE`, `TAP EXCEED`, `TAP MELEE 3` |
| `HOLD input` | `input` is held until released or cleared. | `HOLD LOCK_ON`, `HOLD LOCK_ON+MOVE_LEFT` |
| `RELEASE input` | releases a previously held input. | `RELEASE LOCK_ON`, `RELEASE LOCK_ON+MOVE_LEFT` |
| `SET input` | clears the current macro-held state, then holds this input. | `SET LOCK_ON+MOVE_LEFT` |
| `DIR direction action ticks` | `direction` is a movement input, `action` is an action or button input. | `DIR MOVE_LEFT MELEE 1` |
| `BACK_FORWARD back forward action` | `back` and `forward` are movement inputs, `action` is an action or button input. | `BACK_FORWARD MOVE_RIGHT MOVE_LEFT MELEE` |

## Input Names

Input names describe a controller action, physical button, stick direction, camera direction, or D-pad direction. They do not do anything by themselves. Use them after commands such as `TAP`, `HOLD`, `RELEASE`, `SET`, `DIR`, or `BACK_FORWARD`.

### Character Action Names

Action names mean "press the button assigned to this action in the GUI Action Mapping table." They are usually the best choice for shared scripts because they follow each player's controller layout.

They also make scripts easier to read. `TAP MELEE` or `TAP DEVIL_TRIGGER` describes the intended game action, so the script usually needs fewer comments than a physical-button-only version such as `TAP Y` or `TAP LB`.

Nero and Dante have separate Action Mapping tables. The clip header chooses the table:

```text
[V][Nero]  uses Nero Action Mapping
[V][Dante] uses Dante Action Mapping
```

Shared action names are accepted by both characters. The script word is the same, but the output button is read from that character's table:

```text
[F1][Nero] Nero melee
TAP MELEE

[F1][Dante] Dante melee
TAP MELEE
```

If Nero Action Mapping has `MELEE = Y / TRIANGLE` and Dante Action Mapping has `MELEE = B / CIRCLE`, the Nero clip presses `Y / TRIANGLE` and the Dante clip presses `B / CIRCLE`.

Character-only action names are different. `EXCEED` and `BRINGER` only make sense in `[Nero]` clips. `STYLE_ACTION`, `CHANGE_GUN`, and `CHANGE_SWORD` only make sense in `[Dante]` clips. If an action is not valid for the clip's character, the macro file will fail to load that line.

Action Mapping can point an action to face buttons, bumpers, triggers, stick clicks, Back/Select, Start/Options, or D-pad directions. If your DMC4 controller settings put an action on D-pad, choose the same D-pad direction in the matching Nero or Dante Action Mapping table.

Physical button names do not use Action Mapping. `TAP Y` always presses `Y / TRIANGLE`, even if Nero or Dante melee is set to `B / CIRCLE` in DMC4. `TAP MELEE` presses the current character's mapped melee button.

| Action name | Valid in | Aliases | Default Nero | Default Dante | Notes |
| --- | --- | --- | --- | --- | --- |
| `MELEE` | Nero, Dante | `MELEE_ATTACK`, `SWORD_ATTACK`, `ATTACK` | `Y / TRIANGLE` | `Y / TRIANGLE` | The normal close-range attack button for the current character. |
| `GUN` | Nero, Dante | `GUN_ATTACK`, `SHOOT`, `FIRE` | `X / SQUARE` | `X / SQUARE` | The normal gun/shoot button for the current character. |
| `EXCEED` | Nero | `REV`, `NERO_EXCEED`, `EXCEED_INPUT`, `MAX_ACT`, `MAXACT` | `LT / L2` | not valid | Nero's Red Queen rev/Exceed action. The game timing decides whether the result is a normal rev, Ex-Act, or Max-Act. |
| `JUMP` | Nero, Dante | none | `A / CROSS` | `A / CROSS` | Jump. |
| `BRINGER` | Nero | `DEVIL_BRINGER` | `B / CIRCLE` | not valid | Nero's Devil Bringer/Buster action button. |
| `STYLE_ACTION` | Dante | `STYLE_BUTTON` | not valid | `B / CIRCLE` | Dante's current-style action button, such as Trickster dash, Swordmaster extra attack, Royalguard guard, or Gunslinger action. |
| `DEVIL_TRIGGER` | Nero, Dante | `DT` | `LB / L1` | `LB / L1` | Devil Trigger action. |
| `LOCK_ON` | Nero, Dante | `LOCKON`, `LOCK`, `TARGET_LOCK` | `RB / R1` | `RB / R1` | Lock-on/targeting button. Hold this for lock-on directional moves. |
| `CHANGE_GUN` | Dante | `GUN_CHANGE`, `DANTE_CHANGE_GUN` | not valid | `LT / L2` | Dante gun weapon switch. |
| `CHANGE_SWORD` | Dante | `SWORD_CHANGE`, `DANTE_CHANGE_SWORD` | not valid | `RT / R2` | Dante melee weapon switch. |
| `TAUNT` | Nero, Dante | none | `BACK / SHARE` | `BACK / SHARE` | Taunt action. |
| `CHANGE_TARGET` | Nero, Dante | `TARGET_CHANGE`, `CYCLE_TARGET`, `NEXT_TARGET` | `LS / L3` | `LS / L3` | Change target while locked on. |
| `RESET_CAMERA` | Nero, Dante | `CAMERA_RESET`, `CENTER_CAMERA` | `RS / R3` | `RS / R3` | Reset/center camera action. |

Important:

- Use action names when you want the script to follow the player's Nero/Dante controller layout.
- Use physical button names when you deliberately want one exact controller button.
- If a row says `not valid` for the current character, that action name cannot be used in that character's clip.
- Shared names such as `MELEE` can map to different buttons for Nero and Dante.
- For new shared scripts, prefer the main action name in the first column. The alias column is mainly for reading older scripts.

### Physical Controller Buttons

Physical button names ignore Action Mapping and press the named virtual controller button directly.

| Button group | Accepted names |
| --- | --- |
| Back / Select | `BACK`, `BACK_BUTTON`, `SELECT`, `SELECT_BUTTON`, `VIEW`, `SHARE`, `SHARE_BUTTON` |
| Start / Options | `START`, `START_BUTTON`, `OPTIONS`, `OPTIONS_BUTTON`, `MENU`, `ESC`, `ESCAPE` |
| Stick clicks | `LS`, `L3`, `LEFT_STICK_CLICK`, `RS`, `R3`, `RIGHT_STICK_CLICK` |
| Bumpers | `LB`, `L1`, `LEFT_BUMPER`, `RB`, `R1`, `RIGHT_BUMPER` |
| Triggers | `LT`, `L2`, `LEFT_TRIGGER`, `RT`, `R2`, `RIGHT_TRIGGER` |
| Face buttons | `Y`, `TRIANGLE`, `TRI`, `X`, `SQUARE`, `SQ`, `A`, `CROSS`, `B`, `CIRCLE`, `O` |
| D-pad | `UP`, `DOWN`, `LEFT`, `RIGHT`, `DPAD_UP`, `DPAD_DOWN`, `DPAD_LEFT`, `DPAD_RIGHT` |

Common mistakes:

- `X` means Xbox X / PlayStation Square. PlayStation Cross is `CROSS`.
- `A` means Xbox A / PlayStation Cross. Move left is `MOVE_LEFT`.
- `BACK` means physical Back/Select. Move back is `MOVE_BACK`.
- `RS` means right stick click. Camera left is `CAM_LEFT`, `RS_LEFT`, or `RIGHT_STICK_LEFT`.

### Stick And D-Pad Input Names

#### Movement

Movement tokens are input names for virtual left-stick directions, not physical keyboard keys.

| Name | Meaning |
| --- | --- |
| `MOVE_FORWARD` | Push the virtual left stick forward/up. |
| `MOVE_BACK` | Pull the virtual left stick back/down. |
| `MOVE_LEFT` | Push the virtual left stick left. |
| `MOVE_RIGHT` | Push the virtual left stick right. |

Extra aliases:

```text
MOVE_UP
MOVE_DOWN
FORWARD
BACKWARD
LSTICK_UP
LSTICK_DOWN
LSTICK_LEFT
LSTICK_RIGHT
```

Diagonals:

```text
MOVE_FORWARD_RIGHT
MOVE_FORWARD_LEFT
MOVE_BACK_RIGHT
MOVE_BACK_LEFT
MOVE_UP_RIGHT
MOVE_UP_LEFT
MOVE_DOWN_RIGHT
MOVE_DOWN_LEFT
```

#### Slow Movement

Slow walk tokens use a light left-stick push:

```text
WALK_FORWARD
WALK_BACK
WALK_LEFT
WALK_RIGHT
WALK_UP
WALK_DOWN
```

`SLOW_*` is accepted as an equivalent spelling, for example `SLOW_LEFT`.

Slow diagonals:

```text
WALK_FORWARD_RIGHT
WALK_FORWARD_LEFT
WALK_BACK_RIGHT
WALK_BACK_LEFT
WALK_UP_RIGHT
WALK_UP_LEFT
WALK_DOWN_RIGHT
WALK_DOWN_LEFT
```

#### Camera

Camera tokens are input names for right-stick camera movement:

| Name | Meaning |
| --- | --- |
| `CAM_UP`, `RS_UP`, `RIGHT_STICK_UP` | Push the virtual right stick up. |
| `CAM_DOWN`, `RS_DOWN`, `RIGHT_STICK_DOWN` | Push the virtual right stick down. |
| `CAM_LEFT`, `RS_LEFT`, `RIGHT_STICK_LEFT` | Push the virtual right stick left. |
| `CAM_RIGHT`, `RS_RIGHT`, `RIGHT_STICK_RIGHT` | Push the virtual right stick right. |

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

#### D-Pad Input

These are exact virtual D-pad directions. They do not check Dante's controller settings, and they do not mean "change style" by themselves. If the current character has an action mapped to D-pad in DMC4, pressing that D-pad direction can trigger that action.

`CHANGE_STYLE_*` names are readable aliases for the default Dante D-pad style layout. They still press ordinary D-pad directions. If your Dante style-change controls are not on D-pad, use the physical button or action name that matches your layout instead.

| Name | Meaning |
| --- | --- |
| `UP`, `DPAD_UP`, `CHANGE_STYLE_TS` | Press D-pad Up. |
| `RIGHT`, `DPAD_RIGHT`, `CHANGE_STYLE_SM` | Press D-pad Right. |
| `DOWN`, `DPAD_DOWN`, `CHANGE_STYLE_RG` | Press D-pad Down. |
| `LEFT`, `DPAD_LEFT`, `CHANGE_STYLE_GS` | Press D-pad Left. |

Examples:

```text
TAP CHANGE_STYLE_RG
TAP DOWN
STYLE RG
```

`TAP CHANGE_STYLE_RG` presses D-pad Down. `STYLE RG` is a complete command that changes Dante's current style directly.

## Commands

Commands are complete script lines. Some commands take input names as arguments, and some commands run built-in macro or hook behavior by themselves.

### Input Commands

These commands use the input names from the previous section.

#### TAP

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
TAP LOCK_ON+MELEE 2
```

If `ticks` is omitted, the tap lasts 1 input tick.

#### HOLD

```text
HOLD input
HOLD input+input
```

Alias:

```text
PRESS input
```

Examples:

```text
HOLD LOCK_ON
HOLD MOVE_LEFT
HOLD LOCK_ON+MOVE_LEFT
```

Held input remains active through later `WAIT` lines until released, cleared, or stopped.

#### RELEASE

```text
RELEASE input
RELEASE input+input
RELEASE ALL
```

Alias:

```text
REL input
```

Examples:

```text
RELEASE MELEE
RELEASE LOCK_ON+MOVE_LEFT
RELEASE ALL
```

#### SET

`SET` clears the current macro-held state, then holds the new input.

```text
SET LOCK_ON+MOVE_LEFT
WAIT 5
RELEASE ALL
```

#### CLEAR

`CLEAR` stops holding all macro-held input.

```text
CLEAR
```

#### DIR

```text
DIR direction action ticks
```

Alias:

```text
DIRECTION direction action ticks
```

Temporarily holds a direction and an action, then releases only those inputs.

```text
HOLD LOCK_ON
DIR MOVE_LEFT MELEE 1
RELEASE LOCK_ON
```

Equivalent core input:

```text
HOLD MOVE_LEFT
HOLD MELEE
WAIT 1
RELEASE MELEE
RELEASE MOVE_LEFT
```

#### BACK_FORWARD

```text
BACK_FORWARD back forward action
```

Aliases:

```text
BACKFORWARD back forward action
BF back forward action
```

Performs back, then forward, then taps the action.

```text
HOLD LOCK_ON
BACK_FORWARD MOVE_RIGHT MOVE_LEFT MELEE
RELEASE LOCK_ON
```

Equivalent core input:

```text
HOLD MOVE_RIGHT
WAIT 1
HOLD MOVE_LEFT
RELEASE MOVE_RIGHT
WAIT 1
RELEASE MOVE_LEFT
HOLD MELEE
WAIT 1
RELEASE MELEE
```

### Complete Commands

#### Dante Style Command

`STYLE` is a complete command for setting Dante's current style:

```text
STYLE SM
STYLE GS
STYLE TS
STYLE RG
STYLE DS
```

Aliases for the command:

```text
FORCE_STYLE RG
SET_STYLE RG
FORCE_STYLE DS
SET_STYLE DS
```

Full style names:

```text
STYLE SWORDMASTER
STYLE GUNSLINGER
STYLE TRICKSTER
STYLE ROYALGUARD
STYLE DARKSLAYER
```

`STYLE DS` / `STYLE DARKSLAYER` can directly set Dark Slayer even though the default D-pad style layout has no Dark Slayer direction. `STYLE RG` changes Dante's current style directly. `CHANGE_STYLE_RG` is only a readable name for D-pad Down, so use `TAP CHANGE_STYLE_RG` or `TAP DOWN` if you want to press that D-pad direction normally. `CHANGE_STYLE_RG` does not check Dante's controller settings and does not force the style by itself.

#### Screen Freeze

These are complete command lines:

```text
FREEZE
WAIT 30
UNFREEZE
```

Aliases:

```text
SCREEN_FREEZE
SCREEN_PAUSE
TRAINER_PAUSE
FRAME_PAUSE
RESUME
UNPAUSE
SCREEN_RESUME
SCREEN_UNPAUSE
TOGGLE_FREEZE
TOGGLE_PAUSE
SCREEN_PAUSE_TOGGLE
```

Use:

```text
TOGGLE_FREEZE
```

to flip the current freeze state.

#### Character Switcher

These are complete command lines:

```text
CHARACTER_SWITCH
SWITCH_CHARACTER
```

Aliases:

```text
CHAR_SWITCH
CHARACTER_SWAP
SWAP_CHARACTER
```

This requests one Character Switcher swap. It does not enable Character Switcher by itself.

Requirements:

- enable `Character Switcher` in the hook GUI before entering the stage
- wait until both characters have been created by Character Switcher
- do not use it in missions or moments where Character Switcher itself is unsafe

If Character Switcher is not enabled or ready, the command does nothing and the Macro status line reports it.

#### Hook Actions

One Hit Kill:

```text
ONE_HIT_KILL ON
ONE_HIT_KILL OFF
ONE_HIT_KILL TOGGLE
OHK TOGGLE
```

Aliases:

```text
ONEHITKILL ON
OHK ON
OHK OFF
OHK ENABLE
OHK DISABLE
```

`ON` enables One Hit Kill, `OFF` disables it, and `TOGGLE` flips the current setting. `ENABLE`, `ENABLED`, `TRUE`, and `1` also mean on. `DISABLE`, `DISABLED`, `FALSE`, and `0` also mean off.

## Timing And Conditional Wait

### WAIT

`WAIT n` uses DMC4 input ticks, not milliseconds.

```text
HOLD MELEE
WAIT 1
RELEASE MELEE
```

This is the normal fastest long-form tap. Without `WAIT 1`, the game may not see the input as held.

One tick is one pass through DMC4's input update path. In normal play it is usually close to one rendered frame.

### WAIT_UNTIL

`WAIT_UNTIL` pauses macro playback until a game state becomes true, then continues on the same script.

```text
WAIT_UNTIL condition [max_ticks]
WAIT_UNTIL STYLE style [max_ticks]
WAIT_UNTIL ANIM_FRAME [operator] frame [max_ticks]
WAIT_UNTIL MOVEID2 [operator] value [max_ticks]
```

Aliases:

```text
WAITUNTIL
UNTIL
```

If the condition becomes true earlier, playback continues immediately. `max_ticks` is a safety timeout measured in input ticks. If `max_ticks` is omitted, playback can wait indefinitely.

How to read these templates:

- Words such as `condition`, `style`, `operator`, `frame`, `value`, and `max_ticks` are placeholders. Replace them with real script text.
- Square brackets mean optional. For example, `[max_ticks]` means you may write a timeout such as `120`, or leave it out.
- `WAIT_UNTIL condition [max_ticks]` is for simple yes/no conditions such as `GROUNDED`, `CAN_EXCEED`, `HITSTOP`, or `LOCKED_ON`.
- `WAIT_UNTIL STYLE style [max_ticks]` waits for a Dante style. Replace `style` with `SM`, `GS`, `TS`, `RG`, or `DS`.
- `WAIT_UNTIL ANIM_FRAME [operator] frame [max_ticks]` waits for the current `Frame` number shown by Animation Overlay.
- `WAIT_UNTIL MOVEID2 [operator] value [max_ticks]` waits for the current `MoveID2` shown by Animation Overlay. Decimal and hex values are both accepted.

### Supported Conditions

| Script condition | Meaning | Common use |
| --- | --- | --- |
| `ENEMY_STEP` | Enemy Step / jump-cancel window is available. Aliases: `JC`, `JUMP_CANCEL`. | Jump as soon as an Enemy Step can happen. |
| `GROUNDED` | Player is currently grounded. Alias: `GROUND`. | Continue after landing. |
| `AIRBORNE` | Player is currently airborne. Aliases: `AIR`, `IN_AIR`. | Continue after leaving the ground. |
| `STYLE style` | Current Dante style equals `SM`, `GS`, `TS`, `RG`, or `DS`. | Wait until a style switch has applied. |
| `CAN_EXCEED` | Nero's next Ex-Act/Max-Act timing window after this wait begins. Alias: `EXCEED_WINDOW`. | Press Exceed at the earliest reliable timing after an attack. |
| `HITSTOP` | Player is in hitstop or has a hitstop timer. Alias: `HIT_STOP`. | Continue when hitstop begins. |
| `HIT_CONFIRMED` | Nearby target damage/stun/displacement changed after the wait began. Aliases: `HIT`, `ENEMY_HIT`, `DAMAGE`, `DAMAGE_REGISTERED`. | Continue only after an attack connects. |
| `LOCKED_ON` | Player is currently locked on. Aliases: `LOCKON`, `LOCK_ON`. | Wait before lock-on dependent movement. |
| `ANIM_FRAME` | Current runtime animation frame from Animation Overlay. Aliases: `FRAME`, `ANIMFRAME`, `ANIMATION_FRAME`. | Continue at a specific shown frame. |
| `MOVEID2` | Runtime `MoveID2` value from Animation Overlay. Aliases: `MOVEID`, `MOVE_ID`, `MOVE_ID2`. | Continue during a specific move/action state. |
| `MOVEID2_CHANGED` | Captures current `MoveID2` when the wait begins, then continues after it changes. Aliases: `MOVE_CHANGED`, `MOVE_ID_CHANGED`. | Continue when the current action changes to another state. |
| `FRAME_REACHED_MAX` | Current `animFrame` reached the runtime `animFrameMax`. Aliases: `FRAME_MAX`, `ANIM_FRAME_MAX`. | Approximate "animation reached its listed end" checks. |

### Comparison Operators

Comparison operators are supported for `ANIM_FRAME` and `MOVEID2`:

```text
==  !=  >  >=  <  <=
```

These signs mean:

| Sign | How to read it | Example meaning |
| --- | --- | --- |
| `==` | equals | `MOVEID2 == 30` means "MoveID2 is exactly 30". |
| `!=` | does not equal | `MOVEID2 != 30` means "MoveID2 is anything except 30". |
| `>` | greater than | `ANIM_FRAME > 28` means "frame is later than 28". |
| `>=` | greater than or equal to | `ANIM_FRAME >= 28` means "frame has reached 28 or later". |
| `<` | less than | `ANIM_FRAME < 28` means "frame is still before 28". |
| `<=` | less than or equal to | `ANIM_FRAME <= 28` means "frame is 28 or earlier". |

Word spellings such as `EQ`, `NOT_EQUAL`, `GREATER_THAN_OR_EQUAL`, and `LESS_THAN_OR_EQUAL` are also accepted, but the signs are shorter.

`ANIM_FRAME` compares the current runtime animation frame as a decimal number. If the operator is omitted, it defaults to `>=`:

```text
WAIT_UNTIL ANIM_FRAME 28 120
TAP MELEE
```

This is the same as:

```text
WAIT_UNTIL ANIM_FRAME >= 28 120
TAP MELEE
```

It means: wait until the shown animation frame reaches `28` or later. If that does not happen within `120` input ticks, continue anyway.

`MOVEID2` compares the runtime value shown by Animation Overlay. If the operator is omitted, it defaults to `==`:

```text
WAIT_UNTIL MOVEID2 0x0000001E 120
TAP JUMP
```

This is the same as:

```text
WAIT_UNTIL MOVEID2 == 0x0000001E 120
TAP JUMP
```

It means: wait until the shown `MoveID2` is exactly `0x0000001E`. If that does not happen within `120` input ticks, continue anyway.

Decimal values also work:

```text
WAIT_UNTIL MOVEID2 == 30 120
```

`MOVEID2_CHANGED` captures the current `MoveID2` when the wait begins and continues after it changes. Put it after the command that starts the action you want to leave:

```text
TAP MELEE
WAIT_UNTIL MOVEID2_CHANGED 120
TAP MELEE
```

`FRAME_REACHED_MAX` checks whether `animFrame` has reached the runtime `animFrameMax` value. This is a frame display condition, so confirm the result in-game when using it to time the next action.

## Animation Overlay Reference

Enable `System -> Custom HUD elements -> Animation Overlay` to read runtime values while playing. The script names below use the same data shown by that overlay.

| Overlay field | Script condition | Example |
| --- | --- | --- |
| `Frame: 28.0 / 61.0` | `ANIM_FRAME` compares the left/current value. | `WAIT_UNTIL ANIM_FRAME >= 28 120` |
| `Frame: current / max` | `FRAME_REACHED_MAX` compares current frame against max. | `WAIT_UNTIL FRAME_REACHED_MAX 120` |
| `MoveID2: 0x0000001E` | `MOVEID2` compares the shown value. Hex and decimal are both accepted. | `WAIT_UNTIL MOVEID2 == 0x0000001E 120` |
| `MoveID2` changes after a command | `MOVEID2_CHANGED` waits for any different `MoveID2`. | `WAIT_UNTIL MOVEID2_CHANGED 120` |
| `State: grounded` | `GROUNDED` | `WAIT_UNTIL GROUNDED 180` |
| `State: airborne` | `AIRBORNE` | `WAIT_UNTIL AIRBORNE 60` |
| `State: hitstop` | `HITSTOP` | `WAIT_UNTIL HITSTOP 60` |
| `State: locked-on` | `LOCKED_ON` | `WAIT_UNTIL LOCKED_ON 30` |

The overlay is read-only. It helps you observe the current player state while you practice or tune scripts.

```text
Anim:   current animation ID
MoveID2: runtime action/state value
Part:   action phase value
Frame:  current animation frame / max frame
State:  grounded or airborne, plus hitstop and lock-on flags when active
```

Field notes:

- `Anim` is the current runtime animation ID. It is useful for comparing whether two actions use the same animation, but it is not shown as a move name.
- `MoveID2` is a runtime action/state value shown as an 8-digit hex number. Use it with `WAIT_UNTIL MOVEID2` or `WAIT_UNTIL MOVEID2_CHANGED`.
- `Part` is a phase value inside the current action. It can help you see that an action moved from one part to another, but it is not a universal startup/active/recovery label.
- `Frame` is the current runtime animation frame. When the max frame value is available, the overlay shows `current / max`, such as `Frame: 12.5 / 38.0`.
- `State` shows quick flags such as `grounded`, `airborne`, `hitstop`, and `locked-on`.

`Show Advanced Animation Fields` also displays:

```text
Bank:          animation bank value
Ground Raw:    raw player ground state
Ground2:       secondary player ground flag
Land Flag:     collision land flag
Hitstop Timer: current hitstop timer value
```

Advanced field notes:

- `Ground Raw` is the main value used for grounded/airborne display. In current builds, `1` is treated as grounded and `2` as airborne.
- `Ground2` is a secondary ground flag used as a fallback.
- `Land Flag` can remain set briefly during jumps, so use the main `State` display first for normal scripts.
- `Hitstop Timer` helps you see when timing is being paused or delayed by hitstop.

Recommended tuning workflow:

1. Turn on Animation Overlay.
2. Perform the action manually or play the macro once.
3. Note the displayed `Frame` and `MoveID2` values at the moment you want.
4. Add the matching `WAIT_UNTIL` condition to the script.
5. Keep a timeout such as `120` or `180` while tuning so a missed condition does not lock the macro forever.

## Common Recipes

Triple Lv3 EX melee chain:

```text
[V][X/SQUARE][Nero] Triple EX melee
TAP MELEE
WAIT_UNTIL CAN_EXCEED
TAP EXCEED
TAP MELEE
WAIT_UNTIL CAN_EXCEED
TAP EXCEED
TAP MELEE
WAIT_UNTIL CAN_EXCEED
TAP EXCEED
```

Devil Trigger after a back-forward melee input finishes. `160` is used here because `120` can be too short for this `MOVEID2` change:

```text
[F1][RB/R1][Nero] Back-forward melee to DT
HOLD LOCK_ON
WAIT 1
BACK_FORWARD MOVE_RIGHT MOVE_LEFT MELEE
WAIT_UNTIL MOVEID2_CHANGED 160
TAP DEVIL_TRIGGER
WAIT 1
RELEASE LOCK_ON
```

Shuffle after Split. The `WAIT 15` after landing is important; sending the next directional input earlier can be too early for the game to accept it:

```text
[F2][Y/TRIANGLE][Nero] Split to Shuffle
HOLD LOCK_ON
WAIT 1
DIR MOVE_LEFT Y 1
WAIT_UNTIL GROUNDED
WAIT 15
BACK_FORWARD MOVE_RIGHT MOVE_LEFT MELEE
WAIT 10
RELEASE LOCK_ON
```

Act when the shown animation frame reaches 28:

```text
[F3][A/CROSS][Nero] Frame wait
TAP MELEE
WAIT_UNTIL ANIM_FRAME >= 28 120
TAP MELEE
```

Act when a specific `MoveID2` appears:

```text
[F4][B/CIRCLE][Nero] MoveID2 wait
TAP MELEE
WAIT_UNTIL MOVEID2 == 0x0000001E 120
TAP JUMP
```

Act after landing:

```text
[F5][LT/L2][Nero] Landing action
TAP JUMP
WAIT_UNTIL GROUNDED 180
TAP MELEE
```

Enemy Step as soon as possible:

```text
[F6][RT/R2][Nero] Enemy Step
TAP MELEE
WAIT_UNTIL ENEMY_STEP 120
TAP JUMP
```

Only continue after an attack connects:

```text
[F7][LB/L1][Nero] Hit confirm
TAP MELEE
WAIT_UNTIL HIT_CONFIRMED 120
TAP JUMP
```

Dante Royalguard action:

```text
[F8][B/CIRCLE][Dante] RG action
STYLE RG
WAIT 1
TAP STYLE_ACTION
```

Short screen freeze:

```text
[F9][DPAD_UP][Nero] Freeze test
FREEZE
WAIT 30
UNFREEZE
```

Move, slow walk, and camera control:

```text
[F10][START/OPTIONS][Nero] Movement and camera
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

## Script Troubleshooting

- Invalid token: check spelling, character tag, and Action Mapping validity, then save the macro file so it auto-reloads.
- `Line X: section header must include [Nero] or [Dante]`: add a character tag after the hotkey.
- `Line X: invalid HOLD/TAP/RELEASE input`: the input name is misspelled or not valid for the clip's character.
- Exceed becomes the wrong action: make sure the clip header is `[...][Nero]`, use `EXCEED` / `REV`, and set Nero Action Mapping `EXCEED` to your in-game Exceed button.
- `TAP L2` behaves differently from `TAP EXCEED`: `L2` is the physical trigger; `EXCEED` is the Nero action name.
- Camera does not move: use `CAM_LEFT` / `RS_LEFT`, not plain `RS`.
- Target does not change: use `CHANGE_TARGET` while locked on, and make sure the clip's Nero/Dante Action Mapping table matches your in-game controller settings.
- Directional move comes out on the wrong side: swap `MOVE_LEFT` and `MOVE_RIGHT`, or swap `MOVE_FORWARD` and `MOVE_BACK`.
- `WAIT_UNTIL` never continues: add a timeout while tuning, confirm the value with Animation Overlay, and check whether the condition is for Nero, Dante, grounded state, or hitstop.

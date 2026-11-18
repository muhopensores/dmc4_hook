#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Event scripts (cEventScript): the bytecode rooms and missions run for doors, triggers, enemy waves, placements, fades
// (Vibed/RE/event_script.md). Room classes (sdk/RoomClasses) return them from vtable slots 11 (run once while the room
// loads) and 12 (run every frame, restarted at its end). This file has no game code: sizes, a check, and a text form.
//
// Text form: one instruction per line, `#` starts a comment, operands separated by spaces or commas, numbers decimal or
// 0x hex. Control codes IF / NIF / AND_IF / AND_NIF / OR_IF / OR_NIF take a condition (CK_DOOR ...); DEFAULT, ELSE,
// END_IF, BREAK, SWITCH, END_SWITCH, END. Opcodes by name where their operands are known (ROOM_CHANGE room door ...),
// any opcode as OP_xx with its operand bytes, any condition as C_xx with its bytes. Example (a door):
//     IF CK_DOOR 0
//       ROOM_CHANGE 200 1
//     END_IF
//     END
namespace event_script {

constexpr int kMaxDx9Code = 0x6F;      // DX9 opcodes 0x00-0x6F (SE adds 0x70-0x7E)

int code_size(uint8_t op);             // whole instruction size in bytes, 0 = not an opcode
int condition_size(uint8_t cond);      // the condition with its operands (no control byte), 0 = unknown

// Walks the script as the game would, to its END (0xFF). `length` = its size with the END. Fails on an unknown or
// SE-only opcode / condition, a truncated instruction, or a block closed that wasn't opened.
bool validate(const uint8_t* data, size_t size, std::string* error = nullptr, size_t* length = nullptr);

// Text, one instruction a line, indented by block; stops at END (or at an error, as a `# error:` line).
std::string disassemble(const uint8_t* data, size_t size);

// Text -> bytes (an END is added if the text has none). Errors name the line.
bool assemble(const std::string& text, std::vector<uint8_t>& out, std::string* error = nullptr);

} // namespace event_script

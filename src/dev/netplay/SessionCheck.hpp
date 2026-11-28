#pragma once
// session_check - what every machine of an online session has to have the same (Vibed\Coop\online.md "Mods and
// settings"). The code: the game exe, dmc4_hook.dll and each running plugin DLL, by content hash, never by version
// (players build their own DLLs). The lobby sends each machine's list to the host and the host's to every guest
// (lobby::Blob::Code); a difference blocks the host's Start for everyone.
#include <string>
#include <vector>

namespace session_check {

// "dll <hash>", "exe <hash>", "plugin <file> <hash>" lines, sorted; the files are hashed once (they don't change while
// the game runs)
const std::string& code_text();

// What differs between this machine's list and another's, by name ("dmc4_hook.dll", "plugin X.dll (only on player 2)").
// `them` names the other machine in the text ("player 2", "the host").
std::vector<std::string> differences(const std::string& mine, const std::string& theirs, const char* them);

} // namespace session_check

#ifndef HACKLIB_INJECTOR_H
#define HACKLIB_INJECTOR_H

#include <string>
#include <vector>


namespace hl
{
// If "error" is provided, any warnings and errors are appended to it.
bool Inject(int pid, const std::string& libFileName, std::string* error);
std::vector<int> GetPIDsByProcName(const std::string& pname);
}

#endif
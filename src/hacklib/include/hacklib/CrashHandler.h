#ifndef HACKLIB_CRASHHANDLER_H
#define HACKLIB_CRASHHANDLER_H

#include <cstdint>
#include <functional>


namespace hl
{
/**
 * Runs the body in a protected environment that catches native platform errors.
 * If the body causes an error, the handler is executed. The argument of the
 * handler is the native error code: NTSTATUS on Windows, signal id on Linux.
 * The errors are SEH exceptions on Windows and selected signals on Linux.
 * Note that no actions of the body will be reverted. No destructors are called.
 * \param body The function to run.
 * \param handler The function to execute if an error was handled.
 */
void CrashHandler(const std::function<void()>& body, const std::function<void(uint32_t)>& handler);

/// \overload
/// \return False if an error was handled.
bool CrashHandler(const std::function<void()>& body);
}

#endif
#include "hacklib/MessageBox.h"
#include <Windows.h>


void hl::MsgBox(const std::string& title, const std::string& message)
{
    MessageBoxA(NULL, message.c_str(), title.c_str(), MB_SYSTEMMODAL);
}
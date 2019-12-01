#include "hacklib/MessageBox.h"
#include <cstdio>


void hl::MsgBox(const std::string& title, const std::string& message)
{
    printf("hl::MsgBox: %s: %s\n", title.c_str(), message.c_str());
}

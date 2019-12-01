#ifndef HACKLIB_INPUT_H
#define HACKLIB_INPUT_H

#include <array>


namespace hl
{
class Input
{
public:
    void update();

    bool isDown(int vkey) const;
    bool wentDown(int vkey) const;
    bool wentUp(int vkey) const;

private:
    enum class InputState
    {
        Idle,
        WentDown,
        WentUp
    };

    struct InputStatus
    {
        bool isDown = false;
        InputState state = InputState::Idle;
    };

    std::array<InputStatus, 256> m_status;
};
}

#endif

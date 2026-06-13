#ifndef KEYBOARD_HPP
#define KEYBOARD_HPP

#include "pch.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"

class Keyboard {
public:
    Keyboard() = default;

    void Update();

    bool IsBlowing();
    bool IsOffsetUp() const;
    bool IsOffsetDown() const;

    bool IsEscDown() const;
    bool IsRKeyPressed() const;

private:
    bool m_IsBlowing = false;
    bool m_OffsetUp = false;
    bool m_OffsetDown = false;
    bool m_EscDown = false;
    bool m_RKeyPressed = false;
};

#endif
#ifndef KEYBOARD_HPP
#define KEYBOARD_HPP

#include "pch.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"

class Keyboard {
public:
    Keyboard() = default;

    void Update();
    bool IsBlowing() const;

private:
    bool m_IsBlowing = false;
};

#endif
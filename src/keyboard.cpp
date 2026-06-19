#include "Keyboard.hpp"

void Keyboard::Update() {
    // 偵測所有可以觸發「吹奏」的按鍵（包含滑鼠雙鍵、空白鍵、WASD 以及 ZXC）
    // 這裡使用 IsKeyPressed 來確保長按時能持續觸發
    m_IsBlowing = Util::Input::IsKeyPressed(Util::Keycode::MOUSE_LB) ||
                  Util::Input::IsKeyPressed(Util::Keycode::MOUSE_RB) ||
                  Util::Input::IsKeyPressed(Util::Keycode::SPACE) ||
                  Util::Input::IsKeyPressed(Util::Keycode::W) ||
                  Util::Input::IsKeyPressed(Util::Keycode::A) ||
                  Util::Input::IsKeyPressed(Util::Keycode::S) ||
                  Util::Input::IsKeyPressed(Util::Keycode::D) ||
                  Util::Input::IsKeyPressed(Util::Keycode::Z) ||
                  Util::Input::IsKeyPressed(Util::Keycode::X) ||
                  Util::Input::IsKeyPressed(Util::Keycode::C);

    // 偵測單次按下的功能鍵
    m_OffsetUp = Util::Input::IsKeyDown(Util::Keycode::UP);
    m_OffsetDown = Util::Input::IsKeyDown(Util::Keycode::DOWN);

    m_EscDown = Util::Input::IsKeyDown(Util::Keycode::ESCAPE);
    m_RKeyPressed = Util::Input::IsKeyPressed(Util::Keycode::R);
}

bool Keyboard::IsBlowing() {
    return m_IsBlowing;
}

bool Keyboard::IsOffsetUp() const {
    return m_OffsetUp;
}

bool Keyboard::IsOffsetDown() const {
    return m_OffsetDown;
}

bool Keyboard::IsEscDown() const {
    return m_EscDown;
}

bool Keyboard::IsRKeyPressed() const {
    return m_RKeyPressed;
}
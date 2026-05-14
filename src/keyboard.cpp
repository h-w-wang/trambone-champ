#include "Keyboard.hpp"

void Keyboard::Update() {
    m_IsBlowing = Util::Input::IsKeyPressed(Util::Keycode::MOUSE_LB) ||
                  Util::Input::IsKeyPressed(Util::Keycode::MOUSE_RB) ||
                  Util::Input::IsKeyPressed(Util::Keycode::W) ||
                  Util::Input::IsKeyPressed(Util::Keycode::A) ||
                  Util::Input::IsKeyPressed(Util::Keycode::S) ||
                  Util::Input::IsKeyPressed(Util::Keycode::D) ||
                  Util::Input::IsKeyPressed(Util::Keycode::Z) ||
                  Util::Input::IsKeyPressed(Util::Keycode::X) ||
                  Util::Input::IsKeyPressed(Util::Keycode::C);

    m_OffsetUp = Util::Input::IsKeyDown(Util::Keycode::UP);
    m_OffsetDown = Util::Input::IsKeyDown(Util::Keycode::DOWN);

    m_EscDown = Util::Input::IsKeyDown(Util::Keycode::ESCAPE);
    m_RKeyPressed = Util::Input::IsKeyPressed(Util::Keycode::R);
}

bool Keyboard::IsBlowing() const { return m_IsBlowing; }
bool Keyboard::IsOffsetUp() const { return m_OffsetUp; }
bool Keyboard::IsOffsetDown() const { return m_OffsetDown; }
bool Keyboard::IsEscDown() const { return m_EscDown; }
bool Keyboard::IsRKeyPressed() const { return m_RKeyPressed; }
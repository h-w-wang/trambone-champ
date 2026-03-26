#include "Keyboard.hpp"

void Keyboard::Update() {
    m_IsBlowing = Util::Input::IsKeyDown(Util::Keycode::MOUSE_LB) ||
                  Util::Input::IsKeyDown(Util::Keycode::MOUSE_RB) ||
                  Util::Input::IsKeyDown(Util::Keycode::W) ||
                  Util::Input::IsKeyDown(Util::Keycode::A) ||
                  Util::Input::IsKeyDown(Util::Keycode::S) ||
                  Util::Input::IsKeyDown(Util::Keycode::D) ||
                  Util::Input::IsKeyDown(Util::Keycode::Z) ||
                  Util::Input::IsKeyDown(Util::Keycode::X) ||
                  Util::Input::IsKeyDown(Util::Keycode::C);
}

bool Keyboard::IsBlowing() const {
    return m_IsBlowing;
}
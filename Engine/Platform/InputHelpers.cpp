#include "InputHelpers.hpp"
#include "Input.hpp"

namespace GameEngine {
namespace InputHelpers {

    bool Press(KeyCode key) {
        return Input::IsKeyJustPressed(key);
    }

    bool Down(KeyCode key) {
        return Input::IsKeyPressed(key);
    }

}

}

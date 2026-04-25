#pragma once

#include <cstdint>

struct ReshadeVKHostInputState {
    uint32_t version = 1;
    bool focused = false;
    double mouseX = 0.0;
    double mouseY = 0.0;
    bool mouseButtons[5] = {false, false, false, false, false};
    bool keyTab = false;
    bool keyLeft = false;
    bool keyRight = false;
    bool keyUp = false;
    bool keyDown = false;
    bool keyEnter = false;
    bool keyEscape = false;
    bool keyBackspace = false;
    bool keySpace = false;
    bool keyToggleF8 = false;
};

extern "C" {
const ReshadeVKHostInputState* reshadevk_host_get_input_state();
}

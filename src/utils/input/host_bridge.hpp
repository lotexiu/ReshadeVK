#pragma once

#include <cstdint>

// ── Shared input state ────────────────────────────────────────────────────────
// Plain-C struct exposed by the host process so the layer can read input
// without going through the display server.
// The host exports reshadevk_host_get_input_state(); the layer does a dlsym
// lookup at runtime and falls back to X11/GLFW if the symbol is absent.

struct ReshadeVKHostInputState {
	uint32_t version    = 1;
	bool focused        = false;
	double mouseX       = 0.0;
	double mouseY       = 0.0;
	bool mouseButtons[5]= { false, false, false, false, false };
	bool keyTab         = false;
	bool keyLeft        = false;
	bool keyRight       = false;
	bool keyUp          = false;
	bool keyDown        = false;
	bool keyEnter       = false;
	bool keyEscape      = false;
	bool keyBackspace   = false;
	bool keySpace       = false;
	bool keyToggleF8    = false;
};

extern "C" {
const ReshadeVKHostInputState* reshadevk_host_get_input_state();
}

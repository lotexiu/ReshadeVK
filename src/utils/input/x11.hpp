#pragma once

#include "utils/api/vulkan/state.hpp"

#if RESHADEVK_HAS_X11

namespace reshadevk::input {

// Queries the active X11 window, verifies it belongs to the current process,
// and feeds keyboard + pointer events into ImGui IO.
// Called by renderer::render_frame() when no GLFW window or host bridge is available.
void update_x11(SwapchainData& swData);

} // namespace reshadevk::input

#endif // RESHADEVK_HAS_X11

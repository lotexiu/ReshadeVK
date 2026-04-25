#pragma once

#include "utils/api/vulkan/state.hpp"

#if RESHADEVK_HAS_GLFW

namespace reshadevk::input {

// Polls the GLFW window associated with swData and feeds events into ImGui IO.
// Called by renderer::render_frame() when the swapchain has a GLFW window.
void update_glfw(SwapchainData& swData);

} // namespace reshadevk::input

#endif // RESHADEVK_HAS_GLFW

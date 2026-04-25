#pragma once

#include "state.hpp"

namespace reshadevk {

// ImGui Vulkan function loader callback.
// Resolves symbols through our per-swapchain dispatch tables, bypassing the
// loader's own dispatch chain (intentional for a Vulkan layer).
// Pass as the pfnLoader argument to ImGui_ImplVulkan_LoadFunctions().
PFN_vkVoidFunction imgui_loader(const char* name, void* userData);

} // namespace reshadevk

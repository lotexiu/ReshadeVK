#pragma once

#include "state.hpp"

#include <vector>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

namespace reshadevk::renderer {

// Called from hook_CreateSwapchainKHR. Sets up all overlay resources for a swapchain.
void init(VkSwapchainKHR swapchain, SwapchainData& swData);

// Called from hook_QueuePresentKHR. Renders one overlay frame and chains semaphores.
void render_frame(SwapchainData& swData, VkQueue queue, uint32_t imageIndex,
		const std::vector<VkSemaphore>& waitSems, VkSemaphore signalSem);

// Called from hook_DestroySwapchainKHR. Destroys all overlay resources.
void destroy(SwapchainData& swData);

} // namespace reshadevk::renderer

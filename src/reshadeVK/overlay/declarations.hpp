#pragma once

#include "../state.hpp"
#include <vector>

namespace reshadevk::overlay {

// Called from hook_CreateSwapchainKHR after the swapchain is created.
// Fills swData with overlay resources and initializes ImGui for this swapchain.
void init(VkSwapchainKHR swapchain, SwapchainData& swData);

// Called from hook_QueuePresentKHR once per swapchain per frame.
// Renders the ImGui overlay and submits it, chaining semaphores.
void render(SwapchainData& swData, VkQueue queue, uint32_t imageIndex,
		const std::vector<VkSemaphore>& waitSems, VkSemaphore signalSem);

// Called from hook_DestroySwapchainKHR. Destroys all overlay resources.
void destroy(SwapchainData& swData);

} // namespace reshadevk::overlay

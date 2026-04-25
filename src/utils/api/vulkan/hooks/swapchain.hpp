#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

namespace reshadevk {

VKAPI_ATTR VkResult VKAPI_CALL hook_CreateSwapchainKHR(
		VkDevice device,
		const VkSwapchainCreateInfoKHR* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkSwapchainKHR* pSwapchain);

VKAPI_ATTR void VKAPI_CALL hook_DestroySwapchainKHR(
		VkDevice device,
		VkSwapchainKHR swapchain,
		const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR VkResult VKAPI_CALL hook_QueuePresentKHR(
		VkQueue queue, const VkPresentInfoKHR* pPresentInfo);

} // namespace reshadevk

#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

namespace reshadevk {

VKAPI_ATTR VkResult VKAPI_CALL hook_CreateDevice(
		VkPhysicalDevice physicalDevice,
		const VkDeviceCreateInfo* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDevice* pDevice);

VKAPI_ATTR void VKAPI_CALL hook_DestroyDevice(
		VkDevice device, const VkAllocationCallbacks* pAllocator);

} // namespace reshadevk

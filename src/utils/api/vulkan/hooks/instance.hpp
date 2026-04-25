#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

namespace reshadevk {

VKAPI_ATTR VkResult VKAPI_CALL hook_CreateInstance(
		const VkInstanceCreateInfo* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkInstance* pInstance);

VKAPI_ATTR void VKAPI_CALL hook_DestroyInstance(
		VkInstance instance, const VkAllocationCallbacks* pAllocator);

} // namespace reshadevk

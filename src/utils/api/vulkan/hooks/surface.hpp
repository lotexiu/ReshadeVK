#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

namespace reshadevk {

VKAPI_ATTR void VKAPI_CALL hook_DestroySurfaceKHR(
		VkInstance instance,
		VkSurfaceKHR surface,
		const VkAllocationCallbacks* pAllocator);

} // namespace reshadevk

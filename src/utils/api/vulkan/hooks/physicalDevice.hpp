#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

namespace reshadevk {

VKAPI_ATTR VkResult VKAPI_CALL hook_EnumeratePhysicalDevices(
		VkInstance instance,
		uint32_t* pPhysicalDeviceCount,
		VkPhysicalDevice* pPhysicalDevices);

} // namespace reshadevk

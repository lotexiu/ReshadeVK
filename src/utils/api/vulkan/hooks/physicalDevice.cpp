#include "physicalDevice.hpp"

#include "utils/api/vulkan/state.hpp"

namespace reshadevk {

VKAPI_ATTR VkResult VKAPI_CALL hook_EnumeratePhysicalDevices(
		VkInstance instance,
		uint32_t* pPhysicalDeviceCount,
		VkPhysicalDevice* pPhysicalDevices) {
	InstanceData* instData = nullptr;
	{
		std::lock_guard<std::mutex> lock(g_lock);
		auto it = g_instanceData.find(dispatch_key(instance));
		if (it == g_instanceData.end())
			return VK_ERROR_INITIALIZATION_FAILED;
		instData = &it->second;
	}

	VkResult result = instData->dispatch.EnumeratePhysicalDevices(
			instance, pPhysicalDeviceCount, pPhysicalDevices);

	if ((result == VK_SUCCESS || result == VK_INCOMPLETE)
			&& pPhysicalDevices && pPhysicalDeviceCount
			&& instData->setInstanceLoaderData) {
		for (uint32_t i = 0; i < *pPhysicalDeviceCount; i++)
			instData->setInstanceLoaderData(instance, pPhysicalDevices[i]);
	}
	return result;
}

} // namespace reshadevk

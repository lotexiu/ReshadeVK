#include "device.hpp"

#include "utils/api/vulkan/state.hpp"

#include <vector>

namespace reshadevk {

VKAPI_ATTR VkResult VKAPI_CALL hook_CreateDevice(
		VkPhysicalDevice physicalDevice,
		const VkDeviceCreateInfo* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDevice* pDevice) {
	auto* chain = reinterpret_cast<VkLayerDeviceCreateInfo*>(
			const_cast<void*>(pCreateInfo->pNext));

	PFN_vkGetInstanceProcAddr  gipa               = nullptr;
	PFN_vkGetDeviceProcAddr    gdpa               = nullptr;
	PFN_vkSetDeviceLoaderData  setDeviceLoaderData = nullptr;

	while (chain) {
		if (chain->sType == VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO) {
			if (chain->function == VK_LAYER_LINK_INFO) {
				gipa = chain->u.pLayerInfo->pfnNextGetInstanceProcAddr;
				gdpa = chain->u.pLayerInfo->pfnNextGetDeviceProcAddr;
				chain->u.pLayerInfo = chain->u.pLayerInfo->pNext;
			} else if (chain->function == VK_LOADER_DATA_CALLBACK) {
				setDeviceLoaderData = chain->u.pfnSetDeviceLoaderData;
			}
		}
		chain = reinterpret_cast<VkLayerDeviceCreateInfo*>(const_cast<void*>(chain->pNext));
	}

	if (!gipa || !gdpa || !setDeviceLoaderData)
		return VK_ERROR_INITIALIZATION_FAILED;

	auto createFn = reinterpret_cast<PFN_vkCreateDevice>(gipa(VK_NULL_HANDLE, "vkCreateDevice"));
	VkResult result = createFn(physicalDevice, pCreateInfo, pAllocator, pDevice);
	if (result != VK_SUCCESS)
		return result;

	setDeviceLoaderData(*pDevice, *pDevice);

	DeviceData data{};
	fill_device_dispatch(*pDevice, gdpa, data.dispatch);
	data.device             = *pDevice;
	data.physicalDevice     = physicalDevice;
	data.setDeviceLoaderData = setDeviceLoaderData;
	data.graphicsQueueFamily = 0;

	if (pCreateInfo->queueCreateInfoCount > 0)
		data.graphicsQueueFamily = pCreateInfo->pQueueCreateInfos[0].queueFamilyIndex;

	// Resolve instance dispatch and pick the graphics queue family
	{
		std::lock_guard<std::mutex> lock(g_lock);
		auto instIt = g_instanceData.find(dispatch_key(physicalDevice));
		if (instIt != g_instanceData.end()) {
			data.instance         = instIt->second.instance;
			data.instanceDispatch = instIt->second.dispatch;

			uint32_t count = 0;
			instIt->second.dispatch.GetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
			std::vector<VkQueueFamilyProperties> families(count);
			instIt->second.dispatch.GetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, families.data());

			for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; i++) {
				const uint32_t qf = pCreateInfo->pQueueCreateInfos[i].queueFamilyIndex;
				if (qf < count && (families[qf].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
					data.graphicsQueueFamily = qf;
					break;
				}
			}
		}
	}

	data.dispatch.GetDeviceQueue(*pDevice, data.graphicsQueueFamily, 0, &data.graphicsQueue);
	data.setDeviceLoaderData(*pDevice, data.graphicsQueue);

	std::lock_guard<std::mutex> lock(g_lock);
	g_deviceData[dispatch_key(*pDevice)] = std::move(data);
	return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL hook_DestroyDevice(
		VkDevice device, const VkAllocationCallbacks* pAllocator) {
	DeviceDispatch dispatch{};
	{
		std::lock_guard<std::mutex> lock(g_lock);
		auto it = g_deviceData.find(dispatch_key(device));
		if (it == g_deviceData.end())
			return;
		dispatch = it->second.dispatch;
		g_deviceData.erase(it);
	}
	dispatch.DestroyDevice(device, pAllocator);
}

} // namespace reshadevk

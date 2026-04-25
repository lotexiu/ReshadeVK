#include "instance.hpp"

#include "utils/api/vulkan/state.hpp"

namespace reshadevk {

VKAPI_ATTR VkResult VKAPI_CALL hook_CreateInstance(
		const VkInstanceCreateInfo* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkInstance* pInstance) {
	auto* chain = reinterpret_cast<VkLayerInstanceCreateInfo*>(
			const_cast<void*>(pCreateInfo->pNext));

	PFN_vkGetInstanceProcAddr gipa = nullptr;
	PFN_vkSetInstanceLoaderData setInstanceLoaderData = nullptr;

	while (chain) {
		if (chain->sType == VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO) {
			if (chain->function == VK_LAYER_LINK_INFO) {
				gipa = chain->u.pLayerInfo->pfnNextGetInstanceProcAddr;
				chain->u.pLayerInfo = chain->u.pLayerInfo->pNext;
			} else if (chain->function == VK_LOADER_DATA_CALLBACK) {
				setInstanceLoaderData = chain->u.pfnSetInstanceLoaderData;
			}
		}
		chain = reinterpret_cast<VkLayerInstanceCreateInfo*>(const_cast<void*>(chain->pNext));
	}

	if (!gipa || !setInstanceLoaderData)
		return VK_ERROR_INITIALIZATION_FAILED;

	auto createFn = reinterpret_cast<PFN_vkCreateInstance>(gipa(VK_NULL_HANDLE, "vkCreateInstance"));
	VkResult result = createFn(pCreateInfo, pAllocator, pInstance);
	if (result != VK_SUCCESS)
		return result;

	setInstanceLoaderData(*pInstance, *pInstance);

	InstanceData data{};
	fill_instance_dispatch(*pInstance, gipa, data.dispatch);
	data.setInstanceLoaderData = setInstanceLoaderData;
	data.instance              = *pInstance;

	std::lock_guard<std::mutex> lock(g_lock);
	g_instanceData[dispatch_key(*pInstance)] = std::move(data);
	return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL hook_DestroyInstance(
		VkInstance instance, const VkAllocationCallbacks* pAllocator) {
	InstanceDispatch dispatch{};
	{
		std::lock_guard<std::mutex> lock(g_lock);
		auto it = g_instanceData.find(dispatch_key(instance));
		if (it == g_instanceData.end())
			return;
		dispatch = it->second.dispatch;
		g_instanceData.erase(it);
	}
	dispatch.DestroyInstance(instance, pAllocator);
}

} // namespace reshadevk

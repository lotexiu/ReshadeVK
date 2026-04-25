#include "imgui_loader.hpp"

#include <array>
#include <cstring>

#include <imgui_impl_vulkan.h>

namespace reshadevk {

// ── Wrappers that fix loader dispatch after ImGui allocates handles ───────────
// ImGui calls vkAllocateCommandBuffers and vkGetDeviceQueue directly; the
// resulting handles need setDeviceLoaderData so the loader can dispatch them.

static PFN_vkAllocateCommandBuffers s_imguiAllocateCmdBufs = nullptr;
static PFN_vkGetDeviceQueue         s_imguiGetDeviceQueue  = nullptr;

static PFN_vkSetDeviceLoaderData find_set_loader_data(VkDevice device) {
	std::lock_guard<std::mutex> lock(g_lock);
	auto it = g_deviceData.find(dispatch_key(device));
	if (it == g_deviceData.end())
		return nullptr;
	return it->second.setDeviceLoaderData;
}

VKAPI_ATTR VkResult VKAPI_CALL imgui_vkAllocateCommandBuffers(
		VkDevice device,
		const VkCommandBufferAllocateInfo* pAllocateInfo,
		VkCommandBuffer* pCommandBuffers) {
	if (!s_imguiAllocateCmdBufs)
		return VK_ERROR_INITIALIZATION_FAILED;

	VkResult result = s_imguiAllocateCmdBufs(device, pAllocateInfo, pCommandBuffers);
	if (result != VK_SUCCESS || !pAllocateInfo || !pCommandBuffers)
		return result;

	if (PFN_vkSetDeviceLoaderData setLoaderData = find_set_loader_data(device)) {
		for (uint32_t i = 0; i < pAllocateInfo->commandBufferCount; i++)
			setLoaderData(device, pCommandBuffers[i]);
	}
	return result;
}

VKAPI_ATTR void VKAPI_CALL imgui_vkGetDeviceQueue(
		VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue* pQueue) {
	if (!s_imguiGetDeviceQueue || !pQueue)
		return;
	s_imguiGetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
	if (*pQueue != VK_NULL_HANDLE) {
		if (PFN_vkSetDeviceLoaderData setLoaderData = find_set_loader_data(device))
			setLoaderData(device, *pQueue);
	}
}

// ── imgui_loader ──────────────────────────────────────────────────────────────
PFN_vkVoidFunction imgui_loader(const char* name, void* userData) {
	auto* swData = static_cast<SwapchainData*>(userData);

	// These are instance-level functions in ImGui's Vulkan function map.
	constexpr std::array<const char*, 8> instanceLevelFuncs = {
		"vkDestroySurfaceKHR",
		"vkEnumeratePhysicalDevices",
		"vkGetPhysicalDeviceProperties",
		"vkGetPhysicalDeviceMemoryProperties",
		"vkGetPhysicalDeviceQueueFamilyProperties",
		"vkGetPhysicalDeviceSurfaceCapabilitiesKHR",
		"vkGetPhysicalDeviceSurfaceFormatsKHR",
		"vkGetPhysicalDeviceSurfacePresentModesKHR",
	};
	for (const char* fnName : instanceLevelFuncs) {
		if (std::strcmp(name, fnName) == 0) {
			if (swData->instance != VK_NULL_HANDLE && swData->instanceDispatch.GetInstanceProcAddr)
				return swData->instanceDispatch.GetInstanceProcAddr(swData->instance, name);
			return nullptr;
		}
	}

	// Intercept allocate/queue so we can tag handles for the loader.
	if (std::strcmp(name, "vkAllocateCommandBuffers") == 0) {
		s_imguiAllocateCmdBufs = swData->dispatch.AllocateCommandBuffers;
		return reinterpret_cast<PFN_vkVoidFunction>(&imgui_vkAllocateCommandBuffers);
	}
	if (std::strcmp(name, "vkGetDeviceQueue") == 0) {
		s_imguiGetDeviceQueue = swData->dispatch.GetDeviceQueue;
		return reinterpret_cast<PFN_vkVoidFunction>(&imgui_vkGetDeviceQueue);
	}

	// All remaining symbols are device-level.
	if (swData->dispatch.GetDeviceProcAddr)
		return swData->dispatch.GetDeviceProcAddr(swData->device, name);
	return nullptr;
}

} // namespace reshadevk

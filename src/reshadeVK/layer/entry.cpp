#include "../state.hpp"
#include "../overlay/declarations.hpp"
#include "dispatch.hpp"

#include <cstring>
#include <cstdlib>
#include <dlfcn.h>
#include <vector>

#if RESHADEVK_HAS_GLFW
#include <GLFW/glfw3.h>
#endif

namespace reshadevk {

// ── vkCreateInstance ──────────────────────────────────────────────────────────
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

	InstanceData data { };
	fill_instance_dispatch(*pInstance, gipa, data.dispatch);
	data.setInstanceLoaderData = setInstanceLoaderData;
	data.instance = *pInstance;

	std::lock_guard<std::mutex> lock(g_lock);
	g_instanceData[dispatch_key(*pInstance)] = std::move(data);
	return VK_SUCCESS;
}

// ── vkEnumeratePhysicalDevices ───────────────────────────────────────────────
VKAPI_ATTR VkResult VKAPI_CALL hook_EnumeratePhysicalDevices(
		VkInstance instance, uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices) {
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

	if ((result == VK_SUCCESS || result == VK_INCOMPLETE) && pPhysicalDevices && pPhysicalDeviceCount && instData->setInstanceLoaderData) {
		for (uint32_t i = 0; i < *pPhysicalDeviceCount; i++)
			instData->setInstanceLoaderData(instance, pPhysicalDevices[i]);
	}

	return result;
}

// ── vkDestroyInstance ─────────────────────────────────────────────────────────
VKAPI_ATTR void VKAPI_CALL hook_DestroyInstance(
		VkInstance instance, const VkAllocationCallbacks* pAllocator) {
	InstanceDispatch dispatch { };
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

// ── vkDestroySurfaceKHR ──────────────────────────────────────────────────────
VKAPI_ATTR void VKAPI_CALL hook_DestroySurfaceKHR(
		VkInstance instance,
		VkSurfaceKHR surface,
		const VkAllocationCallbacks* pAllocator) {
	InstanceDispatch dispatch { };
	{
		std::lock_guard<std::mutex> lock(g_lock);
		auto it = g_instanceData.find(dispatch_key(instance));
		if (it == g_instanceData.end())
			return;
		dispatch = it->second.dispatch;
#if RESHADEVK_HAS_GLFW
		g_surfaceGlfwWindow.erase(surface);
#endif
	}

	if (dispatch.DestroySurfaceKHR)
		dispatch.DestroySurfaceKHR(instance, surface, pAllocator);
}

// ── vkCreateDevice ────────────────────────────────────────────────────────────
VKAPI_ATTR VkResult VKAPI_CALL hook_CreateDevice(
		VkPhysicalDevice physicalDevice,
		const VkDeviceCreateInfo* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDevice* pDevice) {
	auto* chain = reinterpret_cast<VkLayerDeviceCreateInfo*>(
			const_cast<void*>(pCreateInfo->pNext));

	PFN_vkGetInstanceProcAddr gipa = nullptr;
	PFN_vkGetDeviceProcAddr gdpa = nullptr;
	PFN_vkSetDeviceLoaderData setDeviceLoaderData = nullptr;

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

	DeviceData data { };
	fill_device_dispatch(*pDevice, gdpa, data.dispatch);
	data.device = *pDevice;
	data.physicalDevice = physicalDevice;
	data.setDeviceLoaderData = setDeviceLoaderData;
	data.graphicsQueueFamily = 0;

	if (pCreateInfo->queueCreateInfoCount > 0)
		data.graphicsQueueFamily = pCreateInfo->pQueueCreateInfos[0].queueFamilyIndex;

	// Find graphics queue family via instance dispatch
	{
		std::lock_guard<std::mutex> lock(g_lock);
		auto instIt = g_instanceData.find(dispatch_key(physicalDevice));
		if (instIt != g_instanceData.end()) {
			data.instance = instIt->second.instance;
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

// ── vkDestroyDevice ───────────────────────────────────────────────────────────
VKAPI_ATTR void VKAPI_CALL hook_DestroyDevice(
		VkDevice device, const VkAllocationCallbacks* pAllocator) {
	DeviceDispatch dispatch { };
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

// ── vkCreateSwapchainKHR ──────────────────────────────────────────────────────
VKAPI_ATTR VkResult VKAPI_CALL hook_CreateSwapchainKHR(
		VkDevice device,
		const VkSwapchainCreateInfoKHR* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkSwapchainKHR* pSwapchain) {
	DeviceData* devData = nullptr;
	{
		std::lock_guard<std::mutex> lock(g_lock);
		auto it = g_deviceData.find(dispatch_key(device));
		if (it == g_deviceData.end())
			return VK_ERROR_INITIALIZATION_FAILED;
		devData = &it->second;
	}

	VkResult result = devData->dispatch.CreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
	if (result != VK_SUCCESS)
		return result;

	if (pCreateInfo->oldSwapchain != VK_NULL_HANDLE) {
		SwapchainData oldData { };
		bool hadOldData = false;
		{
			std::lock_guard<std::mutex> lock(g_lock);
			auto oldIt = g_swapchainData.find(pCreateInfo->oldSwapchain);
			if (oldIt != g_swapchainData.end()) {
				oldData = std::move(oldIt->second);
				g_swapchainData.erase(oldIt);
				hadOldData = true;
			}
		}

		if (hadOldData)
			overlay::destroy(oldData);
	}

	SwapchainData swData { };
	swData.dispatch = devData->dispatch;
	swData.instanceDispatch = devData->instanceDispatch;
	swData.setDeviceLoaderData = devData->setDeviceLoaderData;
	swData.instance = devData->instance;
	swData.physicalDevice = devData->physicalDevice;
	swData.device = devData->device;
	swData.graphicsQueue = devData->graphicsQueue;
	swData.graphicsQueueFamily = devData->graphicsQueueFamily;
	swData.format = pCreateInfo->imageFormat;
	swData.extent = pCreateInfo->imageExtent;
	swData.imguiCtx = nullptr;
	swData.interactiveEnabled = true;
	swData.interactionAvailable = false;

	const char* interactiveEnv = std::getenv("RESHADEVK_INTERACTIVE");
	if (interactiveEnv && interactiveEnv[0] == '0')
		swData.interactiveEnabled = false;

#if RESHADEVK_HAS_GLFW
	{
		std::lock_guard<std::mutex> lock(g_lock);
		auto wit = g_surfaceGlfwWindow.find(pCreateInfo->surface);
		if (wit != g_surfaceGlfwWindow.end()) {
			swData.glfwWindow = wit->second;
			swData.interactionAvailable = true;
		}
	}
#endif

	overlay::init(*pSwapchain, swData);

	std::lock_guard<std::mutex> lock(g_lock);
	g_swapchainData[*pSwapchain] = std::move(swData);
	return VK_SUCCESS;
}

// ── vkDestroySwapchainKHR ─────────────────────────────────────────────────────
VKAPI_ATTR void VKAPI_CALL hook_DestroySwapchainKHR(
		VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator) {
	SwapchainData swData { };
	DeviceDispatch dispatch { };
	bool hasSwapchainData = false;
	{
		std::lock_guard<std::mutex> lock(g_lock);
		auto swIt = g_swapchainData.find(swapchain);
		if (swIt != g_swapchainData.end()) {
			swData = std::move(swIt->second);
			g_swapchainData.erase(swIt);
			hasSwapchainData = true;
		}
		auto devIt = g_deviceData.find(dispatch_key(device));
		if (devIt == g_deviceData.end())
			return;
		dispatch = devIt->second.dispatch;
	}

	if (hasSwapchainData)
		overlay::destroy(swData);
	dispatch.DestroySwapchainKHR(device, swapchain, pAllocator);
}

// ── vkQueuePresentKHR ─────────────────────────────────────────────────────────
VKAPI_ATTR VkResult VKAPI_CALL hook_QueuePresentKHR(
		VkQueue queue, const VkPresentInfoKHR* pPresentInfo) {
	DeviceData* devData = nullptr;
	{
		std::lock_guard<std::mutex> lock(g_lock);
		auto it = g_deviceData.find(dispatch_key(queue));
		if (it != g_deviceData.end())
			devData = &it->second;
	}

	if (!devData)
		return VK_ERROR_INITIALIZATION_FAILED;

	// Build a mutable semaphore chain: start with the app's wait semaphores
	std::vector<VkSemaphore> waitSems(
			pPresentInfo->pWaitSemaphores,
			pPresentInfo->pWaitSemaphores + pPresentInfo->waitSemaphoreCount);

	for (uint32_t i = 0; i < pPresentInfo->swapchainCount; i++) {
		VkSwapchainKHR sc = pPresentInfo->pSwapchains[i];
		uint32_t imgIdx = pPresentInfo->pImageIndices[i];

		SwapchainData* swData = nullptr;
		{
			std::lock_guard<std::mutex> lock(g_lock);
			auto it = g_swapchainData.find(sc);
			if (it != g_swapchainData.end())
				swData = &it->second;
		}

		if (!swData || !swData->imguiCtx || imgIdx >= swData->semaphores.size())
			continue;

		VkSemaphore doneSem = swData->semaphores[imgIdx];
		overlay::render(*swData, queue, imgIdx, waitSems, doneSem);
		waitSems = { doneSem };
	}

	VkPresentInfoKHR modInfo = *pPresentInfo;
	modInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSems.size());
	modInfo.pWaitSemaphores = waitSems.data();
	return devData->dispatch.QueuePresentKHR(queue, &modInfo);
}

// ── Dispatch lookup ───────────────────────────────────────────────────────────
static PFN_vkVoidFunction get_hook(const char* name) {
#define HOOK(vkName, fn)           \
	if (!std::strcmp(name, #vkName)) \
		return reinterpret_cast<PFN_vkVoidFunction>(fn);
	HOOK(vkCreateInstance, hook_CreateInstance)
	HOOK(vkEnumeratePhysicalDevices, hook_EnumeratePhysicalDevices)
	HOOK(vkDestroySurfaceKHR, hook_DestroySurfaceKHR)
	HOOK(vkDestroyInstance, hook_DestroyInstance)
	HOOK(vkCreateDevice, hook_CreateDevice)
	HOOK(vkDestroyDevice, hook_DestroyDevice)
	HOOK(vkCreateSwapchainKHR, hook_CreateSwapchainKHR)
	HOOK(vkDestroySwapchainKHR, hook_DestroySwapchainKHR)
	HOOK(vkQueuePresentKHR, hook_QueuePresentKHR)
#undef HOOK
	return nullptr;
}

} // namespace reshadevk

// ── Exported entry points (listed in the layer JSON) ─────────────────────────
#define RESHADEVK_EXPORT __attribute__((visibility("default")))

extern "C" {

#if RESHADEVK_HAS_GLFW
RESHADEVK_EXPORT VkResult glfwCreateWindowSurface(
		VkInstance instance,
		GLFWwindow* window,
		const VkAllocationCallbacks* allocator,
		VkSurfaceKHR* surface) {
	using GlfwCreateWindowSurfaceFn = VkResult (*)(
			VkInstance, GLFWwindow*, const VkAllocationCallbacks*, VkSurfaceKHR*);

	static GlfwCreateWindowSurfaceFn realFn = nullptr;
	if (!realFn) {
		realFn = reinterpret_cast<GlfwCreateWindowSurfaceFn>(
				dlsym(RTLD_NEXT, "glfwCreateWindowSurface"));
	}

	if (!realFn)
		return VK_ERROR_INITIALIZATION_FAILED;

	const VkResult result = realFn(instance, window, allocator, surface);
	if (result != VK_SUCCESS || !surface || *surface == VK_NULL_HANDLE)
		return result;

	std::lock_guard<std::mutex> lock(reshadevk::g_lock);
	reshadevk::g_surfaceGlfwWindow[*surface] = window;
	return result;
}
#endif

RESHADEVK_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL reshadevk_GetInstanceProcAddr(
		VkInstance instance, const char* name) {
	if (PFN_vkVoidFunction hook = reshadevk::get_hook(name))
		return hook;

	if (instance == VK_NULL_HANDLE)
		return nullptr;

	std::lock_guard<std::mutex> lock(reshadevk::g_lock);
	auto it = reshadevk::g_instanceData.find(reshadevk::dispatch_key(instance));
	if (it == reshadevk::g_instanceData.end())
		return nullptr;
	return it->second.dispatch.GetInstanceProcAddr(instance, name);
}

RESHADEVK_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL reshadevk_GetDeviceProcAddr(
		VkDevice device, const char* name) {
	if (PFN_vkVoidFunction hook = reshadevk::get_hook(name))
		return hook;

	if (device == VK_NULL_HANDLE)
		return nullptr;

	std::lock_guard<std::mutex> lock(reshadevk::g_lock);
	auto it = reshadevk::g_deviceData.find(reshadevk::dispatch_key(device));
	if (it == reshadevk::g_deviceData.end())
		return nullptr;
	return it->second.dispatch.GetDeviceProcAddr(device, name);
}

} // extern "C"

// ── reshadeVK entry point ─────────────────────────────────────────────────────
// Exports the two symbols the Vulkan Loader requires (listed in layer.json.in).
// All hook implementations live in utils/api/vulkan/hooks/.
// The overlay component (ReshadeOverlay) is registered on library load.

#include "overlay.hpp"

#include "utils/layer.hpp"
#include "utils/api/vulkan/state.hpp"
#include "utils/api/vulkan/hooks/instance.hpp"
#include "utils/api/vulkan/hooks/physicalDevice.hpp"
#include "utils/api/vulkan/hooks/device.hpp"
#include "utils/api/vulkan/hooks/surface.hpp"
#include "utils/api/vulkan/hooks/swapchain.hpp"

#include <cstring>

#define RESHADEVK_EXPORT __attribute__((visibility("default")))

namespace reshadevk {

static PFN_vkVoidFunction get_hook(const char* name) {
#define HOOK(vkName, fn)           \
	if (!std::strcmp(name, #vkName)) \
		return reinterpret_cast<PFN_vkVoidFunction>(fn);
	HOOK(vkCreateInstance,           hook_CreateInstance)
	HOOK(vkEnumeratePhysicalDevices, hook_EnumeratePhysicalDevices)
	HOOK(vkDestroySurfaceKHR,        hook_DestroySurfaceKHR)
	HOOK(vkDestroyInstance,          hook_DestroyInstance)
	HOOK(vkCreateDevice,             hook_CreateDevice)
	HOOK(vkDestroyDevice,            hook_DestroyDevice)
	HOOK(vkCreateSwapchainKHR,       hook_CreateSwapchainKHR)
	HOOK(vkDestroySwapchainKHR,      hook_DestroySwapchainKHR)
	HOOK(vkQueuePresentKHR,          hook_QueuePresentKHR)
#undef HOOK
	return nullptr;
}

} // namespace reshadevk

extern "C" {

__attribute__((constructor)) static void reshadevk_on_load() {
	reshadevk::set_layer_component(new reshadevk::ReshadeOverlay());
}

__attribute__((destructor)) static void reshadevk_on_unload() {
	reshadevk::set_layer_component(nullptr);
}

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

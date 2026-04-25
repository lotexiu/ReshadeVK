#include "surface.hpp"

#include "utils/api/vulkan/state.hpp"

#if RESHADEVK_HAS_GLFW
#include <dlfcn.h>
#include <GLFW/glfw3.h>
#endif

#define RESHADEVK_EXPORT __attribute__((visibility("default")))

namespace reshadevk {

VKAPI_ATTR void VKAPI_CALL hook_DestroySurfaceKHR(
		VkInstance instance,
		VkSurfaceKHR surface,
		const VkAllocationCallbacks* pAllocator) {
	InstanceDispatch dispatch{};
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

} // namespace reshadevk

// ── Intercept glfwCreateWindowSurface ─────────────────────────────────────────
// Overrides GLFW's symbol so we can associate the GLFWwindow* with the
// VkSurfaceKHR and later pass it to the swapchain's input provider.

#if RESHADEVK_HAS_GLFW
extern "C" RESHADEVK_EXPORT VkResult glfwCreateWindowSurface(
		VkInstance instance,
		GLFWwindow* window,
		const VkAllocationCallbacks* allocator,
		VkSurfaceKHR* surface) {
	using Fn = VkResult (*)(VkInstance, GLFWwindow*, const VkAllocationCallbacks*, VkSurfaceKHR*);
	static Fn realFn = nullptr;
	if (!realFn)
		realFn = reinterpret_cast<Fn>(dlsym(RTLD_NEXT, "glfwCreateWindowSurface"));
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

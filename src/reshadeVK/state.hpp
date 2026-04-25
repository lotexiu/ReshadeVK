#pragma once

#include <mutex>
#include <unordered_map>
#include <vector>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <vulkan/vk_layer.h>

#include "layer/dispatch.hpp"

struct ImGuiContext;

namespace reshadevk {

// Dispatch key: first pointer inside any Vulkan dispatchable handle
// (the loader stores its dispatch table pointer there)
template <typename T>
inline void* dispatch_key(T handle) {
	return *reinterpret_cast<void**>(handle);
}

// ── Per-instance state ────────────────────────────────────────────────────────
struct InstanceData {
	InstanceDispatch dispatch;
	PFN_vkSetInstanceLoaderData setInstanceLoaderData;
	VkInstance instance;
};

// ── Per-device state ──────────────────────────────────────────────────────────
struct DeviceData {
	DeviceDispatch dispatch;
	InstanceDispatch instanceDispatch;
	PFN_vkSetDeviceLoaderData setDeviceLoaderData;
	VkInstance instance;
	VkDevice device;
	VkPhysicalDevice physicalDevice;
	VkQueue graphicsQueue;
	uint32_t graphicsQueueFamily;
};

// ── Per-swapchain state (overlay resources) ───────────────────────────────────
struct SwapchainData {
	DeviceDispatch dispatch;
	InstanceDispatch instanceDispatch;
	PFN_vkSetDeviceLoaderData setDeviceLoaderData;
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkQueue graphicsQueue;
	uint32_t graphicsQueueFamily;
	VkFormat format;
	VkExtent2D extent;
	uint32_t imageCount;
	std::vector<VkImage> images;
	std::vector<VkImageView> imageViews;
	std::vector<VkFramebuffer> framebuffers;
	VkRenderPass renderPass;
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;
	VkDescriptorPool descriptorPool;
	std::vector<VkSemaphore> semaphores;
	std::vector<VkFence> fences;
	ImGuiContext* imguiCtx;
	bool imguiBackendInitialized = false;
	bool interactiveEnabled = false;
	bool interactionAvailable = false;
	bool inputFocused = false;
	bool prevMouseDown[5] = { false, false, false, false, false };
	bool prevToggleKeyDown = false;
#if RESHADEVK_HAS_GLFW
	void* glfwWindow = nullptr;
#endif
#if RESHADEVK_HAS_X11
	void* nativeDisplay = nullptr;
	bool ownsDisplayConnection = false;
	unsigned long nativeWindow = 0;
#endif
};

// ── Global maps ───────────────────────────────────────────────────────────────
extern std::mutex g_lock;
extern std::unordered_map<void*, InstanceData> g_instanceData;
extern std::unordered_map<void*, DeviceData> g_deviceData;
extern std::unordered_map<VkSwapchainKHR, SwapchainData> g_swapchainData;
#if RESHADEVK_HAS_GLFW
extern std::unordered_map<VkSurfaceKHR, void*> g_surfaceGlfwWindow;
#endif

} // namespace reshadevk

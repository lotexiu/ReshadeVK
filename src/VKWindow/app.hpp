#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

namespace vk_window {

struct AppConfig {
	std::string appName    = "vk-window";
	std::string windowTitle= "Vulkan Window";
	uint32_t width         = 800;
	uint32_t height        = 400;
	std::array<float, 4> clearColor = { 0.35f, 0.35f, 0.35f, 1.0f };
	bool enableValidationLayers = true;
};

struct VulkanContext {
	GLFWwindow* window     = nullptr;
	bool framebufferResized = false;
	double lastFramebufferResizeTime = 0.0;

	VkInstance instance    = VK_NULL_HANDLE;
	VkSurfaceKHR surface   = VK_NULL_HANDLE;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device        = VK_NULL_HANDLE;
	VkQueue graphicsQueue  = VK_NULL_HANDLE;
	VkQueue presentQueue   = VK_NULL_HANDLE;
	uint32_t graphicsFamily = 0;
	uint32_t presentFamily  = 0;

	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	VkFormat swapchainFormat = VK_FORMAT_UNDEFINED;
	VkExtent2D swapchainExtent{};
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;

	VkRenderPass renderPass = VK_NULL_HANDLE;
	std::vector<VkFramebuffer> framebuffers;

	VkCommandPool commandPool = VK_NULL_HANDLE;
	static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
	std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> commandBuffers{};
	std::array<VkSemaphore,     MAX_FRAMES_IN_FLIGHT> imageAvailableSemaphores{};
	std::array<VkSemaphore,     MAX_FRAMES_IN_FLIGHT> renderFinishedSemaphores{};
	std::array<VkFence,         MAX_FRAMES_IN_FLIGHT> inFlightFences{};
	uint32_t frameIndex = 0;
};

bool init_all(VulkanContext& context, GLFWwindow* window, const AppConfig& config);
bool render_once(VulkanContext& context, const AppConfig& config);
void device_wait_idle(VulkanContext& context);
void destroy_all(VulkanContext& context);

} // namespace vk_window

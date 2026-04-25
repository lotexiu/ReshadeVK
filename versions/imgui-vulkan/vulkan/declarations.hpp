#pragma once

#include <vector>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

// Estrutura que vai guardar tudo relacionado ao Vulkan
struct VulkanContext {
	VkInstance       instance;
	VkSurfaceKHR     surface;
	VkPhysicalDevice physical_device;
	VkDevice         device;
	VkQueue          graphics_queue;
	uint32_t         graphics_family;
	VkSwapchainKHR        swapchain;
	VkFormat              swapchain_format;
	VkExtent2D            swapchain_extent;
	std::vector<VkImage>     swapchain_images;
	std::vector<VkImageView> swapchain_image_views;
	VkRenderPass             render_pass;
	std::vector<VkFramebuffer> framebuffers;
};
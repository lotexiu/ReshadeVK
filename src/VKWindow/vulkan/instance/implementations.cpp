#define GLFW_INCLUDE_VULKAN
#include "declarations.hpp"

#include <cstdio>
#include <cstring>
#include <vector>

namespace {

constexpr const char* VALIDATION_LAYERS[] = { "VK_LAYER_KHRONOS_validation" };

bool has_validation_layers() {
	uint32_t count = 0;
	vkEnumerateInstanceLayerProperties(&count, nullptr);
	std::vector<VkLayerProperties> available(count);
	vkEnumerateInstanceLayerProperties(&count, available.data());

	for (const char* layerName : VALIDATION_LAYERS) {
		bool found = false;
		for (const auto& layer : available) {
			if (std::strcmp(layer.layerName, layerName) == 0) {
				found = true;
				break;
			}
		}
		if (!found)
			return false;
	}
	return true;
}

} // namespace

namespace vk_window::instance {

bool create_instance(VulkanContext& context, const AppConfig& config) {
	VkApplicationInfo appInfo{};
	appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName   = config.appName.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName        = "vk-window";
	appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion         = VK_API_VERSION_1_0;

	uint32_t     glfwExtensionCount = 0;
	const char** glfwExtensions     = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	VkInstanceCreateInfo createInfo{};
	createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo        = &appInfo;
	createInfo.enabledExtensionCount   = glfwExtensionCount;
	createInfo.ppEnabledExtensionNames = glfwExtensions;

	const bool useValidation = config.enableValidationLayers && has_validation_layers();
	if (useValidation) {
		createInfo.enabledLayerCount   = 1;
		createInfo.ppEnabledLayerNames = VALIDATION_LAYERS;
	} else {
		createInfo.enabledLayerCount = 0;
		if (config.enableValidationLayers)
			std::printf("Aviso: validation layer nao disponivel\n");
	}

	const VkResult result = vkCreateInstance(&createInfo, nullptr, &context.instance);
	if (result != VK_SUCCESS) {
		std::printf("Erro: nao foi possivel criar a instancia Vulkan (%d)\n", result);
		context.instance = VK_NULL_HANDLE;
		return false;
	}
	std::printf("Vulkan: instancia criada\n");
	return true;
}

bool create_surface(VulkanContext& context, GLFWwindow* window) {
	if (context.instance == VK_NULL_HANDLE) {
		std::printf("Erro: instancia invalida para criar surface\n");
		return false;
	}
	const VkResult result = glfwCreateWindowSurface(context.instance, window, nullptr, &context.surface);
	if (result != VK_SUCCESS) {
		std::printf("Erro: nao foi possivel criar a surface (%d)\n", result);
		context.surface = VK_NULL_HANDLE;
		return false;
	}
	std::printf("Vulkan: surface criada\n");
	return true;
}

void destroy_instance_resources(VulkanContext& context) {
	if (context.surface != VK_NULL_HANDLE) {
		vkDestroySurfaceKHR(context.instance, context.surface, nullptr);
		context.surface = VK_NULL_HANDLE;
	}
	if (context.instance != VK_NULL_HANDLE) {
		vkDestroyInstance(context.instance, nullptr);
		context.instance = VK_NULL_HANDLE;
	}
}

} // namespace vk_window::instance

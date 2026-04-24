#include <cstdio>
#include <cstring>
#include <vector>

#include "declarations.hpp"
#include "../validation/declarations.hpp"

void vk_create_instance(VulkanContext& ctx) {
	if (!check_validation_layers())
		printf("Aviso: validation layers nao disponiveis\n");

	// Informações sobre sua aplicação (opcional mas boa prática)
	VkApplicationInfo app_info{};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "ReShade Dev";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;

	// Extensões necessárias pro GLFW funcionar com Vulkan
	uint32_t glfw_ext_count = 0;
	const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_ext_count);

	// Configuração da instância
	VkInstanceCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	ci.pApplicationInfo = &app_info;
	ci.enabledExtensionCount = glfw_ext_count;
	ci.ppEnabledExtensionNames = glfw_extensions;
	ci.enabledLayerCount = 1;
	ci.ppEnabledLayerNames = VALIDATION_LAYERS;

	if (vkCreateInstance(&ci, nullptr, &ctx.instance) != VK_SUCCESS) {
		printf("Erro: nao foi possivel criar a instancia Vulkan\n");
		return;
	}

	printf("Vulkan: instancia criada\n");
}

void vk_create_surface(VulkanContext& ctx, GLFWwindow* window) {
	if (glfwCreateWindowSurface(ctx.instance, window, nullptr, &ctx.surface) != VK_SUCCESS) {
		printf("Erro: nao foi possivel criar a surface\n");
		return;
	}

	printf("Vulkan: surface criada\n");
}

void vk_destroy(VulkanContext& ctx) {
	vkDestroyDevice(ctx.device, nullptr);
	vkDestroySurfaceKHR(ctx.instance, ctx.surface, nullptr);
	vkDestroyInstance(ctx.instance, nullptr);
	printf("Vulkan: recursos destruidos\n");
}
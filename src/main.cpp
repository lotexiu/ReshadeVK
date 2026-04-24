#include <GLFW/glfw3.h>
#include "vulkan/instance/declarations.hpp"
#include "vulkan/video/declarations.hpp"
#include <cstdio>
#include <csignal>

static bool processActive = true;

void signal_handler(int signal) {
	processActive = false;
}

int main() {
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	if (!glfwInit()) {
		printf("Erro: nao foi possivel inicializar o GLFW\n");
		return 1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(1280, 720, "ReShade Dev", nullptr, nullptr);

	if (!window) {
		printf("Erro: nao foi possivel criar a janela\n");
		glfwTerminate();
		return 1;
	}

	// Cria os recursos Vulkan
	VulkanContext ctx{};
	vk_create_instance(ctx);
	vk_create_surface(ctx, window);
	vk_pick_physical_device(ctx);   // novo
	vk_create_logical_device(ctx);  // novo

	while (!glfwWindowShouldClose(window) && processActive) {
		glfwPollEvents();
	}

	// Limpeza na ordem inversa da criação
	vk_destroy(ctx);
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
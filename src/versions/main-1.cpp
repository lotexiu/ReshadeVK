#include <GLFW/glfw3.h>
#include <cstdio>


int main() {

	// Inicializa o GLFW (sistema de janelas)
	if (!glfwInit()) {
		printf("Erro: nao foi possivel inicializar o GLFW\n");
		return 1;
	}

	// Fala pro GLFW que NÃO vamos usar OpenGL (vamos usar Vulkan depois)
	// glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	// glfwWindowHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);

	// Cria a janela: largura, altura, título
	GLFWwindow* window = glfwCreateWindow(1280, 720, "ReShade Dev", nullptr, nullptr);

	glfwMakeContextCurrent(window);
	// Evita screen tearing
	glfwSwapInterval(1);

	if (!window) {
		printf("Erro: nao foi possivel criar a janela\n");
		glfwTerminate();
		return 1;
	}

	// Loop principal — fica rodando até fechar a janela
	while (!glfwWindowShouldClose(window)) {
		// Processa eventos (mouse, teclado, fechar janela, etc)
		glfwPollEvents();
		//  Limpa tela com cor branca
		glClearColor(0.4f, 0.4f, 0.4f, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		glfwSwapBuffers(window);
	}

	// Limpeza
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
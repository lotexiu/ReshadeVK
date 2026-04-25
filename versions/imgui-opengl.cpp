#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <cstdio>

int main() {

	if (!glfwInit()) {
		printf("Erro: nao foi possivel inicializar o GLFW\n");
		return 1;
	}

	GLFWwindow* window = glfwCreateWindow(1280, 720, "ReShade Dev", nullptr, nullptr);

	if (!window) {
		printf("Erro: nao foi possivel criar a janela\n");
		glfwTerminate();
		return 1;
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	// Inicializa o ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// Liga o ImGui ao GLFW e ao OpenGL
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");

	// Tema escuro
	ImGui::StyleColorsDark();

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		// Avisa o ImGui que um novo frame vai começar
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// ── Sua UI aqui ──────────────────────
		ImGui::ShowDemoWindow(); // janela de demo do ImGui
		// ─────────────────────────────────────

		// Renderiza o que o ImGui preparou
		ImGui::Render();

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	// Limpeza do ImGui
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
#include "glfw.hpp"

#if RESHADEVK_HAS_GLFW

#include <cstdio>
#include <limits>

#include <GLFW/glfw3.h>
#include <imgui.h>

namespace reshadevk::input {

void update_glfw(SwapchainData& swData) {
	auto* window = static_cast<GLFWwindow*>(swData.glfwWindow);
	if (!window)
		return;

	ImGuiIO& io         = ImGui::GetIO();
	const bool hasFocus = glfwGetWindowAttrib(window, GLFW_FOCUSED) == GLFW_TRUE;
	swData.inputFocused = hasFocus;
	io.AddFocusEvent(hasFocus);

	const bool toggleDown = glfwGetKey(window, GLFW_KEY_F8) == GLFW_PRESS;
	if (toggleDown && !swData.prevToggleKeyDown && hasFocus) {
		swData.interactiveEnabled = !swData.interactiveEnabled;
		std::printf("[reshadeVK] input %s (F8)\n", swData.interactiveEnabled ? "ativado" : "desativado");
	}
	swData.prevToggleKeyDown = toggleDown;

	const bool canCapture = hasFocus && swData.interactiveEnabled;

	double mouseX = 0.0;
	double mouseY = 0.0;
	glfwGetCursorPos(window, &mouseX, &mouseY);
	if (canCapture)
		io.AddMousePosEvent(static_cast<float>(mouseX), static_cast<float>(mouseY));
	else
		io.AddMousePosEvent(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

	const bool nextMouseDown[5] = {
		canCapture && (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)   == GLFW_PRESS),
		canCapture && (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT)  == GLFW_PRESS),
		canCapture && (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS),
		canCapture && (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_4)      == GLFW_PRESS),
		canCapture && (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_5)      == GLFW_PRESS),
	};
	for (int i = 0; i < 5; i++) {
		if (nextMouseDown[i] != swData.prevMouseDown[i]) {
			io.AddMouseButtonEvent(i, nextMouseDown[i]);
			swData.prevMouseDown[i] = nextMouseDown[i];
		}
	}

	io.AddKeyEvent(ImGuiKey_Tab,
		canCapture && glfwGetKey(window, GLFW_KEY_TAB)       == GLFW_PRESS);
	io.AddKeyEvent(ImGuiKey_LeftArrow,
		canCapture && glfwGetKey(window, GLFW_KEY_LEFT)      == GLFW_PRESS);
	io.AddKeyEvent(ImGuiKey_RightArrow,
		canCapture && glfwGetKey(window, GLFW_KEY_RIGHT)     == GLFW_PRESS);
	io.AddKeyEvent(ImGuiKey_UpArrow,
		canCapture && glfwGetKey(window, GLFW_KEY_UP)        == GLFW_PRESS);
	io.AddKeyEvent(ImGuiKey_DownArrow,
		canCapture && glfwGetKey(window, GLFW_KEY_DOWN)      == GLFW_PRESS);
	io.AddKeyEvent(ImGuiKey_Enter,
		canCapture && (glfwGetKey(window, GLFW_KEY_ENTER)    == GLFW_PRESS
		            || glfwGetKey(window, GLFW_KEY_KP_ENTER) == GLFW_PRESS));
	io.AddKeyEvent(ImGuiKey_Escape,
		canCapture && glfwGetKey(window, GLFW_KEY_ESCAPE)    == GLFW_PRESS);
	io.AddKeyEvent(ImGuiKey_Backspace,
		canCapture && glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS);
	io.AddKeyEvent(ImGuiKey_Space,
		canCapture && glfwGetKey(window, GLFW_KEY_SPACE)     == GLFW_PRESS);
}

} // namespace reshadevk::input

#endif // RESHADEVK_HAS_GLFW

#include <cstdio>

#include <GLFW/glfw3.h>

#include "shared/host_input_bridge.hpp"
#include "vulkan/context.hpp"

namespace {

ReshadeVKHostInputState g_hostInputState{};

void update_host_input_state(GLFWwindow* window) {
    g_hostInputState.focused = glfwGetWindowAttrib(window, GLFW_FOCUSED) == GLFW_TRUE;

    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(window, &x, &y);
    g_hostInputState.mouseX = x;
    g_hostInputState.mouseY = y;

    g_hostInputState.mouseButtons[0] = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    g_hostInputState.mouseButtons[1] = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    g_hostInputState.mouseButtons[2] = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    g_hostInputState.mouseButtons[3] = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_4) == GLFW_PRESS;
    g_hostInputState.mouseButtons[4] = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_5) == GLFW_PRESS;

    g_hostInputState.keyTab = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;
    g_hostInputState.keyLeft = glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS;
    g_hostInputState.keyRight = glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS;
    g_hostInputState.keyUp = glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
    g_hostInputState.keyDown = glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;
    g_hostInputState.keyEnter =
        glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_KP_ENTER) == GLFW_PRESS;
    g_hostInputState.keyEscape = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    g_hostInputState.keyBackspace = glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS;
    g_hostInputState.keySpace = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    g_hostInputState.keyToggleF8 = glfwGetKey(window, GLFW_KEY_F8) == GLFW_PRESS;
}

void on_framebuffer_resize(GLFWwindow* window, int, int) {
    auto* context = static_cast<vk_window::VulkanContext*>(glfwGetWindowUserPointer(window));
    if (context) {
        context->framebufferResized = true;
        context->lastFramebufferResizeTime = glfwGetTime();
    }
}

} // namespace

extern "C" const ReshadeVKHostInputState* reshadevk_host_get_input_state() {
    return &g_hostInputState;
}

int main() {
    if (!glfwInit()) {
        std::printf("Erro: nao foi possivel inicializar o GLFW\n");
        return 1;
    }

    vk_window::AppConfig config{};

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(
        static_cast<int>(config.width),
        static_cast<int>(config.height),
        config.windowTitle.c_str(),
        nullptr,
        nullptr
    );

    if (!window) {
        std::printf("Erro: nao foi possivel criar a janela\n");
        glfwTerminate();
        return 1;
    }

    vk_window::VulkanContext context{};
    glfwSetWindowUserPointer(window, &context);
    glfwSetFramebufferSizeCallback(window, on_framebuffer_resize);

    if (!vk_window::init_all(context, window, config)) {
        std::printf("Erro: falha ao inicializar Vulkan\n");
        vk_window::destroy_all(context);
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        update_host_input_state(window);
        if (!vk_window::render_once(context, config)) {
            break;
        }
    }

    vk_window::destroy_all(context);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

#include "context.hpp"

#include <cstdio>

#include "device/declarations.hpp"
#include "instance/declarations.hpp"
#include "render/declarations.hpp"
#include "swapchain/declarations.hpp"

namespace vk_window {

bool init_all(VulkanContext& context, GLFWwindow* window, const AppConfig& config) {
    context.window = window;
    context.framebufferResized = false;
    context.lastFramebufferResizeTime = 0.0;

    // Context only orchestrates modules in startup order.
    if (!instance::create_instance(context, config)) {
        destroy_all(context);
        return false;
    }

    if (!instance::create_surface(context, window)) {
        destroy_all(context);
        return false;
    }

    if (!device::pick_physical_device(context)) {
        destroy_all(context);
        return false;
    }

    if (!device::create_logical_device(context)) {
        destroy_all(context);
        return false;
    }

    if (!swapchain::create_swapchain(context, window)) {
        destroy_all(context);
        return false;
    }

    if (!render::create_render_resources(context)) {
        destroy_all(context);
        return false;
    }

    return true;
}

bool render_once(VulkanContext& context, const AppConfig& config) {
    return render::draw_frame(context, config);
}

void device_wait_idle(VulkanContext& context) {
    if (context.device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(context.device);
    }
}

void destroy_all(VulkanContext& context) {
    // Teardown is the reverse order of startup.
    device_wait_idle(context);
    render::destroy_render_resources(context);
    swapchain::destroy_swapchain(context);
    device::destroy_device_resources(context);
    instance::destroy_instance_resources(context);
    std::printf("Vulkan: recursos destruidos\n");
}

} // namespace vk_window

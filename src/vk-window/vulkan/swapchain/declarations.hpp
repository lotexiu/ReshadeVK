#pragma once

#include "../context.hpp"

namespace vk_window::swapchain {

bool create_swapchain(VulkanContext& context, GLFWwindow* window);
void destroy_swapchain(VulkanContext& context);

} // namespace vk_window::swapchain

#pragma once

#include "app.hpp"

namespace vk_window::instance {

bool create_instance(VulkanContext& context, const AppConfig& config);
bool create_surface(VulkanContext& context, GLFWwindow* window);
void destroy_instance_resources(VulkanContext& context);

} // namespace vk_window::instance

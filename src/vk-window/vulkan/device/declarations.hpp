#pragma once

#include "../context.hpp"

namespace vk_window::device {

bool pick_physical_device(VulkanContext& context);
bool create_logical_device(VulkanContext& context);
void destroy_device_resources(VulkanContext& context);

} // namespace vk_window::device

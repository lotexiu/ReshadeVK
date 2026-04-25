#pragma once

#include "app.hpp"

namespace vk_window::render {

bool create_render_resources(VulkanContext& context);
bool draw_frame(VulkanContext& context, const AppConfig& config);
void destroy_render_resources(VulkanContext& context);

} // namespace vk_window::render

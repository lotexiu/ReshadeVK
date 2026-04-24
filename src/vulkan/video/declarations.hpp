#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>

#include "../declarations.hpp"

void vk_pick_physical_device(VulkanContext& ctx);
void vk_create_logical_device(VulkanContext& ctx);
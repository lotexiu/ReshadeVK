#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "../declarations.hpp"

// Funções que vamos implementar no .cpp
void vk_create_instance(VulkanContext& ctx);
void vk_create_surface(VulkanContext& ctx, GLFWwindow* window);
void vk_destroy(VulkanContext& ctx);
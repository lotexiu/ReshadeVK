#pragma once

#include <vulkan/vulkan.h>

// Estrutura que vai guardar tudo relacionado ao Vulkan
struct VulkanContext {
    VkInstance       instance;
    VkSurfaceKHR     surface;
    VkPhysicalDevice physical_device;
    VkDevice         device;
    VkQueue          graphics_queue;
    uint32_t         graphics_family;
};
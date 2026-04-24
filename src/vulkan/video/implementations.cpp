#include "declarations.hpp"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstring>
#include <vector>

static bool is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t& out_family) {
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);

    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

    for (uint32_t i = 0; i < count; i++) {
        // tem suporte a gráficos?
        bool has_graphics = families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;

        // tem suporte a apresentação na surface?
        VkBool32 has_present = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &has_present);

        if (has_graphics && has_present) {
            out_family = i;
            return true;
        }
    }
    return false;
}

void vk_pick_physical_device(VulkanContext& ctx) {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(ctx.instance, &count, nullptr);

    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(ctx.instance, &count, devices.data());

    for (auto& device : devices) {
        if (is_device_suitable(device, ctx.surface, ctx.graphics_family)) {
            ctx.physical_device = device;

            // mostra o nome da GPU selecionada
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device, &props);
            printf("Vulkan: GPU selecionada: %s\n", props.deviceName);
            return;
        }
    }

    printf("Erro: nenhuma GPU compativel encontrada\n");
}

void vk_create_logical_device(VulkanContext& ctx) {
    // configura a queue que queremos
    float priority = 1.0f;
    VkDeviceQueueCreateInfo queue_ci{};
    queue_ci.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_ci.queueFamilyIndex = ctx.graphics_family;
    queue_ci.queueCount       = 1;
    queue_ci.pQueuePriorities = &priority;

    // extensão necessária pra poder apresentar imagens na tela
    const char* extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo ci{};
    ci.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    ci.queueCreateInfoCount    = 1;
    ci.pQueueCreateInfos       = &queue_ci;
    ci.enabledExtensionCount   = 1;
    ci.ppEnabledExtensionNames = extensions;

    if (vkCreateDevice(ctx.physical_device, &ci, nullptr, &ctx.device) != VK_SUCCESS) {
        printf("Erro: nao foi possivel criar o logical device\n");
        return;
    }

    // pega o handle da queue criada junto com o device
    vkGetDeviceQueue(ctx.device, ctx.graphics_family, 0, &ctx.graphics_queue);

    printf("Vulkan: logical device criado\n");
    printf("Vulkan: queue de graficos obtida (family %u)\n", ctx.graphics_family);
}
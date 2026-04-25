#include "declarations.hpp"

#include <cstdio>
#include <cstring>
#include <vector>

namespace {

bool find_queue_families(
    VkPhysicalDevice device,
    VkSurfaceKHR surface,
    uint32_t& graphicsFamily,
    uint32_t& presentFamily
) {
    uint32_t familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);

    std::vector<VkQueueFamilyProperties> families(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, families.data());

    bool hasGraphics = false;
    bool hasPresent = false;

    for (uint32_t i = 0; i < familyCount; i++) {
        if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0U) {
            graphicsFamily = i;
            hasGraphics = true;
        }

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport == VK_TRUE) {
            presentFamily = i;
            hasPresent = true;
        }

        if (hasGraphics && hasPresent) {
            return true;
        }
    }

    return false;
}

bool has_swapchain_support(VkPhysicalDevice device) {
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

    for (const auto& extension : extensions) {
        if (std::strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            return true;
        }
    }

    return false;
}

} // namespace

namespace vk_window::device {

bool pick_physical_device(VulkanContext& context) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(context.instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        std::printf("Erro: nenhuma GPU com suporte Vulkan encontrada\n");
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(context.instance, &deviceCount, devices.data());

    for (VkPhysicalDevice candidate : devices) {
        uint32_t graphicsFamily = 0;
        uint32_t presentFamily = 0;
        if (!find_queue_families(candidate, context.surface, graphicsFamily, presentFamily)) {
            continue;
        }
        if (!has_swapchain_support(candidate)) {
            continue;
        }

        context.physicalDevice = candidate;
        context.graphicsFamily = graphicsFamily;
        context.presentFamily = presentFamily;

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(candidate, &properties);
        std::printf("Vulkan: GPU selecionada: %s\n", properties.deviceName);
        return true;
    }

    std::printf("Erro: nenhuma GPU compativel com swapchain encontrada\n");
    return false;
}

bool create_logical_device(VulkanContext& context) {
    if (context.physicalDevice == VK_NULL_HANDLE) {
        std::printf("Erro: physical device invalido\n");
        return false;
    }

    std::vector<uint32_t> families;
    families.push_back(context.graphicsFamily);
    if (context.presentFamily != context.graphicsFamily) {
        families.push_back(context.presentFamily);
    }

    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    queueInfos.reserve(families.size());

    const float queuePriority = 1.0f;
    for (uint32_t family : families) {
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = family;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &queuePriority;
        queueInfos.push_back(queueInfo);
    }

    const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
    createInfo.pQueueCreateInfos = queueInfos.data();
    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = deviceExtensions;

    const VkResult result = vkCreateDevice(context.physicalDevice, &createInfo, nullptr, &context.device);
    if (result != VK_SUCCESS) {
        std::printf("Erro: nao foi possivel criar o logical device (%d)\n", result);
        context.device = VK_NULL_HANDLE;
        return false;
    }

    vkGetDeviceQueue(context.device, context.graphicsFamily, 0, &context.graphicsQueue);
    vkGetDeviceQueue(context.device, context.presentFamily, 0, &context.presentQueue);

    std::printf("Vulkan: logical device criado\n");
    return true;
}

void destroy_device_resources(VulkanContext& context) {
    if (context.device != VK_NULL_HANDLE) {
        vkDestroyDevice(context.device, nullptr);
        context.device = VK_NULL_HANDLE;
    }
    context.physicalDevice = VK_NULL_HANDLE;
}

} // namespace vk_window::device

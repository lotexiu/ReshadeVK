#include "declarations.hpp"

#include <algorithm>
#include <cstdio>
#include <vector>

namespace {

VkSurfaceFormatKHR pick_surface_format(VkPhysicalDevice device, VkSurfaceKHR surface) {
    uint32_t count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr);

    std::vector<VkSurfaceFormatKHR> formats(count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, formats.data());

    for (const auto& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }

    return formats[0];
}

VkPresentModeKHR pick_present_mode(VkPhysicalDevice device, VkSurfaceKHR surface) {
    uint32_t count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, nullptr);

    std::vector<VkPresentModeKHR> modes(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, modes.data());

    for (const auto& mode : modes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D pick_extent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D extent{};
    extent.width = static_cast<uint32_t>(width);
    extent.height = static_cast<uint32_t>(height);

    extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return extent;
}

} // namespace

namespace vk_window::swapchain {

bool create_swapchain(VulkanContext& context, GLFWwindow* window) {
    VkSurfaceCapabilitiesKHR capabilities{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.physicalDevice, context.surface, &capabilities);

    const VkSurfaceFormatKHR surfaceFormat = pick_surface_format(context.physicalDevice, context.surface);
    const VkPresentModeKHR presentMode = pick_present_mode(context.physicalDevice, context.surface);
    const VkExtent2D extent = pick_extent(capabilities, window);

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = context.surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = {context.graphicsFamily, context.presentFamily};
    if (context.graphicsFamily != context.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    const VkResult createResult = vkCreateSwapchainKHR(context.device, &createInfo, nullptr, &context.swapchain);
    if (createResult != VK_SUCCESS) {
        std::printf("Erro: nao foi possivel criar a swapchain (%d)\n", createResult);
        context.swapchain = VK_NULL_HANDLE;
        return false;
    }

    context.swapchainFormat = surfaceFormat.format;
    context.swapchainExtent = extent;

    uint32_t realImageCount = 0;
    vkGetSwapchainImagesKHR(context.device, context.swapchain, &realImageCount, nullptr);
    context.swapchainImages.resize(realImageCount);
    vkGetSwapchainImagesKHR(context.device, context.swapchain, &realImageCount, context.swapchainImages.data());

    context.swapchainImageViews.resize(realImageCount);
    for (uint32_t i = 0; i < realImageCount; i++) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = context.swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = context.swapchainFormat;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        const VkResult viewResult = vkCreateImageView(context.device, &viewInfo, nullptr, &context.swapchainImageViews[i]);
        if (viewResult != VK_SUCCESS) {
            std::printf("Erro: nao foi possivel criar image view %u (%d)\n", i, viewResult);
            return false;
        }
    }

    std::printf(
        "Vulkan: swapchain criada (%ux%u, %u imagens)\n",
        extent.width,
        extent.height,
        realImageCount
    );
    return true;
}

void destroy_swapchain(VulkanContext& context) {
    for (VkImageView imageView : context.swapchainImageViews) {
        vkDestroyImageView(context.device, imageView, nullptr);
    }
    context.swapchainImageViews.clear();
    context.swapchainImages.clear();

    if (context.swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(context.device, context.swapchain, nullptr);
        context.swapchain = VK_NULL_HANDLE;
    }
}

} // namespace vk_window::swapchain

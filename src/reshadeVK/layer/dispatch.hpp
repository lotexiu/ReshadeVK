#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <vulkan/vk_layer.h>

namespace reshadevk {

// ── Function lists ────────────────────────────────────────────────────────────
// Each entry expands to a field: PFN_vk<name> <name> = nullptr;

#define RESHADEVK_INSTANCE_FUNCS   \
    FUNC(DestroyInstance)          \
    FUNC(DestroySurfaceKHR)        \
    FUNC(GetInstanceProcAddr)      \
    FUNC(EnumeratePhysicalDevices) \
    FUNC(GetPhysicalDeviceMemoryProperties)   \
    FUNC(GetPhysicalDeviceQueueFamilyProperties) \
    FUNC(GetPhysicalDeviceProperties)          \
    FUNC(EnumerateDeviceExtensionProperties)

#define RESHADEVK_DEVICE_FUNCS          \
    FUNC(GetDeviceProcAddr)             \
    FUNC(DestroyDevice)                 \
    FUNC(GetDeviceQueue)                \
    FUNC(CreateSwapchainKHR)            \
    FUNC(DestroySwapchainKHR)           \
    FUNC(GetSwapchainImagesKHR)         \
    FUNC(QueuePresentKHR)               \
    FUNC(CreateCommandPool)             \
    FUNC(DestroyCommandPool)            \
    FUNC(ResetCommandPool)              \
    FUNC(AllocateCommandBuffers)        \
    FUNC(FreeCommandBuffers)            \
    FUNC(BeginCommandBuffer)            \
    FUNC(EndCommandBuffer)              \
    FUNC(ResetCommandBuffer)            \
    FUNC(CmdBeginRenderPass)            \
    FUNC(CmdEndRenderPass)              \
    FUNC(CmdPipelineBarrier)            \
    FUNC(CreateRenderPass)              \
    FUNC(DestroyRenderPass)             \
    FUNC(CreateFramebuffer)             \
    FUNC(DestroyFramebuffer)            \
    FUNC(CreateImageView)               \
    FUNC(DestroyImageView)              \
    FUNC(CreateDescriptorPool)          \
    FUNC(DestroyDescriptorPool)         \
    FUNC(AllocateDescriptorSets)        \
    FUNC(UpdateDescriptorSets)          \
    FUNC(CreateDescriptorSetLayout)     \
    FUNC(DestroyDescriptorSetLayout)    \
    FUNC(CreateSampler)                 \
    FUNC(DestroySampler)                \
    FUNC(CreatePipelineLayout)          \
    FUNC(DestroyPipelineLayout)         \
    FUNC(CreateShaderModule)            \
    FUNC(DestroyShaderModule)           \
    FUNC(CreateGraphicsPipelines)       \
    FUNC(DestroyPipeline)               \
    FUNC(CmdBindPipeline)               \
    FUNC(CmdBindDescriptorSets)         \
    FUNC(CmdBindVertexBuffers)          \
    FUNC(CmdBindIndexBuffer)            \
    FUNC(CmdDraw)                       \
    FUNC(CmdDrawIndexed)                \
    FUNC(CmdSetViewport)                \
    FUNC(CmdSetScissor)                 \
    FUNC(CmdCopyBufferToImage)          \
    FUNC(CreateBuffer)                  \
    FUNC(DestroyBuffer)                 \
    FUNC(CreateImage)                   \
    FUNC(DestroyImage)                  \
    FUNC(BindBufferMemory)              \
    FUNC(BindImageMemory)               \
    FUNC(AllocateMemory)                \
    FUNC(FreeMemory)                    \
    FUNC(MapMemory)                     \
    FUNC(UnmapMemory)                   \
    FUNC(FlushMappedMemoryRanges)       \
    FUNC(GetBufferMemoryRequirements)   \
    FUNC(GetImageMemoryRequirements)    \
    FUNC(CreateSemaphore)               \
    FUNC(DestroySemaphore)              \
    FUNC(CreateFence)                   \
    FUNC(DestroyFence)                  \
    FUNC(WaitForFences)                 \
    FUNC(ResetFences)                   \
    FUNC(QueueSubmit)                   \
    FUNC(DeviceWaitIdle)                \
    FUNC(QueueWaitIdle)

// ── Dispatch table structs ────────────────────────────────────────────────────

struct InstanceDispatch {
#define FUNC(name) PFN_vk##name name = nullptr;
    RESHADEVK_INSTANCE_FUNCS
#undef FUNC
};

struct DeviceDispatch {
#define FUNC(name) PFN_vk##name name = nullptr;
    RESHADEVK_DEVICE_FUNCS
#undef FUNC
};

void fill_instance_dispatch(VkInstance instance, PFN_vkGetInstanceProcAddr gipa, InstanceDispatch& out);
void fill_device_dispatch(VkDevice device, PFN_vkGetDeviceProcAddr gdpa, DeviceDispatch& out);

} // namespace reshadevk

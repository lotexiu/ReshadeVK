#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

namespace reshadevk {

// ── RenderContext ─────────────────────────────────────────────────────────────
// Passed to LayerComponent::onRender() each frame.
// Provides the command buffer, swapchain extent and current image index.
// For overlay work, ImGui calls suffice; cmd is available for low-level Vulkan.

struct RenderContext {
	VkCommandBuffer cmd        = VK_NULL_HANDLE;
	VkExtent2D      extent     = {};
	uint32_t        imageIndex = 0;
};

// ── LayerComponent ────────────────────────────────────────────────────────────
// Base class for concrete layers. Inherit and override onRender().
// Registered once at library load by entry.cpp via set_layer_component().

struct LayerComponent {
	virtual void onInit()                          {}
	virtual void onRender(const RenderContext& ctx) = 0;
	virtual void onDestroy()                       {}
	virtual ~LayerComponent()                      = default;
};

// Registers the active component. Deletes the previous one. Called from entry.cpp.
void          set_layer_component(LayerComponent* component);
LayerComponent* get_layer_component();

} // namespace reshadevk

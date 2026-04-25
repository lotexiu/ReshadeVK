#pragma once

#include "utils/layer.hpp"

namespace reshadevk {

// Concrete overlay component.
// All Vulkan infrastructure is handled by utils/api/vulkan/renderer.
// onRender() only needs to issue ImGui widget calls.

struct ReshadeOverlay : LayerComponent {
	void onRender(const RenderContext& ctx) override;
};

} // namespace reshadevk

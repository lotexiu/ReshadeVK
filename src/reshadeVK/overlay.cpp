#include "overlay.hpp"

#include <imgui.h>

namespace reshadevk {

void ReshadeOverlay::onRender(const RenderContext& ctx) {
	(void)ctx; // extent, imageIndex and cmd are available for low-level use
	ImGui::ShowDemoWindow();
}

} // namespace reshadevk

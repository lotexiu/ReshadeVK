#include "layer.hpp"

namespace reshadevk {

static LayerComponent* g_component = nullptr;

void set_layer_component(LayerComponent* component) {
	delete g_component;
	g_component = component;
}

LayerComponent* get_layer_component() {
	return g_component;
}

} // namespace reshadevk

#include <cstdio>
#include <cstring>
#include <vector>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "declarations.hpp"

// Validation layers que queremos ativar
const char* VALIDATION_LAYERS[] = {
	"VK_LAYER_KHRONOS_validation"
};

// Verifica se as validation layers estão disponíveis no sistema
bool check_validation_layers() {
	uint32_t count = 0;
	vkEnumerateInstanceLayerProperties(&count, nullptr);

	std::vector<VkLayerProperties> available(count);
	vkEnumerateInstanceLayerProperties(&count, available.data());

	for (const char* name : VALIDATION_LAYERS) {
		bool found = false;
		for (auto& layer : available)
			if (strcmp(layer.layerName, name) == 0) { found = true; break; }
		if (!found) return false;
	}
	return true;
}
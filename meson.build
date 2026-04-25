project(
	'ReshadeVK',
	'cpp',
	version: '0.1',
)

# ── System dependencies ────────────────────────────────────────────────────────
glfw_dep = dependency('glfw3')
api_dep  = dependency('vulkan')
x11_dep  = dependency('x11', required: false)

# ── Include directories ────────────────────────────────────────────────────────
imgui_inc      = include_directories('libs/imgui', 'libs/imgui/backends')
src_inc        = include_directories('src')
vk_window_inc  = include_directories('src/VKWindow')

# ── ImGui sources (shared between layer and VKWindow if needed) ────────────────
imgui_vulkan_sources = files(
	'libs/imgui/imgui.cpp',
	'libs/imgui/imgui_demo.cpp',
	'libs/imgui/imgui_draw.cpp',
	'libs/imgui/imgui_tables.cpp',
	'libs/imgui/imgui_widgets.cpp',
	'libs/imgui/backends/imgui_impl_vulkan.cpp',
)

# ── VKWindow — standalone Vulkan window for layer development ──────────────────
vk_window_sources = files(
	'src/VKWindow/main.cpp',
	'src/VKWindow/app.cpp',
	'src/VKWindow/vulkan/instance/implementations.cpp',
	'src/VKWindow/vulkan/device/implementations.cpp',
	'src/VKWindow/vulkan/swapchain/implementations.cpp',
	'src/VKWindow/vulkan/render/implementations.cpp',
)

executable(
	'VKWindow',
	sources: vk_window_sources,
	include_directories: [vk_window_inc, src_inc],
	dependencies: [glfw_dep, api_dep],
	link_args: ['-Wl,--export-dynamic'],
)

# ── reshadeVK layer ────────────────────────────────────────────────────────────
reshade_vk_layer_sources = files(
	# Concrete layer
	'src/reshadeVK/entry.cpp',
	'src/reshadeVK/overlay.cpp',
	# Framework core
	'src/utils/layer.cpp',
	# Vulkan dispatch + state
	'src/utils/api/vulkan/dispatch.cpp',
	'src/utils/api/vulkan/state.cpp',
	# ImGui Vulkan loader shim
	'src/utils/api/vulkan/imgui_loader.cpp',
	# Renderer (Vulkan infra + overlay frame loop)
	'src/utils/api/vulkan/renderer.cpp',
	# Vulkan lifecycle hooks
	'src/utils/api/vulkan/hooks/instance.cpp',
	'src/utils/api/vulkan/hooks/physicalDevice.cpp',
	'src/utils/api/vulkan/hooks/device.cpp',
	'src/utils/api/vulkan/hooks/surface.cpp',
	'src/utils/api/vulkan/hooks/swapchain.cpp',
	# Input — GLFW provider (always compiled; GLFW is always available)
	'src/utils/input/glfw.cpp',
)

layer_cpp_args = ['-DIMGUI_IMPL_VULKAN_NO_PROTOTYPES']
layer_deps     = [api_dep, glfw_dep]
layer_cpp_args += ['-DRESHADEVK_HAS_GLFW=1']

if x11_dep.found()
	reshade_vk_layer_sources += files('src/utils/input/x11.cpp')
	layer_cpp_args += ['-DRESHADEVK_HAS_X11=1']
	layer_deps     += [x11_dep]
else
	layer_cpp_args += ['-DRESHADEVK_HAS_X11=0']
endif

shared_library(
	'reshadevk',
	sources: reshade_vk_layer_sources + imgui_vulkan_sources,
	include_directories: [src_inc, imgui_inc],
	cpp_args: layer_cpp_args,
	dependencies: layer_deps,
	gnu_symbol_visibility: 'hidden',
	install: true,
	install_dir: get_option('libdir'),
)

# ── Layer manifest ─────────────────────────────────────────────────────────────
# Installed to ~/.local/share/vulkan/implicit_layer.d/ (or system equivalent)
layer_conf = configuration_data()
layer_conf.set('library_path',
	join_paths(get_option('prefix'), get_option('libdir'), 'libreshadevk.so'))

configure_file(
	input:         'src/reshadeVK/layer.json.in',
	output:        'reshadevk.json',
	configuration: layer_conf,
	install:       true,
	install_dir:   join_paths(get_option('datadir'), 'vulkan', 'implicit_layer.d'),
)
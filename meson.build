project(
	'ReshadeVK',
	'cpp',
	version: '0.1',
)

# Dependencias do sistema
glfw_dep = dependency('glfw3')
api_dep  = dependency('vulkan')
x11_dep  = dependency('x11', required: false)

imgui_inc = include_directories('libs/imgui', 'libs/imgui/backends')
src_inc = include_directories('src')

vk_window_sources = files(
	'src/vk-window/main.cpp',
	'src/vk-window/vulkan/context.cpp',
	'src/vk-window/vulkan/instance/implementations.cpp',
	'src/vk-window/vulkan/device/implementations.cpp',
	'src/vk-window/vulkan/swapchain/implementations.cpp',
	'src/vk-window/vulkan/render/implementations.cpp',
)

vk_window_inc = include_directories('src/vk-window')

executable(
	'VKWindow',
	sources: vk_window_sources,
	include_directories: [vk_window_inc, src_inc],
	dependencies: [glfw_dep, api_dep],
	link_args: ['-Wl,--export-dynamic'],
)

reshade_vk_layer_sources = files(
	'src/reshadeVK/state.cpp',
	'src/reshadeVK/layer/dispatch.cpp',
	'src/reshadeVK/layer/entry.cpp',
	'src/reshadeVK/overlay/implementations.cpp',
)

imgui_vulkan_sources = files(
	'libs/imgui/imgui.cpp',
	'libs/imgui/imgui_demo.cpp',
	'libs/imgui/imgui_draw.cpp',
	'libs/imgui/imgui_tables.cpp',
	'libs/imgui/imgui_widgets.cpp',
	'libs/imgui/backends/imgui_impl_vulkan.cpp',
)

reshade_vk_layer_inc = include_directories('src/reshadeVK')

layer_cpp_args = ['-DIMGUI_IMPL_VULKAN_NO_PROTOTYPES']
layer_deps = [api_dep, glfw_dep]
layer_cpp_args += ['-DRESHADEVK_HAS_GLFW=1']

if x11_dep.found()
	layer_cpp_args += ['-DRESHADEVK_HAS_X11=1']
	layer_deps += [x11_dep]
else
	layer_cpp_args += ['-DRESHADEVK_HAS_X11=0']
endif

shared_library(
	'reshadevk',
	sources: reshade_vk_layer_sources + imgui_vulkan_sources,
	include_directories: [reshade_vk_layer_inc, imgui_inc, src_inc],
	cpp_args: layer_cpp_args,
	dependencies: layer_deps,
	gnu_symbol_visibility: 'hidden',
	install: true,
	install_dir: get_option('libdir'),
)

# Layer manifest — installed to ~/.local/share/vulkan/implicit_layer.d/
layer_conf = configuration_data()
layer_conf.set('library_path',
	join_paths(get_option('prefix'), get_option('libdir'), 'libreshadevk.so'))

configure_file(
	input:         'src/reshadeVK/reshadevk.json.in',
	output:        'reshadevk.json',
	configuration: layer_conf,
	install:       true,
	install_dir:   join_paths(get_option('datadir'), 'vulkan', 'implicit_layer.d'),
)
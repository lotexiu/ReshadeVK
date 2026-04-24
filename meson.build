project(
	'C++',
	'cpp',
	version: '0.1',
)

# Dependencias do sistema
glfw_dep = dependency('glfw3')
api_dep = dependency('vulkan')

# Arquivos do ImGui (core + backends que precisamos)
imgui_sources = files(
	'libs/imgui/backends/imgui_impl_glfw.cpp',
	'libs/imgui/backends/imgui_impl_opengl3.cpp',
	# 'libs/imgui/backends/imgui_impl_vulkan.cpp',
	'libs/imgui/imgui.cpp',
	'libs/imgui/imgui_demo.cpp', # janela de demo — útil pra aprender
	'libs/imgui/imgui_draw.cpp',
	'libs/imgui/imgui_tables.cpp',
	'libs/imgui/imgui_widgets.cpp',
)

imgui_inc = include_directories('libs/imgui', 'libs/imgui/backends')

src = files(
	'src/main.cpp',
	'src/vulkan/instance/implementations.cpp',
	'src/vulkan/validation/implementations.cpp',
	'src/vulkan/video/implementations.cpp',
)

executable(
	'reshadeVK',
	sources: src + imgui_sources,
	include_directories: imgui_inc,
	# sources : ['src/main.cpp'],
	dependencies: [glfw_dep, api_dep],
)
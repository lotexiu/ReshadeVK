#include "declarations.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <limits>
#include <vector>

#include <imgui.h>
#include <imgui_impl_vulkan.h>

#include "shared/host_input_bridge.hpp"

#if RESHADEVK_HAS_GLFW
#include <GLFW/glfw3.h>
#endif

#if RESHADEVK_HAS_X11
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <unistd.h>
#endif

namespace reshadevk::overlay {

static PFN_vkAllocateCommandBuffers g_imguiAllocateCommandBuffers = nullptr;
static PFN_vkGetDeviceQueue g_imguiGetDeviceQueue = nullptr;
static const ReshadeVKHostInputState* (*g_hostInputGetter)() = nullptr;
static bool g_hostInputLookupDone = false;
static bool g_loggedHostBridge = false;
static bool g_loggedGlfwInput = false;
static bool g_loggedOverlayInit = false;

static const ReshadeVKHostInputState* get_host_input_state() {
	if (!g_hostInputLookupDone) {
		g_hostInputLookupDone = true;
		g_hostInputGetter = reinterpret_cast<const ReshadeVKHostInputState* (*)()>(
				dlsym(RTLD_DEFAULT, "reshadevk_host_get_input_state"));
	}

	if (!g_hostInputGetter)
		return nullptr;

	return g_hostInputGetter();
}

static void apply_host_input(SwapchainData& swData, const ReshadeVKHostInputState& host) {
	ImGuiIO& io = ImGui::GetIO();
	const bool hasFocus = host.focused;
	swData.inputFocused = hasFocus;
	io.AddFocusEvent(hasFocus);

	const bool toggleDown = host.keyToggleF8;
	if (toggleDown && !swData.prevToggleKeyDown && hasFocus) {
		swData.interactiveEnabled = !swData.interactiveEnabled;
		std::printf("[reshadeVK] input %s (F8)\n", swData.interactiveEnabled ? "ativado" : "desativado");
	}
	swData.prevToggleKeyDown = toggleDown;

	const bool canCapture = hasFocus && swData.interactiveEnabled;

	if (canCapture) {
		io.AddMousePosEvent(static_cast<float>(host.mouseX), static_cast<float>(host.mouseY));
	} else {
		io.AddMousePosEvent(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
	}

	for (int i = 0; i < 5; i++) {
		const bool down = canCapture && host.mouseButtons[i];
		if (down != swData.prevMouseDown[i]) {
			io.AddMouseButtonEvent(i, down);
			swData.prevMouseDown[i] = down;
		}
	}

	io.AddKeyEvent(ImGuiKey_Tab, canCapture && host.keyTab);
	io.AddKeyEvent(ImGuiKey_LeftArrow, canCapture && host.keyLeft);
	io.AddKeyEvent(ImGuiKey_RightArrow, canCapture && host.keyRight);
	io.AddKeyEvent(ImGuiKey_UpArrow, canCapture && host.keyUp);
	io.AddKeyEvent(ImGuiKey_DownArrow, canCapture && host.keyDown);
	io.AddKeyEvent(ImGuiKey_Enter, canCapture && host.keyEnter);
	io.AddKeyEvent(ImGuiKey_Escape, canCapture && host.keyEscape);
	io.AddKeyEvent(ImGuiKey_Backspace, canCapture && host.keyBackspace);
	io.AddKeyEvent(ImGuiKey_Space, canCapture && host.keySpace);
}

#if RESHADEVK_HAS_GLFW
static void update_input_glfw(SwapchainData& swData) {
	auto* window = static_cast<GLFWwindow*>(swData.glfwWindow);
	if (!window)
		return;

	ImGuiIO& io = ImGui::GetIO();
	const bool hasFocus = glfwGetWindowAttrib(window, GLFW_FOCUSED) == GLFW_TRUE;
	swData.inputFocused = hasFocus;
	io.AddFocusEvent(hasFocus);

	const bool toggleDown = glfwGetKey(window, GLFW_KEY_F8) == GLFW_PRESS;
	if (toggleDown && !swData.prevToggleKeyDown && hasFocus) {
		swData.interactiveEnabled = !swData.interactiveEnabled;
		std::printf("[reshadeVK] input %s (F8)\n", swData.interactiveEnabled ? "ativado" : "desativado");
	}
	swData.prevToggleKeyDown = toggleDown;

	const bool canCapture = hasFocus && swData.interactiveEnabled;

	double mouseX = 0.0;
	double mouseY = 0.0;
	glfwGetCursorPos(window, &mouseX, &mouseY);
	if (canCapture) {
		io.AddMousePosEvent(static_cast<float>(mouseX), static_cast<float>(mouseY));
	} else {
		io.AddMousePosEvent(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
	}

	const bool nextMouseDown[5] = {
		canCapture && (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS),
		canCapture && (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS),
		canCapture && (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS),
		canCapture && (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_4) == GLFW_PRESS),
		canCapture && (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_5) == GLFW_PRESS),
	};

	for (int i = 0; i < 5; i++) {
		if (nextMouseDown[i] != swData.prevMouseDown[i]) {
			io.AddMouseButtonEvent(i, nextMouseDown[i]);
			swData.prevMouseDown[i] = nextMouseDown[i];
		}
	}

	io.AddKeyEvent(ImGuiKey_Tab, canCapture && glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS);
	io.AddKeyEvent(ImGuiKey_LeftArrow, canCapture && glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS);
	io.AddKeyEvent(ImGuiKey_RightArrow, canCapture && glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS);
	io.AddKeyEvent(ImGuiKey_UpArrow, canCapture && glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS);
	io.AddKeyEvent(ImGuiKey_DownArrow, canCapture && glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS);
	io.AddKeyEvent(ImGuiKey_Enter, canCapture && (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_KP_ENTER) == GLFW_PRESS));
	io.AddKeyEvent(ImGuiKey_Escape, canCapture && glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS);
	io.AddKeyEvent(ImGuiKey_Backspace, canCapture && glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS);
	io.AddKeyEvent(ImGuiKey_Space, canCapture && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);
}
#endif

#if RESHADEVK_HAS_X11
static bool x11_key_down(Display* display, const char keymap[32], KeySym keysym) {
	KeyCode keycode = XKeysymToKeycode(display, keysym);
	if (keycode == 0)
		return false;
	return (keymap[keycode >> 3] & (1 << (keycode & 7))) != 0;
}

static Window x11_get_active_window(Display* display) {
	Atom activeAtom = XInternAtom(display, "_NET_ACTIVE_WINDOW", True);
	if (activeAtom == None)
		return 0;

	Atom actualType = None;
	int actualFormat = 0;
	unsigned long itemCount = 0;
	unsigned long bytesAfter = 0;
	unsigned char* data = nullptr;

	int rc = XGetWindowProperty(
			display,
			DefaultRootWindow(display),
			activeAtom,
			0,
			1,
			False,
			AnyPropertyType,
			&actualType,
			&actualFormat,
			&itemCount,
			&bytesAfter,
			&data);

	if (rc != Success || !data || itemCount == 0) {
		if (data)
			XFree(data);
		return 0;
	}

	Window active = *reinterpret_cast<Window*>(data);
	XFree(data);
	return active;
}

static pid_t x11_get_window_pid(Display* display, Window window) {
	Atom pidAtom = XInternAtom(display, "_NET_WM_PID", True);
	if (pidAtom == None || window == 0)
		return -1;

	Atom actualType = None;
	int actualFormat = 0;
	unsigned long itemCount = 0;
	unsigned long bytesAfter = 0;
	unsigned char* data = nullptr;

	int rc = XGetWindowProperty(
			display,
			window,
			pidAtom,
			0,
			1,
			False,
			XA_CARDINAL,
			&actualType,
			&actualFormat,
			&itemCount,
			&bytesAfter,
			&data);

	if (rc != Success || !data || itemCount == 0) {
		if (data)
			XFree(data);
		return -1;
	}

	pid_t pid = static_cast<pid_t>(*reinterpret_cast<unsigned long*>(data));
	XFree(data);
	return pid;
}

static void update_input_x11(SwapchainData& swData) {
	auto* display = static_cast<Display*>(swData.nativeDisplay);
	if (!display) {
		display = XOpenDisplay(nullptr);
		if (!display)
			return;
		swData.nativeDisplay = display;
		swData.ownsDisplayConnection = true;
	}

	const Window window = x11_get_active_window(display);
	swData.nativeWindow = static_cast<unsigned long>(window);

	ImGuiIO& io = ImGui::GetIO();

	const pid_t myPid = getpid();
	const pid_t activePid = x11_get_window_pid(display, window);
	const bool hasFocus = (window != 0 && activePid == myPid);
	swData.inputFocused = hasFocus;
	io.AddFocusEvent(hasFocus);

	char keymap[32] = { };
	XQueryKeymap(display, keymap);

	const bool toggleDown = x11_key_down(display, keymap, XK_F8);
	if (toggleDown && !swData.prevToggleKeyDown && hasFocus) {
		swData.interactiveEnabled = !swData.interactiveEnabled;
		std::printf("[reshadeVK] input %s (F8)\n", swData.interactiveEnabled ? "ativado" : "desativado");
	}
	swData.prevToggleKeyDown = toggleDown;

	const bool canCapture = hasFocus && swData.interactiveEnabled;

	Window root = 0;
	Window child = 0;
	int rootX = 0;
	int rootY = 0;
	int winX = 0;
	int winY = 0;
	unsigned int mask = 0;
	Bool pointerInside = False;
	if (window != 0)
		pointerInside = XQueryPointer(display, window, &root, &child, &rootX, &rootY, &winX, &winY, &mask);

	if (canCapture && pointerInside) {
		io.AddMousePosEvent(static_cast<float>(winX), static_cast<float>(winY));
	} else {
		io.AddMousePosEvent(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
	}

	const bool nextMouseDown[5] = {
		canCapture && ((mask & Button1Mask) != 0),
		canCapture && ((mask & Button3Mask) != 0),
		canCapture && ((mask & Button2Mask) != 0),
		false,
		false,
	};

	for (int i = 0; i < 5; i++) {
		if (nextMouseDown[i] != swData.prevMouseDown[i]) {
			io.AddMouseButtonEvent(i, nextMouseDown[i]);
			swData.prevMouseDown[i] = nextMouseDown[i];
		}
	}

	const bool keyTab = canCapture && x11_key_down(display, keymap, XK_Tab);
	const bool keyLeft = canCapture && x11_key_down(display, keymap, XK_Left);
	const bool keyRight = canCapture && x11_key_down(display, keymap, XK_Right);
	const bool keyUp = canCapture && x11_key_down(display, keymap, XK_Up);
	const bool keyDown = canCapture && x11_key_down(display, keymap, XK_Down);
	const bool keyEnter = canCapture && (x11_key_down(display, keymap, XK_Return) || x11_key_down(display, keymap, XK_KP_Enter));
	const bool keyEscape = canCapture && x11_key_down(display, keymap, XK_Escape);
	const bool keyBackspace = canCapture && x11_key_down(display, keymap, XK_BackSpace);
	const bool keySpace = canCapture && x11_key_down(display, keymap, XK_space);

	io.AddKeyEvent(ImGuiKey_Tab, keyTab);
	io.AddKeyEvent(ImGuiKey_LeftArrow, keyLeft);
	io.AddKeyEvent(ImGuiKey_RightArrow, keyRight);
	io.AddKeyEvent(ImGuiKey_UpArrow, keyUp);
	io.AddKeyEvent(ImGuiKey_DownArrow, keyDown);
	io.AddKeyEvent(ImGuiKey_Enter, keyEnter);
	io.AddKeyEvent(ImGuiKey_Escape, keyEscape);
	io.AddKeyEvent(ImGuiKey_Backspace, keyBackspace);
	io.AddKeyEvent(ImGuiKey_Space, keySpace);
}
#endif

static PFN_vkSetDeviceLoaderData find_set_loader_data(VkDevice device) {
	std::lock_guard<std::mutex> lock(g_lock);
	auto it = g_deviceData.find(dispatch_key(device));
	if (it == g_deviceData.end())
		return nullptr;
	return it->second.setDeviceLoaderData;
}

VKAPI_ATTR VkResult VKAPI_CALL imgui_vkAllocateCommandBuffers(
		VkDevice device,
		const VkCommandBufferAllocateInfo* pAllocateInfo,
		VkCommandBuffer* pCommandBuffers) {
	if (!g_imguiAllocateCommandBuffers)
		return VK_ERROR_INITIALIZATION_FAILED;

	VkResult result = g_imguiAllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);
	if (result != VK_SUCCESS || !pAllocateInfo || !pCommandBuffers)
		return result;

	if (PFN_vkSetDeviceLoaderData setLoaderData = find_set_loader_data(device)) {
		for (uint32_t i = 0; i < pAllocateInfo->commandBufferCount; i++)
			setLoaderData(device, pCommandBuffers[i]);
	}

	return result;
}

VKAPI_ATTR void VKAPI_CALL imgui_vkGetDeviceQueue(
		VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue* pQueue) {
	if (!g_imguiGetDeviceQueue || !pQueue)
		return;

	g_imguiGetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);

	if (*pQueue != VK_NULL_HANDLE) {
		if (PFN_vkSetDeviceLoaderData setLoaderData = find_set_loader_data(device))
			setLoaderData(device, *pQueue);
	}
}

// ── ImGui Vulkan function loader ──────────────────────────────────────────────
// ImGui_ImplVulkan_LoadFunctions requires a callback that resolves Vulkan
// function names. We use the device's GetDeviceProcAddr from our dispatch table.
// This bypasses the loader dispatch chain (intentional for a layer).

static PFN_vkVoidFunction imgui_loader(const char* name, void* userData) {
	auto* swData = static_cast<SwapchainData*>(userData);

	// Functions below are instance-level in ImGui's Vulkan function map.
	constexpr std::array<const char*, 8> instanceLevelFuncs = {
		"vkDestroySurfaceKHR",
		"vkEnumeratePhysicalDevices",
		"vkGetPhysicalDeviceProperties",
		"vkGetPhysicalDeviceMemoryProperties",
		"vkGetPhysicalDeviceQueueFamilyProperties",
		"vkGetPhysicalDeviceSurfaceCapabilitiesKHR",
		"vkGetPhysicalDeviceSurfaceFormatsKHR",
		"vkGetPhysicalDeviceSurfacePresentModesKHR",
	};

	for (const char* fnName : instanceLevelFuncs) {
		if (std::strcmp(name, fnName) == 0) {
			if (swData->instance != VK_NULL_HANDLE && swData->instanceDispatch.GetInstanceProcAddr)
				return swData->instanceDispatch.GetInstanceProcAddr(swData->instance, name);
			return nullptr;
		}
	}

	if (std::strcmp(name, "vkAllocateCommandBuffers") == 0) {
		g_imguiAllocateCommandBuffers = swData->dispatch.AllocateCommandBuffers;
		return reinterpret_cast<PFN_vkVoidFunction>(&imgui_vkAllocateCommandBuffers);
	}

	if (std::strcmp(name, "vkGetDeviceQueue") == 0) {
		g_imguiGetDeviceQueue = swData->dispatch.GetDeviceQueue;
		return reinterpret_cast<PFN_vkVoidFunction>(&imgui_vkGetDeviceQueue);
	}

	// All remaining symbols in ImGui's map are device-level.
	if (swData->dispatch.GetDeviceProcAddr)
		return swData->dispatch.GetDeviceProcAddr(swData->device, name);

	return nullptr;
}

// ── Render pass (loadOp=LOAD, keeps game frame) ───────────────────────────────
static VkRenderPass create_overlay_render_pass(const DeviceDispatch& dispatch, VkDevice device, VkFormat format) {
	VkAttachmentDescription colorAttach { };
	colorAttach.format = format;
	colorAttach.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttach.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // keep game frame
	colorAttach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttach.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	colorAttach.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorRef { };
	colorRef.attachment = 0;
	colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass { };
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorRef;

	VkSubpassDependency dep { };
	dep.srcSubpass = VK_SUBPASS_EXTERNAL;
	dep.dstSubpass = 0;
	dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo ci { };
	ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	ci.attachmentCount = 1;
	ci.pAttachments = &colorAttach;
	ci.subpassCount = 1;
	ci.pSubpasses = &subpass;
	ci.dependencyCount = 1;
	ci.pDependencies = &dep;

	VkRenderPass rp = VK_NULL_HANDLE;
	dispatch.CreateRenderPass(device, &ci, nullptr, &rp);
	return rp;
}

// ── init ──────────────────────────────────────────────────────────────────────
void init(VkSwapchainKHR swapchain, SwapchainData& swData) {
	VkDevice device = swData.device;
	swData.imguiBackendInitialized = false;

	// 1. Get swapchain images
	uint32_t count = 0;
	swData.dispatch.GetSwapchainImagesKHR(device, swapchain, &count, nullptr);
	swData.images.resize(count);
	swData.dispatch.GetSwapchainImagesKHR(device, swapchain, &count, swData.images.data());
	swData.imageCount = count;

	// 2. Render pass (overlay on top, loadOp=LOAD)
	swData.renderPass = create_overlay_render_pass(swData.dispatch, swData.device, swData.format);
	if (swData.renderPass == VK_NULL_HANDLE) {
		std::printf("[reshadeVK] Erro: falha ao criar render pass\n");
		return;
	}

	// 3. Image views + framebuffers
	swData.imageViews.resize(count);
	swData.framebuffers.resize(count);
	for (uint32_t i = 0; i < count; i++) {
		VkImageViewCreateInfo viewCI { };
		viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCI.image = swData.images[i];
		viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCI.format = swData.format;
		viewCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewCI.subresourceRange.baseMipLevel = 0;
		viewCI.subresourceRange.levelCount = 1;
		viewCI.subresourceRange.baseArrayLayer = 0;
		viewCI.subresourceRange.layerCount = 1;
		swData.dispatch.CreateImageView(device, &viewCI, nullptr, &swData.imageViews[i]);

		VkFramebufferCreateInfo fbCI { };
		fbCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbCI.renderPass = swData.renderPass;
		fbCI.attachmentCount = 1;
		fbCI.pAttachments = &swData.imageViews[i];
		fbCI.width = swData.extent.width;
		fbCI.height = swData.extent.height;
		fbCI.layers = 1;
		swData.dispatch.CreateFramebuffer(device, &fbCI, nullptr, &swData.framebuffers[i]);
	}

	// 4. Command pool + command buffers (one per swapchain image)
	VkCommandPoolCreateInfo poolCI { };
	poolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolCI.queueFamilyIndex = swData.graphicsQueueFamily;
	if (swData.dispatch.CreateCommandPool(device, &poolCI, nullptr, &swData.commandPool) != VK_SUCCESS || swData.commandPool == VK_NULL_HANDLE) {
		std::printf("[reshadeVK] Erro: falha ao criar command pool para fila %u\n", swData.graphicsQueueFamily);
		return;
	}

	swData.commandBuffers.resize(count);
	VkCommandBufferAllocateInfo allocInfo { };
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = swData.commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = count;
	if (swData.dispatch.AllocateCommandBuffers(device, &allocInfo, swData.commandBuffers.data()) != VK_SUCCESS) {
		std::printf("[reshadeVK] Erro: falha ao alocar command buffers\n");
		return;
	}

	if (swData.setDeviceLoaderData) {
		for (VkCommandBuffer cmd : swData.commandBuffers)
			swData.setDeviceLoaderData(device, cmd);
	}

	// 5. Per-image semaphores (signal after overlay render, waited by present)
	swData.semaphores.resize(count, VK_NULL_HANDLE);
	VkSemaphoreCreateInfo semCI { };
	semCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	for (uint32_t i = 0; i < count; i++)
		swData.dispatch.CreateSemaphore(device, &semCI, nullptr, &swData.semaphores[i]);

	// 5b. Per-image fences for command buffer reuse synchronization
	swData.fences.resize(count, VK_NULL_HANDLE);
	VkFenceCreateInfo fenceCI { };
	fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	for (uint32_t i = 0; i < count; i++)
		swData.dispatch.CreateFence(device, &fenceCI, nullptr, &swData.fences[i]);

	// 6. Descriptor pool for ImGui
	VkDescriptorPoolSize poolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 16 },
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 4 },
	};
	VkDescriptorPoolCreateInfo descPoolCI { };
	descPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descPoolCI.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	descPoolCI.maxSets = 32;
	descPoolCI.poolSizeCount = 3;
	descPoolCI.pPoolSizes = poolSizes;
	swData.dispatch.CreateDescriptorPool(device, &descPoolCI, nullptr, &swData.descriptorPool);

	// 7. ImGui context for this swapchain
	swData.imguiCtx = ImGui::CreateContext();
	ImGui::SetCurrentContext(swData.imguiCtx);
	ImGui::StyleColorsDark();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	if (get_host_input_state()) {
		swData.interactionAvailable = true;
		if (!g_loggedHostBridge) {
			std::printf("[reshadeVK] input bridge do host detectado\n");
			g_loggedHostBridge = true;
		}
	}

#if RESHADEVK_HAS_GLFW
	if (swData.glfwWindow) {
		swData.interactionAvailable = true;
		if (!g_loggedGlfwInput) {
			std::printf("[reshadeVK] input GLFW detectado\n");
			g_loggedGlfwInput = true;
		}
	}
#endif

#if RESHADEVK_HAS_X11
	if (!swData.interactionAvailable)
		swData.interactionAvailable = true;
#else
	std::printf("[reshadeVK] build sem suporte X11; input interativo indisponivel\n");
#endif

	// Load Vulkan functions through our dispatch table (bypasses loader, correct for a layer)
	if (!ImGui_ImplVulkan_LoadFunctions(VK_API_VERSION_1_0, &imgui_loader, &swData)) {
		std::printf("[reshadeVK] Erro: falha ao carregar funções Vulkan para ImGui\n");
		return;
	}

	ImGui_ImplVulkan_InitInfo initInfo { };
	initInfo.ApiVersion = VK_API_VERSION_1_0;
	initInfo.Instance = swData.instance;
	initInfo.PhysicalDevice = swData.physicalDevice;
	initInfo.Device = swData.device;
	initInfo.QueueFamily = swData.graphicsQueueFamily;
	initInfo.Queue = swData.graphicsQueue;
	initInfo.DescriptorPool = swData.descriptorPool;
	initInfo.MinImageCount = 2;
	initInfo.ImageCount = count;
	initInfo.PipelineInfoMain.RenderPass = swData.renderPass;

	ImGui_ImplVulkan_Init(&initInfo);
	swData.imguiBackendInitialized = true;

	// Font texture is uploaded lazily on first render in this version of ImGui

	if (!g_loggedOverlayInit) {
		std::printf("[reshadeVK] overlay inicializado (%ux%u, %u imagens)\n",
				swData.extent.width, swData.extent.height, count);
		g_loggedOverlayInit = true;
	}
}

// ── render ────────────────────────────────────────────────────────────────────
void render(SwapchainData& swData, VkQueue queue, uint32_t imageIndex,
		const std::vector<VkSemaphore>& waitSems, VkSemaphore signalSem) {
	const DeviceDispatch& dispatch = swData.dispatch;
	VkCommandBuffer cmd = swData.commandBuffers[imageIndex];
	VkFence frameFence = swData.fences[imageIndex];

	dispatch.WaitForFences(swData.device, 1, &frameFence, VK_TRUE, UINT64_MAX);
	dispatch.ResetFences(swData.device, 1, &frameFence);

	// ── ImGui frame (CPU) ──────────────────────────────────────────────────
	ImGui::SetCurrentContext(swData.imguiCtx);
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(static_cast<float>(swData.extent.width),
			static_cast<float>(swData.extent.height));
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
	io.DeltaTime = 1.0f / 60.0f;

	if (const ReshadeVKHostInputState* host = get_host_input_state()) {
		apply_host_input(swData, *host);
#if RESHADEVK_HAS_GLFW
	} else if (swData.glfwWindow) {
		update_input_glfw(swData);
#endif
#if RESHADEVK_HAS_X11
	} else if (swData.interactionAvailable) {
		update_input_x11(swData);
	}
#endif

	ImGui_ImplVulkan_NewFrame();
	ImGui::NewFrame();
	ImGui::ShowDemoWindow();
	ImGui::Render();

	// ── Record command buffer ──────────────────────────────────────────────
	dispatch.ResetCommandBuffer(cmd, 0);

	VkCommandBufferBeginInfo beginInfo { };
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	dispatch.BeginCommandBuffer(cmd, &beginInfo);

	VkRenderPassBeginInfo rpInfo { };
	rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpInfo.renderPass = swData.renderPass;
	rpInfo.framebuffer = swData.framebuffers[imageIndex];
	rpInfo.renderArea.offset = { 0, 0 };
	rpInfo.renderArea.extent = swData.extent;
	rpInfo.clearValueCount = 0; // loadOp=LOAD, no clear needed
	dispatch.CmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	dispatch.CmdEndRenderPass(cmd);
	dispatch.EndCommandBuffer(cmd);

	// ── Submit (wait on app semaphores, signal ours) ───────────────────────
	std::vector<VkPipelineStageFlags> waitStages(
			waitSems.size(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	VkSubmitInfo submitInfo { };
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSems.size());
	submitInfo.pWaitSemaphores = waitSems.data();
	submitInfo.pWaitDstStageMask = waitStages.data();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &signalSem;
	dispatch.QueueSubmit(queue, 1, &submitInfo, frameFence);
}

// ── destroy ───────────────────────────────────────────────────────────────────
void destroy(SwapchainData& swData) {
	const DeviceDispatch& dispatch = swData.dispatch;
	VkDevice device = swData.device;

	if (device == VK_NULL_HANDLE || !dispatch.DeviceWaitIdle)
		return;

	dispatch.DeviceWaitIdle(device);

	if (swData.imguiCtx) {
		ImGui::SetCurrentContext(swData.imguiCtx);
		if (swData.imguiBackendInitialized)
			ImGui_ImplVulkan_Shutdown();
		ImGui::DestroyContext(swData.imguiCtx);
		swData.imguiCtx = nullptr;
		swData.imguiBackendInitialized = false;
	}

#if RESHADEVK_HAS_X11
	if (swData.ownsDisplayConnection && swData.nativeDisplay) {
		XCloseDisplay(static_cast<Display*>(swData.nativeDisplay));
		swData.nativeDisplay = nullptr;
		swData.ownsDisplayConnection = false;
	}
#endif

	for (VkSemaphore sem : swData.semaphores)
		if (sem && dispatch.DestroySemaphore)
			dispatch.DestroySemaphore(device, sem, nullptr);
	swData.semaphores.clear();

	for (VkFence fence : swData.fences)
		if (fence && dispatch.DestroyFence)
			dispatch.DestroyFence(device, fence, nullptr);
	swData.fences.clear();

	if (swData.commandPool && dispatch.DestroyCommandPool) {
		dispatch.DestroyCommandPool(device, swData.commandPool, nullptr);
		swData.commandPool = VK_NULL_HANDLE;
	}

	for (VkFramebuffer fb : swData.framebuffers)
		if (fb && dispatch.DestroyFramebuffer)
			dispatch.DestroyFramebuffer(device, fb, nullptr);
	swData.framebuffers.clear();

	for (VkImageView iv : swData.imageViews)
		if (iv && dispatch.DestroyImageView)
			dispatch.DestroyImageView(device, iv, nullptr);
	swData.imageViews.clear();

	if (swData.renderPass && dispatch.DestroyRenderPass) {
		dispatch.DestroyRenderPass(device, swData.renderPass, nullptr);
		swData.renderPass = VK_NULL_HANDLE;
	}

	if (swData.descriptorPool && dispatch.DestroyDescriptorPool) {
		dispatch.DestroyDescriptorPool(device, swData.descriptorPool, nullptr);
		swData.descriptorPool = VK_NULL_HANDLE;
	}
}

} // namespace reshadevk::overlay

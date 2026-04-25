#include "renderer.hpp"
#include "imgui_loader.hpp"

#include "utils/layer.hpp"
#include "utils/input/host_bridge.hpp"

#if RESHADEVK_HAS_GLFW
#include "utils/input/glfw.hpp"
#endif

#if RESHADEVK_HAS_X11
#include "utils/input/x11.hpp"
#include <X11/Xlib.h>
#endif

#include <array>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <limits>
#include <vector>

#include <imgui.h>
#include <imgui_impl_vulkan.h>

namespace reshadevk::renderer {

// ── Host-bridge helpers (inline, renderer-private) ────────────────────────────

static const ReshadeVKHostInputState* (*s_hostInputGetter)() = nullptr;
static bool s_hostInputLookupDone = false;
static bool s_loggedHostBridge    = false;
static bool s_loggedGlfwInput     = false;
static bool s_loggedOverlayInit   = false;

static const ReshadeVKHostInputState* get_host_input_state() {
	if (!s_hostInputLookupDone) {
		s_hostInputLookupDone = true;
		s_hostInputGetter     = reinterpret_cast<const ReshadeVKHostInputState* (*)()>(
				dlsym(RTLD_DEFAULT, "reshadevk_host_get_input_state"));
	}
	return s_hostInputGetter ? s_hostInputGetter() : nullptr;
}

static void apply_host_input(SwapchainData& swData, const ReshadeVKHostInputState& host) {
	ImGuiIO& io         = ImGui::GetIO();
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

	if (canCapture)
		io.AddMousePosEvent(static_cast<float>(host.mouseX), static_cast<float>(host.mouseY));
	else
		io.AddMousePosEvent(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

	for (int i = 0; i < 5; i++) {
		const bool down = canCapture && host.mouseButtons[i];
		if (down != swData.prevMouseDown[i]) {
			io.AddMouseButtonEvent(i, down);
			swData.prevMouseDown[i] = down;
		}
	}

	io.AddKeyEvent(ImGuiKey_Tab,        canCapture && host.keyTab);
	io.AddKeyEvent(ImGuiKey_LeftArrow,  canCapture && host.keyLeft);
	io.AddKeyEvent(ImGuiKey_RightArrow, canCapture && host.keyRight);
	io.AddKeyEvent(ImGuiKey_UpArrow,    canCapture && host.keyUp);
	io.AddKeyEvent(ImGuiKey_DownArrow,  canCapture && host.keyDown);
	io.AddKeyEvent(ImGuiKey_Enter,      canCapture && host.keyEnter);
	io.AddKeyEvent(ImGuiKey_Escape,     canCapture && host.keyEscape);
	io.AddKeyEvent(ImGuiKey_Backspace,  canCapture && host.keyBackspace);
	io.AddKeyEvent(ImGuiKey_Space,      canCapture && host.keySpace);
}

// ── Render pass (loadOp=LOAD keeps the game frame underneath) ────────────────
static VkRenderPass create_overlay_render_pass(
		const DeviceDispatch& dispatch, VkDevice device, VkFormat format) {
	VkAttachmentDescription colorAttach{};
	colorAttach.format         = format;
	colorAttach.samples        = VK_SAMPLE_COUNT_1_BIT;
	colorAttach.loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD;
	colorAttach.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttach.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttach.initialLayout  = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	colorAttach.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorRef{};
	colorRef.attachment = 0;
	colorRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments    = &colorRef;

	VkSubpassDependency dep{};
	dep.srcSubpass    = VK_SUBPASS_EXTERNAL;
	dep.dstSubpass    = 0;
	dep.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dep.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo ci{};
	ci.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	ci.attachmentCount = 1;
	ci.pAttachments    = &colorAttach;
	ci.subpassCount    = 1;
	ci.pSubpasses      = &subpass;
	ci.dependencyCount = 1;
	ci.pDependencies   = &dep;

	VkRenderPass rp = VK_NULL_HANDLE;
	dispatch.CreateRenderPass(device, &ci, nullptr, &rp);
	return rp;
}

// ── init ──────────────────────────────────────────────────────────────────────
void init(VkSwapchainKHR swapchain, SwapchainData& swData) {
	VkDevice device            = swData.device;
	swData.imguiBackendInitialized = false;

	// 1. Swapchain images
	uint32_t count = 0;
	swData.dispatch.GetSwapchainImagesKHR(device, swapchain, &count, nullptr);
	swData.images.resize(count);
	swData.dispatch.GetSwapchainImagesKHR(device, swapchain, &count, swData.images.data());
	swData.imageCount = count;

	// 2. Render pass
	swData.renderPass = create_overlay_render_pass(swData.dispatch, device, swData.format);
	if (swData.renderPass == VK_NULL_HANDLE) {
		std::printf("[reshadeVK] Erro: falha ao criar render pass\n");
		return;
	}

	// 3. Image views + framebuffers
	swData.imageViews.resize(count);
	swData.framebuffers.resize(count);
	for (uint32_t i = 0; i < count; i++) {
		VkImageViewCreateInfo viewCI{};
		viewCI.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCI.image                           = swData.images[i];
		viewCI.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
		viewCI.format                          = swData.format;
		viewCI.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCI.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCI.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCI.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCI.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		viewCI.subresourceRange.baseMipLevel   = 0;
		viewCI.subresourceRange.levelCount     = 1;
		viewCI.subresourceRange.baseArrayLayer = 0;
		viewCI.subresourceRange.layerCount     = 1;
		swData.dispatch.CreateImageView(device, &viewCI, nullptr, &swData.imageViews[i]);

		VkFramebufferCreateInfo fbCI{};
		fbCI.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbCI.renderPass      = swData.renderPass;
		fbCI.attachmentCount = 1;
		fbCI.pAttachments    = &swData.imageViews[i];
		fbCI.width           = swData.extent.width;
		fbCI.height          = swData.extent.height;
		fbCI.layers          = 1;
		swData.dispatch.CreateFramebuffer(device, &fbCI, nullptr, &swData.framebuffers[i]);
	}

	// 4. Command pool + command buffers (one per image)
	VkCommandPoolCreateInfo poolCI{};
	poolCI.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolCI.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolCI.queueFamilyIndex = swData.graphicsQueueFamily;
	if (swData.dispatch.CreateCommandPool(device, &poolCI, nullptr, &swData.commandPool) != VK_SUCCESS
			|| swData.commandPool == VK_NULL_HANDLE) {
		std::printf("[reshadeVK] Erro: falha ao criar command pool para fila %u\n", swData.graphicsQueueFamily);
		return;
	}

	swData.commandBuffers.resize(count);
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool        = swData.commandPool;
	allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = count;
	if (swData.dispatch.AllocateCommandBuffers(device, &allocInfo, swData.commandBuffers.data()) != VK_SUCCESS) {
		std::printf("[reshadeVK] Erro: falha ao alocar command buffers\n");
		return;
	}

	if (swData.setDeviceLoaderData) {
		for (VkCommandBuffer cmd : swData.commandBuffers)
			swData.setDeviceLoaderData(device, cmd);
	}

	// 5. Per-image semaphores + fences
	swData.semaphores.resize(count, VK_NULL_HANDLE);
	VkSemaphoreCreateInfo semCI{};
	semCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	for (uint32_t i = 0; i < count; i++)
		swData.dispatch.CreateSemaphore(device, &semCI, nullptr, &swData.semaphores[i]);

	swData.fences.resize(count, VK_NULL_HANDLE);
	VkFenceCreateInfo fenceCI{};
	fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	for (uint32_t i = 0; i < count; i++)
		swData.dispatch.CreateFence(device, &fenceCI, nullptr, &swData.fences[i]);

	// 6. Descriptor pool for ImGui
	VkDescriptorPoolSize poolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          16 },
		{ VK_DESCRIPTOR_TYPE_SAMPLER,                 4 },
	};
	VkDescriptorPoolCreateInfo descPoolCI{};
	descPoolCI.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descPoolCI.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	descPoolCI.maxSets       = 32;
	descPoolCI.poolSizeCount = 3;
	descPoolCI.pPoolSizes    = poolSizes;
	swData.dispatch.CreateDescriptorPool(device, &descPoolCI, nullptr, &swData.descriptorPool);

	// 7. ImGui context for this swapchain
	swData.imguiCtx = ImGui::CreateContext();
	ImGui::SetCurrentContext(swData.imguiCtx);
	ImGui::StyleColorsDark();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	if (get_host_input_state()) {
		swData.interactionAvailable = true;
		if (!s_loggedHostBridge) {
			std::printf("[reshadeVK] input bridge do host detectado\n");
			s_loggedHostBridge = true;
		}
	}

#if RESHADEVK_HAS_GLFW
	if (swData.glfwWindow) {
		swData.interactionAvailable = true;
		if (!s_loggedGlfwInput) {
			std::printf("[reshadeVK] input GLFW detectado\n");
			s_loggedGlfwInput = true;
		}
	}
#endif

#if RESHADEVK_HAS_X11
	if (!swData.interactionAvailable)
		swData.interactionAvailable = true;
#else
	if (!swData.interactionAvailable)
		std::printf("[reshadeVK] build sem suporte X11; input interativo indisponivel\n");
#endif

	// 8. Load Vulkan functions for ImGui (through our dispatch table)
	if (!ImGui_ImplVulkan_LoadFunctions(VK_API_VERSION_1_0, &imgui_loader, &swData)) {
		std::printf("[reshadeVK] Erro: falha ao carregar funcoes Vulkan para ImGui\n");
		return;
	}

	ImGui_ImplVulkan_InitInfo initInfo{};
	initInfo.ApiVersion                  = VK_API_VERSION_1_0;
	initInfo.Instance                    = swData.instance;
	initInfo.PhysicalDevice              = swData.physicalDevice;
	initInfo.Device                      = swData.device;
	initInfo.QueueFamily                 = swData.graphicsQueueFamily;
	initInfo.Queue                       = swData.graphicsQueue;
	initInfo.DescriptorPool              = swData.descriptorPool;
	initInfo.MinImageCount               = 2;
	initInfo.ImageCount                  = count;
	initInfo.PipelineInfoMain.RenderPass = swData.renderPass;
	ImGui_ImplVulkan_Init(&initInfo);
	swData.imguiBackendInitialized = true;

	if (!s_loggedOverlayInit) {
		std::printf("[reshadeVK] overlay inicializado (%ux%u, %u imagens)\n",
				swData.extent.width, swData.extent.height, count);
		s_loggedOverlayInit = true;
	}
}

// ── render_frame ──────────────────────────────────────────────────────────────
void render_frame(SwapchainData& swData, VkQueue queue, uint32_t imageIndex,
		const std::vector<VkSemaphore>& waitSems, VkSemaphore signalSem) {
	const DeviceDispatch& dispatch = swData.dispatch;
	VkCommandBuffer cmd            = swData.commandBuffers[imageIndex];
	VkFence         frameFence     = swData.fences[imageIndex];

	dispatch.WaitForFences(swData.device, 1, &frameFence, VK_TRUE, UINT64_MAX);
	dispatch.ResetFences(swData.device, 1, &frameFence);

	// ── ImGui CPU frame ────────────────────────────────────────────────────
	ImGui::SetCurrentContext(swData.imguiCtx);
	ImGuiIO& io            = ImGui::GetIO();
	io.DisplaySize         = ImVec2(static_cast<float>(swData.extent.width),
	                                static_cast<float>(swData.extent.height));
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
	io.DeltaTime           = 1.0f / 60.0f;

	// ── Input dispatch (host bridge → GLFW → X11) ──────────────────────────
	if (const ReshadeVKHostInputState* host = get_host_input_state()) {
		apply_host_input(swData, *host);
#if RESHADEVK_HAS_GLFW
	} else if (swData.glfwWindow) {
		input::update_glfw(swData);
#endif
#if RESHADEVK_HAS_X11
	} else if (swData.interactionAvailable) {
		input::update_x11(swData);
#endif
	}

	ImGui_ImplVulkan_NewFrame();
	ImGui::NewFrame();

	// ── Layer content (delegated to the registered LayerComponent) ─────────
	if (LayerComponent* component = get_layer_component()) {
		RenderContext ctx{};
		ctx.cmd        = cmd;
		ctx.extent     = swData.extent;
		ctx.imageIndex = imageIndex;
		component->onRender(ctx);
	}

	ImGui::Render();

	// ── Record command buffer ──────────────────────────────────────────────
	dispatch.ResetCommandBuffer(cmd, 0);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	dispatch.BeginCommandBuffer(cmd, &beginInfo);

	VkRenderPassBeginInfo rpInfo{};
	rpInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpInfo.renderPass        = swData.renderPass;
	rpInfo.framebuffer       = swData.framebuffers[imageIndex];
	rpInfo.renderArea.offset = { 0, 0 };
	rpInfo.renderArea.extent = swData.extent;
	rpInfo.clearValueCount   = 0;
	dispatch.CmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
	dispatch.CmdEndRenderPass(cmd);
	dispatch.EndCommandBuffer(cmd);

	// ── Submit (wait on app semaphores, signal ours) ───────────────────────
	std::vector<VkPipelineStageFlags> waitStages(
			waitSems.size(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	VkSubmitInfo submitInfo{};
	submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount   = static_cast<uint32_t>(waitSems.size());
	submitInfo.pWaitSemaphores      = waitSems.data();
	submitInfo.pWaitDstStageMask    = waitStages.data();
	submitInfo.commandBufferCount   = 1;
	submitInfo.pCommandBuffers      = &cmd;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores    = &signalSem;
	dispatch.QueueSubmit(queue, 1, &submitInfo, frameFence);
}

// ── destroy ───────────────────────────────────────────────────────────────────
void destroy(SwapchainData& swData) {
	const DeviceDispatch& dispatch = swData.dispatch;
	VkDevice device                = swData.device;

	if (device == VK_NULL_HANDLE || !dispatch.DeviceWaitIdle)
		return;

	dispatch.DeviceWaitIdle(device);

	if (swData.imguiCtx) {
		ImGui::SetCurrentContext(swData.imguiCtx);
		if (swData.imguiBackendInitialized)
			ImGui_ImplVulkan_Shutdown();
		ImGui::DestroyContext(swData.imguiCtx);
		swData.imguiCtx                = nullptr;
		swData.imguiBackendInitialized = false;
	}

#if RESHADEVK_HAS_X11
	if (swData.ownsDisplayConnection && swData.nativeDisplay) {
		XCloseDisplay(static_cast<Display*>(swData.nativeDisplay));
		swData.nativeDisplay         = nullptr;
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

} // namespace reshadevk::renderer

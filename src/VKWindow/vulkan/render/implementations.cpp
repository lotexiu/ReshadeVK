#include "declarations.hpp"

#include <cstdio>

#include "../swapchain/declarations.hpp"

namespace {

bool create_render_pass(vk_window::VulkanContext& context) {
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format         = context.swapchainFormat;
	colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments    = &colorAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass    = 0;
	dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo createInfo{};
	createInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = 1;
	createInfo.pAttachments    = &colorAttachment;
	createInfo.subpassCount    = 1;
	createInfo.pSubpasses      = &subpass;
	createInfo.dependencyCount = 1;
	createInfo.pDependencies   = &dependency;

	const VkResult result = vkCreateRenderPass(context.device, &createInfo, nullptr, &context.renderPass);
	if (result != VK_SUCCESS) {
		std::printf("Erro: nao foi possivel criar o render pass (%d)\n", result);
		context.renderPass = VK_NULL_HANDLE;
		return false;
	}
	return true;
}

bool create_framebuffers(vk_window::VulkanContext& context) {
	context.framebuffers.resize(context.swapchainImageViews.size());
	for (size_t i = 0; i < context.swapchainImageViews.size(); i++) {
		VkImageView attachments[] = { context.swapchainImageViews[i] };
		VkFramebufferCreateInfo createInfo{};
		createInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass      = context.renderPass;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments    = attachments;
		createInfo.width           = context.swapchainExtent.width;
		createInfo.height          = context.swapchainExtent.height;
		createInfo.layers          = 1;
		const VkResult result = vkCreateFramebuffer(context.device, &createInfo, nullptr, &context.framebuffers[i]);
		if (result != VK_SUCCESS) {
			std::printf("Erro: nao foi possivel criar framebuffer %zu (%d)\n", i, result);
			return false;
		}
	}
	return true;
}

bool create_command_resources(vk_window::VulkanContext& context) {
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = context.graphicsFamily;
	const VkResult poolResult = vkCreateCommandPool(context.device, &poolInfo, nullptr, &context.commandPool);
	if (poolResult != VK_SUCCESS) {
		std::printf("Erro: nao foi possivel criar command pool (%d)\n", poolResult);
		context.commandPool = VK_NULL_HANDLE;
		return false;
	}

	VkCommandBufferAllocateInfo allocateInfo{};
	allocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandPool        = context.commandPool;
	allocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = vk_window::VulkanContext::MAX_FRAMES_IN_FLIGHT;
	const VkResult allocResult = vkAllocateCommandBuffers(context.device, &allocateInfo, context.commandBuffers.data());
	if (allocResult != VK_SUCCESS) {
		std::printf("Erro: nao foi possivel alocar command buffers (%d)\n", allocResult);
		return false;
	}
	return true;
}

bool create_sync_objects(vk_window::VulkanContext& context) {
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	for (uint32_t i = 0; i < vk_window::VulkanContext::MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &context.imageAvailableSemaphores[i]) != VK_SUCCESS
				|| vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &context.renderFinishedSemaphores[i]) != VK_SUCCESS
				|| vkCreateFence(context.device, &fenceInfo, nullptr, &context.inFlightFences[i]) != VK_SUCCESS) {
			std::printf("Erro: nao foi possivel criar objetos de sincronizacao\n");
			return false;
		}
	}
	return true;
}

bool record_command_buffer(
		vk_window::VulkanContext& context,
		VkCommandBuffer commandBuffer,
		uint32_t imageIndex,
		const vk_window::AppConfig& config) {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		std::printf("Erro: nao foi possivel iniciar command buffer\n");
		return false;
	}

	VkClearValue clearValue{};
	clearValue.color = {{ config.clearColor[0], config.clearColor[1],
	                      config.clearColor[2], config.clearColor[3] }};

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass        = context.renderPass;
	renderPassInfo.framebuffer       = context.framebuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = context.swapchainExtent;
	renderPassInfo.clearValueCount   = 1;
	renderPassInfo.pClearValues      = &clearValue;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		std::printf("Erro: nao foi possivel finalizar command buffer\n");
		return false;
	}
	return true;
}

bool recreate_swapchain_and_render_resources(vk_window::VulkanContext& context) {
	if (!context.window) {
		std::printf("Erro: janela GLFW invalida para recriar swapchain\n");
		return false;
	}
	int width = 0, height = 0;
	glfwGetFramebufferSize(context.window, &width, &height);
	while (width == 0 || height == 0) {
		glfwWaitEvents();
		glfwGetFramebufferSize(context.window, &width, &height);
	}
	vkDeviceWaitIdle(context.device);
	vk_window::render::destroy_render_resources(context);
	vk_window::swapchain::destroy_swapchain(context);
	if (!vk_window::swapchain::create_swapchain(context, context.window))
		return false;
	if (!vk_window::render::create_render_resources(context))
		return false;
	context.frameIndex         = 0;
	context.framebufferResized = false;
	return true;
}

} // namespace

namespace vk_window::render {

bool create_render_resources(VulkanContext& context) {
	if (!create_render_pass(context))     return false;
	if (!create_framebuffers(context))    return false;
	if (!create_command_resources(context)) return false;
	if (!create_sync_objects(context))    return false;
	std::printf("Vulkan: pipeline minima de render criada\n");
	return true;
}

bool draw_frame(VulkanContext& context, const AppConfig& config) {
	if (context.framebufferResized) {
		const double now = glfwGetTime();
		constexpr double resizeDebounceSeconds = 0.08;
		if ((now - context.lastFramebufferResizeTime) >= resizeDebounceSeconds)
			return recreate_swapchain_and_render_resources(context);
	}

	const uint32_t frame = context.frameIndex;
	vkWaitForFences(context.device, 1, &context.inFlightFences[frame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex  = 0;
	VkResult acquireResult = vkAcquireNextImageKHR(
			context.device, context.swapchain, UINT64_MAX,
			context.imageAvailableSemaphores[frame], VK_NULL_HANDLE, &imageIndex);

	if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
		return recreate_swapchain_and_render_resources(context);
	if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
		std::printf("Erro: falha ao adquirir imagem da swapchain (%d)\n", acquireResult);
		return false;
	}

	vkResetFences(context.device, 1, &context.inFlightFences[frame]);
	vkResetCommandBuffer(context.commandBuffers[frame], 0);

	if (!record_command_buffer(context, context.commandBuffers[frame], imageIndex, config))
		return false;

	VkSemaphore          waitSemaphores[]  = { context.imageAvailableSemaphores[frame] };
	VkPipelineStageFlags waitStages[]      = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore          signalSemaphores[]= { context.renderFinishedSemaphores[frame] };

	VkSubmitInfo submitInfo{};
	submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount   = 1;
	submitInfo.pWaitSemaphores      = waitSemaphores;
	submitInfo.pWaitDstStageMask    = waitStages;
	submitInfo.commandBufferCount   = 1;
	submitInfo.pCommandBuffers      = &context.commandBuffers[frame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores    = signalSemaphores;

	if (vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, context.inFlightFences[frame]) != VK_SUCCESS) {
		std::printf("Erro: falha no submit da fila grafica\n");
		return false;
	}

	VkSwapchainKHR swapchains[] = { context.swapchain };
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores    = signalSemaphores;
	presentInfo.swapchainCount     = 1;
	presentInfo.pSwapchains        = swapchains;
	presentInfo.pImageIndices      = &imageIndex;

	const VkResult presentResult = vkQueuePresentKHR(context.presentQueue, &presentInfo);
	if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || context.framebufferResized)
		return recreate_swapchain_and_render_resources(context);
	if (presentResult != VK_SUCCESS) {
		std::printf("Erro: falha ao apresentar frame (%d)\n", presentResult);
		return false;
	}

	context.frameIndex = (context.frameIndex + 1) % VulkanContext::MAX_FRAMES_IN_FLIGHT;
	return true;
}

void destroy_render_resources(VulkanContext& context) {
	for (VkFence fence : context.inFlightFences)
		if (fence != VK_NULL_HANDLE)
			vkDestroyFence(context.device, fence, nullptr);
	for (VkSemaphore semaphore : context.renderFinishedSemaphores)
		if (semaphore != VK_NULL_HANDLE)
			vkDestroySemaphore(context.device, semaphore, nullptr);
	for (VkSemaphore semaphore : context.imageAvailableSemaphores)
		if (semaphore != VK_NULL_HANDLE)
			vkDestroySemaphore(context.device, semaphore, nullptr);

	if (context.commandPool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(context.device, context.commandPool, nullptr);
		context.commandPool = VK_NULL_HANDLE;
	}
	for (VkFramebuffer framebuffer : context.framebuffers)
		vkDestroyFramebuffer(context.device, framebuffer, nullptr);
	context.framebuffers.clear();

	if (context.renderPass != VK_NULL_HANDLE) {
		vkDestroyRenderPass(context.device, context.renderPass, nullptr);
		context.renderPass = VK_NULL_HANDLE;
	}
}

} // namespace vk_window::render

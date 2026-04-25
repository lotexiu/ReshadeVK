#include <cstdio>
#include <vector>

#include "declarations.hpp"

// Escolhe o formato de cor da swapchain
// Preferimos BGRA8 com colorspace SRGB — padrão na maioria das GPUs
static VkSurfaceFormatKHR pick_surface_format(VkPhysicalDevice device, VkSurfaceKHR surface) {
	uint32_t count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr);

	std::vector<VkSurfaceFormatKHR> formats(count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, formats.data());

	for (auto& f : formats)
		if (f.format == VK_FORMAT_B8G8R8A8_UNORM &&
			f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return f;

	// se não achou o preferido, usa o primeiro disponível
	return formats[0];
}

// Escolhe o modo de apresentação
// MAILBOX = triple buffering (sem tearing, baixa latência)
// FIFO    = vsync (sempre disponível, fallback seguro)
static VkPresentModeKHR pick_present_mode(VkPhysicalDevice device, VkSurfaceKHR surface) {
	uint32_t count = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, nullptr);

	std::vector<VkPresentModeKHR> modes(count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, modes.data());

	for (auto& m : modes)
		if (m == VK_PRESENT_MODE_MAILBOX_KHR)
			return m;

	return VK_PRESENT_MODE_FIFO_KHR;
}

// Escolhe a resolução da swapchain
// Normalmente é o tamanho da janela em pixels
static VkExtent2D pick_extent(const VkSurfaceCapabilitiesKHR& caps, GLFWwindow* window) {
	// se o compositor já definiu um tamanho fixo, usa ele
	if (caps.currentExtent.width != UINT32_MAX)
		return caps.currentExtent;

	// senão pega o tamanho real da janela em pixels
	int w, h;
	glfwGetFramebufferSize(window, &w, &h);

	VkExtent2D extent = { (uint32_t)w, (uint32_t)h };

	// garante que está dentro dos limites da GPU
	extent.width = std::max(caps.minImageExtent.width, std::min(caps.maxImageExtent.width, extent.width));
	extent.height = std::max(caps.minImageExtent.height, std::min(caps.maxImageExtent.height, extent.height));

	return extent;
}

void vk_create_swapchain(VulkanContext& ctx, GLFWwindow* window) {
	if (ctx.device == VK_NULL_HANDLE) {
		printf("Erro: swapchain nao criada pois device e invalido\n");
		return;
	}

	// coleta as capacidades da surface na GPU
	VkSurfaceCapabilitiesKHR caps;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physical_device, ctx.surface, &caps);

	auto format = pick_surface_format(ctx.physical_device, ctx.surface);
	auto present_mode = pick_present_mode(ctx.physical_device, ctx.surface);
	auto extent = pick_extent(caps, window);

	// quantas imagens na fila — preferimos o mínimo + 1 pra não esperar a GPU
	uint32_t image_count = caps.minImageCount + 1;
	if (caps.maxImageCount > 0 && image_count > caps.maxImageCount)
		image_count = caps.maxImageCount;

	VkSwapchainCreateInfoKHR ci{};
	ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	ci.surface = ctx.surface;
	ci.minImageCount = image_count;
	ci.imageFormat = format.format;
	ci.imageColorSpace = format.colorSpace;
	ci.imageExtent = extent;
	ci.imageArrayLayers = 1; // sempre 1, exceto pra VR
	ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // só uma queue usa as imagens
	ci.preTransform = caps.currentTransform;     // sem rotação extra
	ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // janela opaca
	ci.presentMode = present_mode;
	ci.clipped = VK_TRUE; // ignora pixels cobertos por outras janelas

	if (vkCreateSwapchainKHR(ctx.device, &ci, nullptr, &ctx.swapchain) != VK_SUCCESS) {
		printf("Erro: nao foi possivel criar a swapchain\n");
		return;
	}

	// salva formato e tamanho pra usar nas próximas etapas
	ctx.swapchain_format = format.format;
	ctx.swapchain_extent = extent;

	// pega as imagens criadas pela swapchain
	uint32_t img_count = 0;
	vkGetSwapchainImagesKHR(ctx.device, ctx.swapchain, &img_count, nullptr);
	ctx.swapchain_images.resize(img_count);
	vkGetSwapchainImagesKHR(ctx.device, ctx.swapchain, &img_count, ctx.swapchain_images.data());

	// cria as image views — são o "handle" pra acessar cada imagem
	ctx.swapchain_image_views.resize(img_count);
	for (uint32_t i = 0; i < img_count; i++) {
		VkImageViewCreateInfo view_ci{};
		view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_ci.image = ctx.swapchain_images[i];
		view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view_ci.format = ctx.swapchain_format;
		view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view_ci.subresourceRange.baseMipLevel = 0;
		view_ci.subresourceRange.levelCount = 1;
		view_ci.subresourceRange.baseArrayLayer = 0;
		view_ci.subresourceRange.layerCount = 1;

		if (vkCreateImageView(ctx.device, &view_ci, nullptr, &ctx.swapchain_image_views[i]) != VK_SUCCESS) {
			printf("Erro: nao foi possivel criar image view %u\n", i);
			return;
		}
	}

	printf("Vulkan: swapchain criada (%ux%u, %u imagens)\n",
		extent.width, extent.height, img_count);
}

void vk_create_render_pass(VulkanContext& ctx) {
	if (ctx.device == VK_NULL_HANDLE) {
		printf("Erro: render pass nao criado pois device e invalido\n");
		return;
	}

	// descreve a imagem que vamos usar para desenhar
	// chamada de "attachment" — um slot de imagem no render pass
	VkAttachmentDescription color_attachment{};
	color_attachment.format = ctx.swapchain_format; // mesmo formato da swapchain
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT; // sem multisampling por enquanto
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;   // limpa antes de desenhar
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;  // salva depois de desenhar
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;  // não usamos stencil
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;          // não importa o estado inicial
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;   // pronta pra exibir na tela

	// referência ao attachment acima — usada dentro do subpass
	VkAttachmentReference color_ref{};
	color_ref.attachment = 0;                                        // índice do attachment acima
	color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // layout ideal pra desenhar

	// subpass — uma etapa dentro do render pass
	// por enquanto temos só um, mas shaders de pós-processamento usariam múltiplos
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_ref;

	// dependency — garante que a imagem está pronta antes de começar a desenhar
	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // "antes do render pass"
	dependency.dstSubpass = 0;                   // nosso subpass
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	ci.attachmentCount = 1;
	ci.pAttachments = &color_attachment;
	ci.subpassCount = 1;
	ci.pSubpasses = &subpass;
	ci.dependencyCount = 1;
	ci.pDependencies = &dependency;

	if (vkCreateRenderPass(ctx.device, &ci, nullptr, &ctx.render_pass) != VK_SUCCESS) {
		printf("Erro: nao foi possivel criar o render pass\n");
		return;
	}

	printf("Vulkan: render pass criado\n");
}

void vk_create_framebuffers(VulkanContext& ctx) {
	if (ctx.render_pass == VK_NULL_HANDLE) {
		printf("Erro: framebuffers nao criados pois render pass e invalido\n");
		return;
	}

	// um framebuffer pra cada imagem da swapchain
	ctx.framebuffers.resize(ctx.swapchain_image_views.size());

	for (size_t i = 0; i < ctx.swapchain_image_views.size(); i++) {
		// conecta a image view ao render pass
		VkImageView attachments[] = { ctx.swapchain_image_views[i] };

		VkFramebufferCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		ci.renderPass = ctx.render_pass;
		ci.attachmentCount = 1;
		ci.pAttachments = attachments;
		ci.width = ctx.swapchain_extent.width;
		ci.height = ctx.swapchain_extent.height;
		ci.layers = 1;

		if (vkCreateFramebuffer(ctx.device, &ci, nullptr, &ctx.framebuffers[i]) != VK_SUCCESS) {
			printf("Erro: nao foi possivel criar framebuffer %zu\n", i);
			return;
		}
	}

	printf("Vulkan: %zu framebuffers criados\n", ctx.framebuffers.size());
}
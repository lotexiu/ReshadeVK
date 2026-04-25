#include "swapchain.hpp"

#include "utils/api/vulkan/state.hpp"
#include "utils/api/vulkan/renderer.hpp"

#include <cstdlib>
#include <vector>

namespace reshadevk {

VKAPI_ATTR VkResult VKAPI_CALL hook_CreateSwapchainKHR(
		VkDevice device,
		const VkSwapchainCreateInfoKHR* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkSwapchainKHR* pSwapchain) {
	DeviceData* devData = nullptr;
	{
		std::lock_guard<std::mutex> lock(g_lock);
		auto it = g_deviceData.find(dispatch_key(device));
		if (it == g_deviceData.end())
			return VK_ERROR_INITIALIZATION_FAILED;
		devData = &it->second;
	}

	VkResult result = devData->dispatch.CreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
	if (result != VK_SUCCESS)
		return result;

	// Destroy old swapchain overlay resources
	if (pCreateInfo->oldSwapchain != VK_NULL_HANDLE) {
		SwapchainData oldData{};
		bool hadOldData = false;
		{
			std::lock_guard<std::mutex> lock(g_lock);
			auto oldIt = g_swapchainData.find(pCreateInfo->oldSwapchain);
			if (oldIt != g_swapchainData.end()) {
				oldData    = std::move(oldIt->second);
				g_swapchainData.erase(oldIt);
				hadOldData = true;
			}
		}
		if (hadOldData)
			renderer::destroy(oldData);
	}

	SwapchainData swData{};
	swData.dispatch             = devData->dispatch;
	swData.instanceDispatch     = devData->instanceDispatch;
	swData.setDeviceLoaderData  = devData->setDeviceLoaderData;
	swData.instance             = devData->instance;
	swData.physicalDevice       = devData->physicalDevice;
	swData.device               = devData->device;
	swData.graphicsQueue        = devData->graphicsQueue;
	swData.graphicsQueueFamily  = devData->graphicsQueueFamily;
	swData.format               = pCreateInfo->imageFormat;
	swData.extent               = pCreateInfo->imageExtent;
	swData.imguiCtx             = nullptr;
	swData.interactiveEnabled   = true;
	swData.interactionAvailable = false;

	const char* interactiveEnv = std::getenv("RESHADEVK_INTERACTIVE");
	if (interactiveEnv && interactiveEnv[0] == '0')
		swData.interactiveEnabled = false;

#if RESHADEVK_HAS_GLFW
	{
		std::lock_guard<std::mutex> lock(g_lock);
		auto wit = g_surfaceGlfwWindow.find(pCreateInfo->surface);
		if (wit != g_surfaceGlfwWindow.end()) {
			swData.glfwWindow           = wit->second;
			swData.interactionAvailable = true;
		}
	}
#endif

	renderer::init(*pSwapchain, swData);

	std::lock_guard<std::mutex> lock(g_lock);
	g_swapchainData[*pSwapchain] = std::move(swData);
	return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL hook_DestroySwapchainKHR(
		VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator) {
	SwapchainData swData{};
	DeviceDispatch dispatch{};
	bool hasSwapchainData = false;
	{
		std::lock_guard<std::mutex> lock(g_lock);
		auto swIt = g_swapchainData.find(swapchain);
		if (swIt != g_swapchainData.end()) {
			swData           = std::move(swIt->second);
			g_swapchainData.erase(swIt);
			hasSwapchainData = true;
		}
		auto devIt = g_deviceData.find(dispatch_key(device));
		if (devIt == g_deviceData.end())
			return;
		dispatch = devIt->second.dispatch;
	}
	if (hasSwapchainData)
		renderer::destroy(swData);
	dispatch.DestroySwapchainKHR(device, swapchain, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL hook_QueuePresentKHR(
		VkQueue queue, const VkPresentInfoKHR* pPresentInfo) {
	DeviceData* devData = nullptr;
	{
		std::lock_guard<std::mutex> lock(g_lock);
		auto it = g_deviceData.find(dispatch_key(queue));
		if (it != g_deviceData.end())
			devData = &it->second;
	}
	if (!devData)
		return VK_ERROR_INITIALIZATION_FAILED;

	// Start with the application's wait semaphores
	std::vector<VkSemaphore> waitSems(
			pPresentInfo->pWaitSemaphores,
			pPresentInfo->pWaitSemaphores + pPresentInfo->waitSemaphoreCount);

	for (uint32_t i = 0; i < pPresentInfo->swapchainCount; i++) {
		VkSwapchainKHR sc     = pPresentInfo->pSwapchains[i];
		uint32_t       imgIdx = pPresentInfo->pImageIndices[i];

		SwapchainData* swData = nullptr;
		{
			std::lock_guard<std::mutex> lock(g_lock);
			auto it = g_swapchainData.find(sc);
			if (it != g_swapchainData.end())
				swData = &it->second;
		}

		if (!swData || !swData->imguiCtx || imgIdx >= swData->semaphores.size())
			continue;

		VkSemaphore doneSem = swData->semaphores[imgIdx];
		renderer::render_frame(*swData, queue, imgIdx, waitSems, doneSem);
		waitSems = { doneSem };
	}

	VkPresentInfoKHR modInfo         = *pPresentInfo;
	modInfo.waitSemaphoreCount       = static_cast<uint32_t>(waitSems.size());
	modInfo.pWaitSemaphores          = waitSems.data();
	return devData->dispatch.QueuePresentKHR(queue, &modInfo);
}

} // namespace reshadevk

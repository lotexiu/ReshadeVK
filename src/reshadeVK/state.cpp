#include "state.hpp"

namespace reshadevk {

std::mutex                                        g_lock;
std::unordered_map<void*, InstanceData>           g_instanceData;
std::unordered_map<void*, DeviceData>             g_deviceData;
std::unordered_map<VkSwapchainKHR, SwapchainData> g_swapchainData;
#if RESHADEVK_HAS_GLFW
std::unordered_map<VkSurfaceKHR, void*>            g_surfaceGlfwWindow;
#endif

} // namespace reshadevk

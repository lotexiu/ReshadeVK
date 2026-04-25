#include "dispatch.hpp"

namespace reshadevk {

void fill_instance_dispatch(VkInstance instance, PFN_vkGetInstanceProcAddr gipa, InstanceDispatch& out) {
    out.GetInstanceProcAddr = gipa;
#define FUNC(name) \
    if (!out.name) \
        out.name = reinterpret_cast<PFN_vk##name>(gipa(instance, "vk" #name));
    RESHADEVK_INSTANCE_FUNCS
#undef FUNC
}

void fill_device_dispatch(VkDevice device, PFN_vkGetDeviceProcAddr gdpa, DeviceDispatch& out) {
    out.GetDeviceProcAddr = gdpa;
#define FUNC(name) \
    if (!out.name) \
        out.name = reinterpret_cast<PFN_vk##name>(gdpa(device, "vk" #name));
    RESHADEVK_DEVICE_FUNCS
#undef FUNC
}

} // namespace reshadevk

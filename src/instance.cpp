#include <instance.hpp>
#include <device.hpp>

#ifdef GX_WIN64 
#include <vulkan/vulkan_win32.h>
#endif

namespace gx {
	InstanceInfo::InstanceInfo() noexcept {
		u32 count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
		supported_extensions.resize(count);
		vkEnumerateInstanceExtensionProperties(nullptr, &count, supported_extensions.data());

		count = 0;
		vkEnumerateInstanceLayerProperties(&count, nullptr);
		supported_layers.resize(count);
		vkEnumerateInstanceLayerProperties(&count, supported_layers.data());
	}

	namespace details {
		Result<VkSurfaceKHR> GetSurfaceExtImpl_<Platform::eWindows>::create_surface(VkInstance instance, HINSTANCE app, HWND window) noexcept {
#ifdef GX_WIN64
			VkSurfaceKHR surface;

			VkWin32SurfaceCreateInfoKHR createInfo = {
				.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
				.hinstance = app,
				.hwnd = window,
			};

			auto res = vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface);
			if (res == VK_SUCCESS) {
				return surface;
			}
			return eh::Error{ convert_vk_result(res) };
#endif
		}
	}
}
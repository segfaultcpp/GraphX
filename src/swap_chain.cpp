#include "swap_chain.hpp"

#ifdef GX_WIN64 
#include <vulkan/vulkan_win32.h>
#endif

namespace gx {
	VkPresentModeKHR presentation_mode_to_vk(PresentationMode mode) noexcept {
		switch (mode) {
		case gx::PresentationMode::eFifo:
			return VK_PRESENT_MODE_FIFO_KHR;
		case gx::PresentationMode::eFifoRelaxed:
			return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
		case gx::PresentationMode::eMailbox:
			return VK_PRESENT_MODE_MAILBOX_KHR;
		}
		return VK_PRESENT_MODE_IMMEDIATE_KHR;
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
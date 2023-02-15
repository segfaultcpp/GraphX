#include "swap_chain.hpp"

#ifdef GX_WIN64 
#include <vulkan/vulkan_win32.h>
#endif

namespace gx {
	namespace ext {
		VkPresentModeKHR presentation_mode_to_vk(PresentationMode mode) noexcept {
			switch (mode) {
			case PresentationMode::eFifo:
				return VK_PRESENT_MODE_FIFO_KHR;
			case PresentationMode::eFifoRelaxed:
				return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
			case PresentationMode::eMailbox:
				return VK_PRESENT_MODE_MAILBOX_KHR;
			}
			return VK_PRESENT_MODE_IMMEDIATE_KHR;
		}

		namespace details {
			VkResult GetSurfaceExtImpl_<Platform::eWindows>::create_surface(VkInstance instance, HINSTANCE app, HWND window, VkSurfaceKHR& surface) noexcept {
#ifdef GX_WIN64
				VkWin32SurfaceCreateInfoKHR createInfo = {
					.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
					.hinstance = app,
					.hwnd = window,
				};

				return vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface);
#endif
			}

			void GetSurfaceExtImpl_<Platform::eWindows>::destroy_surface(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks* allocator) noexcept {
				vkDestroySurfaceKHR(instance, surface, allocator);
			}
		}
	}
}
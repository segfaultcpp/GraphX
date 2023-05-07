#include <extensions.hpp>

namespace gx::ext {
	MessageSeverity message_severity_from_vk(VkDebugUtilsMessageSeverityFlagBitsEXT severity) noexcept {
		switch (severity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			return MessageSeverity::eDiagnostic;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			return MessageSeverity::eInfo;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			return MessageSeverity::eWarning;
		}
		return MessageSeverity::eError;
	}

	MessageType message_type_from_vk(VkDebugUtilsMessageTypeFlagsEXT type) noexcept {
		switch (type) {
		case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
			return MessageType::eGeneral;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
			return MessageType::eValidation;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
			return MessageType::ePerformance;
		}
	}

	VkDebugUtilsMessageSeverityFlagsEXT message_severity_to_vk(u8 from) noexcept {
		VkDebugUtilsMessageSeverityFlagsEXT ret = 0;
		if (from & MessageSeverity::eDiagnostic) {
			ret |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
		}
		if (from & MessageSeverity::eInfo) {
			ret |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
		}
		if (from & MessageSeverity::eWarning) {
			ret |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
		}
		if (from & MessageSeverity::eError) {
			ret |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		}
		return ret;
	}

	VkDebugUtilsMessageTypeFlagsEXT message_type_to_vk(u8 from) noexcept {
		VkDebugUtilsMessageTypeFlagsEXT ret = 0;
		if (from & MessageType::eGeneral) {
			ret |= VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
		}
		if (from & MessageType::eValidation) {
			ret |= VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
		}
		if (from & MessageType::ePerformance) {
			ret |= VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		}
		return ret;
	}

	DebugUtilsBuilder DebugUtilsBuilder::with_instance(this const DebugUtilsBuilder self, VkInstance instance) noexcept {
		return DebugUtilsBuilder(instance, self.msg_severity_, self.msg_type_);
	}

	DebugUtilsBuilder DebugUtilsBuilder::with_msg_severity(this const DebugUtilsBuilder self, u8 severity) noexcept {
		return DebugUtilsBuilder(self.instance_, severity, self.msg_type_);
	}

	DebugUtilsBuilder DebugUtilsBuilder::with_msg_type(this const DebugUtilsBuilder self, u8 type) noexcept {
		return DebugUtilsBuilder(self.instance_, self.msg_severity_, type);
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsBuilder::debug_utils_callback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT type,
		const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
		void* user_data)
	{
		if (user_data != nullptr) {
			static_cast<void(*)(MessageSeverity, MessageType)>(user_data)(message_severity_from_vk(severity), message_type_from_vk(type));
		}
		else {
			[](MessageSeverity severity, MessageType type) noexcept {

			}(message_severity_from_vk(severity), message_type_from_vk(type));
		}

		return VK_FALSE;
	}

#ifdef GX_WIN64
	Win32SurfaceBuilder Win32SurfaceBuilder::with_instance(this const Win32SurfaceBuilder self, VkInstance instance) noexcept {
		return Win32SurfaceBuilder(instance, self.window_, self.app_);
	}

	Win32SurfaceBuilder Win32SurfaceBuilder::with_app_info(this const Win32SurfaceBuilder self, HWND window, HINSTANCE app) noexcept {
		return Win32SurfaceBuilder(self.instance_, window, app);
	}

	auto Win32SurfaceBuilder::build(this const Win32SurfaceBuilder self) noexcept -> std::expected<Surface, ErrorCode> {
		VkWin32SurfaceCreateInfoKHR create_info{
			.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			.hinstance = self.app_,
			.hwnd = self.window_
		};

		VkSurfaceKHR surface = VK_NULL_HANDLE;
		VkResult res = vkCreateWin32SurfaceKHR(self.instance_, &create_info, nullptr, &surface);

		if (res == VK_SUCCESS) {
			return Surface{ SurfaceValue{ self.instance_, surface } };
		}

		return std::unexpected(convert_vk_result(res));
	}
#endif

	ColorSpace color_space_from_vk(VkColorSpaceKHR color_space) noexcept {
		switch (color_space) {
		case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:
			return ColorSpace::eSRGB_Nonlinear;
		}
		return ColorSpace::eSRGB_Nonlinear;
	}

	VkColorSpaceKHR color_space_to_vk(ColorSpace color_space) noexcept {
		static constexpr std::array kSpaces = {
			VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
		};

		return kSpaces[std::to_underlying(color_space)];
	}

	PresentMode present_mode_from_vk(VkPresentModeKHR mode) noexcept {
		switch (mode) {
		case VK_PRESENT_MODE_IMMEDIATE_KHR:
			return PresentMode::eImmediate;
		case VK_PRESENT_MODE_FIFO_KHR:
			return PresentMode::eFifo;
		case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
			return PresentMode::eFifoRelaxed;
		case VK_PRESENT_MODE_MAILBOX_KHR:
			return PresentMode::eMailbox;
		}
		return PresentMode::eImmediate;
	}

	VkPresentModeKHR present_mode_to_vk(PresentMode mode) noexcept {
		static constexpr std::array kModes = {
			VK_PRESENT_MODE_IMMEDIATE_KHR,
			VK_PRESENT_MODE_FIFO_KHR,
			VK_PRESENT_MODE_FIFO_RELAXED_KHR,
			VK_PRESENT_MODE_MAILBOX_KHR,
		};
		return kModes[std::to_underlying(mode)];
	}

	SurfaceCapabilities SurfaceCapabilities::from_vk(const VkSurfaceCapabilitiesKHR& caps) noexcept {
		return SurfaceCapabilities{
			.current_extent = Extent2D{ caps.currentExtent.width, caps.currentExtent.height },
			.min_image_extent = Extent2D{ caps.minImageExtent.width, caps.minImageExtent.height },
			.max_image_extent = Extent2D{ caps.maxImageExtent.width, caps.maxImageExtent.height },
			.min_image_count = caps.minImageCount,
			.max_image_count = caps.maxImageCount,
			.max_image_array_layers = caps.maxImageArrayLayers,
		};
	}

	SurfaceFormat SurfaceFormat::from_vk(const VkSurfaceFormatKHR& format) noexcept {
		return SurfaceFormat {
			.format = format_from_vk(format.format),
			.color_space = color_space_from_vk(format.colorSpace),
		};
	}

	VkSurfaceFormatKHR SurfaceFormat::to_vk(this SurfaceFormat self) noexcept {
		return VkSurfaceFormatKHR{
			.format = format_to_vk(self.format),
			.colorSpace = color_space_to_vk(self.color_space),
		};
	}
}
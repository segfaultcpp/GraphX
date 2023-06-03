#include <extensions.hpp>

namespace gx::ext {
	MessageSeverityFlags message_severity_flags_from_vk(VkDebugUtilsMessageSeverityFlagsEXT severity) noexcept {
		MessageSeverityFlags ret = std::to_underlying(MessageSeverity::eError);

		if (test_bit(severity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)) {
			ret |= MessageSeverity::eDiagnostic;
		}
		if (test_bit(severity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)) {
			ret |= MessageSeverity::eInfo;
		}
		if (test_bit(severity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)) {
			ret |= MessageSeverity::eWarning;
		}
		return ret;
	}

	MessageTypeFlags message_type_flags_from_vk(VkDebugUtilsMessageTypeFlagsEXT type) noexcept {
		MessageTypeFlags ret = 0;

		if (test_bit(type, VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)) {
			ret |= MessageType::eGeneral;
		}
		if (test_bit(type, VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)) {
			ret |= MessageType::eValidation;
		}
		if (test_bit(type, VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)) {
			ret |= MessageType::ePerformance;
		}
		return ret;
	}

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

	MessageType message_type_from_vk(VkDebugUtilsMessageTypeFlagBitsEXT type) noexcept {
		switch (type) {
		case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
			return MessageType::eGeneral;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
			return MessageType::eValidation;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
			return MessageType::ePerformance;
		}
		return MessageType::eGeneral;
	}

	VkDebugUtilsMessageSeverityFlagsEXT message_severity_flags_to_vk(MessageSeverityFlags from) noexcept {
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

	VkDebugUtilsMessageSeverityFlagBitsEXT message_severity_to_vk(MessageSeverity from) noexcept {
		switch (from) {
		case MessageSeverity::eDiagnostic:
			return VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
		case MessageSeverity::eInfo:
			return VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
		case MessageSeverity::eWarning:
			return VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
		}
		return VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	}

	VkDebugUtilsMessageTypeFlagsEXT message_type_flags_to_vk(MessageTypeFlags from) noexcept {
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

	VkDebugUtilsMessageTypeFlagBitsEXT message_type_to_vk(MessageType from) noexcept {
		switch (from) {
		case MessageType::eGeneral:
			return VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
		case MessageType::eValidation:
			return VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
		case MessageType::ePerformance:
			return VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		}
		return VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
	}

	DebugUtilsBuilder DebugUtilsBuilder::with_instance(this const DebugUtilsBuilder self, VkInstance instance) noexcept {
		return DebugUtilsBuilder(instance, self.severity_flags_, self.type_flags_);
	}

	DebugUtilsBuilder DebugUtilsBuilder::with_msg_severity(this const DebugUtilsBuilder self, MessageSeverityFlags severity) noexcept {
		return DebugUtilsBuilder(self.instance_, severity, self.type_flags_);
	}

	DebugUtilsBuilder DebugUtilsBuilder::with_msg_type(this const DebugUtilsBuilder self, MessageTypeFlags type) noexcept {
		return DebugUtilsBuilder(self.instance_, self.severity_flags_, type);
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsBuilder::debug_utils_callback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT types,
		const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
		void* user_data)
	{
		if (user_data != nullptr) {
			static_cast<void(*)(MessageSeverity, MessageTypeFlags)>(user_data)(message_severity_from_vk(severity), message_type_flags_from_vk(types));
		}
		else {
			[](MessageSeverity severity, MessageTypeFlags type) noexcept {

			}(message_severity_from_vk(severity), message_type_flags_from_vk(types));
		}

		return VK_FALSE;
	}

	void DebugUtilsBuilder::validate(this const DebugUtilsBuilder self) noexcept {
		// VUID-VkDebugUtilsMessengerCreateInfoEXT-messageSeverity-parameter
		assert(self.severity_flags_ <= std::to_underlying(MessageSeverity::eAll) &&
			"severity_flags_ must be a valid combination of MessageSeverity values");
		// VUID-VkDebugUtilsMessengerCreateInfoEXT-messageType-parameter
		assert(self.type_flags_ <= std::to_underlying(MessageType::eAll) &&
			"type_flags_ must be a valid combination of MessageType values");
		// VUID-VkDebugUtilsMessengerCreateInfoEXT-messageSeverity-requiredbitmask
		assert(self.severity_flags_ != 0 &&
			"severity_flags_ must not be zero");
		// VUID-VkDebugUtilsMessengerCreateInfoEXT-messageType-requiredbitmask
		assert(self.type_flags_ != 0 &&
			"type_flags_ must not be zero");
		// VUID-vkCreateDebugUtilsMessengerEXT-instance-parameter
		assert(self.instance_ != VK_NULL_HANDLE &&
			"instance_ must be a valid VkInstance handle");
	}

#ifdef GX_WIN64
	Win32SurfaceBuilder Win32SurfaceBuilder::with_instance(this const Win32SurfaceBuilder self, VkInstance instance) noexcept {
		return Win32SurfaceBuilder(instance, self.window_, self.app_);
	}

	Win32SurfaceBuilder Win32SurfaceBuilder::with_app_info(this const Win32SurfaceBuilder self, HWND window, HINSTANCE app) noexcept {
		return Win32SurfaceBuilder(self.instance_, window, app);
	}

	auto Win32SurfaceBuilder::build(this const Win32SurfaceBuilder self) noexcept -> std::expected<Surface, ErrorCode> {
		self.validate();

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

	void Win32SurfaceBuilder::validate(this const Win32SurfaceBuilder self) noexcept {
		// VUID-VkWin32SurfaceCreateInfoKHR-hinstance-01307
		assert(self.app_ != nullptr &&
			"app_ must be a valid Win32 HINSTANCE");
		// VUID-VkWin32SurfaceCreateInfoKHR-hwnd-01308
		assert(self.window_ != nullptr && 
			"window_ must be a valid Win32 HWND");
		// VUID-vkCreateWin32SurfaceKHR-instance-parameter
		assert(self.instance_ != VK_NULL_HANDLE &&
			"instance_ must be a valid VkInstance handle");
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
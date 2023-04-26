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

	auto DebugUtilsBuilder::build(this const DebugUtilsBuilder self, auto& callback) noexcept -> std::expected<DebugUtils, ErrorCode> {
		auto vk_msg_severity = message_severity_to_vk(self.msg_severity_);
		auto vk_msg_type = message_type_to_vk(self.msg_type_);

		VkDebugUtilsMessengerCreateInfoEXT create_info = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity = vk_msg_severity,
			.messageType = vk_msg_type,
			.pfnUserCallback = &debug_utils_callback,
			.pUserData = &callback,
		};

		VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;
		VkResult res = DebugUtilsExt::create_fn(self.instance_, &create_info, nullptr, &messenger);

		if (res == VK_SUCCESS) {
			return DebugUtilsValue{ self.instance_, messenger };
		}

		return std::unexpected(convert_vk_result(res));
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
			return SurfaceValue{ self.instance_, surface };
		}

		return std::unexpected(convert_vk_result(res));
	}
#endif
}
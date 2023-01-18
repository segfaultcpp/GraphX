#include <instance.hpp>
#include <device.hpp>

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

	namespace ext {
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

		u8 message_type_from_vk(VkDebugUtilsMessageTypeFlagsEXT type) noexcept {
			u8 ret = 0;
			if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
				ret |= MessageType::eGeneral;
			}
			if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
				ret |= MessageType::eValidation;
			}
			if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
				ret |= MessageType::ePerformance;
			}
			return ret;
		}

		constexpr VkDebugUtilsMessageSeverityFlagsEXT message_severity_to_vk(u8 from) noexcept {
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

		constexpr VkDebugUtilsMessageTypeFlagsEXT message_type_to_vk(u8 from) noexcept {
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
	}
}
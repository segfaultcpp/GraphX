#include <instance.hpp>

namespace gx {
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
	}
}
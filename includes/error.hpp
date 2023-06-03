#pragma once

#include <misc/result.hpp>
#include <misc/types.hpp>

#include <vulkan/vulkan.h>

#include <array>

namespace gx {
	enum class ErrorCode : u32 {
		eSuccess = 0,
		eOutOfHostMemory,
		eOutOfDeviceMemory,
		eInitializationFailed,
		eLayerNotPresent,
		eExtensionNotPresent,
		eIncompatibleDriver,
		eFeatureNotPresent,
		eTooManyObjects,
		eDeviceLost,
		eQueueNotPresent,

		eUnknown,
	};

	inline ErrorCode convert_vk_result(VkResult result) noexcept {
		switch (result) {
		case VK_SUCCESS:
			return ErrorCode::eSuccess;
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			return ErrorCode::eOutOfHostMemory;
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			return ErrorCode::eOutOfDeviceMemory;
		case VK_ERROR_INITIALIZATION_FAILED:
			return ErrorCode::eInitializationFailed;
		case VK_ERROR_LAYER_NOT_PRESENT:
			return ErrorCode::eLayerNotPresent;
		case VK_ERROR_EXTENSION_NOT_PRESENT:
			return ErrorCode::eExtensionNotPresent;
		case VK_ERROR_INCOMPATIBLE_DRIVER:
			return ErrorCode::eIncompatibleDriver;
		case VK_ERROR_FEATURE_NOT_PRESENT:
			return ErrorCode::eFeatureNotPresent;
		case VK_ERROR_TOO_MANY_OBJECTS:
			return ErrorCode::eTooManyObjects;
		case VK_ERROR_DEVICE_LOST:
			return ErrorCode::eDeviceLost;
		}

		return ErrorCode::eUnknown;
	}

	template<typename T>
	using Result = eh::Result<T, ErrorCode>;
}

template<>
struct eh::ErrorTypeTrait<gx::ErrorCode> {
	static constexpr std::array descriptions = {
		"Command successfully completed",
		"A host memory allocation has failed.",
		"A device memory allocation has failed.",
		"Initialization of an object could not be completed for implementation-specific reasons.",
		"A requested layer is not present or could not be loaded.",
		"A requested extension is not supported.",
		"The requested version of Vulkan is not supported by the driver or is otherwise incompatible for implementation-specific reasons.",
		"A requested feature is not supported.",
		"Too many objects of the type have already been created.",
		"The logical or physical device has been lost.",
		"A requested queue is not supported by device.",

		"Unknown error"
	};

	static constexpr std::array names = {
		"gx::ErrorCode::eSuccess",
		"gx::ErrorCode::eOutOfHostMemory",
		"gx::ErrorCode::eOutOfDeviceMemory",
		"gx::ErrorCode::eInitializationFailed",
		"gx::ErrorCode::eLayerNotPresent",
		"gx::ErrorCode::eExtensionNotPresent",
		"gx::ErrorCode::eIncompatibleDriver",
		"gx::ErrorCode::eFeatureNotPresent",
		"gx::ErrorCode::eTooManyObjects",
		"gx::ErrorCode::eDeviceLost",
		"gx::ErrorCode::eQueueNotPresent",

		"gx::ErrorCode::eUnknown",
	};

	static constexpr std::string_view description(gx::ErrorCode value) noexcept {
		EH_ASSERT(static_cast<usize>(value) <= static_cast<usize>(gx::ErrorCode::eUnknown), "Undefined error code");
		return descriptions[static_cast<usize>(value)];
	}

	static constexpr std::string_view stringify(gx::ErrorCode value) noexcept {
		EH_ASSERT(static_cast<usize>(value) <= static_cast<usize>(gx::ErrorCode::eUnknown), "Undefined error code");
		return names[static_cast<usize>(value)];
	}

	static consteval gx::ErrorCode default_value() noexcept {
		return gx::ErrorCode::eSuccess;
	}
};

namespace gx {
	[[nodiscard]]
	inline std::string stringify_error(ErrorCode code) noexcept {
		return std::format("Error code name: {}\nError code description: {}\n", 
			::eh::ErrorTypeTrait<ErrorCode>::stringify(code),
			::eh::ErrorTypeTrait<ErrorCode>::description(code));
	}
}
#pragma once

#include <vector>
#include <optional>
#include <map>

#include <vulkan/vulkan.h>

#include <misc/functional.hpp>
#include <misc/types.hpp>
#include <misc/assert.hpp>
#include <misc/result.hpp>

#include "utils.hpp"
#include "cmd_exec.hpp"
#include "error.hpp"
#include "types.hpp"

namespace gx {
	class DeviceOwner;
	class DeviceView;

	enum class VendorType : u8 {
		eNone = 0, 
		eAmd,
		eNvidia,
		eIntel,
	};

	enum class MemoryProperties : u8 {
		eDeviceLocal = bit<u8, 0>(),
		eHostVisible = bit<u8, 1>(),
		eHostCoherent = bit<u8, 2>(),
	};

	OVERLOAD_BIT_OPS(MemoryProperties, u8);

	struct MemoryInfo {
		usize budget = 0;
		u8 memory_properties = 0;
	};

	enum class QueueTypes {};

	struct QueueInfo {
		QueueTypes type;
		usize index;
		usize count;

		auto operator<=>(const QueueInfo& rhs) const = default;
	};

	enum class PhysicalDeviceType : u8 {
		eNone = 0,
		eDiscreteGpu,
		eIntegratedGpu,
	};

	struct [[nodiscard]] PhysicalDeviceInfo {
		PhysicalDeviceInfo() noexcept = default;

		std::vector<MemoryInfo> memory_infos;
		std::vector<QueueInfo> queue_infos;
		std::string device_name;
		VendorType vendor = VendorType::eNone;
		PhysicalDeviceType device_type = PhysicalDeviceType::eNone;

		std::optional<usize> get_queue_index(QueueTypes type) const noexcept;
	};

	struct PhysDevice {
		VkPhysicalDevice handle;

		PhysDevice() noexcept = default;

		PhysDevice(VkPhysicalDevice device) noexcept 
			: handle{ device }
		{}
	};
}
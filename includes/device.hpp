#pragma once

#include <vector>
#include <optional>

#include <vulkan/vulkan.h>
#include <misc/functional.hpp>

#include "types.hpp"
#include "utils.hpp"

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

	inline u8 operator|(MemoryProperties lhs, MemoryProperties rhs) noexcept {
		return static_cast<u8>(lhs) | static_cast<u8>(rhs);
	}

	inline u8 operator&(u8 lhs, MemoryProperties rhs) noexcept {
		return lhs & static_cast<u8>(rhs);
	}

	inline u8 operator |=(const u8 lhs, MemoryProperties rhs) noexcept {
		return static_cast<MemoryProperties>(lhs) | rhs;
	}

	struct MemoryInfo {
		usize budget = 0;
		u8 memory_properties = 0;
	};

	enum class QueueTypes {
		eGraphics = 0,
		eCopy,
		eCompute,
	};

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

	struct PhysicalDeviceInfo {
		PhysicalDeviceInfo() noexcept = default;

		std::vector<MemoryInfo> memory_infos;
		std::vector<QueueInfo> queue_infos;
		std::string device_name;
		VendorType vendor = VendorType::eNone;
		PhysicalDeviceType device_type = PhysicalDeviceType::eNone;

		std::optional<usize> get_queue_index(QueueTypes type) noexcept;
	};

	struct DeviceDesc {
		std::span<QueueInfo> requested_queues;
	};

	template<std::ranges::random_access_range Rng>
	std::vector<usize> check_supported_queues(Rng&& requested, const PhysicalDeviceInfo& info) noexcept {
		return check_support(requested, info.queue_infos, std::identity{},
			[](QueueInfo lhs, QueueInfo rhs) noexcept {
				return lhs.type == rhs.type;
			});
	}

	/*
	* Represents VkPhysicalDevice. Managed by gx::Instance, so can be copied.
	* Used for obtaining information about underlying physical device (graphics adapter) 
	* and for creating logical device (gx::Device).
	*/
	class PhysicalDevice {
		VkPhysicalDevice handle_ = VK_NULL_HANDLE;
		PhysicalDeviceInfo info_;

	public:
		PhysicalDevice() noexcept = default;
		
		PhysicalDevice(VkPhysicalDevice device) noexcept
			: handle_{ device }
			, info_{}
		{
			fill_info_();
		}

		PhysicalDevice(const PhysicalDevice&) = default;
		PhysicalDevice& operator=(const PhysicalDevice&) = default;

		PhysicalDevice(PhysicalDevice&& rhs) noexcept
			: handle_{ rhs.handle_ }
			, info_{ std::move(rhs.info_) }
		{
			rhs.handle_ = VK_NULL_HANDLE;
			rhs.info_ = {};
		}

		PhysicalDevice& operator=(PhysicalDevice&& rhs) noexcept {
			handle_ = rhs.handle_;
			rhs.handle_ = VK_NULL_HANDLE;

			info_ = std::move(rhs.info_);
			rhs.info_ = {};

			return *this;
		}

	public:
		std::optional<DeviceOwner> get_logical_device(DeviceDesc desc) noexcept;

		const PhysicalDeviceInfo& get_info() const noexcept {
			return info_;
		}

	private:
		void fill_info_();

	};

	constexpr auto enum_phys_device_infos() noexcept {
		return rng::views::transform([](const PhysicalDevice& device) noexcept -> decltype(auto) {
			return device.get_info();
		});
	}

	constexpr auto min_vram_size(usize value) noexcept {
		return [value](const PhysicalDevice& phys_device) noexcept {
			const auto& info = phys_device.get_info();
			auto it = rng::find_if(info.memory_infos,
				[](const auto& el) {
					return  (el.memory_properties & MemoryProperties::eDeviceLocal) != 0 &&
							(el.memory_properties & MemoryProperties::eHostVisible) == 0;
				});

			return it != info.memory_infos.end() ? (*it).budget >= value : false;
		};
	}

	constexpr auto request_queue(QueueTypes type) noexcept {
		return [type](const PhysicalDevice& phys_device) noexcept {
			const auto& info = phys_device.get_info();
			return rng::find(info.queue_infos, type, &QueueInfo::type) != info.queue_infos.end();
		};
	}

	constexpr auto request_graphics_queue() noexcept {
		return request_queue(QueueTypes::eGraphics);
	}

	constexpr auto request_copy_queue() noexcept {
		return request_queue(QueueTypes::eCopy);
	}

	constexpr auto request_compute_queue() noexcept {
		return request_queue(QueueTypes::eCompute);
	}

	constexpr auto request_phys_device_type(PhysicalDeviceType type) {
		return [type](const PhysicalDevice& phys_device) noexcept {
			return phys_device.get_info().device_type == type;
		};
	}

	constexpr auto request_discrete_gpu() noexcept {
		return request_phys_device_type(PhysicalDeviceType::eDiscreteGpu);
	}

	constexpr auto request_integrated_gpu() noexcept {
		return request_phys_device_type(PhysicalDeviceType::eIntegratedGpu);
	}

	inline void destroy_device(VkDevice device) noexcept {
		if (device != VK_NULL_HANDLE) {
			// TODO: glob allocator
			vkDestroyDevice(device, nullptr);
		}
	}

	/*
	* Owns VkDevice object and responsible for destroying it.
	*/
	class DeviceOwner {
	private:
		VkDevice handle_ = VK_NULL_HANDLE;
		VkPhysicalDevice underlying_device_ = VK_NULL_HANDLE;

	public:
		DeviceOwner() noexcept = default;
		DeviceOwner(VkDevice device, VkPhysicalDevice underlying) noexcept
			: handle_{ device }
			, underlying_device_{ underlying }
		{}

		DeviceOwner(const DeviceOwner& rhs) = delete;
		DeviceOwner& operator=(const DeviceOwner& rhs) = delete;

		DeviceOwner(DeviceOwner&& rhs) noexcept
			: handle_{ rhs.handle_ }
			, underlying_device_{ rhs.underlying_device_ }
		{
			rhs.handle_ = VK_NULL_HANDLE;
			rhs.underlying_device_ = VK_NULL_HANDLE;
		}

		DeviceOwner& operator=(DeviceOwner&& rhs) noexcept {
			handle_ = rhs.handle_;
			underlying_device_ = rhs.underlying_device_;

			rhs.underlying_device_ = VK_NULL_HANDLE;
			rhs.handle_ = VK_NULL_HANDLE;

			return *this;
		}

		~DeviceOwner() noexcept {
			destroy_device(handle_);
		}

	public:
		/*
		* WARNING: Unsafe
		* Moves out ownership to caller. The caller must manage this device themselves.
		*/
		[[nodiscard]]
		VkDevice unwrap_native_handle() noexcept {
			VkDevice ret = handle_;
			handle_ = VK_NULL_HANDLE;
			return ret;
		}

	};

	class DeviceView {
		VkDevice handle_;
		VkPhysicalDevice underlying_;

	public:
		DeviceView(VkDevice logical_device, VkPhysicalDevice underlying_phys_device) noexcept 
			: handle_{ logical_device }
			, underlying_{ underlying_phys_device }
		{}

		DeviceView(const DeviceView& rhs) noexcept 
			: handle_{ rhs.handle_ }
			, underlying_ { rhs.underlying_ }
		{}
	};
}
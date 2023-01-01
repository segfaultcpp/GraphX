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
#include "object.hpp"

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
		eTransfer,
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

	struct [[nodiscard]] PhysicalDeviceInfo {
		PhysicalDeviceInfo() noexcept = default;

		std::vector<MemoryInfo> memory_infos;
		std::vector<QueueInfo> queue_infos;
		std::string device_name;
		VendorType vendor = VendorType::eNone;
		PhysicalDeviceType device_type = PhysicalDeviceType::eNone;

		std::optional<usize> get_queue_index(QueueTypes type) const noexcept;
	};

	struct DeviceDesc {
		std::span<QueueInfo> requested_queues;
	};

	template<std::ranges::random_access_range Rng>
	[[nodiscard]]
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
	class PhysicalDevice : public ManagedObject<::gx::Copyable, VkPhysicalDevice> {
	private:
		static std::map<VkPhysicalDevice, PhysicalDeviceInfo> infos_;

	public:
		using Base = ManagedObject<::gx::Copyable, VkPhysicalDevice>;
		using ObjectType = VkPhysicalDevice;

	public:
		PhysicalDevice() noexcept = default;
		
		PhysicalDevice(VkPhysicalDevice device) noexcept
			: Base{ device }
		{
			fill_info_();
		}

		PhysicalDevice(const PhysicalDevice&) = default;
		PhysicalDevice& operator=(const PhysicalDevice&) = default;

		PhysicalDevice(PhysicalDevice&& rhs) noexcept
			: Base{ std::move(rhs) }
		{}

		PhysicalDevice& operator=(PhysicalDevice&& rhs) noexcept {
			static_cast<Base&>(*this) = std::move(rhs);

			return *this;
		}

	public:
		eh::Result<DeviceOwner, ErrorCode> get_logical_device(DeviceDesc desc) noexcept;

		const PhysicalDeviceInfo& get_info() const noexcept {
			return infos_.at(this->handle_);
		}

	public:
		static const PhysicalDeviceInfo& get_info(VkPhysicalDevice device) noexcept {
			EH_ASSERT(infos_.find(device) != infos_.end(), "std::map<VkPhysicalDevice, PhysicalDeviceInfo> infos_ does not contain information about requeted physical device");
			return infos_.at(device);
		}

	private:
		void fill_info_() noexcept;

	};

	constexpr auto enum_phys_device_infos() noexcept {
		return std::ranges::views::transform([](const PhysicalDevice& device) noexcept -> decltype(auto) {
			return device.get_info();
		});
	}

	constexpr auto min_vram_size(usize value) noexcept {
		return [value](const PhysicalDevice& phys_device) noexcept {
			const auto& info = phys_device.get_info();
			auto it = std::ranges::find_if(info.memory_infos,
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
			return std::ranges::find(info.queue_infos, type, &QueueInfo::type) != info.queue_infos.end();
		};
	}

	constexpr auto request_graphics_queue() noexcept {
		return request_queue(QueueTypes::eGraphics);
	}

	constexpr auto request_transfer_queue() noexcept {
		return request_queue(QueueTypes::eTransfer);
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

	class [[nodiscard]] DeviceView : public ObjectView<VkDevice> {
		static std::array<usize, 3> queue_indices;
		VkPhysicalDevice underlying_;

	public:
		using Base = ObjectView<VkDevice>;
		using ObjectType = VkDevice;

	public:
		DeviceView(VkDevice logical_device, VkPhysicalDevice underlying_phys_device) noexcept
			: Base{ logical_device }
			, underlying_{ underlying_phys_device }
		{
			static auto _ = [this] {
				queue_indices[0] = PhysicalDevice::get_info(this->underlying_).get_queue_index(QueueTypes::eGraphics).value_or(~0);
				queue_indices[1] = PhysicalDevice::get_info(this->underlying_).get_queue_index(QueueTypes::eCompute).value_or(~0);
				queue_indices[2] = PhysicalDevice::get_info(this->underlying_).get_queue_index(QueueTypes::eTransfer).value_or(~0);

				return 0;
			}();
		}

	public:
		template<CommandContext Ctx>
		eh::Result<CommandPool<Ctx>, ErrorCode> create_command_pool() const noexcept {
			usize index = [] {
				if constexpr (std::same_as<Ctx, GraphicsContext>) {
					return 0;
				}

				if constexpr (std::same_as<Ctx, ComputeContext>) {
					return 1;
				}

				if constexpr (std::same_as<Ctx, TransferContext>) {
					return 2;
				}
				return ~0;
			}();
			
			EH_ASSERT(queue_indices[index] != static_cast<usize>(~0), "This device does not support requested queue");
			index = queue_indices[index];

			VkCommandPoolCreateInfo create_info = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
				.queueFamilyIndex = static_cast<u32>(index)
			};

			VkCommandPool pool;
			auto result = vkCreateCommandPool(this->handle_, &create_info, nullptr, &pool);
			if (result == VK_SUCCESS) {
				return CommandPool<Ctx>{ pool, this->handle_ };
			}

			return eh::Error{ convert_vk_result(result) };
		}

	};

	/*
	* Owns VkDevice object and responsible for destroying it.
	*/
	class [[nodiscard]] DeviceOwner : public ObjectOwner<DeviceOwner, VkDevice> {
	private:
		VkPhysicalDevice underlying_device_ = VK_NULL_HANDLE;
		std::vector<GraphicsQueue> graphics_qs_;
		std::vector<ComputeQueue> compute_qs_;
		std::vector<TransferQueue> transfer_qs_;

	public:
		using Base = ObjectOwner<DeviceOwner, VkDevice>;
		using ObjectType = VkDevice;

	public:
		DeviceOwner() noexcept = default;
		DeviceOwner(VkDevice device, VkPhysicalDevice underlying, std::span<QueueInfo> req_qs) noexcept
			: Base{ device }
			, underlying_device_{ underlying }
		{
			init_queues_(req_qs);
		}

		DeviceOwner(DeviceOwner&& rhs) noexcept = default;
		DeviceOwner& operator=(DeviceOwner&& rhs) noexcept = default;

		~DeviceOwner() noexcept = default;

	public:
		template<Queue Q>
		[[nodiscard]]
		Q pop_back_queue() noexcept {
			if constexpr (std::same_as<Q, GraphicsQueue>) {
				return pop_back_queue_(graphics_qs_);
			}

			if constexpr (std::same_as<Q, ComputeQueue>) {
				return pop_back_queue_(compute_qs_);
			}

			if constexpr (std::same_as<Q, TransferQueue>) {
				return pop_back_queue_(transfer_qs_);
			}
		}

		[[nodiscard]]
		DeviceView get_view() const noexcept {
			return DeviceView{ this->handle_, underlying_device_ };
		}

	private:
		void init_queues_(std::span<QueueInfo> req_qs) noexcept;
		
		template<Queue Q>
		[[nodiscard]]
		Q pop_back_queue_(std::vector<Q>& v) noexcept {
			EH_ASSERT(!v.empty(), "Device does not contain requested queue!");

			Q ret = std::move(v.back());
			v.pop_back();
			return ret;
		}

	};

	namespace unsafe {
		template<>
		struct ObjectOwnerTrait<DeviceOwner> {
			static void destroy(DeviceOwner& obj) noexcept {
				vkDestroyDevice(obj.handle_, nullptr);
			}

			[[nodiscard]]
			static VkDevice unwrap_native_handle(DeviceOwner& obj) noexcept {
				auto ret = obj.handle_;
				obj.handle_ = VK_NULL_HANDLE;
				return ret;
			}
		};
	}

}
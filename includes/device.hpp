#pragma once

#include <vector>
#include <optional>
#include <map>
#include <expected>
#include <cassert>

#include <vulkan/vulkan.h>

#include <misc/functional.hpp>
#include <misc/types.hpp>
#include <misc/assert.hpp>
#include <misc/result.hpp>

#include "utils.hpp"
#include "cmd_exec.hpp"
#include "error.hpp"
#include "types.hpp"
#include "extensions.hpp"

namespace gx {
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

	enum class QueueType : u8 {
		eGraphics = 0,
		eTransfer,
		eCompute,
	};

	struct QueueInfo {
		QueueType type = QueueType::eGraphics;
		usize index = 0;
		usize count = 0;
	};

	enum class PhysicalDeviceType : u8 {
		eNone = 0,
		eDiscreteGpu,
		eIntegratedGpu,
	};

	struct PhysDevice;

	struct [[nodiscard]] PhysDeviceInfo {
		friend PhysDevice;

	public:
		std::vector<MemoryInfo> memory_infos;
		std::vector<QueueInfo> queue_infos;
		std::string device_name;
		VendorType vendor = VendorType::eNone;
		PhysicalDeviceType device_type = PhysicalDeviceType::eNone;

		[[nodiscard]]
		std::optional<u32> get_queue_index(QueueType type) const noexcept;
	
		static const PhysDeviceInfo& get(PhysDevice phys_device) noexcept;
		static const PhysDeviceInfo& get(VkPhysicalDevice phys_device) noexcept;

	private:
		static std::vector<std::pair<VkPhysicalDevice, PhysDeviceInfo>> phys_device_infos_;
		static PhysDeviceInfo fill_info_(VkPhysicalDevice phys_device) noexcept;
		static void push_(VkPhysicalDevice phys_device) noexcept;

	};

	struct DeviceValue {
		VkDevice handle;

		DeviceValue() noexcept = default;

		DeviceValue(VkDevice device)
			: handle{ device }
		{}

		void destroy() noexcept {
			vkDestroyDevice(handle, nullptr);
		}
	};
	static_assert(Value<DeviceValue>);

	template<typename E = meta::List<>>
	struct DeviceImpl;

	template<typename... Es>
	struct DeviceImpl<meta::List<Es...>> {
		template<typename Self> 
			requires meta::SameAsAny<ext::SwapchainExt, Es...>
		[[nodiscard]]
		ext::SwapchainBuilder get_ext_swapchain_builder(this Self&& self, ext::SurfaceView surface) noexcept {
			return ext::SwapchainBuilder{ self.get_handle() }.with_surface(surface);
		}
	};

	template<typename E>
	using Device = OwnedType<DeviceValue, DeviceImpl<E>, MoveOnlyTag, ViewableTag>;

	template<typename E>
	using DeviceView = decltype(std::declval<Device<E>&>().get_view());

	template<typename Es = meta::List<>>
	struct DeviceBuilder;

	template<typename... Es>
	struct DeviceBuilder<meta::List<Es...>> {
		VkPhysicalDevice phys_device = VK_NULL_HANDLE;
		std::array<QueueInfo, 3> requested_queues;

		DeviceBuilder() noexcept = default;

		DeviceBuilder(VkPhysicalDevice device, std::array<QueueInfo, 3> req_qs) noexcept 
			: phys_device{ device }
			, requested_queues{ req_qs }
		{}

		[[nodiscard]]
		DeviceBuilder& with_phys_device(VkPhysicalDevice device) noexcept {
			phys_device = device;
			return *this;
		}

		template<ext::DeviceExt... Es1>
		auto with_extensions() noexcept {
			return DeviceBuilder<meta::List<Es1..., Es...>>{ phys_device, requested_queues };
		}

		template<ext::DeviceExt... Es1>
		auto with_extensions(meta::List<Es1...>) noexcept {
			return with_extensions<Es1...>();
		}

		[[nodiscard]]
		DeviceBuilder& request_queues(QueueInfo info) noexcept {
			requested_queues[std::to_underlying(info.type)].count += info.count;
			return *this;
		}

		[[nodiscard]]
		DeviceBuilder& request_graphics_queues(usize count = 1) noexcept {
			return request_queues(QueueInfo{ .type = QueueType::eGraphics, .count = count });
		}

		[[nodiscard]]
		DeviceBuilder& request_transfer_queues(usize count = 1) noexcept {
			return request_queues(QueueInfo{ .type = QueueType::eTransfer, .count = count });
		}

		[[nodiscard]]
		DeviceBuilder& request_compute_queues(usize count = 1) noexcept {
			return request_queues(QueueInfo{ .type = QueueType::eCompute, .count = count });
		}

		[[nodiscard]]
		auto build() const noexcept -> std::expected<Device<meta::List<Es...>>, ErrorCode> {
			// validate();

			const auto& qi = PhysDeviceInfo::get(phys_device).queue_infos;
			auto by_count = [](QueueInfo info) noexcept {
				return info.count != 0;
			};
			auto supported_by_device = [&qi](QueueInfo info) noexcept {
				return std::ranges::find(qi, info.type, &QueueInfo::type) != qi.end();
			};
			auto rq = requested_queues | std::views::filter(by_count) | std::views::filter(supported_by_device);

			std::array<f32, 16> priors{ 1.f };
			std::vector<VkDeviceQueueCreateInfo> q_infos;
			for (auto&& el : rq) {
				auto queue_index = PhysDeviceInfo::get(phys_device).get_queue_index(el.type);
				assert(queue_index.has_value() && "TODO: DeviceBuilder::build()");

				q_infos.push_back(VkDeviceQueueCreateInfo {
					.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
					.queueFamilyIndex = queue_index.value(),
					.queueCount = static_cast<u32>(el.count),
					.pQueuePriorities = priors.data()
				});
			}

			VkPhysicalDeviceFeatures features{};

			VkDeviceCreateInfo device_info = {
				.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
				.queueCreateInfoCount = static_cast<u32>(q_infos.size()),
				.pQueueCreateInfos = q_infos.data(),
				.pEnabledFeatures = &features,
			};

			if constexpr (sizeof...(Es) > 0) {
				static constexpr std::array kRequiredExts = ext::to_array<Es...>();
				device_info.enabledExtensionCount = static_cast<u32>(kRequiredExts.size());
				device_info.ppEnabledExtensionNames = kRequiredExts.data();
			}

			DeviceValue device{};
			VkResult res = vkCreateDevice(phys_device, &device_info, nullptr, &device.handle);

			if (res == VK_SUCCESS) {
				return Device<meta::List<Es...>>{ device };
			}

			return std::unexpected(convert_vk_result(res));
		}

	private:
		void validate() const noexcept {
			TODO("");
			const auto& qi = PhysDeviceInfo::get(phys_device).queue_infos;
			auto by_count = [](QueueInfo info) noexcept {
				return info.count != 0;
			};

		}
	};

	struct PhysDevice {
	private:
		VkPhysicalDevice handle_;
	
	public:
		PhysDevice() noexcept = default;

		PhysDevice(VkPhysicalDevice device) noexcept 
			: handle_{ device }
		{
			PhysDeviceInfo::push_(device);
		}

		[[nodiscard]]
		VkPhysicalDevice get_handle(this const PhysDevice self) noexcept {
			return self.handle_;
		}

		[[nodiscard]]
		DeviceBuilder<meta::List<>> get_device_builder(this const PhysDevice self) noexcept {
			return DeviceBuilder{}.with_phys_device(self.handle_);
		}
	
		[[nodiscard]]
		const PhysDeviceInfo& get_info(this const PhysDevice self) noexcept {
			return PhysDeviceInfo::get(self.handle_);
		}

		[[nodiscard]]
		std::optional<QueueType> supports_presentation(this const PhysDevice self, ext::SurfaceView surface) noexcept;
		[[nodiscard]]
		ext::SwapchainSupport query_swapchain_support(this const PhysDevice self, ext::SurfaceView surface) noexcept;
	};
	static_assert(std::is_trivially_copyable_v<PhysDevice>);

	[[nodiscard]]
	constexpr auto enum_phys_device_infos() noexcept {
		return std::views::transform(
			[](PhysDevice device) noexcept -> decltype(auto) {
				return device.get_info();
			}
		);
	}

	[[nodiscard]]
	constexpr auto filter_by_min_vram_size(usize value) noexcept {
		return std::views::filter(
			[value](PhysDevice phys_device) noexcept {
				const auto& infos = phys_device.get_info().memory_infos;
				auto it = std::ranges::find_if(infos,
					[](const auto& el) {
						return  test_bit(el.memory_properties, MemoryProperties::eDeviceLocal) &&
							!test_bit(el.memory_properties, MemoryProperties::eHostVisible);
					});

				return it != infos.end() ? (*it).budget >= value : false;
			}
		);
	}

	[[nodiscard]]
	constexpr auto filter_by_requested_queue(QueueType type) noexcept {
		return std::views::filter(
			[type](PhysDevice phys_device) noexcept {
				const auto& info = phys_device.get_info();
				return std::ranges::find(info.queue_infos, type, &QueueInfo::type) != info.queue_infos.end();
			}
		);
	}

	[[nodiscard]]
	constexpr auto request_graphics_queue() noexcept {
		return filter_by_requested_queue(QueueType::eGraphics);
	}

	[[nodiscard]]
	constexpr auto request_transfer_queue() noexcept {
		return filter_by_requested_queue(QueueType::eTransfer);
	}

	[[nodiscard]]
	constexpr auto request_compute_queue() noexcept {
		return filter_by_requested_queue(QueueType::eCompute);
	}

	[[nodiscard]]
	constexpr auto filter_by_requested_phys_device_type(PhysicalDeviceType type) {
		return std::views::filter(
			[type](PhysDevice phys_device) noexcept {
				return phys_device.get_info().device_type == type;
			}
		);
	}

	[[nodiscard]]
	constexpr auto request_discrete_gpu() noexcept {
		return filter_by_requested_phys_device_type(PhysicalDeviceType::eDiscreteGpu);
	}

	[[nodiscard]]
	constexpr auto request_integrated_gpu() noexcept {
		return filter_by_requested_phys_device_type(PhysicalDeviceType::eIntegratedGpu);
	}

	[[nodiscard]]
	inline auto request_presentation_support(ext::SurfaceView surface) noexcept {
		return std::views::filter(
			[surface](PhysDevice device) noexcept {
				return device.supports_presentation(surface).has_value();
			}
		);
	}
}
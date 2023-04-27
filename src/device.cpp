#include <device.hpp>

namespace gx {
	std::vector<std::pair<VkPhysicalDevice, PhysDeviceInfo>> PhysDeviceInfo::phys_device_infos_{};

	std::optional<u32> PhysDeviceInfo::get_queue_index(QueueType type) const noexcept {
		auto it = std::ranges::find(queue_infos, type, &QueueInfo::type);

		if (it != queue_infos.end()) {
			return it->index;
		}
		else {
			return std::nullopt;
		}
	}

	PhysDeviceInfo PhysDeviceInfo::fill_info_(VkPhysicalDevice phys_device) noexcept {
		VkPhysicalDeviceProperties props{};
		VkPhysicalDeviceMemoryProperties mem_props{};
		vkGetPhysicalDeviceProperties(phys_device, &props);
		vkGetPhysicalDeviceMemoryProperties(phys_device, &mem_props);

		PhysDeviceInfo info;

		info.memory_infos.resize(mem_props.memoryTypeCount);
		for (usize i : std::ranges::views::iota(0u, mem_props.memoryTypeCount)) {
			auto mem_type = mem_props.memoryTypes[i];

			if (test_bit(mem_type.propertyFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
				info.memory_infos[i].memory_properties |= static_cast<u8>(MemoryProperties::eDeviceLocal);
			}
			if (test_bit(mem_type.propertyFlags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
				info.memory_infos[i].memory_properties |= static_cast<u8>(MemoryProperties::eHostVisible);
			}
			if (test_bit(mem_type.propertyFlags, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
				info.memory_infos[i].memory_properties |= static_cast<u8>(MemoryProperties::eHostCoherent);
			}

			info.memory_infos[i].budget = mem_props.memoryHeaps[mem_type.heapIndex].size;
		}

		info.device_name = props.deviceName;

		if (props.vendorID == 0x1022u || props.vendorID == 0x1002u) {
			info.vendor = VendorType::eAmd;
		}
		else if (props.vendorID == 0x10DEu) {
			info.vendor = VendorType::eNvidia;
		}
		else if (props.vendorID == 8086u) {
			info.vendor = VendorType::eIntel;
		}

		u32 q_prop_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(phys_device, &q_prop_count, nullptr);
		std::vector<VkQueueFamilyProperties> q_props(q_prop_count);
		vkGetPhysicalDeviceQueueFamilyProperties(phys_device, &q_prop_count, q_props.data());

		info.queue_infos.clear();

		for (auto [i, el] : std::views::zip(std::views::iota(0u), q_props)) {
			if (test_bit(el.queueFlags, VK_QUEUE_GRAPHICS_BIT)) {
				info.queue_infos.push_back(
					QueueInfo {
						.type = QueueType::eGraphics,
						.index = i,
						.count = el.queueCount
					}
				);
			}
			else if (test_bit(el.queueFlags, VK_QUEUE_COMPUTE_BIT) && test_bit(el.queueFlags, VK_QUEUE_TRANSFER_BIT)) {
				info.queue_infos.push_back(
					QueueInfo {
						.type = QueueType::eCompute,
						.index = i,
						.count = el.queueCount
					}
				);
			}
			else if (test_bit(el.queueFlags, VK_QUEUE_TRANSFER_BIT)) {
				info.queue_infos.push_back(
					QueueInfo {
						.type = QueueType::eTransfer,
						.index = i,
						.count = el.queueCount
					}
				);
			}
		}

		if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			info.device_type = PhysicalDeviceType::eDiscreteGpu;
		}
		else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
			info.device_type = PhysicalDeviceType::eIntegratedGpu;
		}

		return info;
	}

	const PhysDeviceInfo& PhysDeviceInfo::get(PhysDevice phys_device) noexcept {
		return get(phys_device.get_handle());
	}

	const PhysDeviceInfo& PhysDeviceInfo::get(VkPhysicalDevice phys_device) noexcept {
		auto it = std::ranges::find(phys_device_infos_, phys_device, &std::pair<VkPhysicalDevice, PhysDeviceInfo>::first);
		if (it != phys_device_infos_.end()) {
			return it->second;
		}
	
		push_(phys_device);
		return phys_device_infos_.back().second;
	}

	void PhysDeviceInfo::push_(VkPhysicalDevice phys_device) noexcept {
		phys_device_infos_.emplace_back(phys_device, fill_info_(phys_device));
	}

	std::optional<QueueType> PhysDevice::supports_presentation(this const PhysDevice self, ext::SurfaceView surface) noexcept {
		static constexpr auto kQTypes = std::array{ QueueType::eGraphics, QueueType::eCompute, QueueType::eTransfer };
		
		for (auto type : kQTypes) {
			VkBool32 supported = false;
			self.get_info().get_queue_index(type)
				.and_then(
					[self, surface, &supported](u32 index) noexcept -> std::optional<u32> {
						vkGetPhysicalDeviceSurfaceSupportKHR(self.get_handle(), index, surface.get_handle(), &supported);
						return index;
					}
			);

			if (supported) {
				return type;
			}
		}
		return std::nullopt;
	}

	ext::SwapchainSupport PhysDevice::query_swapchain_support(this const PhysDevice self, ext::SurfaceView surface) noexcept
	{
		ext::SwapchainSupport support;

		VkSurfaceCapabilitiesKHR caps{};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(self.handle_, surface.get_handle(), &caps);
		support.caps = ext::SurfaceCapabilities::from_vk(caps);

		u32 count = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(self.handle_, surface.get_handle(), &count, nullptr);

		if (count != 0) {
			std::vector<VkSurfaceFormatKHR> formats(count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(self.handle_, surface.get_handle(), &count, formats.data());
			support.formats = formats | std::views::transform(ext::SurfaceFormat::from_vk) | std::ranges::to<std::vector>();
		}

		count = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(self.handle_, surface.get_handle(), &count, nullptr);

		if (count != 0) {
			std::vector<VkPresentModeKHR> modes(count);
			vkGetPhysicalDeviceSurfacePresentModesKHR(self.handle_, surface.get_handle(), &count, modes.data());
			support.present_modes = modes | std::views::transform(ext::present_mode_from_vk) | std::ranges::to<std::vector>();
		}

		return support;
	}
}
#include <device.hpp>

namespace gx {
	std::map<VkPhysicalDevice, PhysicalDeviceInfo> PhysicalDevice::infos_{};

	std::optional<usize> PhysicalDeviceInfo::get_queue_index(QueueTypes type) const noexcept {
		auto it = std::ranges::find(queue_infos, type, &QueueInfo::type);
		
		if (it != queue_infos.end()) {
			return (*it).index;
		} else {
			return std::nullopt;
		}
	}

	void gx::PhysicalDevice::fill_info_() noexcept {
		VkPhysicalDeviceProperties props{};
		VkPhysicalDeviceMemoryProperties mem_props{};
		vkGetPhysicalDeviceProperties(handle_, &props);
		vkGetPhysicalDeviceMemoryProperties(handle_, &mem_props);

		PhysicalDeviceInfo info;

		info.memory_infos.resize(mem_props.memoryTypeCount);
		for (usize i : std::ranges::views::iota(0u, mem_props.memoryTypeCount)) {
			auto mem_type = mem_props.memoryTypes[i];
			
			if ((mem_type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0) {
				info.memory_infos[i].memory_properties |= static_cast<u8>(MemoryProperties::eDeviceLocal);
			}
			if ((mem_type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
				info.memory_infos[i].memory_properties |= static_cast<u8>(MemoryProperties::eHostVisible);
			}
			if ((mem_type.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0) {
				info.memory_infos[i].memory_properties |= static_cast<u8>(MemoryProperties::eHostCoherent);
			}

			info.memory_infos[i].budget = mem_props.memoryHeaps[mem_type.heapIndex].size;
		}

		info.device_name = props.deviceName;

		if (props.vendorID == 0x1022u || props.vendorID == 0x1002u)
		{
			info.vendor = VendorType::eAmd;
		}
		else if (props.vendorID == 0x10DEu)
		{
			info.vendor = VendorType::eNvidia;
		}
		else if (props.vendorID == 8086u)
		{
			info.vendor = VendorType::eIntel;
		}

		u32 q_prop_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(handle_, &q_prop_count, nullptr);
		std::vector<VkQueueFamilyProperties> q_props(q_prop_count);
		vkGetPhysicalDeviceQueueFamilyProperties(handle_, &q_prop_count, q_props.data());
		
		info.queue_infos.clear();

		for (usize i : std::ranges::views::iota(0u, q_prop_count))
		{
			if (q_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				info.queue_infos.push_back(QueueInfo{
					.type = QueueTypes::eGraphics,
					.index = i,
					.count = q_props[i].queueCount
					});
			}
			else if (q_props[i].queueFlags & VK_QUEUE_COMPUTE_BIT &&
					 q_props[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				info.queue_infos.push_back(QueueInfo{
					.type = QueueTypes::eCompute,
					.index = i,
					.count = q_props[i].queueCount
					});
			}
			else if (q_props[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				info.queue_infos.push_back(QueueInfo{
					.type = QueueTypes::eTransfer,
					.index = i,
					.count = q_props[i].queueCount
					});
			}
		}

		if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			info.device_type = PhysicalDeviceType::eDiscreteGpu;
		}
		else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
			info.device_type = PhysicalDeviceType::eIntegratedGpu;
		}

		infos_.insert(std::make_pair(this->handle_, std::move(info)));
	}

	eh::Result<DeviceOwner, ErrorCode> PhysicalDevice::get_logical_device(DeviceDesc desc) noexcept {
		auto idxs = check_supported_queues(desc.requested_queues, get_info());
		
		if (idxs.size() > 0) {
			std::string unsupported_qs;
			for (usize i : idxs) {
				unsupported_qs += std::format("{} ", i);
			}
			EH_ERROR_MSG(std::format("Some requsted queues are not supported (indices: {})", unsupported_qs));
			return eh::Error{ ErrorCode::eQueueNotPresent };
		}

		// TODO: additional validation
		for (auto& req_q : desc.requested_queues) {
			auto it = std::ranges::find(get_info().queue_infos, req_q.type, &QueueInfo::type);
			if (req_q.count > it->count) {
				EH_ERROR_MSG(std::format("Requested {} but only {} presented", req_q.count , it->count));
				req_q.count = it->count;
			}
		}

		auto& req_qs = desc.requested_queues;
		std::vector<VkDeviceQueueCreateInfo> q_infos(req_qs.size());

		std::vector<f32> priors(32u, 1.f);
		for (usize i : std::ranges::views::iota(0u, q_infos.size())) {
			q_infos[i] = VkDeviceQueueCreateInfo {
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueFamilyIndex = static_cast<u32>(get_info().get_queue_index(req_qs[i].type).value()),
				.queueCount = static_cast<u32>(req_qs[i].count),
				.pQueuePriorities = priors.data()
			};
			req_qs[i].index = q_infos[i].queueFamilyIndex;
		}
		
		VkPhysicalDeviceFeatures features{};

		VkDeviceCreateInfo device_info = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.queueCreateInfoCount = static_cast<u32>(q_infos.size()),
			.pQueueCreateInfos = q_infos.data(),
			.pEnabledFeatures = &features,
		};

		VkDevice device = VK_NULL_HANDLE;
		VkResult res = vkCreateDevice(handle_, &device_info, nullptr, &device);

		if (res == VK_SUCCESS) {
			return DeviceOwner{ device, handle_, req_qs };
		}

		return eh::Error{ convert_vk_result(res) };
	}

	std::array<usize, 3> DeviceView::queue_indices{};

	void DeviceOwner::init_queues_(std::span<QueueInfo> req_qs) noexcept {
		auto fill_vector = [device = this->handle_](auto& v, usize count, usize index) noexcept {
			for (usize i : std::views::iota(0u, count)) {
				VkQueue q = VK_NULL_HANDLE;
				vkGetDeviceQueue(device, static_cast<u32>(index), static_cast<u32>(i), &q);
				v.emplace_back(q, index);
			}
		};

		for (const auto& info : req_qs) {
			switch (info.type)
			{
			case QueueTypes::eGraphics:
				fill_vector(graphics_qs_, info.count, info.index);
				break;

			case QueueTypes::eCompute:
				fill_vector(compute_qs_, info.count, info.index);
				break;

			case QueueTypes::eTransfer:
				fill_vector(transfer_qs_, info.count, info.index);
				break;
			}
		}
	}
}
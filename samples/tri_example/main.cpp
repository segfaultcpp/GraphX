#include <instance.hpp>
#include <device.hpp>
#include <cmd_exec.hpp>

#include <ranges>
#include <array>

#include <misc/types.hpp>

int main() {
	constexpr auto inst_desc = gx::InstanceDesc{}
		.set_app_desc("My App", gx::Version{ 0, 1, 0 })
		.set_engine_desc("My Engine", gx::Version{ 0, 1, 0 })
		.enable_layer<gx::ext::ValidationLayerKhr>();
	
	auto instance = gx::make_instance(inst_desc).unwrap();

	auto filtered_devices = instance.enum_phys_devices()
		| std::ranges::views::filter(gx::min_vram_size(gx::gb_to_bytes(1)))
		| std::ranges::views::filter(gx::request_graphics_queue())
		| std::ranges::views::filter(gx::request_transfer_queue())
		| std::ranges::views::filter(gx::request_discrete_gpu());

	EH_ASSERT(!filtered_devices.empty(), "Couldn't find suitable device");

	std::array requested_queues = {
		gx::QueueInfo {
			.type = gx::QueueTypes::eGraphics,
			.count = 1
		},
		gx::QueueInfo {
			.type = gx::QueueTypes::eTransfer,
			.count = 1
		},
	};

	gx::DeviceDesc device_desc = {
		.requested_queues = requested_queues
	};

	auto device = filtered_devices.begin()->get_logical_device(device_desc).unwrap();
	auto gq = device.pop_back_queue<gx::GraphicsQueue>();
	auto tq = device.pop_back_queue<gx::TransferQueue>();

	auto device_view = device.get_view();
	auto cmd_pool = device_view.create_command_pool<gx::GraphicsContext>().unwrap();

	auto ctx = cmd_pool.create_command_context();

	return 0;
}
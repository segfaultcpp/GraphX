#include <instance.hpp>
#include <device.hpp>
#include <types.hpp>
#include <cmd_exec.hpp>

#include <ranges>
#include <array>

int main() {
	std::array layers = {
		gx::LayerList::khr_validation,
	};
	// TODO: fix constexprness
	auto inst_desc = gx::InstanceDesc{}
		.set_app_desc("My App", gx::Version{ 0, 1, 0 })
		.set_engine_desc("My Engine", gx::Version{ 0, 1, 0 })
		.enable_layers(layers);

	auto instance = gx::make_instance<gx::SurfaceKhrExt<gx::SurfaceWin32KhrExt>>(inst_desc);
	
	auto filtered_devices = instance->enum_phys_devices()
		| rng::views::filter(gx::min_vram_size(gx::gb_to_bytes(1)))
		| rng::views::filter(gx::request_graphics_queue())
		| rng::views::filter(gx::request_transfer_queue())
		| rng::views::filter(gx::request_discrete_gpu());

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

	auto device = filtered_devices.begin()->get_logical_device(device_desc);
	auto gq = device->pop_back_queue<gx::GraphicsQueue>();
	auto tq = device->pop_back_queue<gx::TransferQueue>();

	return 0;
}
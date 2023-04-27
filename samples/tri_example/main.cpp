#include <instance.hpp>
#include <device.hpp>
#include <extensions.hpp>

#include <ranges>
#include <array>
#include <cassert>

#include <misc/types.hpp>

static void hack() noexcept {
	SetEnvironmentVariableA("DISABLE_LAYER_AMD_SWITCHABLE_GRAPHICS_1", "1");
	SetEnvironmentVariableA("DISABLE_LAYER_NV_OPTIMUS_1", "1");
}

int main() {
	hack();

	auto instance = gx::InstanceBuilder{}
		.with_app_info("app name", gx::Version(0, 1))
		.with_engine_info("No Engine", gx::Version(1, 0))
		.with_extensions<gx::ext::DebugUtilsExt>()
		.with_extensions<gx::ext::SurfaceExt, gx::ext::Win32SurfaceExt>()
		.with_layers<gx::ext::ValidationLayer>()
		.build().value();

	auto surface = instance.get_ext_win32_surface_builder()
		.build().value();

	auto phys_devices = instance.enum_phys_devices();
	assert(!phys_devices.empty());
	
	auto suited_devices = phys_devices | gx::request_discrete_gpu() | gx::request_presentation_support(surface.get_view());
	assert(!suited_devices.empty());

	auto phys_device = *suited_devices.begin();
	auto sc_support = phys_device.query_swapchain_support(surface.get_view());

	auto device = phys_device.get_device_builder()
		.with_extensions<gx::ext::SwapchainExt>()
		.build().value();
	

	return 0;
}
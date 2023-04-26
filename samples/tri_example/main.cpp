#include <instance.hpp>
#include <device.hpp>
#include <extensions.hpp>

#include <ranges>
#include <array>
#include <cassert>

#include <misc/types.hpp>

int main() {
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

	auto device = phys_device.get_device_builder().build().value();
	

	return 0;
}
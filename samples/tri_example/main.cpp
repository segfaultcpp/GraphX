#include <instance.hpp>
#include <device.hpp>

#include <ranges>
#include <array>

#include <misc/types.hpp>

int main() {
	auto instance = gx::InstanceBuilder{}
		.with_app_info("app name", gx::Version(0, 1))
		.with_engine_info("Ray Engine", gx::Version(1, 0))
		.with_extension<gx::ext::DebugUtilsExt>()
		.with_extension<gx::ext::SurfaceExt>()
		.with_extension<gx::ext::Win32SurfaceExt>()
		.with_layer<gx::ext::ValidationLayer>()
		.build().value();

	auto phys_devices = instance.enum_phys_devices();

	auto surface = instance.get_ext_win32_surface_builder()
		.build().value();

	return 0;
}
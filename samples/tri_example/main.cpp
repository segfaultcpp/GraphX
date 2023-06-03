#include <instance.hpp>
#include <device.hpp>
#include <extensions.hpp>
#include <image.hpp>

#include <ranges>
#include <array>
#include <cassert>
#include <iostream>

#include <misc/types.hpp>

static void hack() noexcept {
	SetEnvironmentVariableA("DISABLE_LAYER_AMD_SWITCHABLE_GRAPHICS_1", "1");
	SetEnvironmentVariableA("DISABLE_LAYER_NV_OPTIMUS_1", "1");
}

class TriangleExample {
private:
	using InstExts = meta::List<gx::ext::DebugUtilsExt, gx::ext::SurfaceExt, gx::ext::Win32SurfaceExt>;
	using Layers = meta::List<gx::ext::ValidationLayer>;
	using DeviceExts = meta::List<gx::ext::SwapchainExt>;

	static constexpr std::string_view kAppName = "Triangle Example";
	static constexpr u32 kWidth = 1280;
	static constexpr u32 kHeight = 720;
	static constexpr u32 kImageCount = 3;

	gx::Instance<InstExts, Layers> instance_;
	gx::Device<DeviceExts> device_;

	gx::ext::Surface surface_;
	gx::ext::Swapchain swap_chain_;

	std::vector<gx::ImageView> swap_chain_images_;
	std::array<gx::ImageRef, 3> swap_chain_image_refs_;

	HWND window_ = nullptr;

public:
	TriangleExample() noexcept = default;

	TriangleExample(TriangleExample&&) = delete;
	TriangleExample& operator=(TriangleExample&&) = delete;

	~TriangleExample() noexcept {
		destroy_();
	}

	[[nodiscard]]
	std::expected<void, gx::ErrorCode> setup() noexcept {
		create_window_();

		auto inst_res = gx::InstanceBuilder{}
			.with_app_info(kAppName, gx::Version(0, 1, 0))
			.with_extensions(InstExts{})
			.with_layers(Layers{})
			.build();

		if (!inst_res.has_value()) {
			return std::unexpected(inst_res.error());
		}
		instance_ = std::move(inst_res).value();

		auto surf_res = instance_.get_ext_win32_surface_builder()
			.with_app_info(window_, GetModuleHandleA(0))
			.build();

		if (!surf_res.has_value()) {
			return std::unexpected(surf_res.error());
		}
		surface_ = std::move(surf_res).value();

		auto phys_devices = instance_.enum_phys_devices();
		assert(!phys_devices.empty());

		auto suited_devices = phys_devices | gx::request_discrete_gpu() | gx::request_presentation_support(surface_.get_view());
		assert(suited_devices.begin() != suited_devices.end());
		auto phys_device = *suited_devices.begin();

		auto device_res = phys_device.get_device_builder()
			.request_graphics_queues()
			.with_extensions(DeviceExts{})
			.build();

		if (!device_res.has_value()) {
			return std::unexpected(device_res.error());
		}
		device_ = std::move(device_res).value();

		auto sc_support = phys_device.query_swapchain_support(surface_.get_view());
		
		auto sc_res = device_.get_ext_swapchain_builder(surface_.get_view())
			.with_image_sizes(sc_support.caps.current_extent, kImageCount)
			.with_present_mode(gx::ext::PresentMode::eMailbox)
			.with_image_format(gx::Format::eBGRA8_SRGB)
			.set_clipped(true)
			.build();

		if (!sc_res.has_value()) {
			return std::unexpected(sc_res.error());
		}
		swap_chain_ = std::move(sc_res).value();
		swap_chain_images_ = gx::get_images_from_swapchain(swap_chain_.get_view());
		
		for (auto&& [image_ref, image] : std::views::zip(swap_chain_image_refs_, swap_chain_images_)) {
			auto ir_res = gx::ImageRefBuilder(image, device_.get_view())
				.with_format(gx::Format::eBGRA8_SRGB)
				.build();

			if (!ir_res.has_value()) {
				return std::unexpected(ir_res.error());
			}
			image_ref = std::move(ir_res).value();
		}
		return {};
	}

	static LRESULT __stdcall wnd_proc_(HWND window, UINT msg, WPARAM wp, LPARAM lp) noexcept {
		if (msg == WM_CLOSE) {
			*std::bit_cast<bool*>(GetWindowLongPtrA(window, GWLP_USERDATA)) = true;
		}
		return DefWindowProcA(window, msg, wp, lp);
	}

	void run() noexcept {
		bool closed = false;
		SetWindowLongPtrA(window_, GWLP_USERDATA, std::bit_cast<LONG_PTR>(&closed));

		while (!closed) {
			MSG msg{};
			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

private:
	void create_window_() noexcept {
		WNDCLASSEXA wc;
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = wnd_proc_;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = GetModuleHandle(nullptr);
		wc.hIcon = LoadIcon(GetModuleHandle(nullptr), IDI_APPLICATION);
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = "TRIANGLE_EXAMPLE";
		wc.hIconSm = LoadIcon(GetModuleHandle(nullptr), IDI_APPLICATION);

		RegisterClassExA(&wc);

		window_ = CreateWindowA("TRIANGLE_EXAMPLE", kAppName.data(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, kWidth, kHeight, 0, 0, GetModuleHandleA(0), 0);
		assert(window_ != nullptr);

		ShowWindow(window_, SW_SHOWDEFAULT);
		UpdateWindow(window_);
	}

	void destroy_() noexcept {
		DestroyWindow(window_);

		for (auto& image_ref : swap_chain_image_refs_) {
			image_ref.destroy();
		}
	}
};

int main() {
	hack();
	TriangleExample example{};

	auto err_code = example.setup();
	if (!err_code.has_value()) {}
	example.run();
	return 0;
}
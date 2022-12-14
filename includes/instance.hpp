#pragma once
#include "types.hpp"
#include "utils.hpp"

#include <string_view>
#include <optional>
#include <vector>
#include <span>
#include <array>
#include <ranges>
#include <iterator>

#include <vulkan/vulkan.h>

namespace gx {
	struct Version {
		u32 major = 0;
		u32 minor = 0;
		u32 patch = 0;

		constexpr Version() noexcept = default;

		constexpr Version(u32 major_v, u32 minor_v, u32 patch_v) noexcept
			: major{ major_v }
			, minor{ minor_v }
			, patch{ patch_v }
		{}
	};

	struct ExtensionList {
		static constexpr const char* khr_surface = "VK_KHR_surface";
		static constexpr const char* khr_win32_surface = "VK_KHR_win32_surface";
	};

	struct LayerList {
		static constexpr const char* khr_validation = "VK_LAYER_KHRONOS_validation";
	};

	struct InstanceDesc {
		std::string_view app_name = "default_graphx_app";
		Version app_version = Version{ 1,0,0 };
		std::string_view engine_name = "default_graphx_engine";
		Version engine_version = Version{ 1, 0, 0 };
		std::span<const char*> extensions;
		std::span<const char*> layers;

		constexpr InstanceDesc() noexcept = default;

		constexpr InstanceDesc(std::string_view app, Version app_v, std::string_view engine, Version engine_v, std::span<const char*> exts, std::span<const char*> l) noexcept
			: app_name{ app }
			, app_version{ app_v }
			, engine_name{ engine }
			, engine_version{ engine_v }
			, extensions{ exts }
			, layers( l )
		{}

		[[nodiscard]]
		constexpr InstanceDesc set_app_desc(std::string_view name, Version version) const noexcept {
			return InstanceDesc{ name, version, engine_name, engine_version, extensions, layers };
		}

		[[nodiscard]]
		constexpr InstanceDesc set_engine_desc(std::string_view name, Version version) const noexcept {
			return InstanceDesc{ app_name, app_version, name, version, extensions, layers };
		}

		[[nodiscard]]
		constexpr InstanceDesc enable_extensions(std::span<const char*> exts) const noexcept {
			return InstanceDesc{ app_name, app_version, engine_name, engine_version, exts, layers };
		}

		[[nodiscard]]
		constexpr InstanceDesc enable_layers(std::span<const char*> l) const noexcept {
			return InstanceDesc{ app_name, app_version, engine_name, engine_version, extensions, l };
		}
	};

	struct InstanceInfo {
		InstanceInfo() noexcept;
		std::vector<VkExtensionProperties> supported_extensions;
		std::vector<VkLayerProperties> supported_layers;
	};

	template<typename T>
	inline constexpr bool is_extension_v = false;

	template<typename T>
	concept Extension = is_extension_v<T> && rng::sized_range<decltype(T::ext_names)>;
	
	template<typename T>
	concept ExtImpl = is_extension_v<T> && requires {
		T::ext_impl_name;
	};

	template<typename T>
	concept SurfaceImpl = ExtImpl<T> && requires {
		{ T::create_surface(std::declval<typename T::WindowHandle_t>(), std::declval<typename T::AppInstance_t>()) } -> std::same_as<VkSurfaceKHR>;
	};

	template<SurfaceImpl T>
	struct SurfaceKhrExt {
		constexpr static std::array<const char*, 2> ext_names = { ExtensionList::khr_surface, T::ext_impl_name };
	
		SurfaceKhrExt() noexcept = default;
		
		SurfaceKhrExt(SurfaceKhrExt&& rhs) noexcept
			: surface_{ rhs.surface_ }
		{
			rhs.surface_ = VK_NULL_HANDLE;
		}

		SurfaceKhrExt& operator=(SurfaceKhrExt&& rhs) noexcept {
			surface_ = rhs.surface_;
			rhs.surface_ = VK_NULL_HANDLE;

			return *this;
		}

		SurfaceKhrExt(const SurfaceKhrExt&) = delete;
		SurfaceKhrExt& operator=(const SurfaceKhrExt&) = delete;

		[[nodiscard]]
		VkSurfaceKHR get_surface(typename T::WindowHandle_t window, typename T::AppInstance_t app) noexcept;

	protected:
		VkSurfaceKHR surface_ = VK_NULL_HANDLE;
	};

#ifdef GX_WIN64
	#include <Windows.h>
	#include <vulkan/vulkan_win32.h>

	struct SurfaceWin32KhrExt {
		constexpr static const char* ext_impl_name = ExtensionList::khr_win32_surface;
		using WindowHandle_t = HWND;
		using AppInstance_t = HINSTANCE;

		[[nodiscard]]
		static VkSurfaceKHR create_surface(WindowHandle_t window, AppInstance_t app) noexcept {}
	};

	template<>
	inline constexpr bool is_extension_v<SurfaceWin32KhrExt> = true;

	static_assert(SurfaceImpl<SurfaceWin32KhrExt>);
#endif // GX_WIN64

	template<SurfaceImpl T>
	inline constexpr bool is_extension_v<SurfaceKhrExt<T>> = true;

	static_assert(Extension<SurfaceKhrExt<SurfaceWin32KhrExt>>);

	inline void destroy_instance(VkInstance instance) noexcept {
		if (instance != VK_NULL_HANDLE) {
			// TODO: global allocator for allocating and deallocating vk objects.
			vkDestroyInstance(instance, nullptr);
		}
	}

	class PhysicalDevice;

	template<Extension... Exts>
	class Instance : public Exts... {
		VkInstance instance_ = VK_NULL_HANDLE;
		InstanceInfo info_;
		std::vector<PhysicalDevice> devices_;

	public:
		Instance() noexcept = default;
		Instance(VkInstance inst, InstanceInfo&& info) noexcept
			: instance_{ inst }
			, info_{ std::move(info) }
		{}

		Instance(const Instance&) = delete;
		Instance& operator=(const Instance&) = delete;

		Instance(Instance&& rhs) noexcept  
			: instance_{ rhs.instance_ }
			, info_{ std::move(rhs.info_) }
		{
			rhs.instance_ = VK_NULL_HANDLE;
		}

		Instance& operator=(Instance&& rhs) noexcept {
			instance_ = rhs.instance_;
			rhs.instance_ = VK_NULL_HANDLE;
			
			info_ = std::move(rhs.info_);

			return *this;
		}

		~Instance() noexcept {
			destroy_instance(instance_);
		}

	public:
		/*
		* WARNING: Unsafe
		* Moves out ownership to caller. The caller must manage this instance themselves.
		*/
		[[nodiscard]]
		VkInstance unwrap_native_handle() noexcept {
			VkInstance ret = instance_;
			instance_ = VK_NULL_HANDLE;
			return ret;
		}

		[[nodiscard]]
		std::span<PhysicalDevice> enum_phys_devices() noexcept;

	};

	template<std::ranges::random_access_range Rng>
	[[nodiscard]]
	std::vector<usize> check_supported_extensions(Rng&& requested, InstanceInfo& info) noexcept {
		return check_support(requested, info.supported_extensions, &VkExtensionProperties::extensionName,
			[](const char* ext1, const char* ext2) noexcept {
				return std::strcmp(ext1, ext2) == 0;
			});
	}

	template<std::ranges::random_access_range Rng>
	[[nodiscard]]
	std::vector<usize> check_supported_layers(Rng&& requested, InstanceInfo& info) noexcept {
		return check_support(requested, info.supported_layers, &VkLayerProperties::layerName,
			[](const char* layer1, const char* layer2) noexcept {
				return std::strcmp(layer1, layer2) == 0;
			});
	}

	template<SurfaceImpl T>
	[[nodiscard]]
	VkSurfaceKHR SurfaceKhrExt<T>::get_surface(typename T::WindowHandle_t window, typename T::AppInstance_t app) noexcept {
		if (surface_ == VK_NULL_HANDLE) {
			surface_ = T::create_surface(window, app);
		}
		return surface_;
	}

	template<Extension... Exts>
	[[nodiscard]]
	std::span<PhysicalDevice> Instance<Exts...>::enum_phys_devices() noexcept {
		if (devices_.size() != 0) {
			return devices_;
		}

		u32 device_count = 0;
		vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);
		std::vector<VkPhysicalDevice> phys_devices(device_count);
		vkEnumeratePhysicalDevices(instance_, &device_count, phys_devices.data());

		devices_.resize(device_count);
		rng::copy(phys_devices, devices_.begin());

		return devices_;
	}

	template<Extension... Exts>
	[[nodiscard]]
	std::optional<Instance<Exts...>> make_instance(const InstanceDesc& desc) noexcept {
		InstanceInfo info;
		std::vector<usize> idxs;

		constexpr std::array required_exts = [](auto&&... args) {
			constexpr usize size = (0 + ... + std::size(Exts::ext_names));
			std::array<const char*, size> ret = {}; usize i = 0;
			((rng::copy(std::begin(args) + i, std::begin(args) + std::size(args), ret.begin()), ++i), ...);
			return ret;
		}(Exts::ext_names...);

		idxs = check_supported_extensions(required_exts, info);

		if (idxs.size() != 0) {
			// TODO: error message
			return std::nullopt;
		}

		idxs = check_supported_layers(desc.layers, info);

		if (idxs.size() != 0) {
			// TODO: error message
			return std::nullopt;
		}

		VkApplicationInfo app_info{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = desc.app_name.data(),
			.applicationVersion = VK_MAKE_VERSION(desc.app_version.major, desc.app_version.minor, desc.app_version.patch),
			.pEngineName = desc.engine_name.data(),
			.engineVersion = VK_MAKE_VERSION(desc.engine_version.major, desc.engine_version.minor, desc.engine_version.patch),
			.apiVersion = VK_API_VERSION_1_3,
		};

		VkInstanceCreateInfo inst_create_info{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &app_info,
			.enabledLayerCount = static_cast<u32>(desc.layers.size()),
			.ppEnabledLayerNames = desc.layers.data(),
			.enabledExtensionCount = static_cast<u32>(required_exts.size()),
			.ppEnabledExtensionNames = required_exts.data(),
		};

		VkInstance instance = VK_NULL_HANDLE;
		VkResult res = vkCreateInstance(&inst_create_info, nullptr, &instance);

		if (res == VK_SUCCESS) {
			return Instance<Exts...>{ instance, std::move(info) };
		}
		
		return std::nullopt;
	}
}
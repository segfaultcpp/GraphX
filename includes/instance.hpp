#pragma once
#include "utils.hpp"
#include "error.hpp"
#include "types.hpp"
#include "device.hpp"
#include "extensions.hpp"

#include <string_view>
#include <vector>
#include <span>
#include <array>
#include <ranges>
#include <iterator>
#include <format>
#include <expected>
#include <optional>

#include <vulkan/vulkan.h>

#include <misc/assert.hpp>
#include <misc/types.hpp>

namespace gx {
	struct Version {
		u32 major = 0;
		u32 minor = 0;
		u32 patch = 0;

		constexpr Version() noexcept = default;

		constexpr Version(u32 maj, u32 min, u32 pat = 0) noexcept 
			: major{ maj }
			, minor{ min }
			, patch{ pat }
		{}

		static constexpr Version get_graphx_version() noexcept {
			return Version{ 0, 1 };
		}

		static constexpr Version get_target_vulkan_version() noexcept {
			return Version{ 1, 3 };
		}
	};

	struct InstanceInfo {
		InstanceInfo() noexcept;

		std::vector<VkExtensionProperties> supported_extensions;
		std::vector<VkLayerProperties> supported_layers;

		[[nodiscard]]
		static const InstanceInfo& get() noexcept {
			static InstanceInfo info{};
			return info;
		}
	};

	struct InstanceValue {
		VkInstance handle = VK_NULL_HANDLE;

		InstanceValue() noexcept = default;

		InstanceValue(VkInstance instance) noexcept
			: handle{ instance }
		{}

		void destroy() noexcept {
			vkDestroyInstance(handle, nullptr);
		}
	};
	static_assert(Value<InstanceValue>);

	template<typename E = meta::List<>, typename L = meta::List<>>
	struct InstanceImpl;

	template<typename... Es, typename... Ls>
	struct InstanceImpl<meta::List<Es...>, meta::List<Ls...>> {
		template<typename Self> 
			requires meta::SameAsAny<ext::DebugUtilsExt, Es...> && meta::SameAsAny<ext::ValidationLayer, Ls...>
		[[nodiscard]]
		ext::DebugUtilsBuilder get_ext_debug_utils_builder(this Self&& self) noexcept {
			return ext::DebugUtilsBuilder{}.with_instance(self.get_handle());
		}

#ifdef GX_WIN64
		template<typename Self>
			requires meta::SameAsAny<ext::SurfaceExt, Es...> && meta::SameAsAny<ext::Win32SurfaceExt, Es...>
		[[nodiscard]]
		auto get_ext_win32_surface_builder(this Self&& self) noexcept {
			return ext::Win32SurfaceBuilder{}.with_instance(self.get_handle());
		}
#endif

		template<typename Self>
		[[nodiscard]]
		auto enum_phys_devices(this Self&& self) noexcept {
			u32 count = 0;
			vkEnumeratePhysicalDevices(self.get_handle(), &count, nullptr);
			std::vector<VkPhysicalDevice> devices(count);
			vkEnumeratePhysicalDevices(self.get_handle(), &count, devices.data());
			
			return devices | std::views::transform(
				[](VkPhysicalDevice device) {
					return PhysDevice{ device };
				}) | 
				std::ranges::to<std::vector>();
		}
	};

	template<typename E, typename L>
	using Instance = OwnedType<InstanceValue, InstanceImpl<E, L>, MoveOnlyTag>;

	template<typename E = meta::List<>, typename L = meta::List<>>
	struct InstanceBuilder;

	template<ext::InstanceExt... Es, typename... Ls>
	struct InstanceBuilder<meta::List<Es...>, meta::List<Ls...>> {
		std::string_view app_name = "unknown";
		Version app_version = Version::get_graphx_version();
		std::string_view engine_name = "unknown";
		Version engine_version = Version::get_graphx_version();
		Version vulkan_version = Version::get_target_vulkan_version();

		constexpr InstanceBuilder() noexcept = default;

		constexpr InstanceBuilder(std::string_view app_n, Version app_v, std::string_view engine_n, Version engine_v, Version vulkan_v) noexcept 
			: app_name{ app_n }
			, app_version{ app_v }
			, engine_name{ engine_n }
			, engine_version{ engine_v }
			, vulkan_version{ vulkan_v }
		{}

		[[nodiscard]]
		constexpr InstanceBuilder with_app_info(std::string_view name, Version version) const noexcept {
			return InstanceBuilder{ name, version, engine_name, engine_version, vulkan_version };
		}

		[[nodiscard]]
		constexpr InstanceBuilder with_engine_info(std::string_view name, Version version) const noexcept {
			return InstanceBuilder{ app_name, app_version, name, version, vulkan_version };
		}

		[[nodiscard]]
		constexpr InstanceBuilder with_vulkan_version(Version version) const noexcept {
			return InstanceBuilder{ app_name, app_version, engine_name, engine_version, version };
		}

		template<ext::InstanceExt... Es1>
		[[nodiscard]]
		constexpr auto with_extensions() const noexcept {
			return InstanceBuilder<meta::List<Es1..., Es...>, meta::List<Ls...>>{ app_name, app_version, engine_name, engine_version, vulkan_version };
		}

		template<ext::InstanceExt... Es1>
		[[nodiscard]]
		constexpr auto with_extensions(meta::List<Es1...>) const noexcept {
			return with_extensions<Es1...>();
		}

		template<typename... Ls1>
		[[nodiscard]]
		constexpr auto with_layers() const noexcept {
			return InstanceBuilder<meta::List<Es...>, meta::List<Ls1..., Ls...>>{ app_name, app_version, engine_name, engine_version, vulkan_version };
		}

		template<typename... Ls1>
		[[nodiscard]]
		constexpr auto with_layers(meta::List<Ls1...>) const noexcept {
			return with_layers<Ls1...>();
		}
		
		[[nodiscard]]
		auto build() const noexcept -> std::expected<Instance<meta::List<Es...>, meta::List<Ls...>>, ErrorCode> {
			InstanceInfo info{};
			
			VkApplicationInfo app_info{
				.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
				.pApplicationName = app_name.data(),
				.applicationVersion = VK_MAKE_VERSION(app_version.major, app_version.minor, app_version.patch),
				.pEngineName = engine_name.data(),
				.engineVersion = VK_MAKE_VERSION(engine_version.major, engine_version.minor, engine_version.patch),
				.apiVersion = VK_MAKE_API_VERSION(0, vulkan_version.major, vulkan_version.minor, 0),
			};

			VkInstanceCreateInfo inst_create_info{
				.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
				.pApplicationInfo = &app_info,
			};

			if constexpr (sizeof...(Es) > 0) {
				static constexpr std::array kRequiredExts = ext::to_array<Es...>();
			
				inst_create_info.enabledExtensionCount = static_cast<u32>(kRequiredExts.size());
				inst_create_info.ppEnabledExtensionNames = kRequiredExts.data();
			}

			if constexpr (sizeof...(Ls) > 0) {
				static constexpr std::array required_layers = {
					Ls::name...
				};

				inst_create_info.enabledLayerCount = static_cast<u32>(required_layers.size());
				inst_create_info.ppEnabledLayerNames = required_layers.data();
			}

			InstanceValue value{};
			VkResult res = vkCreateInstance(&inst_create_info, nullptr, &value.handle);

			if (res == VK_SUCCESS) {
				(Es::load(value.handle), ...);
				return Instance<meta::List<Es...>, meta::List<Ls...>>{ value };
			}

			return std::unexpected(convert_vk_result(res));
		}
	};
}
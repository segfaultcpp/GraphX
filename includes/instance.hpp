#pragma once
#include "utils.hpp"
#include "error.hpp"
#include "object.hpp"

#include <string_view>
#include <vector>
#include <span>
#include <array>
#include <ranges>
#include <iterator>
#include <format>

#include <vulkan/vulkan.h>

#include <misc/assert.hpp>
#include <misc/types.hpp>

namespace gx {
	namespace ext {
		struct ExtensionList {
			static constexpr const char* khr_surface = "VK_KHR_surface";
			static constexpr const char* khr_win32_surface = "VK_KHR_win32_surface";
			static constexpr const char* ext_debug_utils = "VK_EXT_debug_utils";
			static constexpr const char* khr_swapchain = "VK_KHR_swapchain";
		};

		struct LayerList {
			static constexpr const char* khr_validation = "VK_LAYER_KHRONOS_validation";
		};

		struct ExtensionTag {};
		struct LayerTag {};

		template<typename T>
		concept ExtensionDef = std::derived_from<T, ExtensionTag> && std::ranges::sized_range<decltype(T::ext_names)> &&
			requires(VkInstance instance)
		{
			T::load(instance);
		};

		template<typename T>
		concept Layer = std::derived_from<T, LayerTag> && requires(const char* layer_name) {
			layer_name = T::layer_name;
		};

		using NoReqs = int;

		template<ExtensionDef D, typename RE = NoReqs, typename RL = NoReqs>
		struct RegisterExtension;

		template<ExtensionDef D, ExtensionDef... REs, Layer... RLs>
		struct RegisterExtension<D, meta::List<REs...>, meta::List<RLs...>> {
			using Definition = D;
			using RequiredExtensions = meta::List<REs...>;
			using RequiredLayers = meta::List<RLs...>;
		};

		template<ExtensionDef D, ExtensionDef... REs>
		struct RegisterExtension<D, meta::List<REs...>> {
			using Definition = D;
			using RequiredExtensions = meta::List<REs...>;
			using RequiredLayers = NoReqs;
		};

		template<ExtensionDef D, Layer... RLs>
		struct RegisterExtension<D, meta::List<RLs...>> {
			using Definition = D;
			using RequiredExtensions = NoReqs;
			using RequiredLayers = meta::List<RLs...>;
		};

		template<ExtensionDef D>
		struct RegisterExtension<D> {
			using Definition = D;
			using RequiredExtensions = NoReqs;
			using RequiredLayers = NoReqs;
		};

		template<typename T>
		concept RegisteredExtension = requires {
			typename T::Definition;
			typename T::RequiredExtensions;
			typename T::RequiredLayers;
		};

		template<ExtensionDef... Exts, ExtensionDef... Exts1>
		constexpr void check_requirements(meta::List<Exts...>, meta::List<Exts1...>) noexcept {
			static_assert((meta::SameAsAny<Exts, Exts1...> && ...));
		}

		template<Layer... Lyrs, Layer... Lyrs1>
		constexpr void check_requirements(meta::List<Lyrs...>, meta::List<Lyrs1...>) noexcept {
			static_assert((meta::SameAsAny<Lyrs, Lyrs1...> && ...));
		}

		template<RegisteredExtension E, ExtensionDef... Exts1, Layer... Lyrs1>
		constexpr void check_requirements(E, meta::List<Exts1...> es, meta::List<Lyrs1...> ls) noexcept {
			if constexpr (!std::same_as<typename E::RequiredExtensions, NoReqs>) {
				check_requirements(typename E::RequiredExtensions{}, es);
			}
			if constexpr (!std::same_as<typename E::RequiredLayers, NoReqs>) {
				check_requirements(typename E::RequiredLayers{}, ls);
			}
		}

		struct ValidationLayerKhr : LayerTag {
			static constexpr const char* layer_name = LayerList::khr_validation;
		};
		static_assert(Layer<ValidationLayerKhr>);

		namespace details {
			template<typename Fn>
			struct MakeCreateExtFn {
				MakeCreateExtFn() noexcept = default;
				MakeCreateExtFn(VkInstance instance, Fn create) noexcept
					: parent_{ instance }
					, create_{ create }
				{}

				[[nodiscard]]
				decltype(auto) operator()(auto&&... args) const noexcept {
					return create_(parent_, std::forward<decltype(args)>(args)...);
				}

			private:
				VkInstance parent_ = VK_NULL_HANDLE;
				Fn create_{};
			};

			template<typename Fn>
			struct MakeDestroyExtFn {
				MakeDestroyExtFn() noexcept = default;
				MakeDestroyExtFn(VkInstance instance, Fn destroy) noexcept
					: parent_{ instance }
					, destroy_{ destroy }
				{}

				void operator()(auto& obj) const noexcept {
					return destroy_(parent_, obj, nullptr);
				}

			private:
				VkInstance parent_ = VK_NULL_HANDLE;
				Fn destroy_{};
			};
		}
	}

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

	template<typename Extensions = meta::List<>, typename Layers = meta::List<>>
	struct InstanceDesc;

	template<template<ext::RegisteredExtension...> typename L1, template<ext::Layer...> typename L2, ext::RegisteredExtension... Exts, ext::Layer... Lyrs>
	struct InstanceDesc<L1<Exts...>, L2<Lyrs...>> {
		std::string_view app_name = "default_graphx_app";
		Version app_version = Version{ 1,0,0 };
		std::string_view engine_name = "default_graphx_engine";
		Version engine_version = Version{ 1, 0, 0 };

		constexpr InstanceDesc() noexcept = default;

		constexpr InstanceDesc(std::string_view app, Version app_v, std::string_view engine, Version engine_v) noexcept
			: app_name{ app }
			, app_version{ app_v }
			, engine_name{ engine }
			, engine_version{ engine_v }
		{}

		[[nodiscard]]
		constexpr InstanceDesc set_app_desc(std::string_view name, Version version) const noexcept {
			return InstanceDesc{ name, version, engine_name, engine_version };
		}

		[[nodiscard]]
		constexpr InstanceDesc set_engine_desc(std::string_view name, Version version) const noexcept {
			return InstanceDesc{ app_name, app_version, name, version };
		}

		template<ext::RegisteredExtension E>
		[[nodiscard]]
		constexpr auto enable_extension() const noexcept {
			return InstanceDesc<L1<E, Exts...>, L2<Lyrs...>>{ app_name, app_version, engine_name, engine_version };
		}

		template<ext::Layer L>
		[[nodiscard]]
		constexpr auto enable_layer() const noexcept {
			return InstanceDesc<L1<Exts...>, L2<L, Lyrs...>>{ app_name, app_version, engine_name, engine_version };
		}
	};

	struct InstanceInfo {
		InstanceInfo() noexcept;
		std::vector<VkExtensionProperties> supported_extensions;
		std::vector<VkLayerProperties> supported_layers;
	};

	class PhysicalDevice;
	namespace ext {
		class DebugUtils;
	}

	template<typename L1, typename L2>
	class Instance;

	template<ext::ExtensionDef... Exts, ext::Layer... Lyrs>
	class Instance<meta::List<Exts...>, meta::List<Lyrs...>> : public ObjectOwner<Instance<meta::List<Exts...>, meta::List<Lyrs...>>, VkInstance> {
	private:
		// Friends
		friend struct unsafe::ObjectOwnerTrait<Instance<meta::List<Exts...>, meta::List<Lyrs...>>>;

		// Fields
		std::vector<PhysicalDevice> devices_;

	public:
		// Type aliases
		using Base = ObjectOwner<Instance<meta::List<Exts...>, meta::List<Lyrs...>>, VkInstance>;
		using ObjectType = VkInstance;

		// Constructors/destructor
		Instance() noexcept = default;
		Instance(VkInstance inst) noexcept
			: Base{ inst }
		{}

		Instance(Instance&& rhs) noexcept = default;
		Instance(const Instance&) = delete;
		
		~Instance() noexcept = default;

		// Assignment operators
		Instance& operator=(Instance&& rhs) noexcept = default;
		Instance& operator=(const Instance&) = delete;

		// Methods
		[[nodiscard]]
		std::span<PhysicalDevice> enum_phys_devices() noexcept;

	};

	namespace unsafe {
		template<ext::ExtensionDef... Exts, ext::Layer... Lyrs>
		struct ObjectOwnerTrait<Instance<meta::List<Exts...>, meta::List<Lyrs...>>> {
			static void destroy(Instance<meta::List<Exts...>, meta::List<Lyrs...>>& obj) noexcept {
				vkDestroyInstance(obj.handle_, nullptr);
			}

			[[nodiscard]]
			static VkInstance unwrap_native_handle(Instance<meta::List<Exts...>, meta::List<Lyrs...>>& obj) noexcept {
				auto ret = obj.handle_;
				obj.handle_ = VK_NULL_HANDLE;
				return ret;
			}
		};
	}

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

	template<ext::ExtensionDef... Exts, ext::Layer... Lyrs>
	[[nodiscard]]
	std::span<PhysicalDevice> Instance<meta::List<Exts...>, meta::List<Lyrs...>>::enum_phys_devices() noexcept {
		if (devices_.size() != 0) {
			return devices_;
		}

		EH_ASSERT(this->handle_ != VK_NULL_HANDLE, "VkInstance must be a valid handle");

		u32 device_count = 0;
		vkEnumeratePhysicalDevices(this->handle_, &device_count, nullptr);
		EH_ASSERT(device_count > 0u, "There is no supported devices");
		std::vector<VkPhysicalDevice> phys_devices(device_count);
		vkEnumeratePhysicalDevices(this->handle_, &device_count, phys_devices.data());

		devices_.resize(device_count);
		std::ranges::copy(phys_devices, devices_.begin());

		return devices_;
	}

	template<ext::RegisteredExtension... Exts, ext::Layer... Lyrs>
	[[nodiscard]]
	auto make_instance(const InstanceDesc<meta::List<Exts...>, meta::List<Lyrs...>>& desc) noexcept
		-> Result<Instance<meta::List<typename Exts::Definition...>, meta::List<Lyrs...>>> 
	{
		(ext::check_requirements(Exts{}, meta::List<typename Exts::Definition...>{}, meta::List<Lyrs...>{}), ...);
		
		InstanceInfo info;
		std::vector<usize> idxs;

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
		};

		if constexpr (sizeof...(Exts) > 0) {
			static constexpr std::array required_exts = [](auto&&... args) {
				constexpr usize size = (0 + ... + std::size(Exts::Definition::ext_names));
				std::array<const char*, size> ret = {}; usize i = 0;
				((std::ranges::copy(std::begin(args), std::end(args), ret.begin() + i), i += std::size(args)), ...);
				return ret;
			}(Exts::Definition::ext_names...);

			idxs = check_supported_extensions(required_exts, info);

			if (idxs.size() != 0) {
				std::string unsupported_exts;
				for (usize i : idxs) {
					unsupported_exts += std::format("{} ", required_exts[i]);
				}
				EH_ERROR_MSG(std::format("Some extensions are not supported ({})", unsupported_exts));
				return eh::Error{ ErrorCode::eExtensionNotPresent };
			}

			inst_create_info.enabledExtensionCount = static_cast<u32>(required_exts.size());
			inst_create_info.ppEnabledExtensionNames = required_exts.data();
		}

		if constexpr (sizeof...(Lyrs) > 0) {
			static constexpr std::array required_lyrs = {
				Lyrs::layer_name...
			};

			idxs = check_supported_layers(required_lyrs, info);

			if (idxs.size() != 0) {
				std::string unsupported_layers;
				for (usize i : idxs) {
					unsupported_layers += std::format("{} ", required_lyrs[i]);
				}
				EH_ERROR_MSG(std::format("Some layers are not supported ({})", unsupported_layers));
				return eh::Error{ ErrorCode::eLayerNotPresent };
			}

			inst_create_info.enabledLayerCount = static_cast<u32>(required_lyrs.size());
			inst_create_info.ppEnabledLayerNames = required_lyrs.data();
		}

		VkInstance instance = VK_NULL_HANDLE;
		VkResult res = vkCreateInstance(&inst_create_info, nullptr, &instance);

		if (res == VK_SUCCESS) {
			(Exts::Definition::load(instance), ...);
			return Instance<meta::List<typename Exts::Definition...>, meta::List<Lyrs...>>{ instance };
		}
		
		return eh::Error{ convert_vk_result(res) };
	}
}
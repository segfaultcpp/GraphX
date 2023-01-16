#pragma once
#include "utils.hpp"
#include "error.hpp"
#include "object.hpp"

#include <string_view>
#include <optional>
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
		static constexpr const char* ext_debug_utils = "VK_EXT_debug_utils";
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

	template<typename T>
	inline constexpr bool is_extension_v = false;

	template<typename T>
	concept Extension = is_extension_v<T> && std::ranges::sized_range<decltype(T::ext_names)> && std::constructible_from<T, VkInstance>;

	namespace details {
		template<Platform P>
		struct GetSurfaceExtImpl_;

		template<>
		struct GetSurfaceExtImpl_<Platform::eWindows> {
			static Result<VkSurfaceKHR> create_surface(VkInstance instance, HINSTANCE app, HWND window) noexcept;
		};

		template<>
		struct GetSurfaceExtImpl_<Platform::eLinux> {};
	}

	struct SurfaceKhrExt {
		// Constant expressions
		static constexpr std::array ext_names = { ExtensionList::khr_surface };
	
		// Constructors/destructor
		SurfaceKhrExt() noexcept = default;

		SurfaceKhrExt(VkInstance instance) noexcept 
			: instance_{ instance }
		{}

		SurfaceKhrExt(SurfaceKhrExt&&) noexcept = default;
		SurfaceKhrExt(const SurfaceKhrExt&) = delete;

		~SurfaceKhrExt() noexcept = default;

		// Assignment operators
		SurfaceKhrExt& operator=(SurfaceKhrExt&&) noexcept = default;
		SurfaceKhrExt& operator=(const SurfaceKhrExt&) = delete;

		// Methods
		[[nodiscard]]
		/*
		* TODO: For now gx::Instance object inheriting SurfaceKhrExt contains 2 handles of VkInstance.
		* Possible ways to fix it: 1) Use CRTP. 2) Use explicit object parameter (C++23).
		* First way will 
		*/
		Result<VkSurfaceKHR> get_surface(typename PlatformTraits<get_platform()>::WindowHandle window, typename PlatformTraits<get_platform()>::AppInstance app) noexcept {
			return details::GetSurfaceExtImpl_<get_platform()>::create_surface(instance_, app, window);
		}

	private:
		// Fields
		VkInstance instance_;
	};

	template<>
	inline constexpr bool is_extension_v<SurfaceKhrExt> = true;
	static_assert(Extension<SurfaceKhrExt>);

	enum class MessageSeverity : u8 {
		eDiagnostic = bit<u8, 0>(),
		eInfo = bit<u8, 1>(),
		eWarning = bit<u8, 2>(),
		eError = bit<u8, 3>(),
	};

	enum class MessageType : u8 {
		eGeneral = bit<u8, 0>(),
		eValidation = bit<u8, 1>(),
		ePerformance = bit<u8, 2>(),
	};

	OVERLOAD_BIT_OPS(MessageSeverity, u8);
	OVERLOAD_BIT_OPS(MessageType, u8);

	inline MessageSeverity message_severity_from_vk(VkDebugUtilsMessageSeverityFlagBitsEXT severity) noexcept {
		switch (severity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			return MessageSeverity::eDiagnostic;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			return MessageSeverity::eInfo;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			return MessageSeverity::eWarning;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			return MessageSeverity::eError;
		}
	}

	inline u8 message_type_from_vk(VkDebugUtilsMessageTypeFlagsEXT type) noexcept {
		u8 ret = 0;
		if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
			ret |= MessageType::eGeneral;
		}
		if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
			ret |= MessageType::eValidation;
		}
		if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
			ret |= MessageType::ePerformance;
		}
		return ret;
	}

	constexpr VkDebugUtilsMessageSeverityFlagsEXT message_severity_to_vk(u8 from) noexcept {
		VkDebugUtilsMessageSeverityFlagsEXT ret = 0;
		if (from & MessageSeverity::eDiagnostic) {
			ret |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
		}
		if (from & MessageSeverity::eInfo) {
			ret |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
		}
		if (from & MessageSeverity::eWarning) {
			ret |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
		}
		if (from & MessageSeverity::eError) {
			ret |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		}
		return ret;
	}

	constexpr VkDebugUtilsMessageTypeFlagsEXT message_type_to_vk(u8 from) noexcept {
		VkDebugUtilsMessageTypeFlagsEXT ret = 0;
		if (from & MessageType::eGeneral) {
			ret |= VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
		}
		if (from & MessageType::eValidation) {
			ret |= VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
		}
		if (from & MessageType::ePerformance) {
			ret |= VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		}
		return ret;
	}

	struct CallbackData {
		std::string_view message;
	};

	template<typename T>
	concept DebugUtilsExtCallable = std::invocable<T, MessageSeverity, u8, CallbackData>;

	template<u8 MsgSeverityFlags, u8 MsgTypeFlags, DebugUtilsExtCallable Fn>
	class DebugUtilsExt {
	private:
		// Static functions
		static VKAPI_ATTR VkBool32 VKAPI_CALL 
		debug_callback(
			VkDebugUtilsMessageSeverityFlagBitsEXT severity,
			VkDebugUtilsMessageTypeFlagsEXT type,
			const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
			void* pUserData) noexcept
		{
			CallbackData data = {
				.message = callback_data->pMessage
			};
			Fn{}(message_severity_from_vk(severity), message_type_from_vk(type), data);
			return VK_FALSE;
		}

	public:
		// Constant expressions
		static constexpr std::array ext_names = { ExtensionList::ext_debug_utils };

		// Constructors/destructor
		DebugUtilsExt() noexcept = default;

		DebugUtilsExt(VkInstance instance) noexcept {
			EH_ASSERT(instance != VK_NULL_HANDLE, "VkInstance handle must be a valid value");
			
			constexpr auto vk_msg_severity = message_severity_to_vk(MsgSeverityFlags);
			constexpr auto vk_msg_type = message_type_to_vk(MsgTypeFlags);

			VkDebugUtilsMessengerCreateInfoEXT create_info = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
				.messageSeverity = vk_msg_severity,
				.messageType = vk_msg_type,
				.pfnUserCallback = debug_callback,
			};

			auto create_debug_utils_messenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
			EH_ASSERT(create_debug_utils_messenger != nullptr, "Failed to get vkCreateDebugUtilsMessengerEXT function address");
			
			VkResult result = create_debug_utils_messenger(instance, &create_info, nullptr, &debug_messenger_);
			EH_ASSERT(result == VK_SUCCESS, "Failed to create VkDebugUtilsMessengerEXT handle");
		}

	protected:
		// Fields 
		VkDebugUtilsMessengerEXT debug_messenger_;
		
	};

	template<u8 S, u8 T, DebugUtilsExtCallable Fn>
	inline constexpr bool is_extension_v<DebugUtilsExt<S, T, Fn>> = true;

	struct InstanceInfo {
		InstanceInfo() noexcept;
		std::vector<VkExtensionProperties> supported_extensions;
		std::vector<VkLayerProperties> supported_layers;
	};

	class PhysicalDevice;

	template<Extension... Exts>
	class Instance : public ObjectOwner<Instance<Exts...>, VkInstance>, Exts... {
	private:
		// Friends
		friend struct unsafe::ObjectOwnerTrait<Instance<Exts...>>;

		// Fields
		std::vector<PhysicalDevice> devices_;

	public:
		// Type aliases
		using Base = ObjectOwner<Instance<Exts...>, VkInstance>;
		using ObjectType = VkInstance;

		// Constructors/destructor
		Instance() noexcept = default;
		Instance(VkInstance inst) noexcept
			: Base{ inst }
			, Exts{ inst }...
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
		template<Extension... Exts>
		struct ObjectOwnerTrait<Instance<Exts...>> {
			static void destroy(Instance<Exts...>& obj) noexcept {
				vkDestroyInstance(obj.handle_, nullptr);
			}

			[[nodiscard]]
			static VkInstance unwrap_native_handle(Instance<Exts...>& obj) noexcept {
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

	template<Extension... Exts>
	[[nodiscard]]
	std::span<PhysicalDevice> Instance<Exts...>::enum_phys_devices() noexcept {
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

	template<Extension... Exts>
	[[nodiscard]]
	Result<Instance<Exts...>> make_instance(const InstanceDesc& desc) noexcept {
		InstanceInfo info;
		std::vector<usize> idxs;

		constexpr std::array required_exts = [](auto&&... args) {
			constexpr usize size = (0 + ... + std::size(Exts::ext_names));
			std::array<const char*, size> ret = {}; usize i = 0;
			((std::ranges::copy(std::begin(args), std::end(args), ret.begin() + i), i += std::size(args)), ...);
			return ret;
		}(Exts::ext_names...);

		idxs = check_supported_extensions(required_exts, info);

		if (idxs.size() != 0) {
			std::string unsupported_exts;
			for (usize i : idxs) {
				unsupported_exts += std::format("{} ", required_exts[i]);
			}
			EH_ERROR_MSG(std::format("Some extensions are not supported ({})", unsupported_exts));
			return eh::Error{ ErrorCode::eExtensionNotPresent };
		}

		idxs = check_supported_layers(desc.layers, info);

		if (idxs.size() != 0) {
			std::string unsupported_layers;
			for (usize i : idxs) {
				unsupported_layers += std::format("{} ", desc.layers[i]);
			}
			EH_ERROR_MSG(std::format("Some layers are not supported ({})", unsupported_layers));
			return eh::Error{ ErrorCode::eLayerNotPresent };
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
			return Instance<Exts...>{ instance };
		}
		
		return eh::Error{ convert_vk_result(res) };
	}
}
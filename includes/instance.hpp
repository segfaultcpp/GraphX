#pragma once
#include "utils.hpp"
#include "error.hpp"
#include "types.hpp"

#include "device.hpp"

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

#ifdef GX_WIN64
#include <vulkan/vulkan_win32.h>
#endif

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

		template<typename T>
		concept Extension = requires(VkInstance instance) {
			T::get();
			T::load(instance);
		};

		struct FuncLoader {
			template<typename FnPtr> requires std::is_pointer_v<FnPtr>
			static std::optional<FnPtr> load(VkInstance instance, std::string_view name) noexcept {
				auto ret = std::bit_cast<FnPtr>(vkGetInstanceProcAddr(instance, name.data()));
			
				if (ret != nullptr){
					return ret;
				}
				return std::nullopt;
			}
		};

		struct DebugUtilsExt {
			static PFN_vkCreateDebugUtilsMessengerEXT create_fn;
			static PFN_vkDestroyDebugUtilsMessengerEXT destroy_fn;

			static constexpr auto get() noexcept {
				return std::array{ ExtensionList::ext_debug_utils };
			}

			static void load(VkInstance instance) noexcept {
				// there is no dangling pointer
				create_fn = FuncLoader::load<decltype(create_fn)>(instance, "vkCreateDebugUtilsMessengerEXT").value();
				destroy_fn = FuncLoader::load<decltype(destroy_fn)>(instance, "vkDestroyDebugUtilsMessengerEXT").value();
			}
		};
		static_assert(Extension<DebugUtilsExt>);

		inline PFN_vkCreateDebugUtilsMessengerEXT DebugUtilsExt::create_fn = nullptr;
		inline PFN_vkDestroyDebugUtilsMessengerEXT DebugUtilsExt::destroy_fn = nullptr;

		struct DebugUtilsValue {
			VkDebugUtilsMessengerEXT handle = VK_NULL_HANDLE;
			VkInstance parent = VK_NULL_HANDLE;
		
			DebugUtilsValue() noexcept = default;

			DebugUtilsValue(VkInstance instance, VkDebugUtilsMessengerEXT msger) noexcept 
				: handle{ msger }
				, parent{ instance }
			{}

			void destroy() noexcept {
				DebugUtilsExt::destroy_fn(parent, handle, nullptr);
			}
		};
		static_assert(Value<DebugUtilsValue>);

		using DebugUtils = ValueType<DebugUtilsValue, EmptyImpl, MoveOnlyTag>;

		enum class MessageSeverity : u8 {
			eDiagnostic = bit<u8, 0>(),
			eInfo = bit<u8, 1>(),
			eWarning = bit<u8, 2>(),
			eError = bit<u8, 3>(),
			eAll = eDiagnostic | eInfo | eWarning | eError,
		};

		enum class MessageType : u8 {
			eGeneral = bit<u8, 0>(),
			eValidation = bit<u8, 1>(),
			ePerformance = bit<u8, 2>(),
			eAll = eGeneral | eValidation | ePerformance,
		};

		OVERLOAD_BIT_OPS(MessageSeverity, u8);
		OVERLOAD_BIT_OPS(MessageType, u8);

		MessageSeverity message_severity_from_vk(VkDebugUtilsMessageSeverityFlagBitsEXT severity) noexcept;
		MessageType message_type_from_vk(VkDebugUtilsMessageTypeFlagsEXT type) noexcept;
		VkDebugUtilsMessageSeverityFlagsEXT message_severity_to_vk(u8 from) noexcept;
		VkDebugUtilsMessageTypeFlagsEXT message_type_to_vk(u8 from) noexcept;

		struct DebugUtilsBuilder {
		private:
			VkInstance instance_ = VK_NULL_HANDLE;
			u8 msg_severity_ = static_cast<u8>(MessageSeverity::eAll);
			u8 msg_type_ = static_cast<u8>(MessageType::eAll);

		public:
			DebugUtilsBuilder() noexcept = default;

			DebugUtilsBuilder(VkInstance instance, u8 severity, u8 type) noexcept 
				: instance_{ instance }
				, msg_severity_{ severity }
				, msg_type_{ type }
			{}

			DebugUtilsBuilder with_instance(this const DebugUtilsBuilder self, VkInstance instance) noexcept {
				return DebugUtilsBuilder(instance, self.msg_severity_, self.msg_type_);
			}

			DebugUtilsBuilder with_msg_severity(this const DebugUtilsBuilder self, u8 severity) noexcept {
				return DebugUtilsBuilder(self.instance_, severity, self.msg_type_);
			}

			DebugUtilsBuilder with_msg_type(this const DebugUtilsBuilder self, u8 type) noexcept {
				return DebugUtilsBuilder(self.instance_, self.msg_severity_, type);
			}

			auto build(this const DebugUtilsBuilder self, auto& callback) noexcept -> std::expected<DebugUtils, ErrorCode> {
				auto vk_msg_severity = message_severity_to_vk(self.msg_severity_);
				auto vk_msg_type = message_type_to_vk(self.msg_type_);

				VkDebugUtilsMessengerCreateInfoEXT create_info = {
					.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
					.messageSeverity = vk_msg_severity,
					.messageType = vk_msg_type,
					.pfnUserCallback = &debug_utils_callback,
					.pUserData = &callback,
				};

				VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;
				VkResult res = DebugUtilsExt::create_fn(self.instance_, &create_info, nullptr, &messenger);
				
				if (res == VK_SUCCESS) {
					return DebugUtilsValue{ self.instance_, messenger };
				}

				return std::unexpected(convert_vk_result(res));
			}

		private:
			static VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_callback(
				VkDebugUtilsMessageSeverityFlagBitsEXT severity,
				VkDebugUtilsMessageTypeFlagsEXT type,
				const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
				void* user_data)
			{
				if (user_data != nullptr) {
					static_cast<void(*)(MessageSeverity, MessageType)>(user_data)(message_severity_from_vk(severity), message_type_from_vk(type));
				}
				else {
					[](MessageSeverity severity, MessageType type) noexcept {

					}(message_severity_from_vk(severity), message_type_from_vk(type));
				}

				return VK_FALSE;
			}
		};
	
		struct SurfaceExt {
			static constexpr auto get() noexcept {
				return std::array{ ExtensionList::khr_surface };
			}

			static void load(VkInstance instance) noexcept {}
		};
		static_assert(Extension<SurfaceExt>);

		struct SurfaceValue {
			VkSurfaceKHR handle = VK_NULL_HANDLE;
			VkInstance parent = VK_NULL_HANDLE;

			SurfaceValue() noexcept = default;

			SurfaceValue(VkInstance instance, VkSurfaceKHR surface) noexcept
				: handle{ surface }
				, parent{ instance }
			{}

			void destroy(this SurfaceValue self) noexcept {
				vkDestroySurfaceKHR(self.parent, self.handle, nullptr);
			}
		};
		static_assert(Value<SurfaceValue>);

		struct SurfaceImpl {
			template<typename Self>
			auto get_ext_swapchain_builder() noexcept {
				 
			}
		};

		using Surface = ValueType<SurfaceValue, SurfaceImpl, MoveOnlyTag, ViewableTag>;

		struct SwapchainExt {
			static constexpr auto get() noexcept {
				return std::array{ ExtensionList::khr_swapchain };
			}

			static void load(VkInstance instance) noexcept {}
		};
		static_assert(Extension<SwapchainExt>);

		struct SwapchainBuilder {
		private:


		public:


		};

#ifdef GX_WIN64
		struct Win32SurfaceExt {
			static constexpr auto get() noexcept {
				return std::array{ ExtensionList::khr_win32_surface };
			}

			static void load(VkInstance instance) noexcept {}
		};
		static_assert(Extension<Win32SurfaceExt>);

		struct Win32SurfaceBuilder {
		private:
			VkInstance instance_ = VK_NULL_HANDLE;
			HWND window_ = nullptr;
			HINSTANCE app_ = nullptr;

		public:
			Win32SurfaceBuilder() noexcept = default;

			Win32SurfaceBuilder(VkInstance instance, HWND window, HINSTANCE app) noexcept 
				: instance_{ instance }
				, window_{ window }
				, app_{ app }
			{}

			Win32SurfaceBuilder with_instance(this const Win32SurfaceBuilder self, VkInstance instance) noexcept {
				return Win32SurfaceBuilder(instance, self.window_, self.app_);
			}

			Win32SurfaceBuilder with_app_info(this const Win32SurfaceBuilder self, HWND window, HINSTANCE app) noexcept {
				return Win32SurfaceBuilder(self.instance_, window, app);
			}

			auto build(this const Win32SurfaceBuilder self) noexcept -> std::expected<Surface, ErrorCode> {
				VkWin32SurfaceCreateInfoKHR create_info{
					.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
					.hinstance = self.app_,
					.hwnd = self.window_
				};

				VkSurfaceKHR surface = VK_NULL_HANDLE;
				VkResult res = vkCreateWin32SurfaceKHR(self.instance_, &create_info, nullptr, &surface);
				
				if (res == VK_SUCCESS) {
					return SurfaceValue{ self.instance_, surface };
				}

				return std::unexpected(convert_vk_result(res));
			}
		};
#endif

		struct ValidationLayer {
			static constexpr auto name = LayerList::khr_validation;
		};
	}

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
		InstanceInfo() noexcept {
			u32 count = 0;
			vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
			supported_extensions.resize(count);
			vkEnumerateInstanceExtensionProperties(nullptr, &count, supported_extensions.data());
			
			count = 0;
			vkEnumerateInstanceLayerProperties(&count, nullptr);
			supported_layers.resize(count);
			vkEnumerateInstanceLayerProperties(&count, supported_layers.data());
		}

		std::vector<VkExtensionProperties> supported_extensions;
		std::vector<VkLayerProperties> supported_layers;

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

		template<typename Self>
			requires meta::SameAsAny<ext::SwapchainExt, Es...>
		[[nodiscard]]
		auto get_ext_swapchain_builder(this Self&& self) noexcept {

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
		auto enum_phys_devices(this Self&& self) noexcept {
			u32 count = 0;
			vkEnumeratePhysicalDevices(self.get_handle(), &count, nullptr);
			std::vector<VkPhysicalDevice> devices(count);
			vkEnumeratePhysicalDevices(self.get_handle(), &count, devices.data());
			
			return devices | std::views::transform(
				[](VkPhysicalDevice device) {
					return PhysDevice{ device };
				}) | std::ranges::to<std::vector>();
		}
	};

	template<typename E, typename L>
	using Instance = ValueType<InstanceValue, InstanceImpl<E, L>, MoveOnlyTag>;

	template<typename E = meta::List<>, typename L = meta::List<>>
	struct InstanceBuilder;

	template<typename... Es, typename... Ls>
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

		template<typename E>
		[[nodiscard]]
		constexpr auto with_extension() const noexcept {
			return InstanceBuilder<meta::List<E, Es...>, meta::List<Ls...>>{ app_name, app_version, engine_name, engine_version, vulkan_version };
		}


		template<typename L>
		[[nodiscard]]
		constexpr auto with_layer() const noexcept {
			return InstanceBuilder<meta::List<Es...>, meta::List<L, Ls...>>{ app_name, app_version, engine_name, engine_version, vulkan_version };
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
				static constexpr std::array required_exts = [](auto&&... args) {
					constexpr usize size = (0 + ... + std::size(Es::get()));
					std::array<const char*, size> ret = {}; usize i = 0;
					((std::ranges::copy(std::begin(args), std::end(args), ret.begin() + i), i += std::size(args)), ...);
					return ret;
				}(Es::get()...);
			
				inst_create_info.enabledExtensionCount = static_cast<u32>(required_exts.size());
				inst_create_info.ppEnabledExtensionNames = required_exts.data();
			}

			if constexpr (sizeof...(Ls) > 0) {
				static constexpr std::array required_layers = {
					Ls::name...
				};

				inst_create_info.enabledLayerCount = required_layers.size();
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
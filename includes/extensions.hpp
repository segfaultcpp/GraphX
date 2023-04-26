#pragma once
#include <vulkan/vulkan.h>

#ifdef GX_WIN64
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#endif

#include <misc/meta.hpp>
#include <misc/types.hpp>

#include <optional>
#include <string_view>
#include <array>
#include <expected>
#include <ranges>

#include "types.hpp"
#include "utils.hpp"
#include "error.hpp"

namespace gx::ext {
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

			if (ret != nullptr) {
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

	DECLARE_GX_OBJECT(DebugUtils, DebugUtilsValue, EmptyImpl, MoveOnlyTag);

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

		DebugUtilsBuilder with_instance(this const DebugUtilsBuilder self, VkInstance instance) noexcept;
		DebugUtilsBuilder with_msg_severity(this const DebugUtilsBuilder self, u8 severity) noexcept;
		DebugUtilsBuilder with_msg_type(this const DebugUtilsBuilder self, u8 type) noexcept;
		auto build(this const DebugUtilsBuilder self, auto& callback) noexcept -> std::expected<DebugUtils, ErrorCode>;

	private:
		static VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_callback(
			VkDebugUtilsMessageSeverityFlagBitsEXT severity,
			VkDebugUtilsMessageTypeFlagsEXT type,
			const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
			void* user_data);
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

	DECLARE_VIEWABLE_GX_OBJECT(Surface, SurfaceValue, SurfaceImpl, MoveOnlyTag, ViewableTag);

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

		Win32SurfaceBuilder with_instance(this const Win32SurfaceBuilder self, VkInstance instance) noexcept;
		Win32SurfaceBuilder with_app_info(this const Win32SurfaceBuilder self, HWND window, HINSTANCE app) noexcept;
		auto build(this const Win32SurfaceBuilder self) noexcept -> std::expected<Surface, ErrorCode>;
	};
#endif

	struct ValidationLayer {
		static constexpr auto name = LayerList::khr_validation;
	};
}
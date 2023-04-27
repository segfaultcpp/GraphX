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
	struct InstanceExtensionList {
		static constexpr const char* kKhrSurface = "VK_KHR_surface";
		static constexpr const char* kKhrWin32Surface = "VK_KHR_win32_surface";
		static constexpr const char* kExtDebugUtils = "VK_EXT_debug_utils";
	};

	struct DeviceExtensionList {
		static constexpr const char* kKhrSwapchain = "VK_KHR_swapchain";
	};

	struct LayerList {
		static constexpr const char* kKhrValidation = "VK_LAYER_KHRONOS_validation";
	};

	struct InstanceExtTag {};
	struct DeviceExtTag {};

	template<typename T>
	concept Extension = requires(VkInstance instance) {
		T::get();
		T::load(instance);
	};

	template<typename T>
	concept InstanceExt = Extension<T> && std::derived_from<T, InstanceExtTag>;

	template<typename T>
	concept DeviceExt = Extension<T> && std::derived_from <T, DeviceExtTag>;

	template<Extension... Es>
	constexpr auto to_array() noexcept {
		return [](auto&&... args) {
					constexpr usize size = (0 + ... + std::size(Es::get()));
					std::array<const char*, size> ret = {}; usize i = 0;
					((std::ranges::copy(std::begin(args), std::end(args), ret.begin() + i), i += std::size(args)), ...);
					return ret;
				}(Es::get()...);
	}

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

	struct DebugUtilsExt : InstanceExtTag {
		static PFN_vkCreateDebugUtilsMessengerEXT create_fn;
		static PFN_vkDestroyDebugUtilsMessengerEXT destroy_fn;

		static constexpr auto get() noexcept {
			return std::array{ InstanceExtensionList::kExtDebugUtils };
		}

		static void load(VkInstance instance) noexcept {
			// there is no dangling pointer
			create_fn = FuncLoader::load<decltype(create_fn)>(instance, "vkCreateDebugUtilsMessengerEXT").value();
			destroy_fn = FuncLoader::load<decltype(destroy_fn)>(instance, "vkDestroyDebugUtilsMessengerEXT").value();
		}
	};
	static_assert(InstanceExt<DebugUtilsExt>);

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

	struct SurfaceExt : InstanceExtTag{
		static constexpr auto get() noexcept {
			return std::array{ InstanceExtensionList::kKhrSurface };
		}

		static void load(VkInstance instance) noexcept {}
	};
	static_assert(InstanceExt<SurfaceExt>);

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

	struct SurfaceImpl {};

	DECLARE_VIEWABLE_GX_OBJECT(Surface, SurfaceValue, SurfaceImpl, MoveOnlyTag);

	struct SwapchainExt : DeviceExtTag {
		static constexpr auto get() noexcept {
			return std::array{ DeviceExtensionList::kKhrSwapchain };
		}

		static void load(VkInstance instance) noexcept {}
	};
	static_assert(DeviceExt<SwapchainExt>);

	enum class ColorSpace {
		eSRGB_Nonlinear,
	};

	[[nodiscard]]
	ColorSpace color_space_from_vk(VkColorSpaceKHR color_space) noexcept;

	[[nodiscard]]
	VkColorSpaceKHR color_space_to_vk(ColorSpace color_space) noexcept;

	enum class PresentMode {
		eImmediate,
		eFifo,
		eFifoRelaxed,
		eMailbox,
	};

	[[nodiscard]]
	PresentMode present_mode_from_vk(VkPresentModeKHR mode) noexcept;

	[[nodiscard]]
	VkPresentModeKHR present_mode_to_vk(PresentMode mode) noexcept;

	struct SurfaceCapabilities {
		Extent2D current_extent;
		Extent2D min_image_extent;
		Extent2D max_image_extent;
		u32 min_image_count;
		u32 max_image_count;
		u32 max_image_array_layers;
		// TODO
	
		[[nodiscard]]
		static SurfaceCapabilities from_vk(const VkSurfaceCapabilitiesKHR& caps) noexcept;
	};

	struct SurfaceFormat {
		Format format;
		ColorSpace color_space;

		[[nodiscard]]
		static SurfaceFormat from_vk(const VkSurfaceFormatKHR& format) noexcept;
	};

	struct SwapchainSupport {
		ext::SurfaceCapabilities caps;
		std::vector<ext::SurfaceFormat> formats;
		std::vector<ext::PresentMode> present_modes;
	};

	struct SwapchainValue {
		VkSwapchainKHR handle = VK_NULL_HANDLE;
		VkDevice parent = VK_NULL_HANDLE;

		SwapchainValue() noexcept = default;

		SwapchainValue(VkSwapchainKHR swapchain, VkDevice device) noexcept
			: handle{ swapchain }
			, parent{ device }
		{}

		void destroy(this SwapchainValue self) noexcept {
			vkDestroySwapchainKHR(self.parent, self.handle, nullptr);
		}
	};
	static_assert(Value<SwapchainValue>);

	struct SwapchainImpl {};

	DECLARE_GX_OBJECT(Swapchain, SwapchainValue, SwapchainImpl, MoveOnlyTag);

	struct SwapchainBuilder {
	private:
		VkDevice device_ = VK_NULL_HANDLE;
		SurfaceView surface_{};

	public:


	};

#ifdef GX_WIN64
	struct Win32SurfaceExt : InstanceExtTag {
		static constexpr auto get() noexcept {
			return std::array{ InstanceExtensionList::kKhrWin32Surface };
		}

		static void load(VkInstance instance) noexcept {}
	};
	static_assert(InstanceExt<Win32SurfaceExt>);

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
		static constexpr auto name = LayerList::kKhrValidation;
	};
}
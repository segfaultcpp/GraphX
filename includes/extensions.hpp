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
#include <cassert>

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
		return 
			[](auto&&... args) {
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

	[[nodiscard]]
	MessageSeverityFlags message_severity_flags_from_vk(VkDebugUtilsMessageSeverityFlagsEXT severity) noexcept;
	[[nodiscard]]
	MessageSeverity message_severity_from_vk(VkDebugUtilsMessageSeverityFlagBitsEXT severity) noexcept;
	[[nodiscard]]
	MessageTypeFlags message_type_flags_from_vk(VkDebugUtilsMessageTypeFlagsEXT type) noexcept;
	[[nodiscard]]
	MessageType message_type_from_vk(VkDebugUtilsMessageTypeFlagBitsEXT type) noexcept;
	[[nodiscard]]
	VkDebugUtilsMessageSeverityFlagsEXT message_severity_flags_to_vk(MessageSeverityFlags from) noexcept;
	[[nodiscard]]
	VkDebugUtilsMessageSeverityFlagBitsEXT message_severity_to_vk(MessageSeverity from) noexcept;
	[[nodiscard]]
	VkDebugUtilsMessageTypeFlagsEXT message_type_flags_to_vk(MessageTypeFlags from) noexcept;
	[[nodiscard]]
	VkDebugUtilsMessageTypeFlagBitsEXT message_type_to_vk(MessageType from) noexcept;


	struct DebugUtilsBuilder {
	private:
		VkInstance instance_ = VK_NULL_HANDLE;
		MessageSeverityFlags severity_flags_ = static_cast<u8>(MessageSeverity::eAll);
		MessageTypeFlags type_flags_ = static_cast<u8>(MessageType::eAll);

	public:
		DebugUtilsBuilder() noexcept = default;

		DebugUtilsBuilder(VkInstance instance, MessageSeverityFlags severity_flags, MessageTypeFlags type_flags) noexcept
			: instance_{ instance }
			, severity_flags_{ severity_flags }
			, type_flags_{ type_flags }
		{}

		[[nodiscard]]
		DebugUtilsBuilder with_instance(this const DebugUtilsBuilder self, VkInstance instance) noexcept;
		[[nodiscard]]
		DebugUtilsBuilder with_msg_severity(this const DebugUtilsBuilder self, MessageSeverityFlags severity) noexcept;
		[[nodiscard]]
		DebugUtilsBuilder with_msg_type(this const DebugUtilsBuilder self, MessageTypeFlags type) noexcept;

		template<typename Callback> requires std::invocable<Callback, MessageSeverity, MessageTypeFlags>
		[[nodiscard]]
		auto build(this const DebugUtilsBuilder self, Callback& callback) noexcept -> std::expected<DebugUtils, ErrorCode> {
			validate();

			auto vk_msg_severity = message_severity_flags_to_vk(self.severity_flags_);
			auto vk_msg_type = message_type_flags_to_vk(self.type_flags_);

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
			void* user_data);

		void validate(this const DebugUtilsBuilder self) noexcept;
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
		
		[[nodiscard]]
		VkSurfaceFormatKHR to_vk(this SurfaceFormat self) noexcept;
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

	DECLARE_VIEWABLE_GX_OBJECT(Swapchain, SwapchainValue, SwapchainImpl, MoveOnlyTag);

	struct SwapchainBuilder {
	private:
		VkDevice device_ = VK_NULL_HANDLE;
	
	public:
		SurfaceView surface_{};
		u32 min_image_count_ = 2;
		u32 image_array_layers_ = 1;
		Format image_format_ = Format::eUndefined;
		ColorSpace color_space_ = ColorSpace::eSRGB_Nonlinear;
		Extent2D image_extent_;
		ImageUsageFlags image_usage_ = std::to_underlying(ImageUsage::eColorAttachment);
		SharingMode image_sharing_mode_ = SharingMode::eExclusive;
		std::vector<u32> queue_familiy_indices_;
		PresentMode present_mode_ = PresentMode::eFifo;
		bool clipped_ = false;

	public:
		SwapchainBuilder(VkDevice device) noexcept
			: device_{ device }
		{}

		[[nodiscard]]
		SwapchainBuilder& with_surface(SurfaceView surface) noexcept {
			surface_ = surface;
			return *this;
		}

		[[nodiscard]]
		SwapchainBuilder& with_image_sizes(Extent2D extent, u32 image_count, u32 array_layers = 1) noexcept {
			image_extent_ = extent;
			image_array_layers_ = array_layers;
			min_image_count_ = image_count;
			return *this;
		}

		[[nodiscard]]
		SwapchainBuilder& with_image_format(Format format, ColorSpace color_space = ColorSpace::eSRGB_Nonlinear) noexcept {
			image_format_ = format;
			color_space_ = color_space;
			return *this;
		}

		[[nodiscard]]
		SwapchainBuilder& with_image_format(SurfaceFormat format) noexcept {
			return with_image_format(format.format, format.color_space);
		}

		[[nodiscard]]
		SwapchainBuilder& with_image_usage(ImageUsageFlags usage) noexcept {
			image_usage_ = usage;
			return *this;
		}

		[[nodiscard]]
		SwapchainBuilder& with_queue_indices(std::vector<u32>&& indices) noexcept {
			if (indices.size() > 1) {
				image_sharing_mode_ = SharingMode::eConcurrent;
			}
			queue_familiy_indices_ = std::move(indices);

			return *this;
		}

		[[nodiscard]]
		SwapchainBuilder& with_queue_indices(std::span<u32> indices) noexcept {
			return with_queue_indices(std::vector(indices.begin(), indices.end()));
		}

		[[nodiscard]]
		SwapchainBuilder& with_present_mode(PresentMode mode) noexcept {
			present_mode_ = mode;
			return *this;
		}

		[[nodiscard]]
		SwapchainBuilder& set_clipped(bool clipped) noexcept {
			clipped_ = clipped;
			return *this;
		}

		[[nodiscard]]
		auto build() noexcept -> std::expected<Swapchain, ErrorCode> {
			VkSwapchainCreateInfoKHR ci = {
				.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
				.pNext = nullptr,
				.flags = 0,
				.surface = surface_.get_handle(),
				.minImageCount = min_image_count_,
				.imageFormat = format_to_vk(image_format_),
				.imageColorSpace = color_space_to_vk(color_space_),
				.imageExtent = image_extent_.to_vk(),
				.imageArrayLayers = image_array_layers_,
				.imageUsage = image_usage_to_vk(image_usage_),
				.imageSharingMode = sharing_mode_to_vk(image_sharing_mode_),
				.queueFamilyIndexCount = 0,
				.pQueueFamilyIndices = nullptr,
				.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
				.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR ,
				.presentMode = present_mode_to_vk(present_mode_),
				.clipped = clipped_,
				.oldSwapchain = nullptr,
			};

			if (!queue_familiy_indices_.empty()) {
				ci.pQueueFamilyIndices = queue_familiy_indices_.data();
				ci.queueFamilyIndexCount = static_cast<u32>(queue_familiy_indices_.size());
			}

			SwapchainValue value{ VK_NULL_HANDLE, device_ };
			VkResult res = vkCreateSwapchainKHR(device_, &ci, nullptr, &value.handle);

			if (res == VK_SUCCESS) {
				return Swapchain{ value };
			}
			return std::unexpected(convert_vk_result(res));
		}

		void validate() const noexcept {
			TODO("For Swapchain validation VkPhysicalDevice instance is needed");
		}
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

	private:
		void validate(this const Win32SurfaceBuilder self) noexcept;
	};
#endif

	struct ValidationLayer {
		static constexpr auto name = LayerList::kKhrValidation;
	};
}
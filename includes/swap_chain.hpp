#pragma once 

#include <vulkan/vulkan.h>

#include <misc/types.hpp>
#include <misc/assert.hpp>
#include <misc/result.hpp>

#include "object.hpp"
#include "error.hpp"
#include "cmd_exec.hpp"
#include "instance.hpp"

namespace gx {
	namespace ext {
		namespace details {
			template<Platform P>
			struct GetSurfaceExtImpl_;

			template<>
			struct GetSurfaceExtImpl_<Platform::eWindows> {
				static constexpr const char* ext_impl = ext::ExtensionList::khr_win32_surface;
				[[nodiscard]]
				static VkResult create_surface(VkInstance instance, HINSTANCE app, HWND window, VkSurfaceKHR& surface) noexcept;
				static void destroy_surface(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks* allocator) noexcept;

				using CreateFnPtr = decltype(&create_surface);
				using DestroyFnPtr = decltype(&destroy_surface);
			};

			template<>
			struct GetSurfaceExtImpl_<Platform::eLinux> {};
		}

		struct SurfaceKhrExtDef : ExtensionTag {
		private:
			// Type aliases
			using Impl_ = details::GetSurfaceExtImpl_<get_platform()>;

		public:
			// Constant expressions
			static constexpr std::array ext_names = { ext::ExtensionList::khr_surface, Impl_::ext_impl };

			// Statics
			static details::MakeCreateExtFn<typename Impl_::CreateFnPtr> create_fn;
			static details::MakeDestroyExtFn<typename Impl_::DestroyFnPtr> destroy_fn;

			// Static functions
			static void load(VkInstance instance) noexcept {
				create_fn = details::MakeCreateExtFn<Impl_::CreateFnPtr> {
					instance,
					&Impl_::create_surface
				};

				destroy_fn = details::MakeDestroyExtFn<Impl_::DestroyFnPtr>{
					instance,
					&Impl_::destroy_surface
				};
			}
		};

		using SurfaceKhrExt = RegisterExtension<SurfaceKhrExtDef>;

		inline details::MakeCreateExtFn<typename SurfaceKhrExtDef::Impl_::CreateFnPtr> SurfaceKhrExtDef::create_fn{};
		inline details::MakeDestroyExtFn<typename SurfaceKhrExtDef::Impl_::DestroyFnPtr> SurfaceKhrExtDef::destroy_fn{};

		class Surface : public ObjectOwner<Surface, VkSurfaceKHR> {
		public:
			// Type aliases
			using Base = ObjectOwner<Surface, VkSurfaceKHR>;
			using ObjectType = VkSurfaceKHR;

		private:
			// Friends
			friend struct ::gx::unsafe::ObjectOwnerTrait<Surface>;

		public:
			// Constructors/destructor
			Surface() noexcept = default;

			Surface(VkSurfaceKHR surface) noexcept
				: Base{ surface }
			{}

			Surface(Surface&&) noexcept = default;
			Surface(const Surface&) = delete;

			~Surface() noexcept = default;

			// Assignment operators
			Surface& operator=(Surface&&) noexcept = default;
			Surface& operator=(const Surface&) = delete;
		};

		template<ExtensionDef... Exts, Layer... Lyrs>
			requires meta::SameAsAny<SurfaceKhrExtDef, Exts...>
		Result<Surface> make_surface(
			[[maybe_unused]] const Instance<meta::List<Exts...>, meta::List<Lyrs...>>& unused, 
			typename PlatformTraits<get_platform()>::AppInstance app, 
			typename PlatformTraits<get_platform()>::WindowHandle window) noexcept
		{
			EH_ASSERT(app != NULL, "Passed invalid argument. app must not be 0");
			EH_ASSERT(window != NULL, "Passed invalid argument. window must not be 0");

			VkSurfaceKHR surface = VK_NULL_HANDLE;
			auto res = SurfaceKhrExtDef::create_fn(app, window, surface);
			if (res == VK_SUCCESS) {
				return Surface{ surface };
			}

			return eh::Error{ convert_vk_result(res) };
		}

		struct Extent2D {
			u32 width = 0;
			u32 height = 0;
		};

		enum class PresentationMode : u8 {
			eImmediate = 0,
			eFifo,
			eFifoRelaxed,
			eMailbox,
		};

		VkPresentModeKHR presentation_mode_to_vk(PresentationMode mode) noexcept;

		struct SwapChainDesc {
			typename PlatformTraits<get_platform()>::AppInstance app;
			typename PlatformTraits<get_platform()>::WindowHandle window;
			const GraphicsQueue& queue;
			Extent2D resolution;
			usize image_count;
			usize array_layers;
			PresentationMode present_mode;
			// TODO: format, color space
		};
	}

	namespace unsafe {
		template<>
		struct ObjectOwnerTrait<::gx::ext::Surface> {
			static void destroy(::gx::ext::Surface& obj) noexcept {
				::gx::ext::SurfaceKhrExtDef::destroy_fn(obj.handle_);
			}

			[[nodiscard]]
			static VkSurfaceKHR unwrap_native_handle(::gx::ext::Surface& obj) noexcept {
				auto ret = obj.handle_;
				obj.handle_ = VK_NULL_HANDLE;
				return ret;
			}
		};
	}
}
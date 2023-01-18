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
	class Surface : public ObjectOwner<Surface, VkSurfaceKHR> {
	public:
		// Type aliases
		using Base = ObjectOwner<Surface, VkSurfaceKHR>;
		using ObjectType = VkSurfaceKHR;

	private:
		// Friends
		friend struct unsafe::ObjectOwnerTrait<Surface>;

		// Fields
		VkInstance parent_ = VK_NULL_HANDLE;

	public:
		// Constructors/destructor
		Surface() noexcept = default;

		Surface(VkInstance instance, VkSurfaceKHR surface) noexcept 
			: Base{ surface }
		{}

		Surface(Surface&&) noexcept = default;
		Surface(const Surface&) = delete;

		~Surface() noexcept = default;

		// Assignment operators
		Surface& operator=(Surface&&) noexcept = default;
		Surface& operator=(const Surface&) = delete;
	};

	namespace unsafe {
		template<>
		struct ObjectOwnerTrait<Surface> {
			static void destroy(Surface& obj) noexcept {
				vkDestroySurfaceKHR(obj.parent_, obj.handle_, nullptr);
			}

			[[nodiscard]]
			static VkSurfaceKHR unwrap_native_handle(Surface& obj) noexcept {
				auto ret = obj.handle_;
				obj.handle_ = VK_NULL_HANDLE;
				return ret;
			}
		};
	}

	namespace details {
		template<Platform P>
		struct GetSurfaceExtImpl_;

		template<>
		struct GetSurfaceExtImpl_<Platform::eWindows> {
			static constexpr const char* ext_impl = ext::ExtensionList::khr_win32_surface;
			[[nodiscard]]
			static Result<VkSurfaceKHR> create_surface(VkInstance instance, HINSTANCE app, HWND window) noexcept;
		};

		template<>
		struct GetSurfaceExtImpl_<Platform::eLinux> {};
	}

	struct SurfaceKhrExt {
	private:
		// Type aliases
		using Impl_ = details::GetSurfaceExtImpl_<get_platform()>;

	public:
		using PT = PlatformTraits<get_platform()>;

		// Constant expressions
		static constexpr std::array ext_names = { ext::ExtensionList::khr_surface, Impl_::ext_impl };

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
		*/
		Result<Surface> get_surface(typename PT::WindowHandle window, typename PT::AppInstance app) noexcept {
			VkSurfaceKHR surface = VK_NULL_HANDLE;
			ErrorCode code = ErrorCode::eSuccess;

			auto res = Impl_::create_surface(instance_, app, window);
			std::move(res).match(
				[&surface](VkSurfaceKHR ok) noexcept {
					surface = ok;
				},
				[&code](ErrorCode err) noexcept {
					code = err;
				}
				);

			if (code == ErrorCode::eSuccess) {
				return Surface{ instance_, surface };
			}
			return eh::Error{ code };
		}

	private:
		// Fields
		VkInstance instance_;
	};

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
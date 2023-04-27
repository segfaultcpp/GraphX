#pragma once

#include <misc/meta.hpp>

#include <vulkan/vulkan.h>

#include <array>

namespace gx {
	struct MoveOnlyTag {};
	struct CopyableTag {};
	struct PinnedTag {};

	struct ViewableTag {};

	struct EmptyImpl {};

	template<typename T>
	concept Value = std::is_default_constructible_v<T> && std::is_trivially_copyable_v<T> &&
		requires(T obj) {
			obj.handle;
			obj.destroy();
		};

	template<typename T>
	concept DependentValue = Value<T> && requires(T obj) {
		obj.parent = VK_NULL_HANDLE;
	};

	template<Value V, typename Impl>
	struct View final : Impl {
	private:
		V value_;

	public:
		View() noexcept = default;

		View(V value) noexcept 
			: value_{ value }
		{}

		auto get_handle(this const View self) noexcept {
			return self.value_.handle;
		}

		auto get_parent(this const View self) noexcept requires DependentValue<V> {
			return self.value_.parent;
		}
	};

	template<Value V, typename Impl, typename... Tags>
	struct ValueType final : Impl {
		friend typename Impl;
	private:
		V value_;

	public:
		explicit ValueType() noexcept = default;
		explicit ValueType(V value) noexcept : value_{ value } {}

		~ValueType() noexcept {
			if (is_valid()) {
				value_.destroy();
			}
		}

		ValueType(ValueType&& rhs) noexcept requires !meta::SameAsAny<PinnedTag, Tags...>
			: value_{ rhs.value_ }
		{
			if (&rhs == this) {
				return;
			}

			rhs.value_.handle = VK_NULL_HANDLE;

			if constexpr (DependentValue<V>) {
				rhs.value_.parent = VK_NULL_HANDLE;
			}
		}

		ValueType& operator=(ValueType&& rhs) noexcept requires !meta::SameAsAny<PinnedTag, Tags...>
		{
			if (&rhs == this) {
				return *this;
			}

			value_ = rhs.value_;
			rhs.value_.handle = VK_NULL_HANDLE;

			if constexpr (DependentValue<V>) {
				rhs.value_.parent = VK_NULL_HANDLE;
			}

			return *this;
		}

		ValueType(const ValueType&) noexcept 
			requires meta::SameAsAny<CopyableTag, Tags...> && !meta::SameAsAny<MoveOnlyTag, Tags...> && !meta::SameAsAny<PinnedTag, Tags...>
		= default;

		ValueType& operator=(const ValueType&) noexcept 
			requires meta::SameAsAny<CopyableTag, Tags...> && !meta::SameAsAny<MoveOnlyTag, Tags...> && !meta::SameAsAny<PinnedTag, Tags...>
		= default;

		V unwrap_native_handle() noexcept requires meta::SameAsAny<MoveOnlyTag, Tags...> {
			auto ret = value_;
			value_.handle = VK_NULL_HANDLE;

			if constexpr (DependentValue<V>) {
				value_.parent = VK_NULL_HANDLE;
			}

			return ret;
		}

		bool is_valid() noexcept {
			return value_.handle != VK_NULL_HANDLE;
		}

		View<V, Impl> get_view(this ValueType& self) noexcept requires meta::SameAsAny<ViewableTag, Tags...> {
			return self.value_;
		}

	private:
		auto get_handle() noexcept {
			return value_.handle;
		}
	};

	enum class Format {
		eUndefined,
		eBGRA8_SRGB,
		eCount
	};

	[[nodiscard]]
	inline Format format_from_vk(VkFormat format) noexcept {
		switch (format) {
		case VK_FORMAT_B8G8R8A8_SRGB:
			return Format::eBGRA8_SRGB;
		}
		return Format::eUndefined;
	}

	[[nodiscard]]
	inline VkFormat format_to_vk(Format format) noexcept {
		static constexpr std::array kFormats = {
			VK_FORMAT_UNDEFINED,
			VK_FORMAT_B8G8R8A8_SRGB,
		};
		return kFormats[std::to_underlying(format)];
	}

	struct Extent2D {
		u32 width = static_cast<u32>(~0);
		u32 height = static_cast<u32>(~0);

		constexpr Extent2D() noexcept = default;
		constexpr Extent2D(u32 w, u32 h) noexcept
			: width{ w }
			, height{ h }
		{}
	};
}

#define DECLARE_GX_OBJECT(Name, V, Impl, ...) \
using Name = gx::ValueType<V, Impl, __VA_ARGS__>; 

#define DECLARE_VIEWABLE_GX_OBJECT(Name, V, Impl, ...) \
using Name = gx::ValueType<V, Impl, gx::ViewableTag, __VA_ARGS__>; \
using Name##View = decltype(std::declval<Name&>().get_view()); 
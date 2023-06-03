#pragma once

#include <misc/meta.hpp>

#include <vulkan/vulkan.h>

#include <array>
#include <cassert>

#include "utils.hpp"

namespace gx {
	struct Tag{};

	struct MoveOnlyTag : Tag {};
	struct CopyableTag : Tag {};
	struct PinnedTag : Tag {};

	struct ViewableTag : Tag {};

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

	template<typename T>
	concept IsTag = std::derived_from<T, Tag>;

	void clear_value(Value auto& value) noexcept {
		value.handle = VK_NULL_HANDLE;

		if constexpr (DependentValue<std::remove_cvref_t<decltype(value)>>) {
			value.parent = VK_NULL_HANDLE;
		}
	}

	template<Value V, typename Impl>
	struct [[nodiscard]] View final : Impl {
	private:
		V value_;

	public:
		explicit View() noexcept = default;

		explicit View(V value) noexcept 
			: value_{ value }
		{}

		View(const View&) noexcept = default;
		View& operator=(const View&) noexcept = default;

		View(View&& rhs) noexcept
			: value_{ rhs.value_ }
		{
			if (&rhs == this) {
				return;
			}
			value_ = rhs.value_;
			clear_value(rhs.value_);
		}

		View& operator=(View&& rhs) noexcept
		{
			if (&rhs == this) {
				return *this;
			}
			value_ = rhs.value_;
			clear_value(rhs.value_);

			return *this;
		}

		[[nodiscard]]
		auto get_handle(this const View self) noexcept {
			return self.value_.handle;
		}

		[[nodiscard]]
		auto get_parent(this const View self) noexcept requires DependentValue<V> {
			return self.value_.parent;
		}
	};

	template<Value V, typename Impl, IsTag... Tags>
	struct [[nodiscard]] OwnedType final : Impl {
		friend typename Impl;
	private:
		V value_;

	public:
		explicit OwnedType() noexcept = default;
		explicit OwnedType(V value) noexcept : value_{ value } {}

		~OwnedType() noexcept {
			if (is_valid()) {
				value_.destroy();
			}
		}

		OwnedType(OwnedType&& rhs) noexcept requires !meta::SameAsAny<PinnedTag, Tags...>
			: value_{ rhs.value_ }
		{
			if (&rhs == this) {
				return;
			}
			value_ = rhs.value_;
			clear_value(rhs.value_);
		}

		OwnedType& operator=(OwnedType&& rhs) noexcept requires !meta::SameAsAny<PinnedTag, Tags...>
		{
			if (&rhs == this) {
				return *this;
			}
			value_ = rhs.value_;
			clear_value(rhs.value_);

			return *this;
		}

		OwnedType(const OwnedType&) noexcept 
			requires meta::SameAsAny<CopyableTag, Tags...> && !meta::SameAsAny<MoveOnlyTag, Tags...> && !meta::SameAsAny<PinnedTag, Tags...>
		= default;

		OwnedType& operator=(const OwnedType&) noexcept 
			requires meta::SameAsAny<CopyableTag, Tags...> && !meta::SameAsAny<MoveOnlyTag, Tags...> && !meta::SameAsAny<PinnedTag, Tags...>
		= default;

		[[nodiscard]]
		auto unwrap_native_handle(this OwnedType&& self) noexcept requires meta::SameAsAny<MoveOnlyTag, Tags...> || meta::SameAsAny<PinnedTag, Tags...>
		{
			auto ret = self.value_.handle;
			clear_value(self.value_);

			return ret;
		}

		[[nodiscard]]
		bool is_valid() const noexcept {
			return value_.handle != VK_NULL_HANDLE;
		}

		[[nodiscard]]
		View<V, Impl> get_view(this OwnedType& self) noexcept requires meta::SameAsAny<ViewableTag, Tags...> {
			return View<V, Impl>{ self.value_ };
		}

	private:
		[[nodiscard]]
		auto get_handle() const noexcept {
			return value_.handle;
		}

		[[nodiscard]]
		auto get_parent() const noexcept requires DependentValue<V> {
			return value_.parent;
		}
	};

	template<Value V, typename Impl>
	struct [[nodiscard]] ManagableType final : Impl{
		friend typename Impl;
	private:
		V value_;

#ifdef GX_INDEV
		bool is_guaranteed_to_be_destroyed_ = false;
#endif

	public:
		ManagableType() noexcept = default;
		ManagableType(V value) noexcept 
			: value_{ value }
		{}

		ManagableType(const ManagableType&) noexcept = default;
		ManagableType& operator=(const ManagableType&) noexcept = default;

		ManagableType(ManagableType&& rhs) noexcept
			: value_{ rhs.value_ }
		{
			if (&rhs == this) {
				return;
			}
			value_ = rhs.value_;
			clear_value(rhs.value_);

#ifdef GX_INDEV
			rhs.is_guaranteed_to_be_destroyed_ = true;
#endif
		}

		ManagableType& operator=(ManagableType&& rhs) noexcept
		{
			if (&rhs == this) {
				return *this;
			}
			value_ = rhs.value_;
			clear_value(rhs.value_);

#ifdef GX_INDEV
			rhs.is_guaranteed_to_be_destroyed_ = true;
#endif
			return *this;
		}

#ifdef GX_INDEV
		~ManagableType() noexcept {
			assert(is_guaranteed_to_be_destroyed_ && "gx::ManagableType must be destroyed manually!");
		}
#endif

		[[nodiscard]]
		auto get_handle() const noexcept {
			return value_.handle;
		}

		[[nodiscard]]
		auto get_parent() const noexcept requires DependentValue<V> {
			return value_.parent;
		}

		[[nodiscard]]
		View<V, Impl> get_view(this ManagableType& self) noexcept {
			return View<V, Impl>{ self.value_ };
		}

		void destroy() noexcept {
			if (value_.handle != VK_NULL_HANDLE) {
				value_.destroy();
				clear_value(value_);
#ifdef GX_INDEV
				is_guaranteed_to_be_destroyed_ = true;
#endif
			}
		}

		template<IsTag... Tags>
		[[nodiscard]]
		OwnedType<V, Impl, Tags...> to_owned(this ManagableType&& self) noexcept {
			OwnedType<V, Impl, Tags...> owned{ self.value_ };
			clear_value(self.value_);
#ifdef GX_INDEV
			is_guaranteed_to_be_destroyed_ = true;
#endif
			return owned;
		}

		auto defer_destruction(this ManagableType&& self) noexcept {
#ifdef GX_INDEV
			is_guaranteed_to_be_destroyed_ = true;
#endif
			return std::make_pair(
				View<V, Impl>{ self.value_ },
				defer_exec(
					[value = std::move(self)] {
						value.destroy();
					}
				)
			);
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

	enum class ImageUsage : u32 {
		eColorAttachment = bit<u32, 0>(),
	};

	OVERLOAD_BIT_OPS(ImageUsage, u32);

	[[nodiscard]]
	inline u32 image_usage_from_vk(VkImageUsageFlags flags) noexcept {
		u32 ret = 0;
		if (test_bit(flags, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) {
			ret |= ImageUsage::eColorAttachment;
		}
		return ret;
	}

	[[nodiscard]]
	inline VkImageUsageFlags image_usage_to_vk(u32 flags) noexcept {
		VkImageUsageFlags ret = 0;
		if (test_bit(flags, ImageUsage::eColorAttachment)) {
			ret |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}
		return ret;
	}

	enum class SharingMode {
		eExclusive, 
		eConcurrent,
	};

	[[nodiscard]]
	inline SharingMode sharing_mode_from_vk(VkSharingMode mode) noexcept {
		if (mode == VK_SHARING_MODE_CONCURRENT) {
			return SharingMode::eConcurrent;
		}
		return SharingMode::eExclusive;
	}

	[[nodiscard]]
	inline VkSharingMode sharing_mode_to_vk(SharingMode mode) noexcept {
		if (mode == SharingMode::eConcurrent) {
			return VK_SHARING_MODE_CONCURRENT;
		}
		return VK_SHARING_MODE_EXCLUSIVE;
	}

	struct Extent2D {
		u32 width = static_cast<u32>(~0);
		u32 height = static_cast<u32>(~0);

		constexpr Extent2D() noexcept = default;
		constexpr Extent2D(u32 w, u32 h) noexcept
			: width{ w }
			, height{ h }
		{}

		[[nodiscard]]
		static constexpr Extent2D from_vk(VkExtent2D extent) noexcept {
			return Extent2D{ extent.width, extent.height };
		}

		[[nodiscard]]
		constexpr VkExtent2D to_vk(this Extent2D self) noexcept {
			return VkExtent2D{
				.width = self.width,
				.height = self.height,
			};
		}
	};
}

#define DECLARE_GX_OBJECT(Name, V, Impl, ...) \
using Name = gx::OwnedType<V, Impl, __VA_ARGS__>; 

#define DECLARE_VIEWABLE_GX_OBJECT(Name, V, Impl, ...) \
using Name = gx::OwnedType<V, Impl, gx::ViewableTag, __VA_ARGS__>; \
using Name##View = decltype(std::declval<Name&>().get_view()); 
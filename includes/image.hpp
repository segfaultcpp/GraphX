#pragma once

#include <vector>
#include <array>
#include <expected>

#include <misc/types.hpp>
#include <misc/meta.hpp>

#include "types.hpp"
#include "device.hpp"
#include "extensions.hpp"
#include "error.hpp"

namespace gx {
	struct ImageImpl {};

	struct [[nodiscard]] ImageValue {
		VkImage handle = VK_NULL_HANDLE;
		VkDevice parent = VK_NULL_HANDLE;

		ImageValue() noexcept = default;

		ImageValue(VkImage image, VkDevice device) noexcept 
			: handle{ image }
			, parent{ device }
		{}

		void destroy() noexcept {
			vkDestroyImage(parent, handle, nullptr);
		}
	};
	static_assert(Value<ImageValue>);

	using Image = ManagableType<ImageValue, ImageImpl>;
	using ImageView = decltype(std::declval<Image&>().get_view());

	std::vector<ImageView> get_images_from_swapchain(ext::SwapchainView swapchain) noexcept {
		u32 count = 0;
		vkGetSwapchainImagesKHR(swapchain.get_parent(), swapchain.get_handle(), &count, nullptr);
		std::vector<VkImage> images(count);
		vkGetSwapchainImagesKHR(swapchain.get_parent(), swapchain.get_handle(), &count, images.data());

		return images |
			std::views::transform(
				[device = swapchain.get_parent()](VkImage image) {
					return ImageView{ ImageValue{ image, device } };
				}
			) |
			std::ranges::to<std::vector>();
	}

	struct ImageRefImpl {};

	struct [[nodiscard]] ImageRefValue {
		VkImageView handle;
		VkDevice parent;

		ImageRefValue() noexcept = default;

		ImageRefValue(VkImageView image_view, VkDevice device) noexcept 
			: handle{ image_view }
			, parent{ device }
		{}

		void destroy() noexcept {
			vkDestroyImageView(parent, handle, nullptr);
		}
	};
	static_assert(Value<ImageRefValue>);

	using ImageRef = ManagableType<ImageRefValue, ImageRefImpl>;
	using ImageRefView = decltype(std::declval<ImageRef&>().get_view());

	enum class ImageRefType {
		e1D,
		e2D,
		e3D,
		eCube,
		e1D_Array,
		e2D_Array,
		eCube_Array,
		eCount,
	};

	[[nodiscard]]
	constexpr VkImageViewType image_ref_type_to_vk(ImageRefType type) noexcept {
		return static_cast<VkImageViewType>(std::to_underlying(type));
	}
	static_assert(VK_IMAGE_VIEW_TYPE_1D == image_ref_type_to_vk(ImageRefType::e1D));
	static_assert(VK_IMAGE_VIEW_TYPE_2D == image_ref_type_to_vk(ImageRefType::e2D));
	static_assert(VK_IMAGE_VIEW_TYPE_3D == image_ref_type_to_vk(ImageRefType::e3D));

	[[nodiscard]]
	constexpr ImageRefType image_ref_type_from_vk(VkImageViewType type) noexcept {
		return static_cast<ImageRefType>(std::to_underlying(type));
	}
	static_assert(ImageRefType::e1D == image_ref_type_from_vk(VK_IMAGE_VIEW_TYPE_1D));
	static_assert(ImageRefType::e2D == image_ref_type_from_vk(VK_IMAGE_VIEW_TYPE_2D));
	static_assert(ImageRefType::e3D == image_ref_type_from_vk(VK_IMAGE_VIEW_TYPE_3D));


	enum class ColorComponentSwizzle {
		eIdentity,
		eZero,
		eOne,
		eR,
		eG,
		eB,
		eA,
		eCount,
	};

	[[nodiscard]]
	constexpr VkComponentSwizzle color_component_swizzle_to_vk(ColorComponentSwizzle swizzle) noexcept {
		return static_cast<VkComponentSwizzle>(std::to_underlying(swizzle));
	}
	static_assert(VK_COMPONENT_SWIZZLE_IDENTITY == color_component_swizzle_to_vk(ColorComponentSwizzle::eIdentity));
	static_assert(VK_COMPONENT_SWIZZLE_ZERO == color_component_swizzle_to_vk(ColorComponentSwizzle::eZero));
	static_assert(VK_COMPONENT_SWIZZLE_ONE == color_component_swizzle_to_vk(ColorComponentSwizzle::eOne));

	[[nodiscard]]
	constexpr ColorComponentSwizzle color_component_swizzle_from_vk(VkComponentSwizzle swizzle) noexcept {
		return static_cast<ColorComponentSwizzle>(std::to_underlying(swizzle));
	}
	static_assert(ColorComponentSwizzle::eIdentity == color_component_swizzle_from_vk(VK_COMPONENT_SWIZZLE_IDENTITY));
	static_assert(ColorComponentSwizzle::eZero == color_component_swizzle_from_vk(VK_COMPONENT_SWIZZLE_ZERO));
	static_assert(ColorComponentSwizzle::eOne == color_component_swizzle_from_vk(VK_COMPONENT_SWIZZLE_ONE));

	struct [[nodiscard]] ColorComponentMapping {
		ColorComponentSwizzle r = ColorComponentSwizzle::eIdentity;
		ColorComponentSwizzle g = ColorComponentSwizzle::eIdentity;
		ColorComponentSwizzle b = ColorComponentSwizzle::eIdentity;
		ColorComponentSwizzle a = ColorComponentSwizzle::eIdentity;

		constexpr ColorComponentMapping() noexcept = default;

		constexpr ColorComponentMapping(
			ColorComponentSwizzle r_c, 
			ColorComponentSwizzle g_c,
			ColorComponentSwizzle b_c,
			ColorComponentSwizzle a_c
		) noexcept
			: r{ r_c }
			, g{ g_c }
			, b{ b_c }
			, a{ a_c }
		{}

		[[nodiscard]]
		constexpr VkComponentMapping to_vk(this ColorComponentMapping self) noexcept {
			return VkComponentMapping {
				.r = color_component_swizzle_to_vk(self.r),
				.g = color_component_swizzle_to_vk(self.g),
				.b = color_component_swizzle_to_vk(self.b),
				.a = color_component_swizzle_to_vk(self.a),
			};
		}

		static constexpr ColorComponentMapping from_vk(VkComponentMapping mapping) noexcept {
			return ColorComponentMapping {
				color_component_swizzle_from_vk(mapping.r),
				color_component_swizzle_from_vk(mapping.g),
				color_component_swizzle_from_vk(mapping.b),
				color_component_swizzle_from_vk(mapping.a)
			};
		}
	};

	enum class ImageAspect : u32 {
		eColor = bit<u32, 0>(),
		eDepth = bit<u32, 1>(),
		eStencil = bit<u32, 2>(),
	};

	OVERLOAD_BIT_OPS(ImageAspect, u32);

	constexpr VkImageAspectFlagBits image_aspect_bits_to_vk(ImageAspect aspect) noexcept {
		auto bits = std::to_array(
			{
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_ASPECT_DEPTH_BIT,
				VK_IMAGE_ASPECT_STENCIL_BIT,
			}
		);
		return bits[std::bit_width(static_cast<usize>(aspect)) - 1];
	}
	static_assert(VK_IMAGE_ASPECT_COLOR_BIT == image_aspect_bits_to_vk(ImageAspect::eColor));
	static_assert(VK_IMAGE_ASPECT_DEPTH_BIT == image_aspect_bits_to_vk(ImageAspect::eDepth));
	static_assert(VK_IMAGE_ASPECT_STENCIL_BIT == image_aspect_bits_to_vk(ImageAspect::eStencil));

	constexpr ImageAspect image_aspect_bits_from_vk(VkImageAspectFlagBits aspect) noexcept {
		switch (aspect)
		{
		case VK_IMAGE_ASPECT_COLOR_BIT:
			return ImageAspect::eColor;

		case VK_IMAGE_ASPECT_DEPTH_BIT:
			return ImageAspect::eDepth;

		case VK_IMAGE_ASPECT_STENCIL_BIT:
			return ImageAspect::eStencil;

		default:
			TODO("Assertion");
			return ImageAspect::eColor;
		}
	}

	constexpr VkImageAspectFlags image_aspect_flags_to_vk(ImageAspectFlags aspect) noexcept {
		VkImageAspectFlags flags = 0;
		if (test_bit(aspect, ImageAspect::eColor)) {
			flags |= VK_IMAGE_ASPECT_COLOR_BIT;
		}
		if (test_bit(aspect, ImageAspect::eDepth)) {
			flags |= VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		if (test_bit(aspect, ImageAspect::eStencil)) {
			flags |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		return flags;
	}
	static_assert(VK_IMAGE_ASPECT_COLOR_BIT == image_aspect_flags_to_vk(std::to_underlying(ImageAspect::eColor)));
	static_assert((VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT) == image_aspect_flags_to_vk(ImageAspect::eDepth | ImageAspect::eStencil));
	static_assert((VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_COLOR_BIT) == image_aspect_flags_to_vk(ImageAspect::eDepth | ImageAspect::eStencil | ImageAspect::eColor));

	constexpr ImageAspectFlags image_aspect_flags_from_vk(VkImageAspectFlags aspect) noexcept {
		ImageAspectFlags flags = 0;
		if (test_bit(aspect, VK_IMAGE_ASPECT_COLOR_BIT)) {
			flags |= ImageAspect::eColor;
		}
		if (test_bit(aspect, VK_IMAGE_ASPECT_DEPTH_BIT)) {
			flags |= ImageAspect::eDepth;
		}
		if (test_bit(aspect, VK_IMAGE_ASPECT_STENCIL_BIT)) {
			flags |= ImageAspect::eStencil;
		}
		return flags;
	}
	static_assert(std::to_underlying(ImageAspect::eColor) == image_aspect_flags_from_vk(VK_IMAGE_ASPECT_COLOR_BIT));
	static_assert((ImageAspect::eDepth | ImageAspect::eStencil) == image_aspect_flags_from_vk(VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT));
	static_assert((ImageAspect::eDepth | ImageAspect::eStencil | ImageAspect::eColor) == image_aspect_flags_from_vk(VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_COLOR_BIT));

	struct ImageSubresourceRange {
		ImageAspectFlags aspect_mask = std::to_underlying(ImageAspect::eColor);
		u32 base_mip_level = 0;
		u32 level_count = 1;
		u32 base_array_layer = 0;
		u32 layer_count = 1;

		constexpr ImageSubresourceRange() noexcept = default;

		constexpr ImageSubresourceRange(
			ImageAspectFlags aspect, 
			u32 base_mip, 
			u32 mip_count, 
			u32 base_layer, 
			u32 array_layer_count
		) noexcept 
			: aspect_mask{ aspect }
			, base_mip_level{ base_mip }
			, level_count{ mip_count }
			, base_array_layer{ base_layer }
			, layer_count{ array_layer_count }
		{}

		constexpr VkImageSubresourceRange to_vk(this ImageSubresourceRange self) noexcept {
			return VkImageSubresourceRange{
				.aspectMask = image_aspect_flags_to_vk(self.aspect_mask),
				.baseMipLevel = self.base_mip_level,
				.levelCount = self.level_count,
				.baseArrayLayer = self.base_array_layer,
				.layerCount = self.layer_count,
			};
		}

		static constexpr ImageSubresourceRange from_vk(VkImageSubresourceRange range) noexcept {
			return ImageSubresourceRange {
				image_aspect_flags_from_vk(range.aspectMask),
				range.baseMipLevel,
				range.levelCount,
				range.baseArrayLayer,
				range.layerCount
			};
		}
	};

	template<typename E>
	struct [[nodiscard]] ImageRefBuilder {
		ImageView image;
		DeviceView<E> device;
		ImageRefType type = ImageRefType::e2D;
		Format format = Format::eUndefined;
		ColorComponentMapping mapping;
		ImageSubresourceRange subresource_range;

		ImageRefBuilder() noexcept = default;

		ImageRefBuilder(View<DeviceValue, DeviceImpl<E>> device_view) noexcept
			: device{ device_view }
		{}

		ImageRefBuilder(ImageView image_view, View<DeviceValue, DeviceImpl<E>> device_view) noexcept
			: image{ image_view }
			, device{ device_view }
		{}

		ImageRefBuilder& from_image(ImageView image_view) noexcept {
			image = image_view;
			return *this;
		}

		ImageRefBuilder& with_type(ImageRefType image_ref_type) noexcept {
			type = image_ref_type;
			return *this;
		}

		ImageRefBuilder& with_format(Format image_format) noexcept {
			format = image_format;
			return *this;
		}

		ImageRefBuilder& with_component_mapping(ColorComponentMapping comp_mapping) noexcept {
			mapping = comp_mapping;
			return *this;
		}

		ImageRefBuilder& with_subresource_range(ImageSubresourceRange range) noexcept {
			subresource_range = range;
			return *this;
		}

		auto build() noexcept -> std::expected<ImageRef, ErrorCode> {
			VkImageViewCreateInfo ci = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = image.get_handle(),
				.viewType = image_ref_type_to_vk(type),
				.format = format_to_vk(format),
				.components = mapping.to_vk(),
				.subresourceRange = subresource_range.to_vk(),
			};

			ImageRefValue image_ref{ VK_NULL_HANDLE, device.get_handle() };
			VkResult res = vkCreateImageView(image_ref.parent, &ci, nullptr, &image_ref.handle);

			if (res == VK_SUCCESS) {
				return ImageRef{ image_ref };
			}
			return std::unexpected(convert_vk_result(res));
		}

	private:
		void validate() noexcept {
			TODO("");
		}
	};
}
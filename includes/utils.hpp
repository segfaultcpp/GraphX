#pragma once

#include <ranges>
#include <vector>
#include <misc/functional.hpp>
#include <misc/utils.hpp>
#include <misc/types.hpp>

#define OVERLOAD_BIT_OPS(E, I) \
static_assert(std::is_enum_v<E>); \
static_assert(std::integral<I>); \
constexpr I operator|(E lhs, E rhs) noexcept { \
	return static_cast<I>(lhs) | static_cast<I>(rhs); \
} \
constexpr I operator |=(I lhs, E rhs) noexcept { \
	return static_cast<E>(lhs) | rhs; \
} \
constexpr I operator&(I lhs, E rhs) noexcept { \
	return lhs & static_cast<I>(rhs); \
} \

namespace gx {
	template<std::ranges::random_access_range ReqRng, std::ranges::input_range SupRng, typename Comp = std::equal_to<>, typename Proj = std::identity>
	std::vector<usize> check_support(ReqRng&& requested, SupRng&& supported, Proj proj = {}, Comp&& comp = {}) noexcept {
		auto view = requested | utils::index() |
			std::ranges::views::filter(
				[sup = std::forward<SupRng>(supported), comp, proj]
				(auto p) noexcept {
					return std::ranges::find_if(sup, fn::compare(p.second, comp, proj)) == std::end(sup);
				}
			) |
			std::ranges::views::transform(
				[](auto p) noexcept {
					return p.first;
				}
			);
		return std::vector<usize>(view.begin(), view.end());
	}

	template<std::integral T, u8 Shift>
	consteval T bit() noexcept {
		static_assert(Shift <= sizeof(T) * 8 - 1, "Shift must be less than bit count in type T");
		return static_cast<T>(1) << Shift;
	}

	constexpr usize kb_to_bytes(usize value) noexcept {
		return value * 1024;
	}

	constexpr usize mb_to_bytes(usize value) noexcept {
		return kb_to_bytes(value) * 1024;
	}

	constexpr usize gb_to_bytes(usize value) noexcept {
		return mb_to_bytes(value) * 1024;
	}
}
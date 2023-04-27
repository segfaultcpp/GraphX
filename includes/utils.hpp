#pragma once

#include <ranges>
#include <vector>
#include <misc/functional.hpp>
#include <misc/types.hpp>

#define OVERLOAD_BIT_OPS(E, I) \
static_assert(std::is_enum_v<E>); \
static_assert(std::integral<I>); \
static_assert(std::same_as<decltype(std::to_underlying(E{})), I>); \
using E##Flags = I; \
constexpr I operator|(E lhs, E rhs) noexcept { \
	return static_cast<I>(lhs) | static_cast<I>(rhs); \
} \
constexpr I operator|(I lhs, E rhs) noexcept { \
	return lhs | static_cast<I>(rhs);\
} \
constexpr I operator|(E lhs, I rhs) noexcept { \
	return static_cast<I>(lhs) | rhs;\
} \
constexpr I operator |=(I lhs, E rhs) noexcept { \
	return static_cast<E>(lhs) | rhs; \
} \
constexpr I operator&(I lhs, E rhs) noexcept { \
	return lhs & static_cast<I>(rhs); \
} \

namespace gx {
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

	template<typename E>
		requires std::is_enum_v<E>
	constexpr bool test_bit(decltype(std::to_underlying(E{})) flags, E bit) noexcept {
		return static_cast<bool>(flags & std::to_underlying(bit));
	}

	static_assert(
		[]() {
			enum class TestFlags : u8 {
				e1stBit = bit<u8, 0>(),
				e2ndBit = bit<u8, 1>(),
				e3rdBit = bit<u8, 2>(),
			};
			u8 flags = std::to_underlying(TestFlags::e1stBit) | std::to_underlying(TestFlags::e2ndBit);
			return test_bit(flags, TestFlags::e1stBit) && test_bit(flags, TestFlags::e2ndBit) && !test_bit(flags, TestFlags::e3rdBit);
		}() == true
	);

#ifdef GX_DEBUG
	inline constexpr bool kIsDebugMode = true;
#else
	inline constexpr bool kIsDebugMode = false;
#endif
}
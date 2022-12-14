#pragma once
#include <cstdint>
#include <cmath>
#include <concepts>
#include <ranges>

namespace rng = std::ranges;

namespace gx {

	using i8 = std::int8_t;
	using i16 = std::int16_t;
	using i32 = std::int32_t;
	using i64 = std::int64_t;

	using u8 = std::uint8_t;
	using u16 = std::uint16_t;
	using u32 = std::uint32_t;
	using u64 = std::uint64_t;

	using usize = std::size_t;
	using isize = i64;

	using f32 = float;
	using f64 = double;

	template<std::integral T, u8 Shift>
	consteval T bit() noexcept {
		static_assert(Shift <= sizeof(T) * 8, "Shift must be less than bit count in type T");
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

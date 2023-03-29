#pragma once
#include <cstdint>
#include <cstddef>

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

struct WindowsPlatform {};
struct LinuxPlatform {};

template<typename>
struct PlatformTraits;

#ifdef _WIN32
#include <Windows.h>

template<>
struct PlatformTraits<WindowsPlatform> {
	using AppInstance = HINSTANCE;
	using WindowHandle = HWND;
};

consteval WindowsPlatform get_platform() noexcept {
	return WindowsPlatform{};
}
#elif defined(__linux__)
consteval LinuxPlatform get_platform() noexcept {
	return LinuxPlatform{};
}
#endif

#if defined(_MSC_VER)
#define NOINLINE __declspec(noinline)
#elif defined(__clang__)
#define [[clang::noinline]]
#endif
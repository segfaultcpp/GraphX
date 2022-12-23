#pragma once

#include <string_view>
#include <cstdio>
#include <format>
#include <mutex>

#include "types.hpp"

#ifdef _WIN32
	#include <Windows.h>
#endif

#if defined(_MSC_VER)
	#define breakpoint() __debugbreak()
#elif defined(__clang__)
	#define breakpoint() __builtin_debugtrap()
#endif

#define EH_ASSERT(cond, message) \
do { \
	if(!(cond)) { \
	auto tokens = ::eh::DebugMessenger::make_assert(#cond, message, __FILE__, __LINE__); \
	::eh::DebugMessenger::print(std::move(tokens)); \
	::std::abort(); \
	} } \
while(false); 

#define EH_PANIC(message) \
do { \
	auto tokens = ::eh::DebugMessenger::make_panic(message, __FILE__, __LINE__); \
	::eh::DebugMessenger::print(std::move(tokens)); \
	::std::abort(); \
} \
while(false); 

#define EH_WARN(message)\
do { \
	auto tokens = ::eh::DebugMessenger::make_warning(message, __FILE__, __LINE__); \
	::eh::DebugMessenger::print(std::move(tokens)); \
} \
while(false); 

#define EH_INFO_MSG(message)\
do { \
	auto tokens = ::eh::DebugMessenger::make_info_message(message); \
	::eh::DebugMessenger::print(std::move(tokens)); \
} \
while(false); 

#define EH_ERROR_MSG(message)\
do { \
	auto tokens = ::eh::DebugMessenger::make_err_message(message); \
	::eh::DebugMessenger::print(std::move(tokens)); \
} \
while(false); 

#define EH_WARN_MSG(message)\
do { \
	auto tokens = ::eh::DebugMessenger::make_warn_message(message); \
	::eh::DebugMessenger::print(std::move(tokens)); \
} \
while(false); 

namespace eh {
	inline void default_output(std::string_view msg) noexcept {
		printf("%s", msg.data());
	}
	
	enum class OutputColor {
		eWhite = 0,
		eGreen,
		eYellow,
		eRed,
		eCyan,
	};

#ifdef _WIN32
	static void set_output_color(OutputColor color) noexcept {
		static HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
		
		switch (color) {
		case OutputColor::eWhite:
			SetConsoleTextAttribute(console, 15);
			break;
		case OutputColor::eGreen:
			SetConsoleTextAttribute(console, 10);
			break;
		case OutputColor::eYellow:
			SetConsoleTextAttribute(console, 14);
			break;
		case OutputColor::eRed:
			SetConsoleTextAttribute(console, FOREGROUND_RED);
			break;
		case OutputColor::eCyan:
			SetConsoleTextAttribute(console, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			break;
		}
	}
#endif

	class DebugMessenger {
	public:
		using OutputFn = void(*)(std::string_view) noexcept;
		using SetColorFn = void(*)(OutputColor) noexcept;

	public:
		static OutputFn output;
		static SetColorFn set_color;
		
	private:
		static std::mutex mutex_;

	public:
		static constexpr OutputColor default_color = OutputColor::eWhite;

	public:
		static auto make_assert(std::string_view condition, std::string_view message, std::string_view file, u32 line) noexcept {
			using namespace std::literals;
			std::size_t thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
			return std::vector{
				std::make_pair(OutputColor::eRed, "error: "s),
				std::make_pair(OutputColor::eWhite, "thread (id: "s),
				std::make_pair(OutputColor::eCyan, std::format("{}", thread_id)),
				std::make_pair(OutputColor::eWhite, ") "s),
				std::make_pair(OutputColor::eRed, "failed assertion"s),
				std::make_pair(OutputColor::eWhite, " in file "s),
				std::make_pair(OutputColor::eCyan, std::format("{}", file)),
				std::make_pair(OutputColor::eWhite, " on line "s),
				std::make_pair(OutputColor::eCyan, std::format("{}", line)),
				std::make_pair(OutputColor::eWhite, "\n"s),
				std::make_pair(OutputColor::eYellow, std::format("condition: ")),
				std::make_pair(OutputColor::eWhite, std::format("{}\n", condition)),
				std::make_pair(OutputColor::eYellow, std::format("message: ")),
				std::make_pair(OutputColor::eWhite, std::format("{}\n", message)),
			};
		}

		static auto make_panic(std::string_view message, std::string_view file, u32 line) noexcept {
			using namespace std::literals;
			std::size_t thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
			return std::vector{
				std::make_pair(OutputColor::eRed, "error: "s),
				std::make_pair(OutputColor::eWhite, "thread (id: "s),
				std::make_pair(OutputColor::eCyan, std::format("{}", thread_id)),
				std::make_pair(OutputColor::eWhite, ") "s),
				std::make_pair(OutputColor::eRed, "panicked"s),
				std::make_pair(OutputColor::eWhite, " in file "s),
				std::make_pair(OutputColor::eCyan, std::format("{}", file)),
				std::make_pair(OutputColor::eWhite, " on line "s),
				std::make_pair(OutputColor::eCyan, std::format("{}", line)),
				std::make_pair(OutputColor::eWhite, "\n"s),
				std::make_pair(OutputColor::eYellow, std::format("message: ")),
				std::make_pair(OutputColor::eWhite, std::format("{}\n", message)),
			};
		}

		static auto make_warning(std::string_view message, std::string_view file, u32 line) noexcept {
			using namespace std::literals;
			std::size_t thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
			return std::vector{
				std::make_pair(OutputColor::eYellow, "warning: "s),
				std::make_pair(OutputColor::eWhite, std::format("{}\n", message)),
				std::make_pair(OutputColor::eWhite, "at thread (id: "s),
				std::make_pair(OutputColor::eCyan, std::format("{}", thread_id)),
				std::make_pair(OutputColor::eWhite, ") "s),
				std::make_pair(OutputColor::eWhite, "in file "s),
				std::make_pair(OutputColor::eCyan, std::format("{}", file)),
				std::make_pair(OutputColor::eWhite, " on line "s),
				std::make_pair(OutputColor::eCyan, std::format("{}", line)),
				std::make_pair(OutputColor::eWhite, "\n"s),
			};
		}

		static auto make_info_message(std::string_view message) noexcept {
			using namespace std::literals;
			return std::vector{
				std::make_pair(OutputColor::eGreen, "info: "s),
				std::make_pair(OutputColor::eWhite, std::format("{}\n", message)),
			};
		}

		static auto make_err_message(std::string_view message) noexcept {
			using namespace std::literals;
			return std::vector{
				std::make_pair(OutputColor::eRed, "error: "s),
				std::make_pair(OutputColor::eWhite, std::format("{}\n", message)),
			};
		}

		static auto make_warn_message(std::string_view message) noexcept {
			using namespace std::literals;
			return std::vector{
				std::make_pair(OutputColor::eYellow, "warning: "s),
				std::make_pair(OutputColor::eWhite, std::format("{}\n", message)),
			};
		}

		static void print(std::vector<std::pair<OutputColor, std::string>> tokens) noexcept {
			std::lock_guard lock{ mutex_ };

			for (auto&[color, msg] : tokens) {
				set_color(color);
				output(msg);
			}
			set_color(default_color);
		}
	};

	inline DebugMessenger::OutputFn DebugMessenger::output = &default_output;
	inline DebugMessenger::SetColorFn DebugMessenger::set_color = &set_output_color;
	inline std::mutex DebugMessenger::mutex_{};
}
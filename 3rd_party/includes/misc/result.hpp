#pragma once
#include <string>
#include <string_view>
#include <concepts>
#include <type_traits>

#include "assert.hpp"

namespace eh {
	template<typename T>
		requires std::is_enum_v<T>
	struct ErrorTypeTrait;

	template<typename T>
	concept ErrorType = std::is_enum_v<T> && requires(T e) {
		sizeof(T) >= 4u;
		ErrorTypeTrait<T>::description(e);
		ErrorTypeTrait<T>::stringify(e);
		ErrorTypeTrait<T>::default_value();
	};

	template<ErrorType T>
	struct Error {
		T error_value;

		Error(T value) noexcept
			: error_value{ value }
		{}

		Error(const Error&) noexcept = default;
		Error& operator=(const Error&) noexcept = default;

		Error(Error&& rhs) noexcept
			: error_value{ rhs.error_value }
		{
			rhs.error_value = ErrorTypeTrait<T>::default_value();
		}

		Error& operator=(Error&& rhs) noexcept {
			error_value = rhs.error_value;
			rhs.error_value = ErrorTypeTrait<T>::default_value();

			return *this;
		}
	};

	template<typename Res, typename Err>
	class Result
	{
		using Underlying = std::conditional_t<sizeof(Err) == 8, std::uint64_t, std::conditional_t<sizeof(Err) == 4, std::uint32_t, void>>;
		static_assert(!std::same_as<Underlying, void>);

		bool is_error_ = false;
		union
		{
			Res result_;
			Error<Err> error_;
		};

	public:
		constexpr Result(Res&& res) noexcept
			: result_{ std::move(res) }
			, is_error_{ false }
		{}

		Result(Error<Err> err) noexcept
			: error_{ std::move(err) }
			, is_error_{ true }
		{}

		~Result() noexcept {
			if (is_success()) {
				result_.~Res();
			}
		}

	public:
		constexpr Result(Result&& rhs) noexcept
			: result_{ std::move(rhs.result_) }
			, is_error_{ rhs.is_error_ }
		{
			rhs.is_error_ = false;
		}

	public:
		constexpr bool is_success() const noexcept {
			return !is_error_;
		}

		[[nodiscard]]
		Res unwrap() && noexcept {
			if (!is_success()) {
				EH_PANIC(std::format("Called Result<>::unwrap() on an error value. Error code: {}. Description: {}", ErrorTypeTrait<Err>::stringify(error_.error_value), ErrorTypeTrait<Err>::description(error_.error_value)));
			}
			return std::move(result_);
		}

		[[nodiscard]]
		Res expect(std::string_view error) && noexcept {
			EH_ASSERT(is_success(), error);

			return std::move(result_);
		}

		template<typename ErrorHandler>
		[[nodiscard]]
		Res unwrap_or_else(ErrorHandler&& fn) && noexcept {
			if (is_success()) {
				return std::move(result_);
			}
			else {
				return std::move(fn(error_.error_value));
			}
		}

		template<typename FnOk, typename FnErr>
		void match(FnOk ok, FnErr err) && noexcept {
			if (is_success()) {
				ok(std::move(result_));
			}
			else {
				err(error_.error_value);
			}
		}
	};
}
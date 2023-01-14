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
	class [[nodiscard]] Result {
		union
		{
			Res result_;
			Error<Err> error_;
		};
		bool is_error_ = false;

	public:
		constexpr Result(Res&& res) noexcept
			: result_{ std::move(res) }
			, is_error_{ false }
		{}

		constexpr Result(Error<Err> err) noexcept
			: error_{ std::move(err) }
			, is_error_{ true }
		{}

		~Result() noexcept {
			if (is_ok()) {
				result_.~Res();
			}
		}

	public:
		constexpr Result(Result&& rhs) noexcept 
			: is_error_{ rhs.is_error_ }
		{
			if (rhs.is_error_) {
				error_ = rhs.error_;
			}
			else {
				result_ = std::move(rhs.result_);
			}

			rhs.is_error_ = false;
		}

		constexpr Result& operator=(Result&& rhs) noexcept {
			is_error_ = rhs.is_error_;

			if (rhs.is_error_) {
				error_ = rhs.error_;
			}
			else {
				result_ = std::move(rhs.result_);
			}

			rhs.is_error_ = false;
		}

		Result(const Result&) = delete;
		Result& operator=(const Result&) = delete;

	public:
		constexpr bool is_ok() const noexcept {
			return !is_error_;
		}

		[[nodiscard]]
		Res unwrap() && noexcept {
			if (!is_ok()) {
				EH_PANIC(std::format("Called Result<>::unwrap() on an error value. Error code: {}. Description: {}", ErrorTypeTrait<Err>::stringify(error_.error_value), ErrorTypeTrait<Err>::description(error_.error_value)));
			}
			return std::move(result_);
		}

		[[nodiscard]]
		Res expect(std::string_view error) && noexcept {
			EH_ASSERT(is_ok(), error);

			return std::move(result_);
		}

		template<typename ErrorHandler>
		[[nodiscard]]
		Res unwrap_or_else(ErrorHandler&& fn) && noexcept {
			if (is_ok()) {
				return std::move(result_);
			}
			else {
				return std::move(fn(error_.error_value));
			}
		}

		template<typename FnOk, typename FnErr>
		void match(FnOk ok, FnErr err) && noexcept {
			if (is_ok()) {
				ok(std::move(result_));
			}
			else {
				err(error_.error_value);
			}
		}
	};
}
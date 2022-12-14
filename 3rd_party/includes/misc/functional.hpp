#pragma once

#include "meta.hpp"
#include <concepts>

namespace fn {
	template<typename... Lambdas>
	struct LambdaOverload : Lambdas... {
		using Lambdas::operator()...;
	};

	template<typename... Lambdas>
	constexpr LambdaOverload<Lambdas...> make_overload(Lambdas&&... lambdas) noexcept {
		return LambdaOverload(std::forward<Lambdas>(lambdas)...);
	}

	template<std::totally_ordered T, typename CompareFn, typename Proj>
		requires std::is_invocable_r_v<bool, CompareFn, T, T>
	constexpr auto compare(T&& value, CompareFn&& comp, Proj proj) noexcept {
		return[v = std::forward<T>(value), f = std::forward<CompareFn>(comp), proj]
		(const auto& element) noexcept(noexcept(std::invoke(std::declval<CompareFn>(), std::declval<T>(), std::declval<T>())))
			requires meta::IsMember<decltype(element), Proj>
		{
			return std::invoke(f, std::invoke(proj, element), v);
		};
	}

	template<std::totally_ordered T, typename Proj = std::identity>
	constexpr auto greater_than(T&& value, Proj proj = {}) noexcept {
		return compare(std::forward<T>(value), std::greater{}, proj);
	}

	template<std::totally_ordered T, typename Proj = std::identity>
	constexpr auto less_than(T&& value, Proj proj = {}) noexcept {
		return compare(std::forward<T>(value), std::less{}, proj);
	}

	template<std::totally_ordered T, typename Proj = std::identity>
	constexpr auto greater_eq_than(T&& value, Proj proj = {}) noexcept {
		return compare(std::forward<T>(value), std::greater_equal{}, proj);
	}

	template<std::totally_ordered T, typename Proj = std::identity>
	constexpr auto less_eq_than(T&& value, Proj proj = {}) noexcept {
		return compare(std::forward<T>(value), std::less_equal{}, proj);
	}

	template<std::totally_ordered T, typename Proj = std::identity>
	constexpr auto equal_to(T&& value, Proj proj = {}) noexcept {
		return compare(std::forward<T>(value), std::equal_to{}, proj);
	}
}

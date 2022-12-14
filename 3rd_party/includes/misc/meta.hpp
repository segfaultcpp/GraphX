#pragma once
#include <type_traits>

namespace meta
{
	template<typename...>
	struct List {};

	template<typename... Args>
	constexpr auto make_list() noexcept {
		return List<Args...>{};
	}

	template<typename List, typename... Args>
	struct AddElements;

	template<template<typename...> typename List, typename... Args1, typename... Args2>
	struct AddElements<List<Args1...>, Args2...> {
		using type = List<Args1..., Args2...>;
	};

	template<typename T, typename... Args>
	struct Any : std::disjunction<std::is_same<T, Args>...> {};

	template<typename T, typename... Params>
	concept Lambda = std::is_object_v<T> && !std::is_pointer_v<T> && std::is_invocable_v<T, Params...>;

	template<typename T, typename Proj, typename... Args>
	concept IsMember = requires(T obj, Proj proj, Args... args) {
		std::invoke(proj, obj, std::forward<Args>(args)...);
	};

	template<typename T, typename Proj, typename Ret, typename... Args>
	concept IsMemberR = requires(T obj, Proj proj, Args... args) {
		{ std::invoke(proj, obj, std::forward<Args>(args)...) } -> std::same_as<Ret>;
	};

	namespace details {
		template<typename T, typename... Args>
		struct SameAsAny {
			static constexpr bool value = (std::is_same_v<T, Args> || ...);
		};
	}

	template<typename... Args>
	inline constexpr bool same_as_any_v = details::SameAsAny<Args...>::value;

	template<typename... Args>
	concept SameAsAny = same_as_any_v<Args...>;

	namespace details {
		template<typename T, typename... Args>
		struct AllUnique {
			static constexpr bool value = !same_as_any_v<T, Args...> && AllUnique<Args...>::value;
		};

		template<typename T>
		struct AllUnique<T> {
			static constexpr bool value = true;
		};
	}

	template<typename... Args>
	inline constexpr bool all_unique_v = details::AllUnique<Args...>::value;

	template<typename... Args>
	concept AllUnique = all_unique_v<Args...>;
}

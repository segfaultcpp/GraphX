#pragma once
#include <type_traits>

namespace meta // *запрещенная в России экстримистская организация
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
}
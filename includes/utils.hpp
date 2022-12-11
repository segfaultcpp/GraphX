#pragma once

#include "types.hpp"
#include <ranges>
#include <vector>
#include <misc/functional.hpp>
#include <misc/utils.hpp>

namespace gx {
	template<rng::random_access_range ReqRng, rng::input_range SupRng, typename Comp = std::equal_to<>, typename Proj = std::identity>
	std::vector<usize> check_support(ReqRng&& requested, SupRng&& supported, Proj proj = {}, Comp&& comp = {}) noexcept {
		auto view = requested | utils::index() |
			rng::views::filter(
				[sup = std::forward<SupRng>(supported), comp, proj]
				(auto p) noexcept {
					return rng::find_if(sup, fn::compare(p.second, comp, proj)) == std::end(sup);
				}
			) |
			rng::views::transform(
				[](auto p) noexcept {
					return p.first;
				}
			);
		return std::vector<usize>(view.begin(), view.end());
	}
}
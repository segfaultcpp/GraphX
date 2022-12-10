#pragma once

#include "types.hpp"
#include <ranges>
#include <vector>
#include <misc/functional.hpp>

namespace gx {
	template<rng::random_access_range ReqRng, rng::input_range SupRng, typename Comp = std::equal_to<>, typename Proj = std::identity>
	std::vector<usize> check_support(ReqRng&& requested, SupRng&& supported, Proj proj = {}, Comp&& comp = {}) noexcept {
		std::vector<usize> unsupported_idx;

		if (std::size(requested) != 0) {
			for (usize i : rng::views::iota(0u, std::size(requested))) {
				auto it = rng::find_if(supported, fn::compare(requested[i], std::forward<Comp>(comp), proj));

				if (it == std::end(supported)) {
					unsupported_idx.push_back(i);
				}
			}
		}
		return unsupported_idx;
	}
}
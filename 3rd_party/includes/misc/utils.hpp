#pragma once

namespace utils {
	template<std::ranges::input_range Rng> requires std::ranges::view<Rng>
	class IndexView : public std::ranges::view_interface<IndexView<Rng>> {
	private:
		class Iterator_ : public std::input_iterator_tag {
		public:
			using RngIt = std::ranges::iterator_t<Rng>;
			using difference_type = std::iter_difference_t<RngIt>;
			using RngItRef = typename std::iterator_traits<RngIt>::reference;
			using value_type = std::pair<std::size_t, RngItRef>;

		private:
			std::size_t index_ = 0;
			RngIt iter_;

		public:
			constexpr Iterator_() noexcept = default;

			constexpr Iterator_(std::size_t index, RngIt it) noexcept
				: index_{ index }
				, iter_{ std::forward<RngIt>(it) }
			{}

			constexpr value_type operator*() const noexcept {
				return value_type(index_, *iter_);
			}

			constexpr Iterator_& operator++() noexcept {
				++index_;
				++iter_;
				return *this;
			}

			constexpr Iterator_ operator++(int) noexcept {
				Iterator_ temp = *this;
				++(*this);
				return temp;
			}

			constexpr bool operator==(const Iterator_& rhs) const noexcept {
				return iter_ == rhs.iter_;
			}

			constexpr bool operator!=(const Iterator_& rhs) const noexcept {
				return !(*this == rhs);
			}
		};
		static_assert(std::indirectly_readable<Iterator_>);

	private:
		Rng base_{};
		Iterator_ iter_;

	public:
		constexpr IndexView() noexcept = default;

		constexpr IndexView(Rng rng) noexcept
			: base_{ rng }
			, iter_{ 0u, std::begin(rng) }
		{}

		constexpr Rng base() const& noexcept {
			return base_;
		}

		constexpr Rng base() && noexcept {
			return std::move(base_);
		}

		constexpr Iterator_ begin() const noexcept {
			return iter_;
		}

		constexpr Iterator_ end() const noexcept
			requires std::ranges::sized_range<const Rng>
		{
			return Iterator_{ std::ranges::size(base_), std::end(base_) };
		}

		constexpr auto size() const noexcept
			requires std::ranges::sized_range<const Rng>
		{
			return std::ranges::size(base_);
		}
	};

	template<class Rng>
	IndexView(Rng&& base) -> IndexView<std::ranges::views::all_t<Rng>>;

	namespace details {
		struct IndexViewClosure
		{
			constexpr IndexViewClosure() = default;

			template <std::ranges::viewable_range Rng>
			constexpr auto operator()(Rng&& r) const noexcept {
				return IndexView(std::forward<Rng>(r));
			}
		};

		struct IndexViewRangeAdaptor
		{
			template<std::ranges::viewable_range Rng>
			constexpr auto operator ()(Rng&& r) const noexcept {
				return IndexView(std::forward<Rng>(r));
			}

			constexpr auto operator ()() const noexcept {
				return IndexViewClosure();
			}
		};

		template <std::ranges::viewable_range Rng>
		constexpr auto operator | (Rng&& r, const IndexViewClosure& v)
		{
			return v(std::forward<Rng>(r));
		}

		template<std::ranges::input_range Rng, typename Right>
		constexpr auto operator | (const IndexView<Rng>& left, Right&& right)
			noexcept(noexcept(std::forward<Right>(right)(left)))
			requires requires { std::forward<Right>(right)(left); }
		{
			return std::forward<Right>(right)(left);
		}
	}

	inline constexpr details::IndexViewRangeAdaptor index;
}

namespace std::ranges {
	template<std::ranges::input_range Rng>
	inline constexpr bool enable_borrowed_range<utils::IndexView<Rng>> = true;
}
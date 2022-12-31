#pragma once 

#include <concepts>

namespace gx {
	template<typename T>
	class ObjectOwner;

	namespace unsafe {
		template<typename ObjectType>
		void destroy(ObjectType) noexcept;

		template<typename W>
			requires std::derived_from<W, ObjectOwner<typename W::ObjectType>>
		[[nodiscard]]
		auto unwrap_native_handle(W& wrapper) noexcept -> typename W::ObjectType;
	}
}
#pragma once 

#include "error.hpp"
#include "object.hpp"
#include "instance.hpp"

#include <string_view>
#include <vector>
#include <span>
#include <array>
#include <ranges>
#include <iterator>
#include <format>

#include <vulkan/vulkan.h>

#include <misc/assert.hpp>
#include <misc/types.hpp>

namespace gx {
	namespace ext {
		enum class MessageSeverity : u8 {
			eDiagnostic = bit<u8, 0>(),
			eInfo = bit<u8, 1>(),
			eWarning = bit<u8, 2>(),
			eError = bit<u8, 3>(),
		};

		enum class MessageType : u8 {
			eGeneral = bit<u8, 0>(),
			eValidation = bit<u8, 1>(),
			ePerformance = bit<u8, 2>(),
		};

		OVERLOAD_BIT_OPS(MessageSeverity, u8);
		OVERLOAD_BIT_OPS(MessageType, u8);

		MessageSeverity message_severity_from_vk(VkDebugUtilsMessageSeverityFlagBitsEXT severity) noexcept;
		u8 message_type_from_vk(VkDebugUtilsMessageTypeFlagsEXT type) noexcept;
		constexpr VkDebugUtilsMessageSeverityFlagsEXT message_severity_to_vk(u8 from) noexcept;
		constexpr VkDebugUtilsMessageTypeFlagsEXT message_type_to_vk(u8 from) noexcept;

		struct CallbackData {
			std::string_view message;
		};

		template<typename T>
		concept DebugUtilsExtCallable = std::invocable<T, MessageSeverity, u8, CallbackData>;

		struct DebugUtilsExtDef : ExtensionTag {
			static constexpr std::array ext_names = { ExtensionList::ext_debug_utils };

			static PFN_vkCreateDebugUtilsMessengerEXT create_fn;
			static PFN_vkDestroyDebugUtilsMessengerEXT destroy_fn;

			static void load(VkInstance instance) noexcept {
				create_fn = std::bit_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
				EH_ASSERT(create_fn != nullptr, "Failed to get vkCreateDebugUtilsMessengerEXT function address");

				destroy_fn = std::bit_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
				EH_ASSERT(destroy_fn != nullptr, "Failed to get vkDestroyDebugUtilsMessengerEXT function address");
			}
		};
		inline PFN_vkCreateDebugUtilsMessengerEXT DebugUtilsExtDef::create_fn = nullptr;
		inline PFN_vkDestroyDebugUtilsMessengerEXT DebugUtilsExtDef::destroy_fn = nullptr;
		static_assert(ExtensionDef<DebugUtilsExtDef>);

		struct DebugUtilsExt : RegisterExtension<DebugUtilsExtDef, meta::List<ValidationLayerKhr>> {};
		static_assert(RegisteredExtension<DebugUtilsExt>);

		class DebugUtils : public ObjectOwner<DebugUtils, VkDebugUtilsMessengerEXT> {
		private:
			// Static functions
			//static VKAPI_ATTR VkBool32 VKAPI_CALL
			//	debug_callback(
			//		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
			//		VkDebugUtilsMessageTypeFlagsEXT type,
			//		const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
			//		void* pUserData) noexcept
			//{
			//	CallbackData data = {
			//		.message = callback_data->pMessage
			//	};
			//	Fn{}(message_severity_from_vk(severity), message_type_from_vk(type), data);
			//	return VK_FALSE;
			//}

		public:
			// Friends
			friend struct ::gx::unsafe::ObjectOwnerTrait<DebugUtils>;

			// Type aliases
			using Base = ObjectOwner<DebugUtils, VkDebugUtilsMessengerEXT>;
			using ObjectType = VkDebugUtilsMessengerEXT;

			// Constructors/destructor
			DebugUtils() noexcept = default;

			DebugUtils(VkDebugUtilsMessengerEXT debug_utils, VkInstance instance) noexcept
				: Base{ debug_utils }
				, parent_{ instance }
			{}

			DebugUtils(DebugUtils&&) noexcept = default;
			DebugUtils(const DebugUtils&) = delete;

			~DebugUtils() noexcept = default;

			// Assignment operators
			DebugUtils& operator=(DebugUtils&&) noexcept = default;
			DebugUtils& operator=(const DebugUtils&) = delete;

		private:
			VkInstance parent_;
		};

		template<ext::ExtensionDef... Exts, ext::Layer... Lyrs>
			requires meta::SameAsAny<DebugUtilsExtDef, Exts...>
		Result<DebugUtils> make_debug_utils(const Instance<meta::List<Exts...>, meta::List<Lyrs...>>& instance) noexcept {

		}
	}

	namespace unsafe {
		template<>
		struct ObjectOwnerTrait<::gx::ext::DebugUtils> {
			static void destroy(::gx::ext::DebugUtils& obj) noexcept {
				ext::DebugUtilsExtDef::destroy_fn(obj.parent_, obj.handle_, nullptr);
			}

			[[nodiscard]]
			static VkDebugUtilsMessengerEXT unwrap_native_handle(::gx::ext::DebugUtils& obj) noexcept {
				auto ret = obj.handle_;
				obj.handle_ = VK_NULL_HANDLE;
				return ret;
			}
		};
	}
}
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
		VkDebugUtilsMessageSeverityFlagsEXT message_severity_to_vk(u8 from) noexcept;
		VkDebugUtilsMessageTypeFlagsEXT message_type_to_vk(u8 from) noexcept;

		struct CallbackData {
			std::string_view message;
		};

		template<typename T>
		concept DebugUtilsExtCallable = std::invocable<T, MessageSeverity, u8, CallbackData> && std::is_object_v<T> && !std::is_pointer_v<T>;

		struct DebugUtilsExtDef : ExtensionTag {
			static constexpr std::array ext_names = { ExtensionList::ext_debug_utils };

			static details::MakeCreateExtFn<PFN_vkCreateDebugUtilsMessengerEXT> create_fn;
			static details::MakeDestroyExtFn<PFN_vkDestroyDebugUtilsMessengerEXT> destroy_fn;

			static void load(VkInstance instance) noexcept {
				auto cfn = std::bit_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
				EH_ASSERT(cfn != nullptr, "Failed to get vkCreateDebugUtilsMessengerEXT function address");

				auto dfn = std::bit_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
				EH_ASSERT(dfn != nullptr, "Failed to get vkDestroyDebugUtilsMessengerEXT function address");
			
				create_fn = details::MakeCreateExtFn<PFN_vkCreateDebugUtilsMessengerEXT>{ instance, cfn };
				destroy_fn = details::MakeDestroyExtFn<PFN_vkDestroyDebugUtilsMessengerEXT>{ instance, dfn };
			}
		};
		
		inline details::MakeCreateExtFn<PFN_vkCreateDebugUtilsMessengerEXT> DebugUtilsExtDef::create_fn{};
		inline details::MakeDestroyExtFn<PFN_vkDestroyDebugUtilsMessengerEXT> DebugUtilsExtDef::destroy_fn{};

		static_assert(ExtensionDef<DebugUtilsExtDef>);

		struct DebugUtilsExt : RegisterExtension<DebugUtilsExtDef, meta::List<ValidationLayerKhr>> {};
		static_assert(RegisteredExtension<DebugUtilsExt>);

		class DebugUtils : public ObjectOwner<DebugUtils, VkDebugUtilsMessengerEXT> {
		public:
			// Friends
			friend struct ::gx::unsafe::ObjectOwnerTrait<DebugUtils>;

			// Type aliases
			using Base = ObjectOwner<DebugUtils, VkDebugUtilsMessengerEXT>;
			using ObjectType = VkDebugUtilsMessengerEXT;

			// Constructors/destructor
			DebugUtils() noexcept = default;

			DebugUtils(VkDebugUtilsMessengerEXT debug_utils) noexcept
				: Base{ debug_utils }
			{}

			DebugUtils(DebugUtils&&) noexcept = default;
			DebugUtils(const DebugUtils&) = delete;

			~DebugUtils() noexcept = default;

			// Assignment operators
			DebugUtils& operator=(DebugUtils&&) noexcept = default;
			DebugUtils& operator=(const DebugUtils&) = delete;
		};

		namespace details {
			template<typename Fn>
			static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
				VkDebugUtilsMessageSeverityFlagBitsEXT severity,
				VkDebugUtilsMessageTypeFlagsEXT type,
				const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
				void* pUserData) noexcept
			{
				CallbackData data = {
					.message = callback_data->pMessage
				};
				Fn{}(message_severity_from_vk(severity), message_type_from_vk(type), data);
				return VK_FALSE;
			}
		}

		template<DebugUtilsExtCallable Fn, ExtensionDef... Exts, Layer... Lyrs>
			requires meta::SameAsAny<DebugUtilsExtDef, Exts...>
		Result<DebugUtils> make_debug_utils([[maybe_unused]] const Instance<meta::List<Exts...>, meta::List<Lyrs...>>& unused, u8 severity_flags, u8 type_flags) noexcept {
			auto vk_msg_severity = message_severity_to_vk(severity_flags);
			auto vk_msg_type = message_type_to_vk(type_flags);

			EH_ASSERT(vk_msg_severity != 0, "Passed invalid argument. severity_flags must not be 0");
			EH_ASSERT(vk_msg_type != 0, "Passed invalid argument. type_flags must not be 0");
			
			VkDebugUtilsMessengerCreateInfoEXT create_info = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
				.messageSeverity = vk_msg_severity,
				.messageType = vk_msg_type,
				.pfnUserCallback = &details::debug_callback<Fn>,
				.pUserData = nullptr,
			};

			VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;
			VkResult res = DebugUtilsExtDef::create_fn(&create_info, nullptr, &messenger);

			if (res == VK_SUCCESS) {
				return DebugUtils{ messenger };
			}

			return eh::Error{ convert_vk_result(res) };
		}
	}

	namespace unsafe {
		template<>
		struct ObjectOwnerTrait<::gx::ext::DebugUtils> {
			static void destroy(::gx::ext::DebugUtils& obj) noexcept {
				ext::DebugUtilsExtDef::destroy_fn(obj.handle_);
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
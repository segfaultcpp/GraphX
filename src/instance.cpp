#include <instance.hpp>

namespace gx {
	InstanceInfo::InstanceInfo() noexcept {
		u32 count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
		supported_extensions.resize(count);
		vkEnumerateInstanceExtensionProperties(nullptr, &count, supported_extensions.data());

		count = 0;
		vkEnumerateInstanceLayerProperties(&count, nullptr);
		supported_layers.resize(count);
		vkEnumerateInstanceLayerProperties(&count, supported_layers.data());
	}
}
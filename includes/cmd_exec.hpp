#pragma once 

#include <limits>
#include <vector>
#include <tuple>

#include <vulkan/vulkan.h>

#include <misc/meta.hpp>
#include <misc/types.hpp>
#include <misc/result.hpp>
#include <misc/assert.hpp>

#include "object.hpp"
#include "error.hpp"

namespace gx {
	enum class QueueTypes {
		eGraphics = 0,
		eTransfer,
		eCompute,
	};

	struct CmdListTag{};

	template<typename T>
	concept CommandContext = std::derived_from<T, CmdListTag> && requires(QueueTypes type) {
		type = T::queue_type;
	};

	class GraphicsContext : public CmdListTag, ManagedObject<::gx::Copyable, VkCommandBuffer> {
	public:
		// Type aliases
		using Base = ManagedObject<::gx::Copyable, VkCommandBuffer>;
		using ObjectType = VkCommandBuffer;

		// Constant expressions
		static constexpr QueueTypes queue_type = QueueTypes::eGraphics;
		
		// Constructors/destructor
		GraphicsContext() noexcept = default;

		GraphicsContext(VkCommandBuffer cmd) noexcept 
			: Base{ cmd }
		{}

		GraphicsContext(const GraphicsContext&) noexcept = default;
		GraphicsContext(GraphicsContext&&) noexcept = default;

		~GraphicsContext() noexcept = default;

		// Assignment operators
		GraphicsContext& operator=(const GraphicsContext&) noexcept = default;
		GraphicsContext& operator=(GraphicsContext&&) noexcept = default;
	};
	static_assert(CommandContext<GraphicsContext>);

	class ComputeContext : public CmdListTag, ManagedObject<::gx::Copyable, VkCommandBuffer> {
	public:
		// Type aliases
		using Base = ManagedObject<::gx::Copyable, VkCommandBuffer>;
		using ObjectType = VkCommandBuffer;

		// Constant expressions
		static constexpr QueueTypes queue_type = QueueTypes::eCompute;

		// Constructors/destructor
		ComputeContext() noexcept = default;

		ComputeContext(VkCommandBuffer cmd) noexcept
			: Base{ cmd }
		{}

		ComputeContext(const ComputeContext&) noexcept = default;
		ComputeContext(ComputeContext&&) noexcept = default;

		~ComputeContext() noexcept = default;

		// Assignment operators
		ComputeContext& operator=(const ComputeContext&) noexcept = default;
		ComputeContext& operator=(ComputeContext&&) noexcept = default;
	};
	static_assert(CommandContext<ComputeContext>);
	
	class TransferContext : public CmdListTag, ManagedObject<::gx::Copyable, VkCommandBuffer> {
	public:
		// Type aliases
		using Base = ManagedObject<::gx::Copyable, VkCommandBuffer>;
		using ObjectType = VkCommandBuffer;

		// Constant expressions
		static constexpr QueueTypes queue_type = QueueTypes::eTransfer;
	
		// Constructors/destructor
		TransferContext() noexcept = default;

		TransferContext(VkCommandBuffer cmd) noexcept
			: Base{ cmd }
		{}

		TransferContext(const TransferContext&) noexcept = default;
		TransferContext(TransferContext&&) noexcept = default;

		~TransferContext() noexcept = default;

		// Assignment operators
		TransferContext& operator=(const TransferContext&) noexcept = default;
		TransferContext& operator=(TransferContext&&) noexcept = default;
	};
	static_assert(CommandContext<TransferContext>);

	template<CommandContext Ctx>
	class CommandPool : public ObjectOwner<CommandPool<Ctx>, VkCommandPool> {
	private:
		// Friends
		friend struct unsafe::ObjectOwnerTrait<CommandPool>;
	
		// Fields
		VkDevice owner_;

	public:
		// Type aliases
		using Base = ObjectOwner<CommandPool, VkCommandPool>;
		using ObjectType = VkCommandPool;

		// Constructors/destructor
		CommandPool() noexcept = default;
		CommandPool(VkCommandPool pool, VkDevice device) noexcept 
			: Base{ pool }
			, owner_{ device }
		{}

		CommandPool(CommandPool&&) noexcept = default;
		CommandPool(const CommandPool&) = delete;

		~CommandPool() noexcept = default;

		// Assignment operators
		CommandPool& operator=(CommandPool&&) noexcept = default;
		CommandPool& operator=(const CommandPool&) = delete;

	public:
		// Methods
		[[nodiscard]]
		Result<std::vector<Ctx>> create_command_context(usize count = 1u) const noexcept {
			EH_ASSERT(owner_ != VK_NULL_HANDLE, "VkDevice handle must be a valid value");

			VkCommandBufferAllocateInfo alloc_info = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.commandPool = this->handle_,
				.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, // TODO: secondary buffers support
				.commandBufferCount = static_cast<u32>(count),
			};

			std::vector<VkCommandBuffer> buffers(count);
			auto res = vkAllocateCommandBuffers(owner_, &alloc_info, buffers.data());

			if (res == VK_SUCCESS) {
				std::vector<Ctx> ctxs(count);
				std::ranges::copy(buffers, ctxs.begin());
				return ctxs;
			}

			return eh::Error{ convert_vk_result(res) };
		}

		/*
		* Returns a memory used by all allocated command contexts to the command pool.
		* If release_resources is true, the memory will be returned to the system.
		*/
		[[nodiscard]]
		ErrorCode reset(bool release_resources = false) noexcept {
			EH_ASSERT(owner_ != VK_NULL_HANDLE, "VkDevice handle must be a valid value");
			EH_ASSERT(this->handle_ != VK_NULL_HANDLE, "VkCommandPool must be a valid value");
			auto res = vkResetCommandPool(owner_, this->handle_, release_resources ? VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT : 0);
			return convert_vk_result(res);
		}
		
		/*
		* Returns a memory used by specified command contexts to the command pool.
		*/
		void free_command_contexts(std::span<Ctx> ctxs) noexcept {
			EH_ASSERT(owner_ != VK_NULL_HANDLE, "VkDevice handle must be a valid value");
			EH_ASSERT(this->handle_ != VK_NULL_HANDLE, "VkCommandPool must be a valid value");
			vkFreeCommandBuffers(owner_, this->handle_, ctxs.size(), ctxs.data());
		}
	};

	namespace unsafe {
		template<CommandContext Ctx>
		struct ObjectOwnerTrait<CommandPool<Ctx>> {
			static void destroy(CommandPool<Ctx>& obj) noexcept {
				vkDestroyCommandPool(obj.owner_, obj.handle_, nullptr);
			}

			[[nodiscard]]
			static VkCommandPool unwrap_native_handle(CommandPool<Ctx>& obj) noexcept {
				auto ret = obj.handle_;
				obj.handle_ = VK_NULL_HANDLE;
				return ret;
			}
		};
	}

	/*
	* WARNING: It's not designed for usage in multithreded context!
	* Only one thread can use an instance of this class at a time.
	*/
	template<CommandContext... Ctxs>
	class CommandPoolFactory {
	private:
	};

	template<CommandContext Ctx>
	struct QueueBase : public ManagedObject<::gx::MoveOnly, VkQueue> {
	public:
		// Type aliases
		using Base = ManagedObject<::gx::MoveOnly, VkQueue>;
		using ObjectType = VkQueue;

	protected:
		// Fields
		usize family_index_ = static_cast<usize>(~0u);

	public:
		// Constructors/destuctor
		QueueBase() noexcept = default;

		QueueBase(VkQueue q, usize index) noexcept
			: Base{ q }
			, family_index_{ index }
		{}

		QueueBase(QueueBase&& rhs) noexcept
			: Base{ std::move(rhs) }
			, family_index_{ rhs.family_index_ }
		{
			rhs.family_index_ = static_cast<usize>(~0u);
		}

		QueueBase(const QueueBase&) = delete;

		~QueueBase() noexcept = default;

		// Assignment operator
		QueueBase& operator=(QueueBase&& rhs) noexcept {
			static_cast<Base&>(*this) = std::move(rhs);

			family_index_ = rhs.family_index_;
			rhs.family_index_ = static_cast<usize>(~0u);

			return *this;
		}

		QueueBase& operator=(const QueueBase&) = delete;

	public:
		// Methods
		void submit(Ctx) noexcept {}
		
		// Getters/setters
		[[nodiscard]]
		usize get_family_index() const noexcept {
			return family_index_;
		}

	};

	template<typename T>
	concept Queue =
		std::movable<T> &&
		std::derived_from<T, QueueBase<typename T::CmdCtx>>;

	class GraphicsQueue : public QueueBase<GraphicsContext> {
	public:
		// Type aliases
		using CmdCtx = GraphicsContext;

		// Constructors/destructor
		GraphicsQueue() noexcept = default;

		GraphicsQueue(VkQueue q, usize index) noexcept
			: QueueBase{ q, index }
		{}

		GraphicsQueue(GraphicsQueue&&) noexcept = default;
		GraphicsQueue(const GraphicsQueue&) = delete;
		
		~GraphicsQueue() noexcept = default;

		// Assignment operators
		GraphicsQueue& operator=(GraphicsQueue&&) noexcept = default;
		GraphicsQueue& operator=(const GraphicsQueue&) = delete;
	};
	static_assert(Queue<GraphicsQueue>);

	class ComputeQueue : public QueueBase<ComputeContext> {
	public:
		// Type aliases
		using CmdCtx = ComputeContext;
	
		// Constructors/destructor
		ComputeQueue() noexcept = default;

		ComputeQueue(VkQueue q, usize index) noexcept 
			: QueueBase{ q, index }
		{}

		ComputeQueue(ComputeQueue&&) noexcept = default;
		ComputeQueue(const ComputeQueue&) = delete;

		~ComputeQueue() noexcept = default;

		// Assignment operators
		ComputeQueue& operator=(ComputeQueue&&) noexcept = default;
		ComputeQueue& operator=(const ComputeQueue&) = delete;
	};
	static_assert(Queue<ComputeQueue>);

	class TransferQueue : public QueueBase<TransferContext> {
	public:
		// Type aliases
		using CmdCtx = TransferContext;

		// Constructors/destructor
		TransferQueue() noexcept = default;

		TransferQueue(VkQueue q, usize index) noexcept
			: QueueBase{ q, index }
		{}

		TransferQueue(TransferQueue&&) noexcept = default;
		TransferQueue(const TransferQueue&) = delete;

		~TransferQueue() noexcept = default;

		// Assignment operators
		TransferQueue& operator=(TransferQueue&&) noexcept = default;
		TransferQueue& operator=(const TransferQueue&) = delete;
	};
	static_assert(Queue<TransferQueue>);

	template<Queue... Qs>
		requires meta::AllUnique<Qs...>
	class CommandExecutor {
		std::tuple<std::vector<Qs>...> queues_;
	};
}
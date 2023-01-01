#pragma once 

#include <limits>
#include <vector>
#include <tuple>

#include <vulkan/vulkan.h>

#include <misc/meta.hpp>
#include <misc/types.hpp>

#include "object.hpp"

namespace gx {
	class CmdListBase : public ManagedObject<::gx::Copyable, VkCommandBuffer> {
	public:
		using Base = ManagedObject<::gx::Copyable, VkCommandBuffer>;
		using ObjectType = VkCommandBuffer;

	public:
		CmdListBase() noexcept = default;

		CmdListBase(VkCommandBuffer cmd) noexcept 
			: Base{ cmd } 
		{}

		CmdListBase(const CmdListBase&) noexcept = default;
		CmdListBase& operator=(const CmdListBase&) noexcept = default;

		CmdListBase(CmdListBase&&) noexcept = default;
		CmdListBase& operator=(CmdListBase&&) noexcept = default;
	};

	template<typename T>
	concept CommandContext = std::derived_from<T, CmdListBase>;

	class GraphicsContext : public CmdListBase {
	public:
		GraphicsContext() noexcept = default;

		GraphicsContext(VkCommandBuffer cmd) noexcept 
			: CmdListBase{ cmd }
		{}

		GraphicsContext(const GraphicsContext&) noexcept = default;
		GraphicsContext& operator=(const GraphicsContext&) noexcept = default;
	};
	static_assert(CommandContext<GraphicsContext>);

	class ComputeContext : public CmdListBase {
	public:
		ComputeContext() noexcept = default;

		ComputeContext(VkCommandBuffer cmd) noexcept
			: CmdListBase{ cmd }
		{}

		ComputeContext(const ComputeContext&) noexcept = default;
		ComputeContext& operator=(const ComputeContext&) noexcept = default;
	};
	static_assert(CommandContext<ComputeContext>);
	
	class TransferContext : public CmdListBase {
	public:
		TransferContext() noexcept = default;

		TransferContext(VkCommandBuffer cmd) noexcept
			: CmdListBase{ cmd }
		{}

		TransferContext(const TransferContext&) noexcept = default;
		TransferContext& operator=(const TransferContext&) noexcept = default;
	};
	static_assert(CommandContext<TransferContext>);

	template<CommandContext Ctx>
	class CommandPool : public ObjectOwner<CommandPool<Ctx>, VkCommandPool> {
		friend struct unsafe::ObjectOwnerTrait<CommandPool>;
	private:
		VkDevice owner_;

	public:
		using Base = ObjectOwner<CommandPool, VkCommandPool>;
		using ObjectType = VkCommandPool;

	public:
		CommandPool() noexcept = default;
		CommandPool(VkCommandPool pool, VkDevice device) noexcept 
			: Base{ pool }
			, owner_{ device }
		{}

		CommandPool(CommandPool&&) noexcept = default;
		CommandPool& operator=(CommandPool&&) noexcept = default;

		~CommandPool() noexcept = default;

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
	class CommandPoolFactory {};

	template<typename CmdCtx>
	struct QueueBase : public ManagedObject<::gx::MoveOnly, VkQueue> {
	public:
		using Base = ManagedObject<::gx::MoveOnly, VkQueue>;
		using ObjectType = VkQueue;

	protected:
		usize family_index_ = static_cast<usize>(~0u);

	public:
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

		QueueBase& operator=(QueueBase&& rhs) noexcept {
			static_cast<Base&>(*this) = std::move(rhs);

			family_index_ = rhs.family_index_;
			rhs.family_index_ = static_cast<usize>(~0u);

			return *this;
		}

	public:
		void submit(CmdCtx) noexcept {}

	public:
		[[nodiscard]]
		usize get_family_index() const noexcept {
			return family_index_;
		}

	};

	template<typename T>
	concept Queue =
		std::move_constructible<T> &&
		std::derived_from<T, QueueBase<typename T::CmdCtx>>;

	class GraphicsQueue : public QueueBase<GraphicsContext> {
	public:
		using CmdCtx = GraphicsContext;

	public:
		GraphicsQueue() noexcept = default;

		GraphicsQueue(VkQueue q, usize index) noexcept
			: QueueBase{ q, index }
		{}

		GraphicsQueue(GraphicsQueue&&) noexcept = default;
		GraphicsQueue& operator=(GraphicsQueue&&) noexcept = default;
	};
	static_assert(Queue<GraphicsQueue>);

	class ComputeQueue : public QueueBase<ComputeContext> {
	public:
		using CmdCtx = ComputeContext;
	
	public:
		ComputeQueue() noexcept = default;

		ComputeQueue(VkQueue q, usize index) noexcept 
			: QueueBase{ q, index }
		{}

		ComputeQueue(ComputeQueue&&) noexcept = default;
		ComputeQueue& operator=(ComputeQueue&&) noexcept = default;
	};
	static_assert(Queue<ComputeQueue>);

	class TransferQueue : public QueueBase<TransferContext> {
	public:
		using CmdCtx = TransferContext;

	public:
		TransferQueue() noexcept = default;

		TransferQueue(VkQueue q, usize index) noexcept
			: QueueBase{ q, index }
		{}

		TransferQueue(TransferQueue&&) noexcept = default;
		TransferQueue& operator=(TransferQueue&&) noexcept = default;
	};
	static_assert(Queue<TransferQueue>);

	template<Queue... Qs>
		requires meta::AllUnique<Qs...>
	class CommandExecutor {
		std::tuple<std::vector<Qs>...> queues_;
	};
}
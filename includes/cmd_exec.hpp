#pragma once 

#include <limits>
#include <vector>
#include <tuple>

#include <vulkan/vulkan.h>
#include <misc/meta.hpp>

#include "types.hpp"

namespace gx {
	class CmdListBase {
	protected:
		VkCommandBuffer buffer_ = VK_NULL_HANDLE;

	public:
		CmdListBase() noexcept = default;

		CmdListBase(VkCommandBuffer cmd) noexcept 
			: buffer_{ cmd } 
		{}

		CmdListBase(const CmdListBase&) noexcept = default;
		CmdListBase& operator=(const CmdListBase&) noexcept = default;
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

	/*
	* WARNING: It's not designed for usage in multithreded context!
	* Only one thread can use an instance of this class at a time.
	*/
	class CommandPoolFactory {};

	template<typename CmdCtx>
	struct QueueBase {
	protected:
		VkQueue queue_ = VK_NULL_HANDLE;
		usize family_index_ = static_cast<usize>(~0u);

	public:
		QueueBase() noexcept = default;

		QueueBase(VkQueue q, usize index) noexcept
			: queue_{ q }
			, family_index_{ index }
		{}

		QueueBase(QueueBase&& rhs) noexcept
			: queue_{ rhs.queue_ }
			, family_index_{ rhs.family_index_ }
		{
			rhs.queue_ = VK_NULL_HANDLE;
			rhs.family_index_ = static_cast<usize>(~0u);
		}

		QueueBase& operator=(QueueBase&& rhs) noexcept {
			queue_ = rhs.queue_;
			rhs.queue_ = VK_NULL_HANDLE;

			family_index_ = rhs.family_index_;
			rhs.family_index_ = static_cast<usize>(~0u);

			return *this;
		}

		QueueBase(const QueueBase&) = delete;
		QueueBase& operator=(const QueueBase&) = delete;

	public:
		void submit(CmdCtx) noexcept {}

	public:
		VkQueue get_native_handle() const noexcept {
			return queue_;
		}

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
#pragma once

#include <misc/meta.hpp>

#include <vulkan/vulkan.h>

#include "unsafe.hpp"

namespace gx {
	template<typename T>
	concept Wrapper = requires {
		typename T::Base;
		typename T::ObjectType;
	};

	template<typename T>
	class MoveOnly {
	protected:
		T handle_ = VK_NULL_HANDLE;
	
	public:
		MoveOnly() noexcept = default;

		explicit MoveOnly(T handle) noexcept 
			: handle_{ handle }
		{}

		MoveOnly(MoveOnly&& rhs) noexcept 
			: handle_{ rhs.handle_ }
		{
			rhs.handle_ = VK_NULL_HANDLE;
		}

		MoveOnly& operator=(MoveOnly&& rhs) noexcept {
			handle_ = rhs.handle_;
			rhs.handle_ = VK_NULL_HANDLE;
			return *this;
		}

		MoveOnly(const MoveOnly&) = delete;
		MoveOnly& operator=(const MoveOnly&) = delete;

		~MoveOnly() noexcept = default;
	};

	template<typename T>
	class Copyable {
	protected:
		T handle_ = VK_NULL_HANDLE;

	public:
		Copyable() noexcept = default;

		explicit Copyable(T handle) noexcept 
			: handle_{ handle }
		{}

		Copyable(const Copyable&) noexcept = default;
		Copyable& operator=(const Copyable&) noexcept = default;

		Copyable(Copyable&& rhs) noexcept
			: handle_{ rhs.handle_ }
		{
			rhs.handle_ = VK_NULL_HANDLE;
		}

		Copyable& operator=(Copyable&& rhs) noexcept {
			handle_ = rhs.handle_;
			rhs.handle_ = VK_NULL_HANDLE;
			return *this;
		}

		~Copyable() noexcept = default;
	};

	template<typename T>
	class ObjectOwner : public MoveOnly<T> {
	public:
		ObjectOwner() noexcept = default;

		explicit ObjectOwner(T handle) noexcept 
			: MoveOnly<T>{ handle }
		{}

		ObjectOwner(ObjectOwner&&) noexcept = default;
		ObjectOwner& operator=(ObjectOwner&&) noexcept = default;

		~ObjectOwner() noexcept {
			unsafe::destroy(this->handle_);
		}
	};

	template<template<typename> typename OwnershipPolicy, typename T>
		requires meta::SameAsAny<OwnershipPolicy<T>, MoveOnly<T>, Copyable<T>>
	class ManagedObject : public OwnershipPolicy<T> {
	public:
		ManagedObject() noexcept = default;

		explicit ManagedObject(T handle) noexcept 
			: OwnershipPolicy<T>{ handle }
		{}

		ManagedObject(const ManagedObject&) noexcept = default;
		ManagedObject& operator=(const ManagedObject&) noexcept = default;

		ManagedObject(ManagedObject&&) noexcept = default;
		ManagedObject& operator=(ManagedObject&&) noexcept = default;

		~ManagedObject() noexcept = default;

	public:
		// Unsafe
		[[nodiscard]]
		T get_native_handle() const noexcept {
			return this->handle_;
		}
	};

	template<typename T>
	class ObjectView : public Copyable<T> {
	public:
		ObjectView() noexcept = default;

		explicit ObjectView(T handle) noexcept 
			: Copyable<T>{ handle }
		{}

		~ObjectView() noexcept = default;
	};
}
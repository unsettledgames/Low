#pragma once

namespace Low
{
	template <typename T>
	using Ref = std::shared_ptr<T>;

	template <typename T, typename ... Args>
	constexpr Ref<T> CreateRef(Args&& ...args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	template <typename T, typename ... Args>
	constexpr Ref<T> CreateScope(Args&& ...args)
	{
		return std::unique_ptr<T>(std::forward<Args>(args)...);
	}

	template <typename T>
	constexpr Ref<T> WrapRef(T* p)
	{
		return std::shared_ptr<T>(p);
	}

	template <typename T>
	constexpr Ref<T> WrapScope(T* p)
	{
		return std::unique_ptr<T>(p);
	}
}
#pragma once

#include <future>
#include "Types.h"
#include "thread_pool/thread_pool.h"


class ThreadPool
{
public:

	static void Init(uint32_t threadCount)
	{
		static ThreadPool threadPool{};
		auto& poolImpl{ threadPool.GetThreadPool() };
		if (poolImpl->size() != threadCount)
		{
			poolImpl = std::make_shared<powe::ThreadPool>(threadCount);
		}
	}

	template<typename Func, typename... Args>
	static std::future<void> Enqueue(Func&& func, Args&&... args)
	{
		static ThreadPool threadPool{};
		return threadPool.GetThreadPool()->enqueue(std::forward<Func>(func), std::forward<Args>(args)...);
	}

	template<typename Func, typename... Args>
	static void EnqueueDetach(Func&& func, Args&&... args)
	{
		static ThreadPool threadPool{};
		threadPool.GetThreadPool()->enqueue_detach(std::forward<Func>(func), std::forward<Args>(args)...);
	}

private:

	ThreadPool()
		: m_ThreadPool(std::make_shared<powe::ThreadPool>(std::thread::hardware_concurrency()))
	{
	}

	std::shared_ptr<powe::ThreadPool>& GetThreadPool() { return m_ThreadPool; }

	std::shared_ptr<powe::ThreadPool> m_ThreadPool;
};


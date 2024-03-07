#pragma once

#include <future>
#include <iostream>
#include "Types.h"
#include "thread_pool/thread_pool.h"


class ThreadPool
{
public:

	using threadImpl = dp::thread_pool<std::function<void()>, std::jthread>;

	static void Init(uint32_t threadCount)
	{
		auto& instance = ThreadPool::GetInstance();
		auto& threadPool = instance.GetThreadPoolImpl();
		if (threadPool->size() != threadCount)
		{
			threadPool = std::make_shared<threadImpl>(threadCount);
		}
	}

	template<typename Func, typename... Args>
	static std::future<std::invoke_result_t<Func, Args...>> Enqueue(Func&& func, Args&&... args)
	{
		auto& instance{ GetInstance() };
		auto& threadPool{ instance.GetThreadPoolImpl() };
		return threadPool->enqueue(std::forward<Func>(func), std::forward<Args>(args)...);
	}

	template<typename Func, typename... Args>
	static void EnqueueDetach(Func&& func, Args&&... args)
	{
		auto& instance{ GetInstance() };
		auto& threadPool{ instance.GetThreadPoolImpl() };
		threadPool->enqueue_detach(std::forward<Func>(func), std::forward<Args>(args)...);
	}

private:

	ThreadPool()
		: m_ThreadPool(std::make_shared<threadImpl>(std::thread::hardware_concurrency()))
	{
	}

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

	static ThreadPool& GetInstance()
	{
		static ThreadPool threadPool{};
		return threadPool;
	}

	std::shared_ptr<threadImpl>& GetThreadPoolImpl() { return m_ThreadPool; }

	std::shared_ptr<threadImpl> m_ThreadPool;
};


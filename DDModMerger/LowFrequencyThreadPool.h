#pragma once
#include "Types.h"
#include "thread_pool/thread_pool.h"

#include <barrier>

/// <summary>
/// This implementation of thread pool is loosely way to implement dynamic resizing thread pool
/// Don't follow this meothod for professional use
/// </summary>
class LowFrequencyThreadPool
{
public:

	using threadImpl = dp::thread_pool<std::function<void()>, std::jthread>;

	static void Init(uint32_t threadCount)
	{
		auto& instance = LowFrequencyThreadPool::GetInstance();
		auto& threadPool = instance.GetThreadPoolImpl();
		if (threadPool->size() != threadCount)
		{
			threadPool = std::make_shared<threadImpl>(threadCount);
		}
	}

	static uint32_t Size()
	{
		auto& instance{ GetInstance() };
		return static_cast<uint32_t>(instance.GetThreadPoolImpl()->size());
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

	LowFrequencyThreadPool()
		: m_ThreadPool(std::make_shared<threadImpl>(std::thread::hardware_concurrency()))
	{
	}

	LowFrequencyThreadPool(const LowFrequencyThreadPool&) = delete;
	LowFrequencyThreadPool& operator=(const LowFrequencyThreadPool&) = delete;

	static LowFrequencyThreadPool& GetInstance()
	{
		static LowFrequencyThreadPool threadPool{};
		return threadPool;
	}

	std::shared_ptr<threadImpl>& GetThreadPoolImpl() { return m_ThreadPool; }

	std::shared_ptr<threadImpl> m_ThreadPool;
};


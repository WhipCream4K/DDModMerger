#pragma once

#include <memory>
#include <optional>
#include <stdexcept>

namespace powe
{
	// Lock-Free queue using FIFO context
	template<typename T>
	class LFQueue
	{
		struct Node
		{
			Node(T&& data)
				: tag()
				, pNextNode()
				, data(std::move(data))
			{
			}

			Node(const T& param)
				: tag()
				, pNextNode()
				, data(param)
			{
			}

			Node(const Node&) = delete;
			Node& operator=(const Node&) = delete;
			Node(Node&&) = delete;
			Node& operator=(Node&&) = delete;

			int tag;
			std::shared_ptr<Node> pNextNode;
			T data;
		};

	public:

		void Push(T&& data) noexcept;
		void Push(const T& data) noexcept;

		[[nodiscard]] std::optional<T> PopReturn(); // maybe has data or not c++17
		T& Front();
		void Pop() noexcept;
		bool Empty() const noexcept;

		LFQueue()
			: m_Tail(std::make_shared<Node>(T{}))
		{
			std::shared_ptr<Node> ref{ m_Tail.load() };
			m_Head.store(m_Tail.load());
		}

		~LFQueue();

	protected:

		//https://github.com/darkautism/lfqueue/blob/HP/lfq.h
		// his lock free queue version separate head and tail pointer in different cache line
		// to prevent content or false-sharing to which I agree since the push and pop
		// function happens separately and is local to the thread
		alignas(64) std::atomic<std::shared_ptr<Node>> m_Head; // since c++20
		char pad[64 - sizeof(std::atomic<std::shared_ptr<Node>>)];
		alignas(64) std::atomic<std::shared_ptr<Node>> m_Tail; // since c++20
	};

	template <typename T>
	void LFQueue<T>::Push(T&& data) noexcept
	{
		const std::shared_ptr<Node> newNode{ std::make_shared<Node>(std::forward<T>(data)) };
		std::shared_ptr<Node> oldTail{ m_Tail.load() };

		while (!m_Tail.compare_exchange_weak(oldTail, newNode))
		{
		}

		// when we pass this line it means that we won the thread race any manipulation to the m_Tail
		// doesn't matter, so we can assign our newNode which is local to the caller to the
		// pNextNode of m_Tail
		oldTail->pNextNode = newNode;
	}

	template <typename T>
	void LFQueue<T>::Push(const T& data) noexcept
	{
		const std::shared_ptr<Node> newNode{ std::make_shared<Node>(data) };
		std::shared_ptr<Node> oldTail{ m_Tail.load() };

		while (!m_Tail.compare_exchange_weak(oldTail, newNode))
		{
		}

		// when we pass this line it means that we won the thread race any manipulation to the m_Tail
		// doesn't matter, so we can assign our newNode which is local to the caller to the
		// pNextNode of m_Tail
		oldTail->pNextNode = newNode; // link this person to the next person in queue
	}

	template <typename T>
	std::optional<T> LFQueue<T>::PopReturn()
	{
		std::shared_ptr<Node> oldHead{ m_Head.load() };
		//const std::shared_ptr<Node<T>> newHead{ oldHead->pNextNode };


		// we never remove the first node
		while (oldHead && oldHead->pNextNode &&
			!m_Head.compare_exchange_weak(oldHead, oldHead->pNextNode))
		{
		}

		return oldHead->pNextNode ? std::optional<T>{ oldHead->pNextNode->data } : std::nullopt;
	}

	template <typename T>
	T& LFQueue<T>::Front()
	{
		auto out{ m_Head.load() };
		if (!out || !out->pNextNode)
			throw std::out_of_range("accessing a null head");
		return out->pNextNode->data;
	}

	template <typename T>
	void LFQueue<T>::Pop() noexcept
	{
		std::shared_ptr<Node> oldHead{ m_Head.load() };

		// we never remove the first node
		while (oldHead && oldHead->pNextNode &&
			!m_Head.compare_exchange_weak(oldHead, oldHead->pNextNode))
		{
		}
	}

	template <typename T>
	bool LFQueue<T>::Empty() const noexcept
	{
		return m_Tail.load() == m_Head.load();
	}

	template <typename T>
	LFQueue<T>::~LFQueue()
	{
		// fix the linked list destructor overflow
		// https://softwareengineering.stackexchange.com/questions/271216/will-destructing-a-large-list-overflow-my-stack

		std::shared_ptr<Node> head{ m_Head.load() };
		while (head)
		{
			head = std::move(head->pNextNode);
		}
	}
}


#pragma once

#include <memory>
#include <functional>
#include <string>
#include <thread>

namespace dp
{
	template <typename FunctionType,
		typename ThreadType>
		requires std::invocable<FunctionType>&&
	std::is_same_v<void, std::invoke_result_t<FunctionType>>
		class thread_pool;
}

namespace powe
{
	namespace details
	{
		using ModsOverwriteOrder = std::unordered_map<std::string, std::vector<std::string>>;
		using DirectoryTree = std::unordered_map<std::string, std::string>;
	}
}